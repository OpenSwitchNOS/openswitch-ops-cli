/* NTP CLI commands
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * File: ntp_vty.c
 *
 * Purpose: To add ntp CLI configuration and display commands
 */

#include <sys/wait.h>
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "ntp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "ovsdb-data.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"

VLOG_DEFINE_THIS_MODULE(vtysh_ntp_cli);

/* Global variables */
extern struct ovsdb_idl *idl;
char g_NTP_prefer_default[]  = NTP_ASSOC_ATTRIB_PREFER_DEFAULT;
char g_NTP_version_default[] = NTP_ASSOC_ATTRIB_VERSION_DEFAULT;

/*================================================================================================*/
/* VRF Table Related functions */

/* Find the vrf with matching name */
static const struct ovsrec_vrf *
get_ovsrec_vrf_with_name(char *name)
{
    /* TODO change this later when multiple VRFs are supported */
    return ovsrec_vrf_first(idl);
}

/*================================================================================================*/
/* NTP Association Table Related functions */

static const struct ovsrec_ntp_associations *
ntp_ovsrec_get_assoc(char *vrf_name, char *server_name)
{
    int i = 0;
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;

    OVSREC_NTP_ASSOCIATIONS_FOR_EACH(ntp_assoc_row, idl) {
        i++;

        if (0 == strcmp(server_name, ntp_assoc_row->name)) {
            VLOG_DBG("%s:%d - Server matching %s found at row = %d. Now checking if the VRF it belongs to is \"%s\"...\n", __FILE__, __LINE__, server_name, i, vrf_name);
            vty_out(vty, "%s:%d - Server matching %s found at row = %d. Now checking if the VRF it belongs to is \"%s\"...\n", __FILE__, __LINE__, server_name, i, vrf_name);

            if (NULL == ntp_assoc_row->vrf) {
                VLOG_ERR("%s:%d - No VRF associated with server %s\n", __FILE__, __LINE__, server_name);
            }
            else {
                VLOG_DBG("%s:%d - Corresponding vrf name = %s\n", __FILE__, __LINE__, ((struct ovsrec_vrf *)(*(ntp_assoc_row->vrf)))->name);
                vty_out(vty, "%s:%d - Corresponding vrf name = %s\n", __FILE__, __LINE__, ((struct ovsrec_vrf *)(*(ntp_assoc_row->vrf)))->name);

                /* Now check if it is for the intended VRF */
                if (0 == strcmp(((struct ovsrec_vrf *)(*(ntp_assoc_row->vrf)))->name, vrf_name)) {
                    vty_out(vty, "%s:%d - Server record found at row = %d\n", __FILE__, __LINE__, i);
                    return ntp_assoc_row;
                }
            }
        }
    }

    VLOG_DBG("%s:%d - No matching server record found\n", __FILE__, __LINE__);
    return NULL;
}

static inline void
ntp_server_get_default_parameters(ntp_cli_ntp_server_params_t *pntp_server_params)
{
    memset(pntp_server_params, 0, sizeof(ntp_cli_ntp_server_params_t));
    pntp_server_params->prefer = g_NTP_prefer_default;
    pntp_server_params->version = g_NTP_version_default;
}

