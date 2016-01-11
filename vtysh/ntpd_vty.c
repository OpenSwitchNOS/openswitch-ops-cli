/* Ntpd CLI commands
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
 * File: ntpd_vty.c
 *
 * Purpose: To add ntpd CLI configuration and display commands
 */

#include <sys/wait.h>
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "ntpd_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "ovsdb-data.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"

VLOG_DEFINE_THIS_MODULE(vtysh_ntpd_cli);
extern struct ovsdb_idl *idl;

/* Global variables */
unsigned int g_id = 0;

/*================================================================================================*/
/* VRF Table Related functions */

/* Find the vrf with matching name */
static const struct ovsrec_vrf *
get_ovsrec_vrf_with_name(char *name)
{
    /* TODO change this later when multi vrf's are supported */
    return ovsrec_vrf_first(idl);
}


void
ntp_assoc_insert_to_vrf(const struct ovsrec_vrf *vrf_row, const struct ovsrec_ntp_association *ntp_assoc_row)
{
    struct ovsrec_ntp_association **ntp_assoc_list;
    int i = 0;

    ntp_assoc_list = xmalloc(sizeof(*(vrf_row->ntp_associations)) * (vrf_row->n_ntp_associations + 1));
    for (i = 0; i < vrf_row->n_ntp_associations; i++)
    {
        ntp_assoc_list[i] = vrf_row->ntp_associations[i];
    }
    ntp_assoc_list[i] = CONST_CAST(struct ovsrec_ntp_association *, ntp_assoc_row);

    ovsrec_vrf_set_ntp_associations(vrf_row, ntp_assoc_list, (vrf_row->n_ntp_associations + 1));

    free(ntp_assoc_list);
}

void
ntp_assoc_delete_from_vrf(const struct ovsrec_vrf *vrf_row, const struct ovsrec_ntp_association *ntp_assoc_row)
{
    struct ovsrec_ntp_association **ntp_assoc_list;
    int i = 0, j = 0;

    ntp_assoc_list = xmalloc(sizeof(*(vrf_row->ntp_associations)) * (vrf_row->n_ntp_associations - 1));
    for (i = 0; i < vrf_row->n_ntp_associations; i++)
    {
        if (vrf_row->ntp_associations[i] != CONST_CAST(struct ovsrec_ntp_association *, ntp_assoc_row))
        {
            ntp_assoc_list[j++] = vrf_row->ntp_associations[i];
        }
    }

    ovsrec_vrf_set_ntp_associations(vrf_row, ntp_assoc_list, (vrf_row->n_ntp_associations - 1));

    free(ntp_assoc_list);
}

/*================================================================================================*/
/* NTP Association Table Related functions */

static const struct ovsrec_ntp_association *
get_ovsrec_ntp_assoc(const struct ovsrec_vrf *vrf_row, char *server_name)
{
    int i = 0;

    VLOG_DBG("%s:%d - vrf_row->n_ntp_associations = %d\n", __FILE__, __LINE__, vrf_row->n_ntp_associations);

    for (i = 0; i < vrf_row->n_ntp_associations; i++)
    {
        if (0 == strcmp(server_name, vrf_row->ntp_associations[i]->name))
        {
            VLOG_DBG("%s:%d - Server record found at row = %d\n", __FILE__, __LINE__, i);
            return vrf_row->ntp_associations[i];
        }
    }

    VLOG_DBG("%s:%d - No matching server record found\n", __FILE__, __LINE__);
    return NULL;
}


