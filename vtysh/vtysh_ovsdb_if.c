/* Vtysh daemon ovsdb integration.
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * File: vtysh_ovsdb_if.c
 *
 * Purpose: Main file for integrating vtysh with ovsdb.
*/

#include <stdio.h>
#include "vector.h"
#include "command.h"
#include <pthread.h>
#include <semaphore.h>
#include "vswitch-idl.h"
#include "util.h"
#include "unixctl.h"
#include "config.h"
#include "command-line.h"
#include "daemon.h"
#include "dirs.h"
#include "fatal-signal.h"
#include "poll-loop.h"
#include "timeval.h"
#include "openvswitch/vlog.h"
#include "coverage.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "assert.h"
#include "vtysh_ovsdb_config.h"
#include "lib/lib_vtysh_ovsdb_if.h"
#include <termios.h>


#ifdef HAVE_GNU_REGEX
#include <regex.h>
#else
#include "lib/regex-gnu.h"
#endif /* HAVE_GNU_REGEX */

#include "lib/vty.h"
#include "latch.h"
#include "lib/vty_utils.h"
#include "intf_vty.h"

#define TMOUT_POLL_INTERVAL 20

int64_t timeout_start;
struct termios tp;
unsigned int last_idl_seq_no;
long long int next_poll_msec;
int64_t session_timeout_period;

typedef unsigned char boolean;

VLOG_DEFINE_THIS_MODULE (vtysh_ovsdb_if);

struct ovsdb_idl *idl;
static unsigned int idl_seqno;
static char *appctl_path = NULL;
static struct unixctl_server *appctl;
static int cur_cfg_no = 0;

boolean exiting = false;
volatile boolean vtysh_exit = false;
extern struct vty *vty;

/* Function checks if timeout period has
*  exceeded. If yes, exits cli session.
*/
static void
vtysh_session_timeout_run()
{
    if (last_idl_seq_no != ovsdb_idl_get_seqno(idl))
    {
        last_idl_seq_no = ovsdb_idl_get_seqno(idl);
        session_timeout_period = 60 * vtysh_ovsdb_session_timeout_get();
    }
    if (time_msec() > next_poll_msec) {
        next_poll_msec = time_msec() + (TMOUT_POLL_INTERVAL * 1000);
        if ((session_timeout_period > 0) &&
            ((time_now() - timeout_start) > session_timeout_period))
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &tp);
            vty_out(vty, "%s%s", VTY_NEWLINE, VTY_NEWLINE);
            vty_out(vty, "Idle session timeout reached, logging out.%s",
                    VTY_NEWLINE);
            vty_out(vty, "%s%s", VTY_NEWLINE, VTY_NEWLINE);
            exit(0);
        }
    }
}

/* Running idl run and wait to fetch the data from the DB. */
static void
vtysh_run()
{
    ovsdb_idl_run (idl);
    vtysh_session_timeout_run();
}

static void
vtysh_wait(void)
{
    ovsdb_idl_wait (idl);
    latch_wait (&ovsdb_latch);
}

static void
bgp_ovsdb_init()
{
    /* BGP router table. */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_router);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_router_id);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_networks);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_maximum_paths);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_timers);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_redistribute);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_always_compare_med);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_deterministic_med);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_gr_stale_timer);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_bgp_neighbors);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_fast_external_failover);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_log_neighbor_changes);

    /* BGP neighbor table. */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_neighbor);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_is_peer_group);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_description);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_shutdown);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_bgp_peer_group);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_local_interface);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_remote_as);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_allow_as_in);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_local_as);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_weight);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_tcp_port_number);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_advertisement_interval);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_maximum_prefix_limit);
    ovsdb_idl_add_column(idl,
                         &ovsrec_bgp_neighbor_col_inbound_soft_reconfiguration);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_remove_private_as);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_passive);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_password);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_timers);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_route_maps);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_ebgp_multihop);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_ttl_security_hops);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_update_source);

    /* RIB. */
    ovsdb_idl_add_table(idl, &ovsrec_table_route);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_prefix);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_from);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_nexthops);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_sub_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_selected);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_distance);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_metric);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_vrf);

    /* BGP RIB table. */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_route);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_prefix);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_bgp_nexthops);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_sub_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_distance);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_metric);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_vrf);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_path_attributes);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_route_col_peer);

    /* BGP Nexthop table. */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_nexthop);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_nexthop_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_nexthop_col_type);

}

