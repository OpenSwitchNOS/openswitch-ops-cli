/* Vtysh daemon ovsdb integration.
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "assert.h"
#include "vtysh_ovsdb_config.h"
#include "lib/lib_vtysh_ovsdb_if.h"
#ifdef HAVE_GNU_REGEX
#include <regex.h>
#else
#include "lib/regex-gnu.h"
#endif /* HAVE_GNU_REGEX */
#include "lib/vty.h"

typedef unsigned char boolean;

VLOG_DEFINE_THIS_MODULE(vtysh_ovsdb_if);

struct ovsdb_idl *idl;
static unsigned int idl_seqno;
static char *appctl_path = NULL;
static struct unixctl_server *appctl;
static struct ovsdb_idl_txn *txn;

boolean exiting = false;
volatile boolean vtysh_exit = false;
/* To serialize updates to OVSDB.
 * interface threads calls to update OVSDB states. */
pthread_mutex_t vtysh_ovsdb_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Macros to lock and unlock mutexes in a verbose manner. */
#define VTYSH_OVSDB_LOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_LOCK: taking lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_lock(&vtysh_ovsdb_mutex); \
}

#define VTYSH_OVSDB_UNLOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_UNLOCK: releasing lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_unlock(&vtysh_ovsdb_mutex); \
}

extern struct vty *vty;

/*
 * Running idl run and wait to fetch the data from the DB
 */
static void
vtysh_run()
{
    ovsdb_idl_run(idl);
}

static void
vtysh_wait(void)
{
    ovsdb_idl_wait(idl);
}

static void
bgp_ovsdb_init (struct ovsdb_idl *idl)
{
    /* BGP router table */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_router);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_asn);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_router_id);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_networks);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_maximum_paths);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_timers);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_vrf);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_redistribute);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_gr_stale_timer);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_always_compare_med);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_external_ids);

    /* BGP neighbor table */
    ovsdb_idl_add_table(idl, &ovsrec_table_bgp_neighbor);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_active);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_weight);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_is_peer_group);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_bgp_peer_group);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_bgp_router);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_strict_capability_match);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_tcp_port_number);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_inbound_soft_reconfiguration);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_remote_as);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_shutdown);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_override_capability);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_passive);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_maximum_prefix_limit);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_status);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_description);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_local_as);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_advertisement_interval);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_local_interface);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_external_ids);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_password);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_capability);
    ovsdb_idl_add_column(idl, &ovsrec_bgp_neighbor_col_timers);

    /* RIB */
    ovsdb_idl_add_table(idl, &ovsrec_table_route);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_prefix);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_from);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_nexthops);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_sub_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_protocol_specific);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_selected);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_protocol_private);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_distance);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_metric);
    ovsdb_idl_add_column(idl, &ovsrec_route_col_vrf);
 }

static void
l3static_ovsdb_init(struct ovsdb_idl *idl)
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
vrf_ovsdb_init(struct ovsdb_idl *idl)
{
    ovsdb_idl_add_table(idl, &ovsrec_table_vrf);
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_table(idl, &ovsrec_table_bridge);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_interfaces);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip4_address);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip4_address_secondary);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip6_address);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_ip6_address_secondary);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vrf_col_ports);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_ports);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_vrfs);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_bridges);
}

static void
policy_ovsdb_init(struct ovsdb_idl *idl)
{
    ovsdb_idl_add_table(idl, &ovsrec_table_prefix_list);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_col_description);

    ovsdb_idl_add_table(idl, &ovsrec_table_prefix_list_entries);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entries_col_action);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entries_col_prefix);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entries_col_prefix_list);
    ovsdb_idl_add_column(idl, &ovsrec_prefix_list_entries_col_sequence);


    ovsdb_idl_add_table(idl, &ovsrec_table_route_map);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_col_name);

    ovsdb_idl_add_table(idl, &ovsrec_table_route_map_entries);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_action);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_description);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_match);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_preference);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_route_map);
    ovsdb_idl_add_column(idl, &ovsrec_route_map_entries_col_set);
}

/***********************************************************
 * @func        : intf_ovsdb_init
 * @detail      : Initialise Interface table
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
intf_ovsdb_init(struct ovsdb_idl *idl)
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
}

/***********************************************************
 * @func        : alias_ovsdb_init
 * @detail      : Initialise Alias table
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
alias_ovsdb_init(struct ovsdb_idl *idl)
{
    ovsdb_idl_add_table(idl, &ovsrec_table_cli_alias);
    ovsdb_idl_add_column(idl, &ovsrec_cli_alias_col_alias_name);
    ovsdb_idl_add_column(idl, &ovsrec_cli_alias_col_alias_definition);

    return;
}

static void
radius_server_ovsdb_init(struct ovsd_idl *idl)
{

    /* Add radius-server columns */
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_radius_servers);
    ovsdb_idl_add_table(idl, &ovsrec_table_radius_server);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_retries);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_udp_port);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_timeout);
    ovsdb_idl_add_column(idl, &ovsrec_radius_server_col_passkey);

    return;
}

