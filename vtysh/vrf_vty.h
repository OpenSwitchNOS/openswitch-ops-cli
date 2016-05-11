/*
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
 */
/****************************************************************************
 * @ingroup quagga/vtysh
 *
 * @file vrf_vty.h
 * To add declarations required for vrf_vty.c
 *
 ***************************************************************************/

#ifndef _VRF_VTY_H
#define _VRF_VTY_H

#define VRF_NAME_MAX_LENGTH 32

void
vrf_vty_init(void);

const struct ovsrec_port* port_check_and_add(const char *port_name, bool create,
                                             bool attach_to_default_vrf,
                                             struct ovsdb_idl_txn *txn);

const struct ovsrec_vrf* port_vrf_lookup(const struct ovsrec_port *port_row);

int
vrf_add_port (const char *if_name, const char *vrf_name);

int
vrf_del_port (const char *if_name, const char *vrf_name);

bool check_ip_addr_duplicate (const char *ip_address,
                              const struct ovsrec_port *port_row, bool ipv6,
                              bool *secondary);
bool check_split_iface_conditions (const char *ifname);
#endif /* _VRF_VTY_H */
