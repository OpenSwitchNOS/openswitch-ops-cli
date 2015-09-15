/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0.9

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
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

#define MAX_DHCP_CONFIG_NAME_LENGTH 16
#define MAX_TAG_LENGTH 32
#define MAX_MAC_LENGTH 17

typedef struct dhcp_srv_range_params_s {
    // TODO:
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