/***********************************************************
 * @func        : system_ovsdb_init
 * @detail      : Initialise System Related OVSDB tables
 * @param[in]
 *      idl     : Pointer to idl structure
 ***********************************************************/
static void
system_ovsdb_init(struct ovsd_idl *idl)
{
    /* Add Platform Related Tables */
    ovsdb_idl_add_table(idl,&ovsrec_table_fan);
    ovsdb_idl_add_table(idl,&ovsrec_table_power_supply);
    ovsdb_idl_add_table(idl,&ovsrec_table_led);
    ovsdb_idl_add_table(idl,&ovsrec_table_subsystem);
    ovsdb_idl_add_table(idl,&ovsrec_table_temp_sensor);

    /* Add Columns for System Related Tables */

    //Power Supply
    ovsdb_idl_add_column(idl,&ovsrec_power_supply_col_name);
    ovsdb_idl_add_column(idl,&ovsrec_power_supply_col_status);
    ovsdb_idl_add_column(idl,&ovsrec_power_supply_col_other_config);
    ovsdb_idl_add_column(idl,&ovsrec_power_supply_col_external_ids);

    //LED
    ovsdb_idl_add_column(idl,&ovsrec_led_col_id);
    ovsdb_idl_add_column(idl,&ovsrec_led_col_state);
    ovsdb_idl_add_column(idl,&ovsrec_led_col_status);
    ovsdb_idl_add_column(idl,&ovsrec_led_col_other_config);
    ovsdb_idl_add_column(idl,&ovsrec_led_col_external_ids);

    //Subsystem
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_interfaces);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_leds);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_fans);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_temp_sensors);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_power_supplies);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_asset_tag_number);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_name);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_type);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_hw_desc_dir);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_other_info);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_other_config);
    ovsdb_idl_add_column(idl,&ovsrec_subsystem_col_external_ids);

    //Fan
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_status);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_direction);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_name);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_rpm);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_other_config);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_hw_config);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_external_ids);
    ovsdb_idl_add_column(idl,&ovsrec_fan_col_speed);

    //Temp
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_external_ids);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_fan_state);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_hw_config);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_location);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_max);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_min);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_name);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_other_config);;
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_status);
    ovsdb_idl_add_column(idl,&ovsrec_temp_sensor_col_temperature);

}

static void
logrotate_ovsdb_init(struct ovsdb_idl *idl)
{
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_logrotate_config);
}

static void
vlan_ovsdb_init(struct ovsdb_idl *idl)
{
    ovsdb_idl_add_table(idl, &ovsrec_table_vlan);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_id);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_admin);
}

static void
mgmt_intf_ovsdb_init()
{
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_mgmt_intf);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_mgmt_intf_status);
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
    idl_lock = xasprintf("halon_vtysh_%ld", pid);
    ovsdb_idl_set_lock(idl, idl_lock);
    free(idl_lock);
    idl_seqno = ovsdb_idl_get_seqno(idl);
    ovsdb_idl_enable_reconnect(idl);

    /* Add hostname columns */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_hostname);

    /* Add AAA columns */
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_aaa);

    /* Add Auto Provision Column */
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_auto_provisioning_status);

    /* Add tables and columns for LLDP configuration */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_cur_cfg);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_status);

    /* Interface tables */
    intf_ovsdb_init(idl);

   /* Management interface columns */
    mgmt_intf_ovsdb_init();

    alias_ovsdb_init(idl);

    /* BGP tables */
    bgp_ovsdb_init(idl);
    l3static_ovsdb_init(idl);

    /* VRF tables */
    vrf_ovsdb_init(idl);

    /* Radius server table */
    radius_server_ovsdb_init(idl);

    /* Policy tables */
    policy_ovsdb_init(idl);

    /* System tables */
    system_ovsdb_init(idl);
    /* VLAN internal commands */
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_hw_config);

    /* vlan table */
    vlan_ovsdb_init(idl);

    /* Logrotate tables */
    logrotate_ovsdb_init(idl);

    /* Neighbor table for 'show arp' & 'show ipv6 neighbor' commands */
    ovsdb_idl_add_table(idl, &ovsrec_table_neighbor);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_address_family);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_mac);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_state);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_ip_address);
    ovsdb_idl_add_column(idl, &ovsrec_neighbor_col_port);
}

