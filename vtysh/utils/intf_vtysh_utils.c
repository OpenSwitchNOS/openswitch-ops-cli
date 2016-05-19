/*
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
 *
 * @file intf_vtysh_utils.c
 ***************************************************************************/

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh.h"
#include "vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh_utils.h"
#include "utils/vlan_vtysh_utils.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/utils/intf_vtysh_utils.h"

VLOG_DEFINE_THIS_MODULE(intf_vtysh_utils);

extern struct ovsdb_idl *idl;

/*-----------------------------------------------------------------------------
  | Function : show_ip_addresses
  | Responsibility : Used to show ip addresses for L3 interfaces
  | Parameters :
  |     const char *if_name           : Name of interface
  |     struct vty* vty               : Used for ouput
  -----------------------------------------------------------------------------*/
int
show_ip_addresses(const char *if_name, struct vty *vty)
{
    const struct ovsrec_port *port_row;
    const struct ovsrec_vrf *vrf_row;
    size_t i;

    port_row = port_find(if_name);
    if (!port_row) {
        return 0;
    }

    if (check_iface_in_vrf(if_name)) {
        vrf_row = port_match_in_vrf(port_row);
        if (display_l3_info(port_row, vrf_row)) {
            if (port_row->ip4_address) {
                vty_out(vty, " IPv4 address %s%s", port_row->ip4_address,
                        VTY_NEWLINE);
            }
            for (i = 0; i < port_row->n_ip4_address_secondary; i++) {
                vty_out(vty, " IPv4 address %s secondary%s",
                        port_row->ip4_address_secondary[i],
                        VTY_NEWLINE);
            }
            if (port_row->ip6_address) {
                vty_out(vty, " IPv6 address %s%s", port_row->ip6_address,
                        VTY_NEWLINE);
            }
            for (i = 0; i < port_row->n_ip6_address_secondary; i++) {
                vty_out(vty, " IPv6 address %s secondary%s",
                        port_row->ip6_address_secondary[i],
                        VTY_NEWLINE);
            }
        }
    }
    return 0;
}

/*
 *  Function : port_find
 *  Responsibility : Lookup port table entry for interface name
 *  Parameters :
 *    const char *if_name : Interface name
 *  Return : bool : returns true/false
*/
const struct ovsrec_port*
port_find(const char *if_name)
{
    const struct ovsrec_port *port_row = NULL;
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, if_name) == 0) {
            return port_row;
        }
    }
    return NULL;
}

/*
 * Function : port_match_in_vrf
 * Responsibility : Lookup VRF to which interface is connected
 * Parameters :
 *   const struct ovsrec_port *port_row: pointer to port_row
 *                                       for looking up VRF
 * Return : pointer to VRF row
 */
const struct ovsrec_vrf*
port_match_in_vrf (const struct ovsrec_port *port_row)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  size_t i;
  OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
      for (i = 0; i < vrf_row->n_ports; i++)
        {
          if (vrf_row->ports[i] == port_row)
            return vrf_row;
        }
    }
  return NULL;
}

/*-----------------------------------------------------------------------------
| Function : display_l3_info
| Responsibility : Decide if L3 info needs to be printed
| Parameters :
|   const struct ovsrec_interface *if_row : Interface row data
|   const struct ovsrec_vrf *vrf_row : VRF row data
| Return : bool : returns true/false
-----------------------------------------------------------------------------*/
bool
display_l3_info(const struct ovsrec_port *port_row,
                const struct ovsrec_vrf *vrf_row)
{
   if (port_row->ip4_address || (port_row->n_ip4_address_secondary > 0)
        || port_row->ip6_address || (port_row->n_ip6_address_secondary > 0)
        || smap_get(&port_row->other_config, PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED)
        || smap_get(&port_row->other_config,
                    PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED)
        || (strcmp(vrf_row->name, DEFAULT_VRF_NAME) != 0)) {
     return true;
   }
   return false;
}

