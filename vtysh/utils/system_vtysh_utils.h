/*
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
 * File: system_vtysh_utils.h
 *
 * Purpose: header file for common system vtysh utility macros and functions
 */

#ifndef _SYSTEM_VTYSH_UTILS_H
#define _SYSTEMVTYSH_UTILS_H

#define FAN_SPEED_OVERRIDE_STR "fan_speed_override"

const char **psu_state_string;

#define POWER_SUPPLY_FAULT_ABSENT  "Absent"
#define POWER_SUPPLY_FAULT_INPUT   "Input Fault"
#define POWER_SUPPLY_FAULT_OUTPUT  "Output Fault"
#define POWER_SUPPLY_OK            "OK"
#define POWER_SUPPLY_UNKNOWN       "Unknown"

#endif /* _SYSTEM_VTYSH_UTILS_H  */
