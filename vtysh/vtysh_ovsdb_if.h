/* Vtysh daemon ovsdb integration.
 *
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
 * File: vtysh_ovsdb_if.h
 *
 * Purpose: Main file for integrating vtysh with ovsdb.
 */
#ifndef VTYSH_OVSDB_IF_H
#define VTYSH_OVSDB_IF_H 1

#include <stdbool.h>

#define MAX_MACADDR_LEN 17
#define DEFAULT_SESSION_TIMEOUT_PERIOD 30

void vtysh_ovsdb_init(int argc, char *argv[], char *db_name);

const char *vtysh_ovsdb_os_name_get(void);

const char *vtysh_ovsdb_switch_version_get(void);

void vtysh_ovsdb_domainname_set(const char * in);

int vtysh_ovsdb_domainname_reset(char *domainname_arg);

const char* vtysh_ovsdb_domainname_get(void);

void vtysh_ovsdb_hostname_set(const char * in);

int vtysh_ovsdb_hostname_reset(char *hostname_arg);

const char* vtysh_ovsdb_hostname_get(void);

int vtysh_ovsdb_session_timeout_set(const char * duration);

int64_t vtysh_ovsdb_session_timeout_get(void);

void vtysh_ovsdb_exit(void);

void vtysh_ovsdb_lib_init(void);

int vtysh_ovsdb_interface_match(const char *str);

int vtysh_ovsdb_port_match(const char *str);

int vtysh_ovsdb_vlan_match(const char *str);

int vtysh_regex_match(const char *regString, const char *inp);

void *vtysh_ovsdb_main_thread(void *arg);

bool check_iface_in_lag (const char *if_name);

bool check_iface_in_bridge(const char *if_name);

bool check_iface_in_vrf(const char *if_name);

bool check_port_in_bridge(const char *port_name);

bool check_port_in_vrf(const char *port_name);

bool vtysh_ovsdb_is_loaded(void);

void utils_vtysh_rl_describe_output(struct vty* vty, vector describe, int width);

extern struct ovsdb_idl_txn *txn;
const struct ovsrec_port* port_check_and_add(const char *port_name, bool create,
                                             bool attach_to_default_vrf,
                                             struct ovsdb_idl_txn *txn);

extern struct ovsrec_vlan *vlan_row;

bool check_if_internal_vlan(const struct ovsrec_vlan *vlan_row);

void vtysh_ovsdb_show_version_detail(void);

void vtysh_ovsdb_show_version_detail_ops(void);

#endif /* VTYSH_OVSDB_IF_H */
