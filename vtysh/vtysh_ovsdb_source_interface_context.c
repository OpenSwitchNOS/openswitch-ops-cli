/*
 * Copyright (C) 1997 Kunihiro Ishiguro
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
*/
/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_source_interface_context.c
 * Source for registering source interface client
 * callback with openvswitch table.
 *
 ***************************************************************************/

#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_config_context.h"
#include "vtysh_ovsdb_source_interface_context.h"
#include "openswitch-dflt.h"

char source_interface_context_client_name[] = "vtysh_source_interface_context_\
                                               clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_source_interface_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_source_interface_context_clientcallback (void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_system *row = NULL;
    char *source_interface_buff = NULL;
    uint8_t source_flag = false;

    row = ovsrec_system_first(p_msg->idl);
    if (!row) {
        return e_vtysh_ok;
    }

    source_interface_buff = (char *)smap_get(&row->other_config,
                                             "protocols_source");
    if (source_interface_buff !=NULL) {
        if (!source_flag) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "source interface");
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ", source_interface_buff);
            source_flag = true;
        }
    }

    source_interface_buff = (char *)smap_get(&row->other_config,
                                             "tftp_source");
    if (source_interface_buff !=NULL) {
        if (!source_flag) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "source interface");
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ", source_interface_buff);
            source_flag = true;
        }
    }
    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_source_interface_context_clients
| Responsibility : registers the client callbacks for source interface context
| Return : e_vtysh_ok for success, e_vtysh_error for failure
-----------------------------------------------------------------------------*/
int
vtysh_init_source_interface_context_clients(void)
{
    vtysh_context_client client;
    vtysh_ret_val retval = e_vtysh_error;
    memset(&client, 0, sizeof(vtysh_context_client));
    client.p_client_name = source_interface_context_client_name;
    client.client_id = e_vtysh_source_interface_context_config;
    client.p_callback = &vtysh_source_interface_context_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_source_interface_context,
                                     e_vtysh_source_interface_context_config,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                     "source_interface context unable to add config callback");
        assert(0);
        return retval;
    }

    return e_vtysh_ok;
}
