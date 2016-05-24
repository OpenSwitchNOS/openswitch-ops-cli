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

#include <ctype.h>

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

/* Extract the interface type from interface name */
static char *
extract_intf_type (const char *intf_name)
{
    const char *p = intf_name;
    int count = 0;

    while (*p) {
        if (isdigit (*p)) {
            break;
        }
        else {
            p ++;
            count ++;
        }
    }
    char *intf_type = (char *) malloc(sizeof (char) * (count + 1));
    strncpy (intf_type, intf_name, count);
    intf_type[count] = '\0';

    return intf_type;
}

/* Extract the interface id from the interface-name */
static unsigned long
extract_tag_from_intf_name (const char *intf_name)
{
    unsigned long val = 0;
    const char *p = intf_name;
    while (*p) {
        if (isdigit(*p)) {
             val = val*10 + (*p-'0');
        }
        p++;
   }
   return val;
}


static int
compare_both_proper_intf_names (const char *name_intf1, const char *name_intf2)
{
    char *type_intf1 = extract_intf_type (name_intf1);
    char *type_intf2 = extract_intf_type (name_intf2);

    int str_comparison_result = strcmp (type_intf1, type_intf2);
    free(type_intf1);
    free(type_intf2);
    if (str_comparison_result == 0)
    {
        unsigned long tag_intf1 = extract_tag_from_intf_name (name_intf1);
        unsigned long tag_intf2 = extract_tag_from_intf_name (name_intf2);

        if (tag_intf1 == tag_intf2)
            return 0;
        else if (tag_intf1 < tag_intf2)
            return -1;
        else
            return 1;
     }
     else if (str_comparison_result < 0) {
        return -1;
     }
     else {
        return 1;
     }
}

static int
compare_both_intf_numbers (const char *name_intf1, const char *name_intf2)
{
     uint id_intf1 = 0, id_intf2 = 0;
     uint ext_id_intf1 = 0, ext_id_intf2 = 0;
     unsigned long tag_sub_intf1 = 0, tag_sub_intf2 = 0;

     /* To  handle cases like 10, 54-2, 52-1.1 etc. */
     sscanf(name_intf1, "%d-%d.%lu", &id_intf1, &ext_id_intf1, &tag_sub_intf1);
     sscanf(name_intf2, "%d-%d.%lu", &id_intf2, &ext_id_intf2, &tag_sub_intf2);

     /* To handle cases like 10.1, 54.2 etc. */
     if (strchr (name_intf1, '.') && !strchr (name_intf1, '-')) {
         sscanf(name_intf1, "%d.%lu", &id_intf1, &tag_sub_intf1);
     }

     if (strchr (name_intf2, '.') && !strchr (name_intf2, '-')) {
         sscanf(name_intf2, "%d.%lu", &id_intf2, &tag_sub_intf2);
     }

     if(id_intf1 == id_intf2)
     {
        if(ext_id_intf1 == ext_id_intf2)
        {
           /* For proper positioning of subinterfaces */
           if (tag_sub_intf1 == tag_sub_intf2)
               return 0;
           else if (tag_sub_intf1 < tag_sub_intf2)
               return -1;
           else
               return 1;
        }
        else if(ext_id_intf1 < ext_id_intf2)
           return -1;
        else
           return 1;
     }
     else if (id_intf1 < id_intf2)
           return -1;
     else
           return 1;
}

static int
compare_only_one_proper_intf_name (const char *name1, const char *name2)
{
     if (isdigit(*name1))
         return -1;
     else
         return 1;
}

/*-----------------------------------------------------------------------------
| Function       : compare_nodes_vrf
| Responsibility : the function is used for comparing two VRF element
| Parameters:
|    *a_: Pointer to first VRF element
|    *b_: Pointer to second VRF element
| Return:
|    int: zero if two VRF elements are equal, 1 if the first elemnt is
|         bigger than second, -1 if the first element is smaller than second
| sort comparator function.
| Case 1: When both interface name consists of numbers only
|         Example: 10 and 12-1 etc.
| Case 2: When one interface name consists of numbers and other is proper name
|         Example: 10 and vlan20 etc.
| Case 3: When both interface names are proper names with interface type and id
|         Example: vlan20 and lo23 etc.
-----------------------------------------------------------------------------*/
int
compare_nodes_vrf (void *a_, void *b_)
{
    const struct shash_node *const *a = a_;
    const struct shash_node *const *b = b_;
    char *name_intf1 = (*a)->name;
    char *name_intf2 = (*b)->name;

    /* bridge_normal has to be at the top */
    if (!strcmp (name_intf1, "bridge_normal"))
        return -1;
    else if (!strcmp (name_intf2, "bridge_normal"))
        return 1;

    /* Case 1 */
    if (isdigit(*name_intf1) && isdigit(*name_intf2))
    {
         return compare_both_intf_numbers (name_intf1, name_intf2);
    }
    /* Case 2 */
    else if (isdigit(*name_intf1) || isdigit(*name_intf2))
    {
        return compare_only_one_proper_intf_name (name_intf1, name_intf2);
    }
    /* Case 3 */
    else
    {
        return compare_both_proper_intf_names (name_intf1, name_intf2);
    }

    return 0;
}
