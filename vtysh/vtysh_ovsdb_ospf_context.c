/* OSPF client callback registration source files.
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP.
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
 * File: vtysh_ovsdb_intf_ospf_context.c
 *
 * Purpose: Source for registering client callback with ospf context.
 */

#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "openswitch-dflt.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "command.h"
#include "ospf_vty.h"
#include "vtysh/utils/vlan_vtysh_utils.h"


/*
 * Function : vtysh_ospf_context_clientcallback.
 * Responsibility : client callback routine.
 * Parameters : void *p_private : void type object typecast to required.
 * Return : void.
 */
vtysh_ret_val
vtysh_intf_context_ospf_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *if_row = NULL;
    int i=0;
    int64_t interval = 0;
    int64_t int_val = 0;


    if_row = (struct ovsrec_interface *)p_msg->feature_row;
    port_row = port_lookup(if_row->name, p_msg->idl);
    if (!port_row)
    {
      return e_vtysh_ok;
    }

    if (!p_msg->disp_header_cfg)
    {
        vtysh_ovsdb_cli_print(p_msg, "interface %s", if_row->name);
        p_msg->disp_header_cfg = true;
    }

    interval = ospf_get_port_intervals(port_row,
                                       OSPF_KEY_HELLO_INTERVAL);
    if ((interval > 0) && (interval != OSPF_HELLO_INTERVAL_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf hello-interval", interval);

    interval = ospf_get_port_intervals(port_row, OSPF_KEY_DEAD_INTERVAL);
    if ((interval > 0) && (interval != OSPF_DEAD_INTERVAL_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf dead-interval", interval);

    interval = ospf_get_port_intervals(port_row,
                                       OSPF_KEY_RETRANSMIT_INTERVAL);
    if ((interval > 0) && (interval != OSPF_RETRANSMIT_INTERVAL_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf retransmit-interval", interval);

    interval = ospf_get_port_intervals(port_row, OSPF_KEY_TRANSMIT_DELAY);
    if ((interval > 0) && (interval != OSPF_TRANSMIT_DELAY_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf transmit-delay", interval);

    if (port_row->ospf_priority &&
        (*port_row->ospf_priority != OSPF_ROUTER_PRIORITY_DEFAULT))
    {
        int_val = *port_row->ospf_priority;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf priority", int_val);
    }

    if ((port_row->n_ospf_mtu_ignore > 0) &&
        (*port_row->ospf_mtu_ignore == true))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ",
                              "ip ospf mtu-ignore");

    if (port_row->ospf_if_out_cost &&
        (*port_row->ospf_if_out_cost != OSPF_DEFAULT_COST))
    {
        int_val = *port_row->ospf_if_out_cost;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ",
                              "ip ospf cost", int_val);
    }

    if ((port_row->ospf_if_type) &&
        (strcmp(port_row->ospf_if_type,
                OVSREC_PORT_OSPF_IF_TYPE_OSPF_IFTYPE_POINTOPOINT) == 0))
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", " ",
                              "ip ospf network", "point-to-point");


    if (port_row->ospf_auth_type)
    {
        if (!strcmp(port_row->ospf_auth_type, OVSREC_PORT_OSPF_AUTH_TYPE_TEXT))
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ", "ip ospf authentication");
        else if (!strcmp(port_row->ospf_auth_type,
                         OVSREC_PORT_OSPF_AUTH_TYPE_MD5))
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ",
                                  "ip ospf authentication message-digest");
        else if (!strcmp(port_row->ospf_auth_type,
                         OVSREC_PORT_OSPF_AUTH_TYPE_NULL))
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", " ",
                                  "ip ospf authentication null");
    }

    for (i = 0; i < port_row->n_ospf_auth_md5_keys; i++)
    {
        vtysh_ovsdb_cli_print(p_msg, "%4sip ospf message-digest-key %d md5 %s",
                              port_row->key_ospf_auth_md5_keys[i],
                              port_row->value_ospf_auth_md5_keys[i]);

    }

    if (port_row->ospf_auth_text_key)
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", " ",
                              "ip ospf authentication-key",
                              port_row->ospf_auth_text_key);

    return e_vtysh_ok;
}

/*
 * Function : vtysh_init_ospf_context_clients.
 * Responsibility : Registers the client callback routines for ospf context.
 * Return : On success, returns e_vtysh_ok. On failure, returns erro_no.
 */
int
vtysh_init_intf_ospf_context_clients()
{
  vtysh_ret_val retval = e_vtysh_error;

  retval = install_show_run_config_subcontext(e_vtysh_interface_context,
                                   e_vtysh_interface_context_ospf,
                                   &vtysh_intf_context_ospf_clientcallback,
                                   NULL, NULL);
  if(e_vtysh_ok != retval)
  {
      vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                      "Interface context unable to add OSPF client callback");
      assert(0);
  }

  return e_vtysh_ok;
}
