/* FAN CLI commands header file
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
 * File: fan_vty.h
 *
 * Purpose:  To add declarations required for fan_vty.c
 */

#ifndef _FAN_VTY_H
#define _FAN_VTY_H

#ifndef SYS_STR
#define SYS_STR "System information\n"
#endif

#define FAN_STR "Fan information\n"
#define FAN_SET_STR "Override fan speed\n"
#define FAN_SPEED_OVERRIDE_STR "fan_speed_override"

void
fan_vty_init (void);

#endif /* _FAN_VTY_H */
