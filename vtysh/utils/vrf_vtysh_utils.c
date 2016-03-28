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
 * File: vrf_vtysh_utils.c
 * Responsibility : Definition of common vrf CLI function.
 */

#include "ovsdb-idl.h"
#include "vswitch-idl.h"
#include "vrf_vtysh_utils.h"

extern struct ovsdb_idl *idl;

/*
 * Check if port is part of any VRF and return the VRF row.
 */
const struct ovsrec_vrf*
port_vrf_lookup (const struct ovsrec_port *port_row)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    size_t i;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        for (i = 0; i < vrf_row->n_ports; i++)
        {
            if (vrf_row->ports[i] == port_row)
                return vrf_row;
        }
    }
    return NULL;
}

/* qsort comparator function.
 */
int
compare_nodes_vrf(const void *a_, const void *b_)
{
    struct shash_node **a = a_;
    struct shash_node **b = b_;
    uint i1=0,i2=0;

    sscanf(((*a)->name + VRF_VLAN_INTF_ID_OFFSET), "%d", &i1);
    sscanf(((*b)->name + VRF_VLAN_INTF_ID_OFFSET), "%d", &i2);

    if (i1 == i2)
        return 0;
    else if (i1 < i2)
        return -1;
    else
        return -1;
}