const int
vtysh_ovsdb_ntp_server_set(char *vrf_name, ntp_cli_ntp_server_params_t *ntp_server_params)
{
    struct ovsrec_ntp_association *ntp_assoc_row = NULL;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *ntp_association_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(ntp_association_txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (NULL == vrf_row)
    {
        VLOG_ERR("%s:%d - Error getting row from VRF tbl\n", __FILE__, __LINE__);
        ERRONEOUS_DB_TXN(ntp_association_txn, "no vrf found");
    }

    /* See if it already exists. */
    ntp_assoc_row = get_ovsrec_ntp_assoc(vrf_row, ntp_server_params->server_name);
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

            ntp_assoc_row = ovsrec_ntp_association_insert(ntp_association_txn);
            if (NULL == ntp_assoc_row)
            {
                VLOG_ERR("%s:%d - Could not insert a row into DB\n", __FILE__, __LINE__);
            }
            else
            {
                VLOG_DBG("%s:%d - Inserted a row into DB successfully\n", __FILE__, __LINE__);
                ovsrec_ntp_association_set_name(ntp_assoc_row, ntp_server_params->server_name);
                ovsrec_ntp_association_set_id(ntp_assoc_row, ++g_id);

                VLOG_DBG("%s%d - Inserting to VRF\n", __FILE__, __LINE__);
                ntp_assoc_insert_to_vrf(vrf_row, ntp_assoc_row);
            }
        }
    }
    else
    {
        if (ntp_server_params->no_form)
        {
            VLOG_DBG("%s:%d - Deleting a row from the NTP Assoc table\n", __FILE__, __LINE__);
            ovsrec_ntp_association_delete(ntp_assoc_row);

            VLOG_DBG("%s%d - Deleting from VRF\n", __FILE__, __LINE__);
            ntp_assoc_delete_from_vrf(vrf_row, ntp_assoc_row);
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
vtysh_ovsdb_show_ntp_associations(bool detail)
{
    struct ovsrec_ntp_association *ntp_association_row = NULL;

    vty_out(vty, "NTP associations table:\n");
    vty_out(vty,"---------------------------------------------------------------\n");
    vty_out(vty, "%5s %57s\n", "ID", "NAME");
    vty_out(vty,"---------------------------------------------------------------\n");

    OVSREC_NTP_ASSOCIATION_FOR_EACH(ntp_association_row, idl)
    {
        vty_out(vty, "%5d %57s\n", ntp_association_row->id, ntp_association_row->name);
    }

    vty_out(vty,"---------------------------------------------------------------\n");
}

/*================================================================================================*/
/* CLI Definitions */

DEFUN ( vtysh_show_ntp_associations,
        vtysh_show_ntp_associations_cmd,
        "show ntp associations",
        "Show information\n"
        "NTP Information\n"
        "Associations Table\n"
      )
{
    vtysh_ovsdb_show_ntp_associations(false);
    return CMD_SUCCESS;
}


DEFUN ( vtysh_set_ntp_server,
        vtysh_set_ntp_server_cmd,
        "ntp server WORD "
        "{prefer | version WORD | key-id WORD}",
        "ntp info\n"
        "configure ntp server\n"
        "Enter ntp server IPv4 address or FQDN\n"
        "request priority for this server when switch selects a synchronizing server\n"
        "version can be 3 or 4\n"
        "version #\n"
        "key-id\n"
        "key-id #\n"
      )
{
    int ret_code = CMD_SUCCESS;
    ntp_cli_ntp_server_params_t ntp_server_params;
    memset(&ntp_server_params, 0, sizeof(ntp_cli_ntp_server_params_t));

    /* Set various parameters needed by the "ntp server" command handler */
    ntp_server_params.server_name = (char *)argv[0];

    //ntp_server_params.prefer = ((NULL == argv[1]) ? FALSE : TRUE);
    ntp_server_params.prefer = ((NULL == argv[1]) ? 0 : 1);

    if (vty_flags & CMD_FLAG_NO_CMD)
    {
        ntp_server_params.no_form = 1;
    }

    /* Finally call the handler */
    ret_code = vtysh_ovsdb_ntp_server_set(DEFAULT_VRF_NAME, &ntp_server_params);

    return ret_code;
}


DEFUN_NO_FORM ( vtysh_set_ntp_server,
                vtysh_set_ntp_server_cmd,
                "ntp server WORD "
                "{prefer | version WORD | key-id WORD}",
                "ntp info\n"
                "configure ntp server\n"
                "Enter ntp server IPv4 address or FQDN\n"
                "request priority for this server when switch selects a synchronizing server\n"
                "version can be 3 or 4\n"
                "version #\n"
                "key-id\n"
                "key-id #\n"
      );

/*================================================================================================*/

/*
 * Function       : ntpd_vty_init
 * Responsibility : install all the CLIs in the respective contexts.
 */

void
ntpd_vty_init (void)
{
    install_element (VIEW_NODE, &vtysh_show_ntp_associations_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ntp_associations_cmd);

    install_element (CONFIG_NODE, &vtysh_set_ntp_server_cmd);
    install_element (ENABLE_NODE, &vtysh_set_ntp_server_cmd);

    install_element (CONFIG_NODE, &no_vtysh_set_ntp_server_cmd);
    install_element (ENABLE_NODE, &no_vtysh_set_ntp_server_cmd);
}

/*================================================================================================*/
