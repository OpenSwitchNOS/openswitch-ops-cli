/* LLDP CLI commands header file
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
 * File: intf_vty.h
 *
 * Purpose:  To add declarations required for intf_vty.c
 */

#ifndef _INTF_VTY_H
#define _INTF_VTY_H

#define INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT                   "auto"
#define INTERFACE_USER_CONFIG_MAP_PAUSE_DEFAULT                 "none"
#define INTERFACE_USER_CONFIG_MAP_DUPLEX_DEFAULT                "full"
#define INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT                "auto"

void
intf_vty_init (void);

#endif /* _INFT_VTY_H */
