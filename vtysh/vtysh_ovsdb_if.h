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
 * File: vtysh_ovsdb_if.h
 *
 * Purpose: Main file for integrating vtysh with ovsdb.
 */
#ifndef VTYSH_OVSDB_IF_H
#define VTYSH_OVSDB_IF_H 1

#define MAX_CLI_SESSIONS 16
#define SESSION_CNT_LENGTH 3
#define OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS "cli_num_sessions"

void vtysh_ovsdb_init(int argc, char *argv[]);

void vtysh_ovsdb_hostname_set(const char * in);

char* vtysh_ovsdb_hostname_get(void);

void vtysh_ovsdb_exit(void);

void vtysh_ovsdb_lib_init(void);

int vtysh_ovsdb_interface_match(const char *str);

int vtysh_ovsdb_port_match(const char *str);

int vtysh_ovsdb_vlan_match(const char *str);

int vtysh_regex_match(const char *regString, const char *inp);
#endif /* VTYSH_OVSDB_IF_H */
