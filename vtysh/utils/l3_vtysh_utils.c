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
 * File: l3_vtysh_utils.c
 * Responsibility : Definition of common l3 CLI function.
 */

#include <stdbool.h>
#include "openswitch-idl.h"
#include "vrf-utils.h"
#include "l3_vtysh_utils.h"

#define PORT_NAME_MAX_LEN 32

extern struct ovsdb_idl *idl;

/*
 * Given a port name get the corresponding vrf row
 */
const struct
ovsrec_vrf* get_vrf_row_for_port(const char *port_name)
{
  int iter, count;
  const struct ovsrec_vrf *row_vrf = NULL;

  OVSREC_VRF_FOR_EACH (row_vrf, idl)
    {
      count = row_vrf->n_ports;
      for (iter = 0; iter < count; iter++)
        {
          if (0 == strncmp(port_name, row_vrf->ports[iter]->name,
                                                  PORT_NAME_MAX_LEN))
            return row_vrf;
        }

    }
  return row_vrf;
}

bool
is_ip_configurable(struct vty *vty,
                   const char *ip_address,
                   const char *if_name,
                   bool ipv6,
                   bool secondary)
{
    const struct ovsrec_vrf *vrf_row = NULL;

    if (if_name)
        vrf_row = get_vrf_row_for_port(if_name);

    if (!vrf_row)
        return false;

    if (l3_utils_validate_ip_addr(ip_address, if_name, ipv6, secondary, vrf_row))
    {
        vty_out(vty, "Overlapping networks observed for \"%s\"."
                     " Please configure non overlapping networks.%s",
                     ip_address, VTY_NEWLINE);
        return true;
    }
    return false;
}