/*-----------------------------------------------------------------------------
| Function : show_l3_interface_rx_stats
| Responsibility : Display L3 RX statistics
| Parameters :
|   struct vty* vty            : Used for ouput
|   const struct ovsdb_*datum  : Statitistics data
| Return : void
-----------------------------------------------------------------------------*/
void
show_l3_interface_rx_stats(struct vty *vty, const struct ovsdb_datum *datum)
{
    union ovsdb_atom atom;
    unsigned int index;

    char *if_l3_rx_stat_keys [] = {
        "l3_uc_rx_packets",
        "l3_uc_rx_bytes",
        "l3_mc_rx_packets",
        "l3_mc_rx_bytes"
    };

    vty_out(vty, "       L3:%s", VTY_NEWLINE);
    atom.string = if_l3_rx_stat_keys[0];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "            ucast: %"PRIu64" packets, ",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    atom.string = if_l3_rx_stat_keys[1];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "%"PRIu64" bytes",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    vty_out(vty, "%s", VTY_NEWLINE);

    atom.string = if_l3_rx_stat_keys[2];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "            mcast: %"PRIu64" packets, ",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    atom.string = if_l3_rx_stat_keys[3];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "%"PRIu64" bytes",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    vty_out(vty, "%s", VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Function : show_l3_interface_tx_stats
| Responsibility : Display L3 TX statistics
| Parameters :
|   struct vty* vty            : Used for ouput
|   const struct ovsdb_*datum  : Statitistics data
| Return : void
-----------------------------------------------------------------------------*/
void
show_l3_interface_tx_stats(struct vty *vty, const struct ovsdb_datum *datum)
{
    union ovsdb_atom atom;
    unsigned int index;

    char *if_l3_rx_stat_keys [] = {
        "l3_uc_tx_packets",
        "l3_uc_tx_bytes",
        "l3_mc_tx_packets",
        "l3_mc_tx_bytes"
    };

    vty_out(vty, "       L3:%s", VTY_NEWLINE);
    atom.string = if_l3_rx_stat_keys[0];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "            ucast: %"PRIu64" packets, ",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    atom.string = if_l3_rx_stat_keys[1];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "%"PRIu64" bytes",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    vty_out(vty, "%s", VTY_NEWLINE);

    atom.string = if_l3_rx_stat_keys[2];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "            mcast: %"PRIu64" packets, ",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    atom.string = if_l3_rx_stat_keys[3];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    vty_out(vty, "%"PRIu64" bytes",
            (index == UINT_MAX)? 0 : datum->values[index].integer);
    vty_out(vty, "%s", VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Function : show_l3_stats
| Responsibility : Display L3 interface statistics
| Parameters :
|   struct vty* vty                : Used for ouput
|   struct ovsrec_interface *ifrow : Interface row
| Return : void
-----------------------------------------------------------------------------*/
void
show_l3_stats(struct vty *vty, const struct ovsrec_interface *ifrow)
{
    const struct ovsdb_datum *datum;

    datum = ovsrec_interface_get_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);
    if (NULL == datum)
      return;

    vty_out(vty, " RX%s", VTY_NEWLINE);
    show_l3_interface_rx_stats(vty, datum);
    vty_out(vty, " TX%s", VTY_NEWLINE);
    show_l3_interface_tx_stats(vty, datum);
    vty_out(vty, "%s", VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Function : interface_find
| Responsibility : Lookup interface table entry
| Parameters :
|   const chat *ifname : Interface name
| Return : const struct ovsrec_interface*
-----------------------------------------------------------------------------*/
const struct ovsrec_interface*
interface_find(const char *ifname)
{
    const struct ovsrec_interface *row;

    if (!ifname) {
        return NULL;
    }
    // Search for each interface
    OVSREC_INTERFACE_FOR_EACH(row, idl)
    {
        if ( strcmp(ifname,row->name) == 0) {
            return row;
        }
    }
    return NULL;
}
