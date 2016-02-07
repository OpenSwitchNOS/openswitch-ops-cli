/* LACP CLI utils commands header file
 *
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
 * File: lacp_vtysh_utils.h
 *
 * Purpose: This file contains comman macro for lacp CLI.
 */

#ifndef _LACP_VTYSH_UTILS_H
#define _LACP_VTYSH_UTILS_H

#define LACP_STATUS_FIELD_COUNT 8
#define LAG_PORT_NAME_PREFIX "lag"
#define LACP_DEFAULT_SYS_PRIORITY_LENGTH 6
#define LAG_PORT_NAME_PREFIX_LENGTH 3

#define LACP_OVSDB_TXN_CREATE_ERROR "Couldn't create the OVSDB transaction.Function=%s Line=%d"
#define LACP_OVSDB_ROW_FETCH_ERROR  "Couldn't fetch row from the DB.Function=%s Line=%d"
#define LACP_OVSDB_TXN_COMMIT_ERROR "Committing transaction to DB failed.Function=%s Line=%d"
#define LACP_STR "Configure LACP parameters\n"
#define MAX_INTF_TO_LAG 8
#define MAX_LAG_INTERFACES 256
#define LACP_DEFAULT_PORT_PRIORITY 1

extern int maximum_lag_interfaces;

#endif /* _LACP_VTYSH_UTILS_H */
