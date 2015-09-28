/*
 * Copyright (C) 2015 Hewlett-Packard Enterprise Development LP
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
*/
/****************************************************************************
 * @ingroup dnsmasq
 *
 * @file dnsmasq_vty.h
 * Source to configure dnsmasq configuration details into dnsmasq ovsdb table
 *
 ***************************************************************************/


#ifndef _DHCP_TFTP_VTY_H
#define _DHCP_TFTP_VTY_H

#define MAX_TAG_LENGTH 32
#define MAX_MAC_LENGTH 17

#define MAX_DHCP_CONFIG_NAME_LENGTH 15
typedef struct dhcp_srv_range_params_s {
    // OPS_TODO:
    char *name;
    char *start_ip_address;
    char *end_ip_address;
    char *netmask;
    char *broadcast;
    char *set_tag;
    char *match_tags;
    int64_t lease_duration;
    int64_t prefix_len;
    bool is_static;
} dhcp_srv_range_params_t;

typedef struct dhcp_srv_static_host_params_s {
    char *ip_address;
    char *mac_addresses;
    char *set_tags;
    char *client_hostname;
    char *client_id;
    int64_t lease_duration;
} dhcp_srv_static_host_params_t;

typedef struct dhcp_srv_option_params_s {
    char *match_tags;
    char *option_name;
    int64_t option_number;
    char *option_value;
    bool is_ipv6;
} dhcp_srv_option_params_t;

typedef struct dhcp_srv_match_params_s {
    char *set_tag;
    char *option_name;
    int64_t option_number;
    char *option_value;
} dhcp_srv_match_params_t;

typedef struct dhcp_srv_bootp_params_s {
    char *match_tag;
    char *file;
} dhcp_srv_bootp_params_t;

void
dhcp_tftp_vty_init (void);

#endif /* _DHCP_TFTP_VTY_H */
