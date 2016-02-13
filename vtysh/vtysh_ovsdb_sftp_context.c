/* SFTP functionality client callback resigitration source files.
 *
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: vtysh_ovsdb_sftp_context.c
 *
 * Purpose: Source for registering client callback with SFTP
 *          server context.
 */
/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_sftp_context.c
 * Source for registering SFTP functionality client callback with
 * openvswitch table.
 *
 ***************************************************************************/

#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_config_context.h"
#include "vtysh_ovsdb_sftp_context.h"
#include "utils/system_vtysh_utils.h"

char sftp_server_context_client_name[] = "vtysh_sftp_server_context_\
                                                        clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_sftp_server_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : On success, returns e_vtysh_ok. On failure, returns e_vtysh_error.
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_sftp_server_context_clientcallback (void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_system *row = NULL;
    char *sftp_status = NULL;

    row = ovsrec_system_first(p_msg->idl);
    if (!row) {
        return e_vtysh_error;
    }

    sftp_status = (char *)smap_get(&row->other_config, SFTP_SERVER_CONFIG);
    if (sftp_status && !strcmp(sftp_status, "true")) {
        vtysh_ovsdb_cli_print(p_msg, "%s", "sftp-server");
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "enable");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_sftp_context_clients
| Responsibility : registers the client callbacks for SFTP context
| Return : On success, returns e_vtysh_ok. On failure, returns erro_no.
-----------------------------------------------------------------------------*/
int
vtysh_init_sftp_context_clients (void)
{
    vtysh_context_client client;
    vtysh_ret_val retval = e_vtysh_error;

    retval = install_show_run_config_context(
                                  e_vtysh_sftp_server_context,
                                  &vtysh_sftp_server_context_clientcallback,
                                  NULL, NULL);
    if (e_vtysh_ok != retval)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                           "SFTP server context unable "\
                           "to add config callback");
        assert(0);
        return retval;
    }

#ifdef TO_BE_REMOVED
    client.p_client_name = sftp_server_context_client_name;
    client.client_id = e_vtysh_sftp_server_context_config;
    client.p_callback = &vtysh_sftp_server_context_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_sftp_server_context,
                                     e_vtysh_sftp_server_context_config,
                                     &client);
    if (e_vtysh_ok != retval)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                           "SFTP server context unable "\
                           "to add config callback");
        assert(0);
        return retval;
    }
#endif
    return e_vtysh_ok;
}
