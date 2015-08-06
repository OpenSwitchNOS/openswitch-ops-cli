/* BGP CLI implementation with Halon vtysh.
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
 * File: bgp_vty.h
 *
 * Purpose: This file contains function declarations of BGP CLI.
 */
#ifndef _BGP_VTY_H
#define _BGP_VTY_H

#define CMD_AS_RANGE "<1-4294967295>"
#define NETWORK_MAX_LEN 49

void bgp_vty_init (void);

#endif /* _BGP_VTY_H */