const int
ntp_server_replace_parameters(struct ovsrec_ntp_associations *ntp_assoc_row, ntp_cli_ntp_server_params_t *ntp_server_params)
{
    struct smap smap_assoc_attribs;
    const struct smap *psmap = NULL;

    if (ntp_assoc_row) {
        psmap = &ntp_assoc_row->association_attributes;
        smap_clone(&smap_assoc_attribs, psmap);

        /* Set the server name */
        ovsrec_ntp_associations_set_name(ntp_assoc_row, ntp_server_params->server_name);

        /* Set the VRF name */
        /* TODO: set the "n_vrf" parameter for the following function call correctly when multiple VRFs are supported */
        struct ovsrec_vrf *vrf_row = get_ovsrec_vrf_with_name(ntp_server_params->vrf_name);
        ovsrec_ntp_associations_set_vrf(ntp_assoc_row, &vrf_row, 1);

        if (ntp_server_params->prefer) {
            smap_replace(&smap_assoc_attribs, NTP_ASSOC_ATTRIB_PREFER, NTP_TRUE_STR);
        }

        if (ntp_server_params->version) {
            smap_replace(&smap_assoc_attribs, NTP_ASSOC_ATTRIB_VERSION, ntp_server_params->version);
        }

        if (ntp_server_params->keyid) {
//            smap_replace(&smap_assoc_attribs, NTP_ASSOC_ATTRIB_KEYID, ntp_server_params->keyid);
        }

        ovsrec_ntp_associations_set_association_attributes(ntp_assoc_row, &smap_assoc_attribs);
        smap_destroy(&smap_assoc_attribs);
    }

    //return CMD_OVSDB_FAILURE;
    return CMD_SUCCESS;
}

const int
vtysh_ovsdb_ntp_server_set(ntp_cli_ntp_server_params_t *ntp_server_params)
{
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;
    struct ovsdb_idl_txn *ntp_association_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(ntp_association_txn);

    /* See if it already exists. */
    ntp_assoc_row = ntp_ovsrec_get_assoc(ntp_server_params->vrf_name, ntp_server_params->server_name);
    if (NULL == ntp_assoc_row) {
        if (ntp_server_params->no_form) {
            /* Nothing to delete */
            vty_out(vty, "%s:%d - This server does not exist\n", __FILE__, __LINE__);
        }
        else {
            VLOG_DBG("%s:%d - Inserting a row into the NTP Assoc table\n", __FILE__, __LINE__);

            ntp_assoc_row = ovsrec_ntp_associations_insert(ntp_association_txn);
            if (NULL == ntp_assoc_row) {
                VLOG_ERR("%s:%d - Could not insert a row into the NTP Assoc Table\n", __FILE__, __LINE__);
                ERRONEOUS_DB_TXN(ntp_association_txn, "Could not insert a row into the NTP Assoc Table");
            }
            else {
                VLOG_DBG("%s:%d - Inserted a row into the NTP Assoc Table successfully\n", __FILE__, __LINE__);
                ntp_server_replace_parameters(ntp_assoc_row, ntp_server_params);
            }
        }
    }
    else {
        if (ntp_server_params->no_form) {
            VLOG_DBG("%s:%d - Deleting a row from the NTP Assoc table\n", __FILE__, __LINE__);
            ovsrec_ntp_associations_delete(ntp_assoc_row);
        }
        else {
            vty_out(vty, "%s:%d - This server already exists\n", __FILE__, __LINE__);
            vty_out(vty, "%s:%d - Replacing parameters\n", __FILE__, __LINE__);
            ntp_server_replace_parameters(ntp_assoc_row, ntp_server_params);
        }
    }

    /* End of transaction. */
    END_DB_TXN(ntp_association_txn);
}

/*================================================================================================*/
/* NTP Keys Table Related functions */

static const struct ovsrec_ntp_keys *
ntp_ovsrec_get_auth_key(int64_t key)
{
    int i = 0;
    struct ovsrec_ntp_keys *ntp_auth_key_row = NULL;

    OVSREC_NTP_KEYS_FOR_EACH(ntp_auth_key_row, idl) {
        i++;

        if (ntp_auth_key_row->key_id == key) {
            vty_out(vty, "%s:%d - AuthKey matching %d found at row = %d\n", __FILE__, __LINE__, key, i);
            return ntp_auth_key_row;
        }
    }

    VLOG_DBG("%s:%d - No matching auth-key found\n", __FILE__, __LINE__);
    return NULL;
}

