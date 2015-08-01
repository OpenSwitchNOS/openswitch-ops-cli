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
static struct ovsdb_idl_txn *status_txn;

boolean exiting = false;

extern struct vty *vty;
/*
 * Running idl run and wait to fetch the data from the DB
 */
static void
vtysh_run()
{
    while(!ovsdb_idl_has_lock(idl))
    {
        ovsdb_idl_run(idl);
        unixctl_server_run(appctl);

        ovsdb_idl_wait(idl);
        unixctl_server_wait(appctl);
    }
}

static void
bgp_ovsdb_init(struct ovsdb_idl *idl)
{
  ovsdb_idl_add_table(idl, &ovsrec_table_bgp_router);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_asn);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_router_id);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_status);
  ovsdb_idl_add_table(idl, &ovsrec_table_bgp_neighbor);

  ovsdb_idl_add_table(idl, &ovsrec_table_route);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_prefix);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_from);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_nexthops);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_address_family);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_sub_address_family);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_protocol_specific);
  ovsdb_idl_add_column(idl, &ovsrec_route_col_selected);
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
  ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_ports);
  ovsdb_idl_add_column(idl, &ovsrec_nexthop_col_weight);
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

    /* Add hostname columns */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_hostname);

    /* Add tables and columns for LLDP configuration */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_cur_cfg);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_status);

    /* Interface tables */
    intf_ovsdb_init(idl);

    /* BGP tables */
    bgp_ovsdb_init(idl);
    l3static_ovsdb_init(idl);

    /* VRF tables */
    vrf_ovsdb_init(idl);

    /* System tables */
    system_ovsdb_init(idl);
    /* VLAN internal commands */
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_hw_config);

    /* Fetch data from DB */
    vtysh_run();
}

static void
halon_vtysh_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
                    const char *argv[] OVS_UNUSED, void *exiting_)
{
    boolean *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}

void
vtysh_segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
  vtysh_reduce_session_count();
  exit(0);
}

bool
vtysh_reduce_session_count(void)
{
  const struct ovsrec_open_vswitch *ovs_row = NULL;
  int sesson_count = 0;
  char buffer[SESSION_CNT_LENGTH]={0};
  struct smap smap_other_config;

  if(status_txn != NULL)
    ovsdb_idl_txn_destroy(status_txn);

  if(!cli_do_config_start())
  {
    VLOG_ERR("OVSDB transaction creation failed.");
    return false;
  }

  ovs_row = ovsrec_open_vswitch_first(idl);
  if(!ovs_row)
  {
     VLOG_ERR("Couldn't fetch open_vswitch row.");
     cli_do_config_abort();
     return false;
  }

  sesson_count = smap_get_int(&ovs_row->other_config, OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS, 0);
  snprintf(buffer, SESSION_CNT_LENGTH, "%d", --sesson_count);

  smap_clone(&smap_other_config, &ovs_row->other_config);
  if(sesson_count != 0)
  {
    smap_replace(&smap_other_config, OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS, buffer);
  }
  else if(sesson_count == 0)
  {
    smap_remove(&smap_other_config, OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS);
  }

  ovsrec_open_vswitch_set_other_config(ovs_row, &smap_other_config);

  if(cli_do_config_finish())
    return true;
  else
  {
    VLOG_ERR("OVSDB transaction commit failed.");
    return false;
  }
}

void
vtysh_check_session(void)
{
  const struct ovsrec_open_vswitch *ovs_row = NULL;
  int sesson_count = 0;
  char buffer[SESSION_CNT_LENGTH]={0};
  struct smap smap_other_config;

  if(!cli_do_config_start())
  {
    VLOG_ERR("OVSDB transaction creation failed.");
    exit(EXIT_FAILURE);
  }

  ovs_row = ovsrec_open_vswitch_first(idl);
  if(!ovs_row)
  {
     VLOG_ERR("Couldn't fetch open_vswitch row.");
     cli_do_config_abort();
     exit(EXIT_FAILURE);
  }

  sesson_count = smap_get_int(&ovs_row->other_config, OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS, 0);
  snprintf(buffer, SESSION_CNT_LENGTH, "%d", ++sesson_count);

  smap_clone(&smap_other_config, &ovs_row->other_config);
  if(sesson_count <= MAX_CLI_SESSIONS)
  {
    smap_replace(&smap_other_config, OPEN_VSWITCH_OTHER_CONFIG_CLI_SESSIONS, buffer);
    ovsrec_open_vswitch_set_other_config(ovs_row, &smap_other_config);
  }
  else
  {
    VLOG_ERR("Exceeded max number of sessions.");
    printf("\nError: Maximum number of CLI sessions reached.\n");
    cli_do_config_abort();
    exit(EXIT_FAILURE);
  }

  if(cli_do_config_finish())
    return;
  else
  {
    VLOG_ERR("OVSDB transaction commit failed.");
    exit(EXIT_FAILURE);
  }

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
    vtysh_check_session();
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

    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs)
    {
        txn = ovsdb_idl_txn_create(idl);
        ovsrec_open_vswitch_set_hostname(ovs, in);
        ovsdb_idl_txn_commit(txn);
        ovsdb_idl_txn_destroy(txn);
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
    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);
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
    vtysh_reduce_session_count();
}

/* This API is for fetching contents from DB to Vtysh IDL cache
   and to do initial setup before commiting changes to IDL cache.
   Keeping the return value as boolean so that we can handle any error cases
   in future. */
boolean cli_do_config_start()
{
  ovsdb_idl_run(idl);
  status_txn = ovsdb_idl_txn_create(idl);
  if(status_txn == NULL)
  {
     assert(0);
     return false;
  }
  return true;
}

/* This API is for pushing Vtysh IDL contents to DB */
boolean cli_do_config_finish()
{
  enum ovsdb_idl_txn_status status;

  status = ovsdb_idl_txn_commit_block(status_txn);
  ovsdb_idl_txn_destroy(status_txn);
  status_txn = NULL;

  if ((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED))
     return false;

  return true;
}


void cli_do_config_abort()
{
  ovsdb_idl_txn_destroy(status_txn);
}

/*
 * Check if the input string is a valid interface in the
 * OVSDB table
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