static void
l3routes_ovsdb_init()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_vrf);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_name);
    ovsdb_idl_add_table(idl, &ovsrec_table_nexthop);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_selected);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_ports);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_weight);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_external_ids);
}

static void
vrf_ovsdb_init()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_vrf);
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_table(idl, &ovsrec_table_bridge);
    ovsdb_idl_add_table(idl, &ovsrec_table_interface);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_interfaces);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip4_address);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip4_address_secondary);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip6_address);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip6_address_secondary);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_vlan_mode);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_trunks);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_admin);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_tag);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_ports);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_bgp_routers);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_ports);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_vlans);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_vrfs);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_bridges);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_hw_intf_info);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_user_config);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_split_parent);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_split_children);
}

static void
policy_ovsdb_init ()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_community_filter);
    ovsdb_idl_add_column(idl, &ovsrec_community_filter_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_community_filter_col_action);
    ovsdb_idl_add_column(idl, &ovsrec_community_filter_col_match);
    ovsdb_idl_add_column(idl, &ovsrec_community_filter_col_type);
    ovsdb_idl_add_table(idl, &ovsrec_table_prefix_list);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_col_prefix_list_entries);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_col_description);
    ovsdb_idl_add_table(idl, &ovsrec_table_prefix_list_entry);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entry_col_action);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entry_col_prefix);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entry_col_le);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entry_col_ge);
    ovsdb_idl_add_table(idl, &ovsrec_table_route_map);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_col_route_map_entries);
    ovsdb_idl_add_table(idl, &ovsrec_table_route_map_entry);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_description);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_action);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_exitpolicy);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_goto_target);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_call);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_match);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entry_col_set);
}

/***********************************************************
 * @func        : intf_ovsdb_init
 * @detail      : Initialise Interface table
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
intf_ovsdb_init()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_interface);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_link_state);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_lldp_neighbor_info);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_user_config);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_link_state);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_admin_state);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_duplex);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_mtu);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_mac_in_use);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_link_speed);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_pause);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_type);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_hw_intf_info);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_pm_info);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_error);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_lacp_status);
    ovsdb_idl_add_table(idl, &ovsrec_table_vrf);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_ports);
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_name);
}

/***********************************************************
 * @func        : alias_ovsdb_init
 * @detail      : Initialise Alias table
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
alias_ovsdb_init()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_cli_alias);
    ovsdb_idl_add_column(idl, &ovsrec_cli_alias_col_alias_name);
    ovsdb_idl_add_column(idl, &ovsrec_cli_alias_col_alias_definition);

    return;
}

static void
radius_server_ovsdb_init()
{

    /* Add radius-server columns. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_radius_servers);
    ovsdb_idl_add_table(idl, &ovsrec_table_radius_server);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_retries);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_udp_port);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_timeout);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_passkey);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_priority);

    return;
}

static void
dhcp_tftp_ovsdb_init()
{
    /* Add dhcp-server config tables */
    ovsdb_idl_add_table(idl, &ovsrec_table_system);
    ovsdb_idl_add_table(idl, &ovsrec_table_vrf);
    ovsdb_idl_add_table(idl, &ovsrec_table_dhcp_server);
    ovsdb_idl_add_table(idl, &ovsrec_table_dhcpsrv_range);
    ovsdb_idl_add_table(idl, &ovsrec_table_dhcpsrv_static_host);

    /* Add columns in  System table */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_other_config);

    /* Add columns in VRF table */
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_dhcp_server);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_other_config);

    /* Add columns in DHCP Server table */
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_ranges);
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_static_hosts);
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_dhcp_options);
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_matches);
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_bootp);
    ovsdb_idl_add_column(idl, &ovsrec_dhcp_server_col_other_config);

    /* Add columns in DHCP server ranges table */
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_start_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_end_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_netmask);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_prefix_len);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_broadcast);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_set_tag);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_match_tags);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_is_static);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_range_col_lease_duration);

    /* Add columns in DHCP server static hosts table */
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_mac_addresses);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_set_tags);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_client_hostname);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_client_id);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_static_host_col_lease_duration);

    /* Add columns in DHCP server options table */
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_option_col_match_tags);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_option_col_option_name);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_option_col_option_number);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_option_col_option_value);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_option_col_ipv6);

    /* Add columns in DHCP server matches table */
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_match_col_set_tag);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_match_col_option_name);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_match_col_option_number);
    ovsdb_idl_add_column(idl, &ovsrec_dhcpsrv_match_col_option_value);

}