static inline void
ntp_auth_key_get_default_parameters(ntp_cli_ntp_auth_key_params_t *pntp_auth_key_params)
{
    memset(pntp_auth_key_params, 0, sizeof(ntp_cli_ntp_auth_key_params_t));
}

const int
ntp_auth_key_replace_parameters(struct ovsrec_ntp_keys *ntp_auth_key_row, ntp_cli_ntp_auth_key_params_t *pntp_auth_key_params)
{
    if (ntp_auth_key_row) {
        ovsrec_ntp_keys_set_key_id(ntp_auth_key_row, atoi(pntp_auth_key_params->key));
        ovsrec_ntp_keys_set_key_password(ntp_auth_key_row, pntp_auth_key_params->md5_pwd);
    }

    return CMD_SUCCESS;
}

const int
vtysh_ovsdb_ntp_auth_key_set(ntp_cli_ntp_auth_key_params_t *pntp_auth_key_params)
{
    struct ovsrec_ntp_keys *ntp_auth_key_row = NULL;
    struct ovsdb_idl_txn *ntp_auth_key_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(ntp_auth_key_txn);

    /* See if it already exists. */
    ntp_auth_key_row = ntp_ovsrec_get_auth_key(atoi(pntp_auth_key_params->key));
    if (NULL == ntp_auth_key_row) {
        if (pntp_auth_key_params->no_form) {
            /* Nothing to delete */
            vty_out(vty, "This key does not exist\n");
        }
        else {
            VLOG_DBG("%s:%d - Inserting a row into the NTP Keys table\n", __FILE__, __LINE__);

            ntp_auth_key_row = ovsrec_ntp_keys_insert(ntp_auth_key_txn);
            if (NULL == ntp_auth_key_row) {
                VLOG_ERR("%s:%d - Could not insert a row into DB\n", __FILE__, __LINE__);
                ERRONEOUS_DB_TXN(ntp_auth_key_txn, "Could not insert a row into the NTP Keys Table");
            }
            else {
                VLOG_DBG("%s:%d - Inserted a row into the NTP Keys Table successfully\n", __FILE__, __LINE__);
                vty_out(vty, "%s:%d - Inserted a row into the NTP Keys Table successfully\n", __FILE__, __LINE__);
                ntp_auth_key_replace_parameters(ntp_auth_key_row, pntp_auth_key_params);
            }
        }
    }
    else
    {
        if (pntp_auth_key_params->no_form) {
            VLOG_DBG("%s:%d - Deleting a row from the NTP Keys table\n", __FILE__, __LINE__);
            ovsrec_ntp_keys_delete(ntp_auth_key_row);
        }
        else {
            vty_out(vty, "%s:%d - This key already exists\n", __FILE__, __LINE__);
            vty_out(vty, "%s:%d - Replacing parameters\n", __FILE__, __LINE__);
            //VLOG_DBG("%s:%d - Deleting a row from the NTP Keys table\n", __FILE__, __LINE__);
            //ovsrec_ntp_keys_delete(ntp_auth_key_row);
            ntp_auth_key_replace_parameters(ntp_auth_key_row, pntp_auth_key_params);
        }
    }

    /* End of transaction. */
    END_DB_TXN(ntp_auth_key_txn);
}

/*================================================================================================*/
/* System Table Related functions */

static inline void
ntp_auth_enable_get_default_parameters(ntp_cli_ntp_auth_enable_params_t *pntp_auth_enable_params)
{
    memset(pntp_auth_enable_params, 0, sizeof(ntp_cli_ntp_auth_enable_params_t));
}

