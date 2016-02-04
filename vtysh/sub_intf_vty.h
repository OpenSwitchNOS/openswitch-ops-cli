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
 * @file sub_intf_vty.h
 * To add declarations required for sub_if_vty.c
 *
 ***************************************************************************/

#ifndef _VTY_SUB_INTF_H
#define _VTY_SUB_INTF_H

#define SUB_IF_NAME_MAX_LENGTH          32
#define IP_ADDRESS_LENGTH               18
#define IPV6_ADDRESS_LENGTH             49

#define MIN_PHY_INTF                    1
#define MAX_PHY_INTF                    24
#define MIN_SUB_INTF_RANGE              1
#define MAX_SUB_INTF_RANGE              4294967293
#define MAX_SUB_INTF_COUNT              1024


#define SUB_IF_OVSDB_TXN_COMMIT_ERROR "Committing transaction to DB failed." \
                                      "Function=%s Line=%d"

#define SUB_IF_OVSDB_TXN_CREATE_ERROR "Could not create the OVSDB" \
                                      "transaction.Function=%s Line=%d"

#define INTF_HELP_STR                 "Interface to create"
void sub_if_vty_init(void);
void sub_intf_vty_init(void);
void encapsulation_vty_init(void);
int delete_sub_intf(const char *sub_intf_name);

const struct ovsrec_port* port_check_and_add(const char *port_name, bool create,
                                             bool attach_to_default_sub_if,
                                             struct ovsdb_idl_txn *txn);

const struct ovsrec_sub_if* port_sub_if_lookup(const struct ovsrec_port *port_row);

const struct ovsrec_sub_if* sub_if_lookup(const char *sub_if_name);

#endif /* _VTY_SUB_INTF_H */