/***********************************************************
 * @func        : system_ovsdb_init
 * @detail      : Initialise System Related OVSDB tables
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
system_ovsdb_init()
{
    /* Add Platform Related Tables. */
    ovsdb_idl_add_table(idl, &ovsrec_table_fan);
    ovsdb_idl_add_table(idl, &ovsrec_table_power_supply);
    ovsdb_idl_add_table(idl, &ovsrec_table_led);
    ovsdb_idl_add_table(idl, &ovsrec_table_subsystem);
    ovsdb_idl_add_table(idl, &ovsrec_table_temp_sensor);

    /* Add Columns for System Related Tables. */

    /* Power Supply. */
    ovsdb_idl_add_column(idl, &ovsrec_power_supply_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_power_supply_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_power_supply_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_power_supply_col_external_ids);

    /* LED. */
    ovsdb_idl_add_column(idl, &ovsrec_led_col_id);
    ovsdb_idl_add_column(idl, &ovsrec_led_col_state);
    ovsdb_idl_add_column(idl, &ovsrec_led_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_led_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_led_col_external_ids);

    /* Subsystem .*/
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_interfaces);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_leds);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_fans);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_temp_sensors);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_power_supplies);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_asset_tag_number);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_type);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_hw_desc_dir);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_other_info);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_external_ids);

    /* Fan. */
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_direction);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_rpm);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_hw_config);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_fan_col_speed);

    /* Temp. */
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_fan_state);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_hw_config);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_location);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_max);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_min);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_other_config);;
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_temp_sensor_col_temperature);

}

static void
logrotate_ovsdb_init()
{
    ovsdb_idl_add_column(idl, &ovsrec_system_col_logrotate_config);
}

static void
vlan_ovsdb_init()
{
    ovsdb_idl_add_table(idl, &ovsrec_table_vlan);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_id);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_admin);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_description);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_hw_vlan_config);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_oper_state);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_oper_state_reason);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_internal_usage);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_other_config);
}

static void
mgmt_intf_ovsdb_init()
{
    ovsdb_idl_add_column(idl, &ovsrec_system_col_mgmt_intf);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_mgmt_intf_status);
}

static void
lacp_ovsdb_init()
{
    ovsdb_idl_add_column(idl, &ovsrec_system_col_lacp_config);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_lacp);
}

/*
 * Create a connection to the OVSDB at db_path and create
 * the idl cache.
 */
static void
ovsdb_init(const char *db_path)
{
    char *idl_lock;
    long int pid;

    idl = ovsdb_idl_create(db_path, &ovsrec_idl_class, false, true);
    pid = getpid();
    idl_lock = xasprintf("ops_cli_%ld", pid);
    ovsdb_idl_set_lock(idl, idl_lock);
    free(idl_lock);
    idl_seqno = ovsdb_idl_get_seqno(idl);
    ovsdb_idl_enable_reconnect(idl);
    latch_init(&ovsdb_latch);

    /* Add system table. */
    ovsdb_idl_add_table(idl, &ovsrec_table_system);

    /* Add software_info column */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_software_info);

    /* Add switch version column */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_switch_version);

    /* Add hostname columns. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_hostname);

    /* Add AAA columns. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_aaa);

    /* Add Auto Provision Column. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_auto_provisioning_status);

    /* Add tables and columns for LLDP configuration. */
    ovsdb_idl_add_table(idl, &ovsrec_table_system);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_cur_cfg);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_system_mac);

    /* Add columns for ECMP configuration. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_ecmp_config);

    /* Interface tables. */
    intf_ovsdb_init();

   /* Management interface columns. */
    mgmt_intf_ovsdb_init();

    alias_ovsdb_init();

    /* BGP tables. */
    bgp_ovsdb_init();
    l3routes_ovsdb_init();

    /* VRF tables. */
    vrf_ovsdb_init();

    /* Radius server table. */
    radius_server_ovsdb_init();

    /* Policy tables. */
    policy_ovsdb_init();

    /* System tables. */
    system_ovsdb_init();
    /* VLAN internal commands. */
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_hw_config);

    /* vlan table. */
    vlan_ovsdb_init();
    dhcp_tftp_ovsdb_init();
    /* Logrotate tables */
    logrotate_ovsdb_init();
    /* Add tables/columns needed for LACP config commands. */
    lacp_ovsdb_init();

    /* Neighbor table for 'show arp' & 'show ipv6 neighbor' commands. */
    ovsdb_idl_add_table(idl, &ovsrec_table_neighbor);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_mac);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_state);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_port);

}

