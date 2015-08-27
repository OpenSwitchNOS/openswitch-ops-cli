/* LLDP CLI commands header file
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: intf_vty.h
 *
 * Purpose:  To add declarations required for intf_vty.c
 */

#ifndef _INTF_VTY_H
#define _INTF_VTY_H

#define OVSDB_TXN_CREATE_ERROR "Couldn't create the OVSDB transaction."
#define OVSDB_ROW_FETCH_ERROR "Couldn't fetch row from the DB."
#define OVSDB_TXN_COMMIT_ERROR "Commiting transaction to DB failed."

#define INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT                "auto"
#define INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT                   "auto"
#define INTERFACE_USER_CONFIG_MAP_PAUSE_DEFAULT                 "none"
#define INTERFACE_USER_CONFIG_MAP_DUPLEX_DEFAULT                "full"

#ifdef ENABLE_OVSDB
#define VERIFY_VLAN_IFNAME(s) strncasecmp(s, "vlan", 4)

#define GET_VLANIF(s, a) \
        strcpy(s, "vlan"); \
        strcat(s, (a+4));

#define VLANIF_NAME(vif, s)  \
        strcpy(vif, "vlan"); \
        strcat(vif, s);

void
intf_vty_init (void);
int create_vlan_interface(const char *vlan_if);
int delete_vlan_interface(const char *vlan_if);
bool verify_ifname(char *str);
#endif

#endif /* _INFT_VTY_H */