const int
vtysh_ovsdb_ntp_auth_enable_set(ntp_cli_ntp_auth_enable_params_t *pntp_auth_enable_params)
{
    const struct ovsrec_system *ovs_system = NULL;
    struct ovsdb_idl_txn *ntp_auth_enable_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(ntp_auth_enable_txn);

    /* Get access to the System Table */
    ovs_system = ovsrec_system_first(idl);
    if (NULL == ovs_system) {
         vty_out(vty, "Could not access the System Table\n");
         ERRONEOUS_DB_TXN(ntp_auth_enable_txn, "Could not access the System Table");
    }

    if (pntp_auth_enable_params->no_form) {
        smap_replace((struct smap *)&ovs_system->ntp_config, SYSTEM_NTP_CONFIG_AUTHENTICATION_ENABLE, NTP_FALSE_STR);
    }
    else {
        smap_replace((struct smap *)&ovs_system->ntp_config, SYSTEM_NTP_CONFIG_AUTHENTICATION_ENABLE, NTP_TRUE_STR);
    }

    /* End of transaction. */
    END_DB_TXN(ntp_auth_enable_txn);
}

/*================================================================================================*/
/* SHOW CLI Implementations */

static void
vtysh_ovsdb_show_ntp_associations()
{
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;
    int i = 0;
    char *buf = NULL;

    vty_out(vty, "NTP associations table:\n");
    vty_out(vty,"-----------------------------------------------------------------------------------------------------------\n");
    vty_out(vty, "%5s  %20s  %6s  %3s  %6s  %7s  %4s  %4s  %4s  %5s  %5s  %6s  %6s\n",
        "ID1", "NAME", "REF-ID", "VER", "REMOTE", "STRATUM", "TYPE", "LAST", "POLL", "REACH", "DELAY", "OFFSET", "JITTER");
    vty_out(vty,"-----------------------------------------------------------------------------------------------------------\n");

    OVSREC_NTP_ASSOCIATIONS_FOR_EACH(ntp_assoc_row, idl) {
        vty_out(vty, "%5d %20s", ++i, ntp_assoc_row->name);

        buf = smap_get(&ntp_assoc_row->association_attributes, NTP_ASSOC_ATTRIB_PREFER);
        if (buf) {
            vty_out(vty, "  %6s  ", buf);
        }

        vty_out(vty, "\n");
    }

    vty_out(vty,"-----------------------------------------------------------------------------------------------------------\n");
}

static void
vtysh_ovsdb_show_ntp_status()
{
}

static void
vtysh_ovsdb_show_ntp_statistics()
{
}

static void
vtysh_ovsdb_show_ntp_trusted_keys()
{
    struct ovsrec_ntp_keys *ntp_auth_key_row = NULL;

    vty_out(vty,"------------\n");
    vty_out(vty,"Trusted-keys\n");
    vty_out(vty,"------------\n");

    OVSREC_NTP_KEYS_FOR_EACH(ntp_auth_key_row, idl) {
//        if ((ntp_auth_key_row) && (*(ntp_auth_key_row->trust_enable)))
        {
            vty_out(vty, "%d\n", ntp_auth_key_row->key_id);
        }
    }

    vty_out(vty,"------------\n");
}

static void
vtysh_ovsdb_show_ntp_authentication_keys()
{
    struct ovsrec_ntp_keys *ntp_auth_key_row = NULL;

    vty_out(vty,"---------------------------\n");
    vty_out(vty,"%8s   %16s\n", "Auth-key", "MD5 password");
    vty_out(vty,"---------------------------\n");

    OVSREC_NTP_KEYS_FOR_EACH(ntp_auth_key_row, idl) {
        if (ntp_auth_key_row) {
            vty_out(vty, "%8d   %16s\n", ntp_auth_key_row->key_id, ntp_auth_key_row->key_password);
        }
    }

    vty_out(vty,"---------------------------\n");
}

/*================================================================================================*/
/* CLI Definitions */

/* SHOW CLIs */
DEFUN ( vtysh_show_ntp_associations,
        vtysh_show_ntp_associations_cmd,
        "show ntp associations",
        "show\n"
        "ntp\n"
        "NTP associations\n"
      )
{
    vtysh_ovsdb_show_ntp_associations();
    return CMD_SUCCESS;
}

