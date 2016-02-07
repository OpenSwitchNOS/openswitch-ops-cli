/* Interface CLI commands header file
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
 * File: intf_vty.h
 *
 * Purpose:  To add declarations required for intf_vty.c
 */

#ifndef _INTF_VTY_H
#define _INTF_VTY_H

#define INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT                   "auto"
#define INTERFACE_USER_CONFIG_MAP_PAUSE_DEFAULT                 "none"
#define INTERFACE_USER_CONFIG_MAP_DUPLEX_DEFAULT                "full"
#define INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT                "auto"
#define INTERFACE_USER_CONFIG_MAP_AUTONEG_DEFAULT               "on"

#ifdef ENABLE_OVSDB
void
intf_vty_init (void);
void dyncb_helpstr_speeds(struct cmd_token *token, struct vty *vty, \
                          char * const helpstr, int max_len);
void dyncb_helpstr_mtu(struct cmd_token *token, struct vty *vty, \
                       char * const helpstr, int max_strlen);
const struct shash_node **sort_interface(const struct shash *sh);

#endif

#endif /* _INFT_VTY_H */
