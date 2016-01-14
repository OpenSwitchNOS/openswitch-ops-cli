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
extern struct ovsdb_idl *idl;

/* Global variables */
unsigned int g_id = 0;

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
get_ovsrec_ntp_assoc(char *vrf_name, char *server_name)
{
    int i = 0;
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;

    OVSREC_NTP_ASSOCIATIONS_FOR_EACH(ntp_assoc_row, idl)
    {
        i++;

        if (0 == strcmp(server_name, ntp_assoc_row->name))
        {
            VLOG_DBG("%s:%d - Server matching %s found at row = %d. Now checking the VRF it belongs to...\n", __FILE__, __LINE__, server_name, i);

            if (NULL == ntp_assoc_row->vrf)
            {
                VLOG_ERR("%s:%d - No VRF associated with server %s\n", __FILE__, __LINE__, server_name);
            }
            else
            {
                VLOG_DBG("%s:%d - Corresponding vrf name = %s\n", __FILE__, __LINE__, ((struct ovsrec_vrf *)(*(ntp_assoc_row->vrf)))->name);

                /* Now check if it is for the intended VRF */
                if (0 == strcmp(((struct ovsrec_vrf *)(*(ntp_assoc_row->vrf)))->name, vrf_name))
                {
                    vty_out(vty, "%s:%d - Server record found at row = %d\n", __FILE__, __LINE__, i);
                    return ntp_assoc_row;
                }
            }
        }
    }

    VLOG_DBG("%s:%d - No matching server record found\n", __FILE__, __LINE__);
    return NULL;
}

const int
vtysh_ovsdb_ntp_server_set(ntp_cli_ntp_server_params_t *ntp_server_params)
{
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;
    struct ovsdb_idl_txn *ntp_association_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(ntp_association_txn);

    /* See if it already exists. */
    ntp_assoc_row = get_ovsrec_ntp_assoc(ntp_server_params->vrf_name, ntp_server_params->server_name);
    if (NULL == ntp_assoc_row)
    {
        if (ntp_server_params->no_form)
        {
            /* Nothing to delete */
            vty_out(vty, "%s:%d - This server does not exist\n", __FILE__, __LINE__);
        }
        else
        {
            VLOG_DBG("%s:%d - Inserting a row into the NTP Assoc table\n", __FILE__, __LINE__);

            ntp_assoc_row = ovsrec_ntp_associations_insert(ntp_association_txn);
            if (NULL == ntp_assoc_row)
            {
                VLOG_ERR("%s:%d - Could not insert a row into DB\n", __FILE__, __LINE__);
            }
            else
            {
                VLOG_DBG("%s:%d - Inserted a row into DB successfully\n", __FILE__, __LINE__);

                struct ovsrec_vrf *vrf_row = get_ovsrec_vrf_with_name(ntp_server_params->vrf_name);

                ovsrec_ntp_associations_set_name(ntp_assoc_row, ntp_server_params->server_name);

                /* TODO: set the "n_vrf" parameter for the following function call correctly when multiple VRFs are supported */
                ovsrec_ntp_associations_set_vrf(ntp_assoc_row, &vrf_row, 1);
            }
        }
    }
    else
    {
        if (ntp_server_params->no_form)
        {
            VLOG_DBG("%s:%d - Deleting a row from the NTP Assoc table\n", __FILE__, __LINE__);
            ovsrec_ntp_associations_delete(ntp_assoc_row);
        }
        else
        {
            vty_out(vty, "%s:%d - This server already exists\n", __FILE__, __LINE__);
        }
    }

    /* End of transaction. */
    END_DB_TXN(ntp_association_txn);
}

static void
vtysh_ovsdb_show_ntp_associations()
{
    struct ovsrec_ntp_associations *ntp_assoc_row = NULL;
    int i = 0;

    vty_out(vty, "NTP associations table:\n");
    vty_out(vty,"---------------------------------------------------------------\n");
    vty_out(vty, "%5s %57s\n", "ID1", "NAME");
    vty_out(vty,"---------------------------------------------------------------\n");

    OVSREC_NTP_ASSOCIATIONS_FOR_EACH(ntp_assoc_row, idl)
    {
        vty_out(vty, "%5d %57s\n", ++i, ntp_assoc_row->name);
    }

    vty_out(vty,"---------------------------------------------------------------\n");
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
}

static void
vtysh_ovsdb_show_ntp_authentication_keys()
{
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
    memset(&ntp_server_params, 0, sizeof(ntp_cli_ntp_server_params_t));

    /* Set various parameters needed by the "ntp server" command handler */
    ntp_server_params.vrf_name = DEFAULT_VRF_NAME;
    ntp_server_params.server_name = (char *)argv[0];

    //ntp_server_params.prefer = ((NULL == argv[1]) ? FALSE : TRUE);
    ntp_server_params.prefer = ((NULL == argv[1]) ? 0 : 1);

    if (vty_flags & CMD_FLAG_NO_CMD)
    {
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