DEFUN ( vtysh_show_ntp_status,
        vtysh_show_ntp_status_cmd,
        "show ntp status",
        "show\n"
        "ntp\n"
        "NTP status information\n"
      )
{
    vtysh_ovsdb_show_ntp_status();
    return CMD_SUCCESS;
}

DEFUN ( vtysh_show_ntp_statistics,
        vtysh_show_ntp_statistics_cmd,
        "show ntp statistics",
        "show\n"
        "ntp\n"
        "NTP statistics information\n"
      )
{
    vtysh_ovsdb_show_ntp_statistics();
    return CMD_SUCCESS;
}

DEFUN ( vtysh_show_ntp_trusted_keys,
        vtysh_show_ntp_trusted_keys_cmd,
        "show ntp trusted-keys",
        "show\n"
        "ntp\n"
        "NTP trusted-Keys information\n"
      )
{
    vtysh_ovsdb_show_ntp_trusted_keys();
    return CMD_SUCCESS;
}

DEFUN ( vtysh_show_ntp_authentication_keys,
        vtysh_show_ntp_authentication_keys_cmd,
        "show ntp authentication-keys",
        "show\n"
        "ntp\n"
        "NTP authentication-keys information\n"
      )
{
    vtysh_ovsdb_show_ntp_authentication_keys();
    return CMD_SUCCESS;
}

/* CONFIG CLIs */
DEFUN ( vtysh_set_ntp_server,
        vtysh_set_ntp_server_cmd,
        "ntp server WORD "
        "{prefer | version WORD | key-id WORD}",
        "ntp\n"
        "server\n"
        "NTP server name or IPv4 address\n"
        "Request priority for this server when switch selects a synchronizing server\n"
        "NTP version number\n"
        "Version can be 3 or 4\n"
        "Key-id\n"
        "Peer key number\n"
      )
{
    int ret_code = CMD_SUCCESS;
    ntp_cli_ntp_server_params_t ntp_server_params;
    ntp_server_get_default_parameters(&ntp_server_params);

    /* Set various parameters needed by the "ntp server" command handler */
    ntp_server_params.vrf_name = DEFAULT_VRF_NAME;
    ntp_server_params.server_name = (char *)argv[0];
    ntp_server_params.prefer = (char *)argv[1];
    ntp_server_params.version = (char *)argv[2];
    ntp_server_params.keyid = (char *)argv[3];

    if (vty_flags & CMD_FLAG_NO_CMD) {
        ntp_server_params.no_form = 1;
    }

    /* Finally call the handler */
    ret_code = vtysh_ovsdb_ntp_server_set(&ntp_server_params);

    return ret_code;
}


DEFUN_NO_FORM ( vtysh_set_ntp_server,
        vtysh_set_ntp_server_cmd,
        "ntp server WORD "
        "{prefer | version WORD | key-id WORD}",
        "ntp\n"
        "server\n"
        "NTP server name or IPv4 address\n"
        "Request priority for this server when switch selects a synchronizing server\n"
        "NTP version number\n"
        "Version can be 3 or 4\n"
        "Key-id\n"
        "Peer key number\n"
      );


DEFUN ( vtysh_set_ntp_authentication_enable,
        vtysh_set_ntp_authentication_enable_cmd,
        "ntp authentication enable",
        "ntp info\n"
        "authentication\n"
        "Enable ntp authentication\n"
      )
{
    int ret_code = CMD_SUCCESS;
    ntp_cli_ntp_auth_enable_params_t ntp_auth_enable_params;
    ntp_auth_enable_get_default_parameters(&ntp_auth_enable_params);

    /* Set various parameters needed by the "ntp authentication enable" command handler */
    if (vty_flags & CMD_FLAG_NO_CMD) {
        ntp_auth_enable_params.no_form = 1;
    }

    /* Finally call the handler */
    ret_code = vtysh_ovsdb_ntp_auth_enable_set(&ntp_auth_enable_params);

    return ret_code;
}


