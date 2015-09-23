/* FAN CLI commands header file
 *
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

void fan_vty_init (void);

#endif /* _FAN_VTY_H */
