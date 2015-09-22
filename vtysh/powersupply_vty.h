/* Power Supply CLI commands.
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
 * File: powersupply_vty.h
 *
 * Purpose: To add power supply CLI configuration and display commands.
 */

#ifndef _POWERSUPPLY_VTY_H
#define _POWERSUPPLY_VTY_H

#ifndef SYS_STR
#define SYS_STR         "System information\n"
#endif
#define PSU_STR         "Power supply information\n"

int
cli_system_get_psu();

void
powersupply_vty_init();

#endif //_POWERSUPPLY_VTY_H