static void
halon_vtysh_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
                    const char *argv[] OVS_UNUSED, void *exiting_)
{
    boolean *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}

/*
 * The init for the ovsdb integration called in vtysh main function
 */
void vtysh_ovsdb_init(int argc, char *argv[])
{
    int retval;
    char *ovsdb_sock;

    set_program_name(argv[0]);
    proctitle_init(argc, argv);
    fatal_ignore_sigpipe();

    ovsdb_sock = xasprintf("unix:%s/db.sock", ovs_rundir());
    ovsrec_init();

    retval = unixctl_server_create(appctl_path, &appctl);
    if(retval)
    {
        exit(EXIT_FAILURE);
    }

    unixctl_command_register("exit", "", 0, 0, halon_vtysh_exit, &exiting);

    ovsdb_init(ovsdb_sock);
    vtysh_ovsdb_lib_init();
    free(ovsdb_sock);

    VLOG_DBG("Halon Vtysh OVSDB Integration has been initialized");

    return;
}

/*
 * The set command to set the hostname column in the
 * open_vswitch table from the set-hotname command
 */
void vtysh_ovsdb_hostname_set(const char* in)
{
    const struct ovsrec_open_vswitch *ovs= NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs)
    {
        status_txn = cli_do_config_start();
        if(status_txn == NULL)
        {
            cli_do_config_abort(status_txn);
            VLOG_ERR("Failed to create a transaction");
        }
        ovsrec_open_vswitch_set_hostname(ovs, in);
        status = cli_do_config_finish(txn);
        VLOG_DBG("Hostname set to %s in table",in);
    }
    else
    {
        VLOG_ERR("unable to retrieve any open_vswitch table rows");
    }
}

/*
 * The get command to read from the ovsdb open_vswitch table
 * hostname column from the vtysh get-hostname command
 */
char* vtysh_ovsdb_hostname_get()
{
    const struct ovsrec_open_vswitch *ovs;
    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs)
    {
        vty_out(vty, "hostname in table is %s%s", ovs->hostname, VTY_NEWLINE);
        VLOG_DBG("retrieved hostname %s from table", ovs->hostname);
        return ovs->hostname;
    }
    else
    {
        VLOG_ERR("unable to  retrieve any open_vswitch table rows");
    }

    return NULL;
}

/*
 * When exiting vtysh destroy the idl cache
 */
void vtysh_ovsdb_exit(void)
{
    ovsdb_idl_destroy(idl);
}

/* Take the lock and create a transaction if
   DB connection is available and return the
   transaction pointer */
struct ovsdb_idl_txn* cli_do_config_start()
{
  idl_seqno = ovsdb_idl_get_seqno(idl);
  /* Checking if the connection is alive and if
     we have received atleast one update from DB */
  if(idl_seqno < 1 || !ovsdb_idl_is_alive(idl))
  {
    return NULL;
  }

  /* TO-DO: Move the locking into the infra itself so
     that developers need not worry about the locking */
  VTYSH_OVSDB_LOCK;
  struct ovsdb_idl_txn *status_txn = ovsdb_idl_txn_create(idl);

  if(status_txn  == NULL)
  {
     assert(0);
     return NULL;
  }
  return status_txn;
}

/* Commit the transaction to DB and relase the lock */
enum ovsdb_idl_txn_status cli_do_config_finish(struct ovsdb_idl_txn* status_txn)
{
  if(status_txn == NULL)
  {
    assert(0);
    VTYSH_OVSDB_UNLOCK;
    return TXN_ERROR;
  }

  enum ovsdb_idl_txn_status status;

  status = ovsdb_idl_txn_commit_block(status_txn);
  ovsdb_idl_txn_destroy(status_txn);
  status_txn = NULL;

  VTYSH_OVSDB_UNLOCK;
  return status;
}

/* Destroy the transaction in case of an error */
void cli_do_config_abort(struct ovsdb_idl_txn* status_txn)
{
  if(status_txn == NULL)
  {
    VTYSH_OVSDB_UNLOCK;
    return;
  }
  ovsdb_idl_txn_destroy(status_txn);
  status_txn = NULL;
  VTYSH_OVSDB_UNLOCK;
}

/*
 * check if the input string is a valid interface in the
 * ovsdb table
 */
