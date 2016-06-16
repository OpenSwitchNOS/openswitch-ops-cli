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
 * File: vlan_vtysh_utils.h
 *
 * Purpose: header file for common vtysh utility macros and functions
 */

#ifndef _VLAN_VTYSH_UTILS_H
#define _VLAN_VTYSH_UTILS_H

#include "ops-utils.h"

#define DEFAULT_VLAN    1

#define VERIFY_VLAN_IFNAME(s) strncasecmp(s, "vlan", 4)

#define GET_VLANIF(s, a) \
        strcpy(s, "vlan"); \
        strcat(s, (a+4));

#define VLANIF_NAME(vif, s)  \
        strcpy(vif, "vlan"); \
        strcat(vif, s);

const struct ovsrec_port* port_lookup(const char *if_name,
                                const struct ovsdb_idl *idl);
int check_internal_vlan(uint16_t vlanid);
int create_vlan_interface(const char *vlan_if);
int delete_vlan_interface(const char *vlan_if);
bool verify_ifname(char *str);

#endif /* _VLAN_VTYSH_UTILS_H  */
