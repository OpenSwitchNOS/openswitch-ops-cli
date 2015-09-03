/* LACP CLI commands header file
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
 * File: lacp_vty.h
 *
 * Purpose:  To add declarations required for lacp_vty.c
 */

#ifndef _LACP_VTY_H
#define _LACP_VTY_H

#define LACP_DEFAULT_SYS_PRIORITY_LENGTH 6
#define LAG_PORT_NAME_PREFIX "lag"
#define LAG_PORT_NAME_PREFIX_LENGTH 3

#define LACP_OVSDB_TXN_CREATE_ERROR "Couldn't create the OVSDB transaction.Function=%s Line=%d"
#define LACP_OVSDB_ROW_FETCH_ERROR  "Couldn't fetch row from the DB.Function=%s Line=%d"
#define LACP_OVSDB_TXN_COMMIT_ERROR "Committing transaction to DB failed.Function=%s Line=%d"
#define LACP_STR "Configure LACP parameters.\n"
#define MAX_INTF_TO_LAG 8
#define MAX_LAG_INTERFACES 256

extern int maximum_lag_interfaces;
void
lacp_vty_init (void);

#endif /* _LACP_VTY_H */