DEFUN_NO_FORM ( vtysh_set_ntp_authentication_enable,
        vtysh_set_ntp_authentication_enable_cmd,
        "ntp authentication enable",
        "ntp info\n"
        "authentication\n"
        "Enable ntp authentication\n"
      );


DEFUN ( vtysh_set_ntp_authentication_key,
        vtysh_set_ntp_authentication_key_cmd,
        "ntp authentication-key WORD md5 WORD",
        "ntp\n"
        "authentication-key\n"
        "NTP authentication-key number\n"
        "md5\n"
        "NTP authentication-key\n"
      )
{
    int ret_code = CMD_SUCCESS;
    ntp_cli_ntp_auth_key_params_t ntp_auth_key_params;
    ntp_auth_key_get_default_parameters(&ntp_auth_key_params);

    /* Set various parameters needed by the "ntp auth key" command handler */
    ntp_auth_key_params.key = (char *)argv[0];
    ntp_auth_key_params.md5_pwd = (char *)argv[1];

    if (vty_flags & CMD_FLAG_NO_CMD) {
        ntp_auth_key_params.no_form = 1;
    }

    /* Finally call the handler */
    ret_code = vtysh_ovsdb_ntp_auth_key_set(&ntp_auth_key_params);

    return ret_code;
}


DEFUN_NO_FORM ( vtysh_set_ntp_authentication_key,
        vtysh_set_ntp_authentication_key_cmd,
        "ntp authentication-key WORD md5 WORD",
        "ntp\n"
        "authentication-key\n"
        "NTP authentication-key number\n"
        "md5\n"
        "NTP authentication-key\n"
      );


DEFUN ( vtysh_set_ntp_trusted_key,
        vtysh_set_ntp_trusted_key_cmd,
        "ntp trusted-key WORD",
        "ntp\n"
        "trusted-key\n"
        "Trusted-key number\n"
      )
{
    int ret_code = CMD_SUCCESS;
    return ret_code;
}


DEFUN_NO_FORM ( vtysh_set_ntp_trusted_key,
        vtysh_set_ntp_trusted_key_cmd,
        "ntp trusted-key WORD",
        "ntp\n"
        "trusted-key\n"
        "Trusted-key number\n"
      );

/*================================================================================================*/

/*
 * Function       : ntp_vty_init
 * Responsibility : install all the CLIs in the respective contexts.
 */

void
ntp_vty_init (void)
{
    /* SHOW CMDS */
    install_element (VIEW_NODE, &vtysh_show_ntp_associations_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_associations_cmd);

    install_element (VIEW_NODE, &vtysh_show_ntp_status_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_status_cmd);

    install_element (VIEW_NODE, &vtysh_show_ntp_statistics_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_statistics_cmd);

    install_element (VIEW_NODE, &vtysh_show_ntp_trusted_keys_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_trusted_keys_cmd);

    install_element (VIEW_NODE, &vtysh_show_ntp_authentication_keys_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_authentication_keys_cmd);

    /* CONFIG CMDS */
    install_element (CONFIG_NODE, &vtysh_set_ntp_server_cmd);
    install_element (CONFIG_NODE, &no_vtysh_set_ntp_server_cmd);

    install_element (CONFIG_NODE, &vtysh_set_ntp_authentication_enable_cmd);
    install_element (CONFIG_NODE, &no_vtysh_set_ntp_authentication_enable_cmd);

    install_element (CONFIG_NODE, &vtysh_set_ntp_authentication_key_cmd);
    install_element (CONFIG_NODE, &no_vtysh_set_ntp_authentication_key_cmd);

    install_element (CONFIG_NODE, &vtysh_set_ntp_trusted_key_cmd);
    install_element (CONFIG_NODE, &no_vtysh_set_ntp_trusted_key_cmd);
}

/*================================================================================================*/
