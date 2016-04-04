/*
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: intf_vtysh_utils.h
 *
 * Purpose: header file for common l2 and l3 interface vtysh utility
 * macros and functions.
 */

#ifndef _INTF_VTYSH_UTILS_H
#define _INTF_VTYSH_UTILS_H

#define PRINT_INTERFACE_NAME(name_written, p_msg, if_name)\
  if (!(name_written))\
  {\
    vtysh_ovsdb_cli_print(p_msg, "interface %s", if_name);\
    name_written = true;\
  }

int show_ip_addresses(const char *if_name, struct vty *vty);

const struct ovsrec_port* port_find(const char *if_name);

const struct ovsrec_vrf* port_match_in_vrf(const struct ovsrec_port *port_row);

bool display_l3_info(const struct ovsrec_port *port_row,
                const struct ovsrec_vrf *vrf_row);

/*TODO :Below code will be remove as part of show running infra*/
int cli_show_subinterface_row(const struct ovsrec_interface *ifrow,\
                               bool brief);

void show_subinterface_status(const struct ovsrec_interface *ifrow, bool brief,
        const struct ovsrec_interface *if_parent_row, int64_t key_subintf_parent);

int delete_sub_intf(const char *sub_intf_name);

void show_l3_stats(struct vty *vty, const struct ovsrec_interface *ifrow);
void show_l3_interface_rx_stats(struct vty *vty, const struct ovsdb_datum *datum);
void show_l3_interface_tx_stats(struct vty *vty, const struct ovsdb_datum *datum);
const struct ovsrec_interface* interface_find(const char *ifname);
#endif /* _INTF_VTYSH_UTILS_H  */
