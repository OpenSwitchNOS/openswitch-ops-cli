/* SFLOW CLI commands
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 * File: sflow_vty.h
 *
 * Purpose:  To add SFLOW CLI configuration and display commands.
 */
#ifndef _SFLOW_VTY_H
#define _SFLOW_VTY_H
#define IPV6_ADDRESS_LENGTH 49
#define MAX_PORT_SIZE 5
#define IP_ADDRESS_LENGTH 18

void
sflow_vty_init(void);

#endif
