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
 * File: l3_vtysh_utils.h
 *
 * Purpose: This file contains common macro and function of l3 vtysh utility.
 */

#ifndef _L3_VTY_UTILS_H
#define _L3_VTY_UTILS_H

#include "vty.h"
#include "l3-utils.h"

bool
is_ip_configurable(struct vty *vty,
                   const char *ip_address,
                   const char *if_name,
                   bool ipv6,
                   bool secondary);

#endif  /*_L3_VTY_UTILS_H */
