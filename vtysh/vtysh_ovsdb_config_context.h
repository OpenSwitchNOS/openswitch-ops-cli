/*
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
 */
/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_config_context.h
 * Source for registering client callback with config context.
 *
 ***************************************************************************/

#ifndef VTYSH_OVSDB_CONFIG_CONTEXT_H
#define VTYSH_OVSDB_CONFIG_CONTEXT_H

int vtysh_init_config_context_clients();
vtysh_ret_val vtysh_config_context_global_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_vrf_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_fan_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_led_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_staticroute_clientcallback(void *p_private);

#endif /* VTYSH_OVSDB_CONFIG_CONTEXT_H */
