/* Management interface client callback resigitration source files.
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP.
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
 * File: vtysh_ovsdb_mgmt_intf_context.c
 *
 * Purpose: Source for registering client callback with management
 *          interface context.
 */
#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_mgmt_intf_context.h"
#include "mgmt_intf_vty.h"

char mgmt_intf_context_client_names[] = "vtysh_mgmt_intf_context_\
                                                     clientcallback";

/*
 * Function : vtysh_mgmt_intf_context_clientcallback.
 * Responsibility : client callback routine.
 * Parameters : void *p_private : void type object typecast to required.
 * Return : void.
 */
vtysh_ret_val
vtysh_mgmt_intf_context_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const char *data = NULL;
    const char *ip = NULL;
    const char *subnet = NULL;
    const char *dns_1 = NULL;
    const char *dns_2 = NULL;
    const struct ovsrec_open_vswitch *vswrow;
    vswrow = ovsrec_open_vswitch_first(p_msg->idl);
    if (!vswrow)
    {
        return e_vtysh_error;
    }

    data = smap_get(&vswrow->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if (!data)
    {
        /* If not present then mode is dhcp. So nothing to display since
         * dhcp is the default.
         */
        return e_vtysh_ok;
    }

    if (VTYSH_STR_EQ(data, OPEN_VSWITCH_MGMT_INTF_MAP_MODE_STATIC))
    {
        vtysh_ovsdb_cli_print(p_msg, "interface mgmt");
        ip = smap_get(&vswrow->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IP);
        subnet = smap_get(&vswrow->mgmt_intf,
                                   OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
        if (ip && subnet && (strcmp(ip, MGMT_INTF_DEFAULT_IP) != 0) )
            vtysh_ovsdb_cli_print(p_msg, "%4sip static %s/%s", "", ip, subnet);

        ip = smap_get(&vswrow->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
        if (ip && (strcmp(ip, MGMT_INTF_DEFAULT_IPV6) != 0))
        {
            vtysh_ovsdb_cli_print(p_msg, "%4sip static %s", "", ip);
        }
    }
    else
        return e_vtysh_ok;

    data = smap_get(&vswrow->mgmt_intf,
                               OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if (data && (strcmp(data, MGMT_INTF_DEFAULT_IP) != 0))
    {
        vtysh_ovsdb_cli_print(p_msg, "%4sdefault-gateway %s", "", data);
    }
    /* Ipv6 show running commands. */

    data = smap_get(&vswrow->mgmt_intf,
                          OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
    if (data && (strcmp(data, MGMT_INTF_DEFAULT_IPV6) != 0))
    {
        vtysh_ovsdb_cli_print(p_msg, "%4sdefault-gateway %s", "", data);
    }

    dns_1 = smap_get(&vswrow->mgmt_intf,
                        OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    dns_2 = smap_get(&vswrow->mgmt_intf,
                        OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (dns_1 && dns_2 && (strcmp(dns_1, MGMT_INTF_DEFAULT_IP) != 0) &&
                                    (strcmp(dns_2, MGMT_INTF_DEFAULT_IP) != 0))
    {
           vtysh_ovsdb_cli_print(p_msg, "%4snameserver %s %s", "", dns_1, dns_2);
    } else if (dns_1 && (strcmp(dns_1, MGMT_INTF_DEFAULT_IP) != 0)) {
           vtysh_ovsdb_cli_print(p_msg, "%4snameserver %s", "", dns_1);
    }
    return e_vtysh_ok;
}

/*
 * Function : vtysh_init_mgmt_intf_context_clients.
 * Responsibility : Registers the client callback routines for
 *                  mgmt interface context.
 * Return : On success, returns e_vtysh_ok. On failure, returns erro_no.
 */
int
vtysh_init_mgmt_intf_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = mgmt_intf_context_client_names;
  client.client_id = e_vtysh_mgmt_interface_context_config;
  client.p_callback = &vtysh_mgmt_intf_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_mgmt_interface_context,
                              e_vtysh_mgmt_interface_context_config, &client);
  if (e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                       "Mgmt interface context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
