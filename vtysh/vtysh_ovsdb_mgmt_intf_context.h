/* Management interface client callback resigitration header file.
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: vtysh_ovsdb_mgmt_intf_context.h
 *
 * Purpose:  To add declarations required for vtysh_ovsdb_mgmt_intf_context.c
 *
 */

#ifndef VTYSH_OVSDB_MGMT_INTF_CONTEXT_H
#define VTYSH_OVSDB_MGMT_INTF_CONTEXT_H

int vtysh_init_mgmt_intf_context_clients(void);
vtysh_ret_val vtysh_mgmt_intf_context_clientcallback(void *p_private);

#endif /* VTYSH_OVSDB_MGMT_INTF_CONTEXT_H */
