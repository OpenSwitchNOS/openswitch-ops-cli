/*
 * Copyright (C) 2015, 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/************************************************************************//**
 * @defgroup ops-access-list Access Control List (ACL)
 * Access Control List (ACL) CLI commands and associated code.
 * See ops/doc/access_list_cli.md for command syntax/documentation.
 ***************************************************************************/

/************************************************************************//**
 * @ingroup ops-access-list
 *
 * @file
 * Definition of Access Control List (ACL) CLI functions
 ***************************************************************************/

#ifndef _ACCESS_LIST_VTY_H
#define _ACCESS_LIST_VTY_H

/* ACL Entry (ACE) key indices */
#ifndef ACE_KEY_
#define ACE_KEY_
#define ACE_KEY_ACTION                    0
#define ACE_KEY_IP_PROTOCOL               1
#define ACE_KEY_SOURCE_IP_ADDRESS         2
#define ACE_KEY_SOURCE_PORT_OPERATOR      3
#define ACE_KEY_SOURCE_PORT               4
#define ACE_KEY_SOURCE_PORT_MAX           5
#define ACE_KEY_DESTINATION_IP_ADDRESS    6
#define ACE_KEY_DESTINATION_PORT_OPERATOR 7
#define ACE_KEY_DESTINATION_PORT          8
#define ACE_KEY_DESTINATION_PORT_MAX      9
#define ACE_KEY_LOG                      10
#define ACE_KEY_COUNT                    11
#define ACE_KEY_COMMENT                  12
/* 1. New ACE keys go here ^ with incremented values
 * 2. Bump ACE_KEY_N_ to be new key's value + 1
 * 3. Add string name to ace_key_names below
 */
#define ACE_KEY_N_                       13 /**< For sizing arrays, loops */
#define ACE_KEY_MIN_                      0 /**< For checking bounds */
#define ACE_KEY_MAX_         ACE_KEY_N_ - 1 /**< For checking bounds */

/** ACL Entry (ACE) key string constants (indexed by ACE_KEY_*) */
static const char *ace_key_names[ACE_KEY_N_] = {
    "action",
    "protocol",
    "src_ip",
    "src_l4_op",
    "src_l4_port",
    "src_l4_port_max",
    "dst_ip",
    "dst_l4_op",
    "dst_l4_port",
    "dst_l4_port_max",
    "log",
    "count",
    "comment"
};
#endif

/* Keep initialization in our own module files where possible */
void access_list_vty_init(void);
void access_list_ovsdb_init(void);
vtysh_ret_val vtysh_init_access_list_context_clients(void);

#endif /* _ACCESS_LIST_VTY_H */