static void
ops_vtysh_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
               const char *argv[] OVS_UNUSED, void *exiting_)
{
    boolean *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}

/* The init for the ovsdb integration called in vtysh main function. */
void
vtysh_ovsdb_init(int argc, char *argv[], char *db_name)
{
    int retval;
    char *ovsdb_sock;

    set_program_name(argv[0]);
    proctitle_init(argc, argv);
    fatal_ignore_sigpipe();
    if (db_name != NULL)
    {
        ovsdb_sock = xasprintf("unix:%s/%s", ovs_rundir(), db_name);
    }
    else {
        ovsdb_sock = xasprintf("unix:%s/db.sock", ovs_rundir());
    }
    ovsrec_init();

    retval = unixctl_server_create(appctl_path, &appctl);
    if (retval) {
        exit(EXIT_FAILURE);
    }

    unixctl_command_register("exit", "", 0, 0, ops_vtysh_exit, &exiting);

    ovsdb_init(ovsdb_sock);
    vtysh_ovsdb_lib_init();
    free(ovsdb_sock);

    VLOG_DBG("OPS Vtysh OVSDB Integration has been initialized");

    return;
}

/*
 * The get command to read from the ovsdb system table
 * software_info:os_name value.
 */
const char *
vtysh_ovsdb_os_name_get(void)
{
    const struct ovsrec_system *ovs;
    char *os_name = NULL;

    ovs = ovsrec_system_first(idl);
    if (ovs) {
        os_name = smap_get(&ovs->software_info, SYSTEM_SOFTWARE_INFO_OS_NAME);
    }

    return os_name ? os_name : "OpenSwitch";
}

/*
 * The get command to read from the ovsdb system table
 * switch_version column.
 */
const char *
vtysh_ovsdb_switch_version_get(void)
{
    const struct ovsrec_system *ovs;

    ovs = ovsrec_system_first(idl);
    if (ovs == NULL) {
        VLOG_ERR("unable to retrieve any system table rows");
        return "";
    }

    return ovs->switch_version ? ovs->switch_version : "";
}

/*
 * The set command to set the hostname column in the
 * system table from the set-hotname command.
 */
void
vtysh_ovsdb_hostname_set(const char* in)
{
    const struct ovsrec_system *ovs= NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status = TXN_ERROR;

    ovs = ovsrec_system_first(idl);
    if (ovs) {
        status_txn = cli_do_config_start();
        if(status_txn == NULL) {
            cli_do_config_abort(status_txn);
            VLOG_ERR("Couldn't create the OVSDB transaction.");
        } else {
            ovsrec_system_set_hostname(ovs, in);
            status = cli_do_config_finish(status_txn);
        }
        if(!(status == TXN_SUCCESS || status == TXN_UNCHANGED))
            VLOG_ERR("Committing transaction to DB failed.");
    } else {
        VLOG_ERR("unable to retrieve any system table rows");
    }
}

/*
 * Name : vtysh_ovsdb_hostname_reset
 * Responsibility : To unset hostname set by CLI.
 * Parameters : char *hostname_arg : Stores user's input value
 * Return : CMD_SUCCESS for success, CMD_OVSDB_FAILURE for failure
 */
