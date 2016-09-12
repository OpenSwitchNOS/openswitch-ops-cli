/* Tunnel CLI utils commands header file
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * File: tunnel_vtysh_utils.h
 *
 * Purpose: This file contains comman macro for tunnel CLI.
 */

#ifndef _TUNNEL_VTYSH_UTILS_H
#define _TUNNEL_VTYSH_UTILS_H

#include <stdbool.h>

struct vty;

// const ovsrec_logical_switch *get_matching_logical_switch(int64_t tunnel_key);
// const ovsrec_vlan *get_matching_vlan(char *tunnel_name);
char *get_mode_from_line(const char *);
const struct ovsrec_interface *get_intf_by_name(const char *name);
const struct ovsrec_interface *get_intf_by_name_and_type(const char *name,
                                                         const int intf_type);
bool is_tunnel_intf_type(const struct ovsrec_interface *if_row,
                         const int intf_type);
const struct ovsrec_port *get_port_by_name(const char *name);
int txn_status_and_log(const int txn_status);
int set_intf_tunnel_ip_addr_by_type(struct vty *vty, const char *tunnel_name,
                                    const int intf_type, const char *new_ip);
int set_intf_option(const struct ovsrec_interface *if_row,
                    const char *option, const char *new_value);
int set_intf_src_ip(struct vty *vty, const struct ovsrec_interface *if_row,
                    const char *new_ip);
int set_src_intf(struct vty *vty, const struct ovsrec_interface *if_row,
                 const char *new_if);
int unset_src_intf(const struct ovsrec_interface *if_row);
int unset_intf_src_ip(const struct ovsrec_interface *if_row);
int set_intf_dest_ip(const struct ovsrec_interface *if_row, const char *new_ip);
int unset_intf_dest_ip(const struct ovsrec_interface *if_row);

#endif /* _TUNNEL_VTYSH_UTILS_H */
