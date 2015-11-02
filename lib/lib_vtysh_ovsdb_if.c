/* Vtysh daemon ovsdb integration.
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: lib_vtysh_ovsdb_if.c
 *
 * Purpose: Library functions for integrating vtysh with ovsdb.
 */

#include <stdio.h>

int (*lib_vtysh_ovsdb_interface_match)(const char *str) = NULL;
int (*lib_vtysh_ovsdb_port_match)(const char *str) = NULL;
int (*lib_vtysh_ovsdb_vlan_match)(const char *str) = NULL;
int (*lib_vtysh_ovsdb_mac_match)(const char *str) = NULL;
