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
 */
/****************************************************************************
 * @ingroup quagga/vtysh
 *
 * @file loopback_vty.h.h
 * To add declarations required for loopback_vty.h.c
 *
 ***************************************************************************/

#ifndef _VTY_LOOPBACK_INTF_H
#define _VTY_LOOPBACK_INTF_H

#define IP_ADDRESS_LENGTH               18
#define IPV6_ADDRESS_LENGTH             49

#define MIN_LOOPBACK_INTF_RANGE         0
#define MAX_LOOPBACK_INTF_RANGE         2147483647
#define MAX_LOOPBACK_INTF_COUNT         1024

#define LPBK_OVSDB_TXN_COMMIT_ERROR  "Committing transaction to DB failed."\
                                       "Function=%s Line=%d"
#define LPBK_OVSDB_TXN_CREATE_ERROR  "Couldn't create the OVSDB transaction."\
                                       "Function=%s Line=%d"

#define LPBK_HELP_STR                 "Select loopback interface"

const struct ovsrec_port* port_check_and_add(const char *port_name, bool create,
                                             bool attach_to_default_sub_if,
                                             struct ovsdb_idl_txn *txn);

const struct shash_node **sort_interface(const struct shash *sh);

void loopback_intf_vty_init (void);
#endif /* _VTY_LOOPBACK_INTF_H */
