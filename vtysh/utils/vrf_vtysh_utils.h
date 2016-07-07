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
 * File: vrf_vtysh_utils.h
 *
 * Purpose: This file contains common macro and function of vrf vtysh utility.
 */

#ifndef _VRF_VTY_UTILS_H
#define _VRF_VTY_UTILS_H

#include "ops-utils.h"

const struct ovsrec_vrf* port_vrf_lookup(const struct ovsrec_port *port_row);

int compare_nodes_vrf(const void *a_, const void *b_);
int compare_interface_nodes_vrf(const void *a_, const void *b_);

extern int
ops_sort(const struct shash *sh, void *ptr_func_sort,
         const struct shash_node ** sorted_list);

#endif  /*_VRF_VTY_UTILS_H */
