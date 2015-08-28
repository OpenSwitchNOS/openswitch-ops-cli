/* Vtysh daemon ovsdb integration.
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
 * File: lib_vtysh_ovsdb_if.h
 *
 * Purpose: Library file for integrating vtysh with ovsdb.
 */
#ifndef LIB_VTYSH_OVSDB_IF_H
#define LIB_VTYSH_OVSDB_IF_H 1

extern int (*lib_vtysh_ovsdb_interface_match)(const char *str);
extern int (*lib_vtysh_ovsdb_port_match)(const char *str);
extern int (*lib_vtysh_ovsdb_vlan_match)(const char *str);
extern int (*lib_vtysh_ovsdb_mac_match)(const char *str);

#endif /* LIB_VTYSH_OVSDB_IF_H */
