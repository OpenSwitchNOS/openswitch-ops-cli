/* VRF daemon client callback registration source files.
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
 * File: vtysh_ovsdb_vrf_context.c
 *
 * Purpose: Source for registering sub-context callback.
 */

#include "vty.h"
#include "vector.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/utils/system_vtysh_utils.h"
#include "vtysh_ovsdb_vrf_context.h"
#include "utils/vlan_vtysh_utils.h"

/*-----------------------------------------------------------------------------
| Function : port_vrf_match
| Responsibility : Lookup VRF to which interface is connected
| Parameters :
|    const struct ovsdb_idl *idl : idl for vtysh
|    const struct ovsrec_port *port_row: pointer to port_row for looking up VRF
| Return : pointer to VRF row
-----------------------------------------------------------------------------*/
const struct ovsrec_vrf* port_vrf_match(const struct ovsdb_idl *idl,
                                  const struct ovsrec_port *port_row)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    size_t i;
    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
      for (i = 0; i < vrf_row->n_ports; i++) {
        if (vrf_row->ports[i] == port_row) {
          return vrf_row;
        }
      }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function       : display_udpfwd_info
| Responsibility : To display UDP Bcast forward-protocol related details
| Parameters     :
|    *if_name    : Name of interface
|     p_msg      : Used for idl operations
-----------------------------------------------------------------------------*/
static void
display_udpfwd_info (const char *if_name, vtysh_ovsdb_cbmsg_ptr p_msg)
{
    const struct ovsrec_udp_bcast_forwarder_server *row_serv = NULL;
    const struct ovsdb_datum *datum = NULL;
    int index = 0;
    char *serverip = NULL;
    size_t i = 0;

    OVSREC_UDP_BCAST_FORWARDER_SERVER_FOR_EACH (row_serv, p_msg->idl)
    {
        if (row_serv->src_port)
        {
            if (!strcmp(row_serv->src_port->name, if_name))
            {
                for (i = 0; i < row_serv->n_ipv4_ucast_server; i++)
                {
                    serverip = row_serv->ipv4_ucast_server[i];
                    datum =
                        ovsrec_udp_bcast_forwarder_server_get_udp_dport
                                (row_serv, OVSDB_TYPE_INTEGER);
                    if ((NULL != datum) && (datum->n > 0))
                    {
                        index = datum->keys[0].integer;
                    }
                    /* UDP Broadcast Forwarder inforamtion. */
                    vtysh_ovsdb_cli_print(p_msg, "%4s%s %s %d", "",
                        "ip forward-protocol udp", serverip, index);
                }
            }
        }
    }

    return;
}

/*-----------------------------------------------------------------------------
| Function       : display_helper_address_info
| Responsibility : To display dhcp-relay helper-address details
| Parameters     :
|    *if_name    : Name of interface
|     p_msg      : Used for idl operations
-----------------------------------------------------------------------------*/
static void
display_helper_address_info (const char *if_name, vtysh_ovsdb_cbmsg_ptr p_msg)
{
    const struct ovsrec_dhcp_relay *row_serv;
    char *helper_ip = NULL;
    size_t i = 0;

    /* Displaying the dhcp-relay helper addresses  */
    OVSREC_DHCP_RELAY_FOR_EACH (row_serv, p_msg->idl)
    {
        /* get the interface details. */
        if(row_serv->port)
        {
            if (!strcmp(row_serv->port->name, if_name))
            {
                for (i = 0; i < row_serv->n_ipv4_ucast_server; i++)
                {
                    helper_ip = row_serv->ipv4_ucast_server[i];
                    vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "",
                        "ip helper-address", helper_ip);
                }
            }
        }
    }

    return;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_intf_context_vrf_clientcallback
| Responsibility : Interface context, VRF sub-context callback routine.
| Parameters :
|     p_private: Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
|                structure object.
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_intf_context_vrf_clientcallback(void *p_private)
{
  const struct ovsrec_port *port_row;
  const struct ovsrec_vrf *vrf_row;
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_interface *ifrow = NULL;
  size_t i;

  ifrow = (struct ovsrec_interface *)p_msg->feature_row;
  port_row = port_lookup(ifrow->name, p_msg->idl);
  if (!port_row) {
    return e_vtysh_ok;
  }
  if (check_iface_in_vrf(ifrow->name)) {
    vrf_row = port_vrf_match(p_msg->idl, port_row);
    if (NULL != vrf_row) {
      if (display_l3_info(port_row, vrf_row)) {
        if (!p_msg->disp_header_cfg) {
          vtysh_ovsdb_cli_print(p_msg, "interface %s", ifrow->name);
        }
        if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) != 0) {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "vrf attach ",
                                vrf_row->name);
        }
        if (port_row->ip4_address) {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "ip address ",
                                port_row->ip4_address);
        }
        for (i = 0; i < port_row->n_ip4_address_secondary; i++) {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s%s", "", "ip address ",
                  port_row->ip4_address_secondary[i], " secondary");
        }
        if (port_row->ip6_address) {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "ipv6 address ",
                                port_row->ip6_address);
        }
        for (i = 0; i < port_row->n_ip6_address_secondary; i++) {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s%s", "", "ipv6 address ",
                  port_row->ip6_address_secondary[i], " secondary");
        }
        if (smap_get(&port_row->other_config, PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED)) {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "ip proxy-arp");
        }
        if (smap_get(&port_row->other_config, PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED)) {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "ip local-proxy-arp");
        }
        display_helper_address_info(ifrow->name, p_msg);
        display_udpfwd_info(ifrow->name, p_msg);
      }
      else
      {
          if (!p_msg->disp_header_cfg)
          {
              vtysh_ovsdb_cli_print(p_msg, "interface %s", ifrow->name);
          }
          display_helper_address_info(ifrow->name, p_msg);
          display_udpfwd_info(ifrow->name, p_msg);
      }
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_config_context_vrf_clientcallback
| Responsibility : vrf client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_vrf_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_vrf *vrf_row = NULL;
  OVSREC_VRF_FOR_EACH(vrf_row, p_msg->idl){
    if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
      continue;
    }
    vtysh_ovsdb_cli_print(p_msg, "vrf %s", vrf_row->name);
  }
  return e_vtysh_ok;
}