int vtysh_ovsdb_interface_match(const char *str)
{

  struct ovsrec_interface *row, *next;

  if(!str)
  {
    return 1;
  }

  OVSREC_INTERFACE_FOR_EACH_SAFE(row, next, idl)
  {
    if( strcmp(str,row->name) == 0)
      return 0;
  }

  return 1;
}

/*
 * Check if the input string is a valid port in the
 * OVSDB table
 */
int vtysh_ovsdb_port_match(const char *str)
{
  struct ovsrec_port *row, *next;

  if(!str)
  {
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
 * OVSDB table
 */
int vtysh_ovsdb_vlan_match(const char *str)
{

  struct ovsrec_vlan *row, *next;

  if(!str)
  {
    return 1;
  }

  OVSREC_VLAN_FOR_EACH_SAFE(row, next, idl)
  {
    if (strcmp(str, row->name) == 0)
      return 0;
  }

  return 1;
}

/*
 * Check if the input string matches the given regex
 */
int vtysh_regex_match(const char *regString, const char *inp)
{
  if(!inp || !regString)
  {
    return 1;
  }

  regex_t regex;
  int ret;
  char msgbuf[100];

  ret = regcomp(&regex, regString, 0);
  if(ret)
  {
    VLOG_ERR("Could not compile regex\n");
    return 1;
  }

  ret = regexec(&regex, inp, 0, NULL, 0);
  if (!ret)
  {
    return 0;
  }
  else if(ret == REG_NOMATCH)
  {
    return REG_NOMATCH;
  }
  else
  {
    regerror(ret, &regex, msgbuf, sizeof(msgbuf));
    VLOG_ERR("Regex match failed: %s\n", msgbuf);
  }

  return 1;
}

/* The main thread routine which keeps polling on the
   OVSDB idl socket */
void *
vtysh_ovsdb_main_thread(void *arg)
{
    /* Detach thread to avoid memory leak upon exit. */
    pthread_detach(pthread_self());

    vtysh_exit = false;
    while (!vtysh_exit) {
        VTYSH_OVSDB_LOCK;

        /* This function updates the Cache by running
           ovsdb_idl_run */
        vtysh_run();

        /* This function adds the file descriptor for the
           DB to monitor using poll_fd_wait */
        vtysh_wait();

        VTYSH_OVSDB_UNLOCK;
        if (vtysh_exit) {
            poll_immediate_wake();
        } else {
            /* TO-DO: There is a race condition with the FD's
               used for poll here. Currently since the fd is
               already registered and the events being registered
               in both the threads is same, it shouldn't affect
               functionality. Need to resolve this as soon possible */
            poll_block();
        }
    }

    return NULL;

}

/*
 * Checks if interface is already part of bridge.
 */
bool check_iface_in_bridge(const char *if_name)
{
  struct ovsrec_open_vswitch *ovs_row = NULL;
  struct ovsrec_bridge *br_cfg = NULL;
  struct ovsrec_port *port_cfg = NULL;
  struct ovsrec_interface *iface_cfg = NULL;
  size_t i, j, k;
  ovs_row = ovsrec_open_vswitch_first(idl);
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

/*
 * Checks if interface is already part of a VRF.
 */
bool check_iface_in_vrf(const char *if_name)
{
  struct ovsrec_open_vswitch *ovs_row = NULL;
  struct ovsrec_vrf *vrf_cfg = NULL;
  struct ovsrec_port *port_cfg = NULL;
  struct ovsrec_interface *iface_cfg = NULL;
  size_t i, j, k;
  ovs_row = ovsrec_open_vswitch_first(idl);
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

/*
 * init the vtysh lib routines
 */
void vtysh_ovsdb_lib_init()
{
   lib_vtysh_ovsdb_interface_match = &vtysh_ovsdb_interface_match;
   lib_vtysh_ovsdb_port_match = &vtysh_ovsdb_port_match;
   lib_vtysh_ovsdb_vlan_match = &vtysh_ovsdb_vlan_match;
}

/*
 * Wrapper for changing the help text for commands
 */
void utils_vtysh_rl_describe_output(struct vty* vty, vector describe, int width)
{
  struct cmd_token *token;
  int i;
  for (i = 0; i < vector_active (describe); i++)
  {
    if ((token = vector_slot (describe, i)) != NULL)
      {
        if (token->cmd == NULL || token->cmd[0] == '\0')
          continue;

        if (! token->desc)
          fprintf (stdout,"  %-s\n",
                   token->cmd[0] == '.' ? token->cmd + 1 : token->cmd);
        else
          fprintf (stdout,"  %-*s  %s\n",
                   width,
                   token->cmd[0] == '.' ? token->cmd + 1 : token->cmd,
                   token->desc);
      }
  }
}
