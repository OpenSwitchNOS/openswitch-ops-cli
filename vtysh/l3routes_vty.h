/*
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 * @ingroup quagga
 *
 * @file l3routes_vty.h
 * Source for config l3 static routes into ovsdb tables.
 *
 ***************************************************************************/

#ifndef _L3ROUTES_VTY_H
#define _L3ROUTES_VTY_H

void
l3routes_vty_init (void);

#define DEFAULT_DISTANCE  1
#define MAX_ADDRESS_LEN   256
/* Loopback range lies from 127.0.0.0 - 127.255.255.255
 * Thus, not using the macro 'IS_LOOPBACK_IPV4(i)' defined in vtysh.h */
#define  IS_LOOPBACK_IPV4_ADDRESS(i)  (((long)(i) & 0xff000000) == 0x7f000000)

#endif /* _L3ROUTES_VTY_H */
