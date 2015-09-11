/* Management Interface CLI commands header file
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
 * File: mgmt_intf_vty.h
 *
 * Purpose:  To add declarations required for vty_mgmt_intf.c
 */

#ifndef _VTY_MGMT_INTF_H
#define _VTY_MGMT_INTF_H

void vty_mgmt_int_init (void);

#define MGMT_INTF_MODE_IP_STR         "Set the mode for management interface (static/dhcp)\n"
#define MGMT_INTF_DHCP_STR            "Set the mode as dhcp\n"
#define MGMT_INTF_STATIC_STR          "Set the mode as static\n"
#define MGMT_INTF_DEFAULT_GW_STR      "Configure the Default gateway address (IPv4 and IPv6)\n"
#define MGMT_INTF_DNS_STR             "Configure the nameserver\n"
#define MGMT_INTF_DNS_1_STR           "Configure the primary nameserver IPv4 address\n"
#define MGMT_INTF_DNS_2_STR           "Configure the secondary nameserver IPv4 address\n"
#define MGMT_INTF_DNS_1_IPV6_STR      "Configure the primary nameserver IPv6 address\n"
#define MGMT_INTF_DNS_2_IPV6_STR      "Configure the secondary nameserver IPv6 address\n"
#define MGMT_INTF_IP_STR              "Enter the IP address\n"
#define MGMT_INTF_IPV4_STR            "Enter the IPv4 address\n"
#define MGMT_INTF_IPV6_STR            "Enter the IPv6 address\n"
#define MGMT_INTF_SUBNET_STR          "Enter the subnet mask\n"
#define MGMT_INTF_MGMT_STR            "Management interface details\n"

#define MGMT_INTF_DEFAULT_IP          "0.0.0.0"
#define MGMT_INTF_DEFAULT_IPV6        "::"

#define OVSDB_MODE_ERROR                      "Configurations not allowed in dhcp mode"
#define OVSDB_DNS_DEPENDENCY_ERROR            "Deletion not allowed. Secondary Nameserver present"
#define OVSDB_NO_IP_ERROR                     "IP should be configured first."
#define OVSDB_REMOVE_IPV4_STATIC_CONF         "Remove all IPv4 static configurations"
#define OVSDB_REMOVE_IPV6_STATIC_CONF         "Remove all IPv6 static configurations"

#endif /* _VTY_MGMT_INTF_H */