int
vtysh_ovsdb_hostname_reset(char *hostname_arg)
{
    const struct ovsrec_system *row = NULL;
    const struct ovsdb_datum *data = NULL;
    char *ovsdb_hostname = NULL;
    row = ovsrec_system_first(idl);

    if (row != NULL)
    {
        data = ovsrec_system_get_hostname(row, OVSDB_TYPE_STRING);
        ovsdb_hostname = data->keys->string;

        if ((ovsdb_hostname != "") && (strcmp(ovsdb_hostname, hostname_arg) == 0))
        {
            vtysh_ovsdb_hostname_set("");
        }
        else
        {
            vty_out(vty, "Hostname %s not configured. %s", hostname_arg,
                    VTY_NEWLINE);
        }
    }
    else
    {
        vty_out(vty, "Error in retrieving hostname.%s", VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }
    return CMD_SUCCESS;
}

/*
 * The get command to read from the ovsdb system table
 * hostname column from the vtysh get-hostname command.
 */

const char*
vtysh_ovsdb_hostname_get()
{
    const struct ovsrec_system *ovs;
    ovs = ovsrec_system_first(idl);

    if (ovs) {
        return smap_get(&ovs->mgmt_intf_status, SYSTEM_MGMT_INTF_MAP_HOSTNAME);
    } else {
        VLOG_ERR("unable to  retrieve any system table rows");
    }

    return NULL;
}

/*
 * Function: vtysh_ovsdb_session_timeout_set
 * Responsibility: To set CLI idle session timeout value.
 * Parameters:
 *     duration: session timeout value configured by user.
 * Return: On success returns CMD_SUCCESS,
 *         On failure returns CMD_OVSDB_FAILURE
 */

int
vtysh_ovsdb_session_timeout_set(const char * duration)
{
    const struct ovsrec_system *ovs= NULL;
    enum ovsdb_idl_txn_status status = TXN_ERROR;
    struct smap smap_other_config;
    struct ovsdb_idl_txn* status_txn = cli_do_config_start();

    if (status_txn == NULL)
    {
        VLOG_ERR("Couldn't create the OVSDB transaction");
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs = ovsrec_system_first(idl);
    if (!ovs)
    {
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap_other_config, &ovs->other_config);
    smap_replace(&smap_other_config,
                 SYSTEM_OTHER_CONFIG_MAP_CLI_SESSION_TIMEOUT, duration);
    ovsrec_system_set_other_config(ovs, &smap_other_config);
    status = cli_do_config_finish(status_txn);
    smap_destroy(&smap_other_config);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR("Committing transaction to DB failed");
    }

    return CMD_OVSDB_FAILURE;
}

/*
 * Function: vtysh_ovsdb_session_timeout_get
 * Responsibility: To get CLI idle session timeout value.
 * Return:
 *    Returns the idle session timeout value.
 */

int64_t
vtysh_ovsdb_session_timeout_get()
{
    const struct ovsrec_system *ovs= NULL;
    int64_t timeout = 0;

    ovs = ovsrec_system_first(idl);
    if (!ovs)
    {
        return CMD_OVSDB_FAILURE;
    }

    timeout = smap_get_int(&ovs->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_CLI_SESSION_TIMEOUT,
                           DEFAULT_SESSION_TIMEOUT_PERIOD);

    return timeout;
}

/* Wait for database sysnchronization in case *
 * of command execution outside of vtysh. */
bool
vtysh_ovsdb_is_loaded()
{
   return (ovsdb_idl_has_ever_connected(idl));
}


/* When exiting vtysh destroy the idl cache. */

void
vtysh_ovsdb_exit(void)
{
    ovsdb_idl_destroy(idl);
}

/* Check whether config is initialized by subsystem. */

bool
ovsdb_cfg_initialized()
{
    if (cur_cfg_no < 1) {
        const struct ovsrec_system* ovs = ovsrec_system_first(idl);
        if (ovs != NULL) {
            cur_cfg_no = ovs->cur_cfg;
            if (cur_cfg_no < 1) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

/* Take the lock and create a transaction if
   DB connection is available and return the
   transaction pointer. */
struct ovsdb_idl_txn* cli_do_config_start()
{
    if (!ovsdb_cfg_initialized()) {
        return NULL;
    }

  struct ovsdb_idl_txn *status_txn = ovsdb_idl_txn_create(idl);

    if (status_txn  == NULL) {
        assert(0);
        return NULL;
    }
    return status_txn;
}

/* Commit the transaction to DB and relase the lock. */
enum
ovsdb_idl_txn_status cli_do_config_finish(struct ovsdb_idl_txn* status_txn)
{
    if (status_txn == NULL) {
        assert(0);
        return TXN_ERROR;
    }

    enum ovsdb_idl_txn_status status;

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    return status;
}

/* Destroy the transaction in case of an error. */
void
cli_do_config_abort(struct ovsdb_idl_txn* status_txn)
{
    if (status_txn == NULL) {
        return;
    }
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;
}

/*
 * Check if the input string is a valid interface or
 * lag aggregate in the ovsdb table.
 */
int
vtysh_ovsdb_interface_match(const char *str)
{

    const struct ovsrec_interface *row, *next;
    const struct ovsrec_port *lag_port, *lag_port_next;

    if (!str) {
        return 1;
    }
    // Search for each interface
    OVSREC_INTERFACE_FOR_EACH_SAFE(row, next, idl)
    {
        if ( strncmp(str,row->name, strlen(str)) == 0) {
            return 0;
        }
    }
    // Search for each lag port
    OVSREC_PORT_FOR_EACH_SAFE(lag_port, lag_port_next, idl)
    {
        if ( strncmp(str,lag_port->name,strlen(str)) == 0){
            return 0;
        }
    }
    return 1;
}

/*
 * Check if the input string is a valid port in the
 * OVSDB table.
 */
int
vtysh_ovsdb_port_match(const char *str)
{
    const struct ovsrec_port *row, *next;

    if (!str) {
        return 1;
    }

    OVSREC_PORT_FOR_EACH_SAFE(row, next, idl)
    {
        if (strcmp(str, row->name) == 0)
            return 0;
    }
    return 1;
}

/*
 * Check if the input string is a valid vlan in the
 * OVSDB table.
 */
int
vtysh_ovsdb_vlan_match(const char *str)
{

    const struct ovsrec_vlan *row, *next;

    if (!str ){
        return 1;
    }

    OVSREC_VLAN_FOR_EACH_SAFE(row, next, idl)
    {
        if (strcmp(str, row->name) == 0)
            return 0;
    }
    return 1;
}

/* Validate MAC address that will be used by MAC type tokens */
int
vtysh_ovsdb_mac_match(const char *str)
{
    int i = 0;

/* OPS_TODO : Checking for reserved MAC addresses if needed. */
    if (!str)
        return 1;

    while (i < MAX_MACADDR_LEN) {
        if (!str[i])
            return 1;

        switch (i % 3)
        {
        case 0:
        case 1: if (!isxdigit(str[i]))
                    return 1;
            break;
        case 2: if (str[i] != ':')
                    return 1;
            break;
        }
        i++;
    }
    if('\0' != str[MAX_MACADDR_LEN])
        return 1;

    return 0;
}

/* Check if the input string matches the given regex. */
int
vtysh_regex_match(const char *regString, const char *inp)
{
    if (!inp || !regString) {
        return 1;
    }

    regex_t regex;
    int ret;
    char msgbuf[100];

    ret = regcomp(&regex, regString, 0);
    if (ret) {
        VLOG_ERR("Could not compile regex\n");
        return 1;
    }

    ret = regexec(&regex, inp, 0, NULL, 0);
    if (!ret) {
        return 0;
    } else if (ret == REG_NOMATCH) {
        return REG_NOMATCH;
    } else {
        regerror(ret, &regex, msgbuf, sizeof(msgbuf));
        VLOG_ERR("Regex match failed: %s\n", msgbuf);
    }

    return 1;
}

/* The main thread routine which keeps polling on the
   OVSDB idl socket. */
void *
vtysh_ovsdb_main_thread(void *arg)
{
    /* Detach thread to avoid memory leak upon exit. */
    pthread_detach(pthread_self());

    vtysh_exit = false;
    next_poll_msec = time_msec() + (TMOUT_POLL_INTERVAL * 1000);
    last_idl_seq_no = ovsdb_idl_get_seqno(idl);
    session_timeout_period = DEFAULT_SESSION_TIMEOUT_PERIOD;

    while (!vtysh_exit) {

        poll_timer_wait_until(next_poll_msec);
        VTYSH_OVSDB_LOCK;

        /* This function updates the Cache by running
           ovsdb_idl_run. */
        vtysh_run();

        /* This function adds the file descriptor for the
           DB to monitor using poll_fd_wait. */
        vtysh_wait();

        vtysh_periodic_refresh();

        VTYSH_OVSDB_UNLOCK;
        if (vtysh_exit) {
            poll_immediate_wake();
        } else {
        /* The poll function polls on the OVSDB socket
         * and the latch fd set in vtysh_wait.  The latch
         * is set in command.c whenever user calls a
         * function so that this thread releases the lock
         * and vtysh thread holds the lock to avoid race
         * conditions while commiting the transaction.
         */
            poll_block();
        }
        /* Resets the latch. */
        latch_poll(&ovsdb_latch);
    }
    return NULL;
}

/* Checks if interface is already part of bridge. */

bool
check_iface_in_bridge(const char *if_name)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct ovsrec_bridge *br_cfg = NULL;
    struct ovsrec_port *port_cfg = NULL;
    struct ovsrec_interface *iface_cfg = NULL;
    size_t i, j, k;
    ovs_row = ovsrec_system_first(idl);
    for (i = 0; i < ovs_row->n_bridges; i++) {
        br_cfg = ovs_row->bridges[i];
        for (j = 0; j < br_cfg->n_ports; j++) {
            port_cfg = br_cfg->ports[j];
            if (strcmp(if_name, port_cfg->name) == 0) {
                return true;
            }
            for (k = 0; k < port_cfg->n_interfaces; k++) {
                iface_cfg = port_cfg->interfaces[k];
                if (strcmp(if_name, iface_cfg->name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/* Checks if port is already part of bridge. */

bool
check_port_in_bridge(const char *port_name)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct ovsrec_bridge *br_cfg = NULL;
    struct ovsrec_port *port_cfg = NULL;
    size_t i, j;
    ovs_row = ovsrec_system_first(idl);
    if (ovs_row == NULL) {
        return false;
    }
    for (i = 0; i < ovs_row->n_bridges; i++) {
        br_cfg = ovs_row->bridges[i];
        for (j = 0; j < br_cfg->n_ports; j++) {
            port_cfg = br_cfg->ports[j];
            if (strcmp(port_name, port_cfg->name) == 0) {
                return true;
            }
        }
    }
    return false;
}


/*
 * Check for presence of VRF and return VRF row.
 */
const struct ovsrec_vrf*
vrf_lookup (const char *vrf_name)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
      {
        if (strcmp (vrf_row->name, vrf_name) == 0)
        return vrf_row;
      }
    return NULL;
}

/*
 * This functions is used to check if port row exists.
 *
 * Variables:
 * port_name -> name of port to check
 * create -> flag to create port if not found
 * attach_to_default_vrf -> attach newly created port to default VRF
 */
const struct ovsrec_port*
port_check_and_add (const char *port_name, bool create,
                    bool attach_to_default_vrf, struct ovsdb_idl_txn *txn)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    int i = 0;

    OVSREC_PORT_FOR_EACH (port_row, idl)
    {
        if (strcmp (port_row->name, port_name) == 0)
            return port_row;
        /* The interface can be associated with another port */
        for (i = 0; i < port_row->n_interfaces; i++) {
            intf_row = port_row->interfaces[i];
            if (!strcmp(intf_row->name, port_name)) {
                return port_row;
            }
        }
    }
    if (!port_row && create)
    {
        const struct ovsrec_interface *if_row = NULL;
        struct ovsrec_interface **ifs;

        OVSREC_INTERFACE_FOR_EACH (if_row, idl)
        {
            if (strcmp (if_row->name, port_name) == 0)
            {
                port_row = ovsrec_port_insert (txn);
                ovsrec_port_set_name (port_row, port_name);
                ifs = xmalloc (sizeof *if_row);
                ifs[0] = (struct ovsrec_interface *) if_row;
                ovsrec_port_set_interfaces (port_row, ifs, 1);
                free (ifs);
                break;
            }
        }
        if (attach_to_default_vrf)
        {
            const struct ovsrec_vrf *default_vrf_row = NULL;
            struct ovsrec_port **ports = NULL;
            size_t i;
            default_vrf_row = vrf_lookup (DEFAULT_VRF_NAME);
            ports = xmalloc (
                    sizeof *default_vrf_row->ports * (default_vrf_row->n_ports + 1));
            for (i = 0; i < default_vrf_row->n_ports; i++)
                ports[i] = default_vrf_row->ports[i];

            struct ovsrec_port
                *temp_port_row = CONST_CAST(struct ovsrec_port*,
                        port_row);
            ports[default_vrf_row->n_ports] = temp_port_row;
            ovsrec_vrf_set_ports (default_vrf_row, ports,
                    default_vrf_row->n_ports + 1);
            free (ports);
        }
        return port_row;
    }
    return NULL;
}


/* Checks if interface is already part of a VRF. */
bool
check_iface_in_vrf(const char *if_name)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct ovsrec_vrf *vrf_cfg = NULL;
    struct ovsrec_port *port_cfg = NULL;
    struct ovsrec_interface *iface_cfg = NULL;
    size_t i, j, k;
    ovs_row = ovsrec_system_first(idl);
    for (i = 0; i < ovs_row->n_vrfs; i++) {
        vrf_cfg = ovs_row->vrfs[i];
        for (j = 0; j < vrf_cfg->n_ports; j++) {
            port_cfg = vrf_cfg->ports[j];
            if (strcmp(if_name, port_cfg->name) == 0) {
                return true;
            }
            for (k = 0; k < port_cfg->n_interfaces; k++) {
                iface_cfg = port_cfg->interfaces[k];
                if (strcmp(if_name, iface_cfg->name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}


/* Checks if interface is already part of a VRF. */
bool
check_port_in_vrf(const char *port_name)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct ovsrec_vrf *vrf_cfg = NULL;
    struct ovsrec_port *port_cfg = NULL;
    size_t i, j;
    ovs_row = ovsrec_system_first(idl);

    if (ovs_row == NULL) {
        return false;
    }
    for (i = 0; i < ovs_row->n_vrfs; i++) {
        vrf_cfg = ovs_row->vrfs[i];
        for (j = 0; j < vrf_cfg->n_ports; j++) {
            port_cfg = vrf_cfg->ports[j];
            if (strcmp(port_name, port_cfg->name) == 0) {
                return true;
            }
        }
    }
    return false;
}

/* Checks if the VLAN is used as an internal VLAN */
bool
check_if_internal_vlan(const struct ovsrec_vlan *vlan_row)
{
    char *l3port = NULL;
    l3port = smap_get(&vlan_row->internal_usage,
                                 VLAN_INTERNAL_USAGE_L3PORT);
    return (l3port != NULL) ? true : false;
}

/* Init the vtysh lib routines. */
void
vtysh_ovsdb_lib_init()
{
    lib_vtysh_ovsdb_interface_match = &vtysh_ovsdb_interface_match;
    lib_vtysh_ovsdb_port_match = &vtysh_ovsdb_port_match;
    lib_vtysh_ovsdb_vlan_match = &vtysh_ovsdb_vlan_match;
    lib_vtysh_ovsdb_mac_match = &vtysh_ovsdb_mac_match;
}

/* Wrapper for changing the help text for commands. */
void
utils_vtysh_rl_describe_output(struct vty* vty, vector describe, int width)
{
    struct cmd_token *token;
    int i;
    for (i = 0; i < vector_active (describe); i++) {
        if ((token = vector_slot (describe, i)) != NULL) {
            if (token->cmd == NULL || token->cmd[0] == '\0')
                continue;

            if (! token->desc)
                fprintf (stdout,"  %-s\n",
                         token->cmd[0] == '.' ? token->cmd + 1 : token->cmd);
            else
                fprintf (stdout,"  %-*s  %s\n", width,
                         token->cmd[0] == '.' ? token->cmd + 1 : token->cmd,
                         token->desc);
        }
    }
}
