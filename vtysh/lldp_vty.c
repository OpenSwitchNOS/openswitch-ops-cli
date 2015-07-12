/* LLDP CLI commands
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
 * File: lldp_vty.c
 *
 * Purpose:  To add LLDP CLI configuration and display commands.
 */

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "lldp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"

VLOG_DEFINE_THIS_MODULE(vtysh_lldp_cli);
extern struct ovsdb_idl *idl;

#define INTF_NAME_SIZE 20
typedef enum
{
  LLDP_OFF,
  LLDP_TX,
  LLDP_RX,
  LLDP_TXRX
}lldp_tx_rx;

typedef struct lldp_stats
{
  char name[INTF_NAME_SIZE];
  lldp_tx_rx lldp_dir;
  int tx_packets;
  int rx_packets;
  int rx_discared;
  int rx_unrecognized;
  struct lldp_stats *next;
}lldp_intf_stats;

typedef struct lldp_nbr_info
{
  char name[INTF_NAME_SIZE];
  int insert_count;
  int delete_count;
  int drop_count;
  int ageout_count;
  char  chassis_id[256];
  char  port_id[256];
  char  chassis_ttl[10];
  struct lldp_nbr_info *next;
}lldp_neighbor_info;


/* Sets the LLDP global status.
   Takes true/false argument   */
static int lldp_set_global_status(const char *status)
{
  const struct ovsrec_open_vswitch *row = NULL;

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort();
     return CMD_OVSDB_FAILURE;
  }

  if(strcmp("true",status) == 0)
     smap_replace(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_ENABLE, status);
  else
    smap_remove(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_ENABLE);

  ovsrec_open_vswitch_set_other_config(row, &row->other_config);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_lldp_set_global_status,
       lldp_set_global_status_cmd,
       "feature lldp",
       "Enables or disables the selected feature.\n"
       "Enables or disables LLDP on the device.\n")
{
  return lldp_set_global_status("true");
}

DEFUN (cli_lldp_no_set_global_status,
       lldp_no_set_global_status_cmd,
       "no feature lldp",
        NO_STR
       "Enables or disables the selected feature.\n"
       "Enables or disables LLDP on the device.\n")
{
  return lldp_set_global_status("false");
}

/* Sets LLDP hold time if the hold time passed is non-default.
   If the holdtime is default then deletes the key from the column. */
static int set_global_hold_time(const char *hold_time)
{
  const struct ovsrec_open_vswitch *row = NULL;


  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort();
    return CMD_OVSDB_FAILURE;
  }

  if( OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT != atoi(hold_time))
    smap_replace(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD, hold_time);
  else
    smap_remove(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD);

  ovsrec_open_vswitch_set_other_config(row, &row->other_config);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lldp_set_hold_time,
       lldp_set_global_hold_time_cmd,
       "lldp holdtime <2-10>",
       "Configure LLDP parameters.\n"
       "The amount of time a receiving device should hold the information sent by your device before discarding it.\n"
       "The range is 2 to 10; the default is 4.\n")
{
  return set_global_hold_time(argv[0]);
}

DEFUN (cli_lldp_no_set_hold_time,
       lldp_no_set_global_hold_time_cmd,
       "no lldp holdtime",
        NO_STR
       "Configure LLDP parameters.\n"
       "The amount of time a receiving device should hold the information sent by your device before discarding it.\n")
{
  return set_global_hold_time(OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT);
}

/* Sets LLDP transmission time if the transmision time passed is non-default.
   If the transmission time is default then deletes the key from the column. */
static int lldp_set_global_timer(const char *timer)
{
  const struct ovsrec_open_vswitch *row = NULL;

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort();
    return CMD_OVSDB_FAILURE;
  }

  if(OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT != atoi(timer))
    smap_replace(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL, timer);
  else
    smap_remove(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL);

  ovsrec_open_vswitch_set_other_config(row, &row->other_config);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lldp_set_timer,
       lldp_set_global_timer_cmd,
       "lldp timer <5-32768>",
       "Configure LLDP parameters.\n"
       "The interval at which LLDP status updates are transmitted to neighbors in seconds.\n"
        "The range is 5 to 32768 seconds; the default is 30 seconds.\n")
{
  return lldp_set_global_timer(argv[0]);
}

DEFUN (cli_no_lldp_set_timer,
       lldp_no_set_global_timer_cmd,
       "no lldp timer",
       NO_STR
       "Configure LLDP parameters.\n"
       "The interval at which LLDP status updates are transmitted to neighbors in seconds.\n")
{
  return lldp_set_global_timer(OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT);
}

/* Sets or increments lldp_num_clear_counters_requested value. */
DEFUN (cli_lldp_clear_counters,
       lldp_clear_counters_cmd,
       "lldp clear counters",
       "Configure LLDP parameters.\n"
       "Clear LLDP information.\n"
       "Clear LLDP counters.\n")
{
  const struct ovsrec_open_vswitch *row = NULL;
  int clear_counter = 0;
  char buffer[10];

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);
  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort();
     return CMD_OVSDB_FAILURE;
  }

  clear_counter = smap_get_int(&row->status, "lldp_num_clear_counters_requested", 0);
  clear_counter++;

  sprintf(buffer, "%d", clear_counter);
  smap_replace(&row->status, "lldp_num_clear_counters_requested", buffer);
  ovsrec_open_vswitch_set_status(row, &row->status);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

/* Sets or increments lldp_num_clear_table_requested value. */
DEFUN (cli_lldp_clear_neighbors,
       lldp_clear_neighbors_cmd,
       "lldp clear neighbors",
       "Configure LLDP parameters.\n"
       "Clear LLDP information.\n"
       "Clear LLDP neighbor tables.\n")
{
  const struct ovsrec_open_vswitch *row = NULL;
  int clear_neighbor_table = 0;
  char buffer[10];

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);
  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort();
    return CMD_OVSDB_FAILURE;
  }

  clear_neighbor_table = smap_get_int(&row->status, "lldp_num_clear_table_requested", 0);
  clear_neighbor_table++;

  sprintf(buffer, "%d", clear_neighbor_table);
  smap_replace(&row->status, "lldp_num_clear_table_requested", buffer);
  ovsrec_open_vswitch_set_status(row, &row->status);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

/* Sets the LLDP TLV value to true/false. */
static int
lldp_set_tlv(const char *tlv_name, const char *status)
{
  const struct ovsrec_open_vswitch *row = NULL;
  char tlv[50]={0};

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort();
    return CMD_OVSDB_FAILURE;
  }

  if(0 == strcmp(tlv_name, "ManagementAddress"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE);
  else if (0 == strcmp(tlv_name, "PortDescription"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE);
  else if (0 == strcmp(tlv_name, "Port-VlanID"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "Port-VlanName"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE);
  else if(0 == strcmp(tlv_name, "Port-Protocol-VlanId"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "Port-ProtocolID"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "SystemCapabilites"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE);
  else if(0 == strcmp(tlv_name, "SystemDescription"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE);
  else if(0 == strcmp(tlv_name, "SystemName"))
    strcpy(tlv, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE);

  if(strcmp("false",status) == 0)
    smap_replace(&row->other_config, tlv, status);
  else
    smap_remove(&row->other_config, tlv);

  ovsrec_open_vswitch_set_other_config(row, &row->other_config);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_lldp_select_tlv,
       lldp_select_tlv_cmd,
       "lldp select-tlv (ManagementAddress | PortDescription | Port-VlanID | Port-VlanName |\
                         Port-Protocol-VlanId | Port-ProtocolID | SystemCapabilites |\
                         SystemDescription | SystemName)",
       "Configure LLDP parameters\n"
       "Specifies the TLVs to send and receive in LLDP packets.\n")
{
  return lldp_set_tlv(argv[0],"true");
}

DEFUN (cli_no_lldp_select_tlv,
       lldp_no_select_tlv_cmd,
       "no lldp select-tlv (ManagementAddress | PortDescription | Port-VlanID | Port-VlanName |\
                            Port-Protocol-VlanId | Port-ProtocolID | SystemCapabilites |\
                            SystemDescription | SystemName)",
       NO_STR
       "Configure LLDP parameters.\n"
       "Specifies the TLVs to send and receive in LLDP packets.\n")
{
  return lldp_set_tlv(argv[0],"false");
}

/* Sets management address to be advertised by LLDP. */
static int lldp_set_mgmt_address(const char *status, boolean set)
{
  const struct ovsrec_open_vswitch *row = NULL;

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);
  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort();
    return CMD_OVSDB_FAILURE;
  }

  if(set)
    smap_replace(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR, status);
  else
    smap_remove(&row->other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR);

  ovsrec_open_vswitch_set_other_config(row, &row->other_config);

  if(cli_do_config_finish())
    return CMD_SUCCESS;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lldp_set_mgmt_address,
       lldp_set_mgmt_address_cmd,
       "lldp management-address (A.B.C.D | X:X::X:X)",
       "Configure LLDP parameters.\n"
       "LLDP Management IP Address to be sent in TLV.\n")
{
  return lldp_set_mgmt_address(argv[0], true);
}

DEFUN (cli_lldp_set_no_mgmt_address,
       lldp_set_no_mgmt_address_cmd,
       "no lldp management-address",
       NO_STR
       "Configure LLDP parameters.\n"
       "LLDP Management IP Address to be sent in TLV.\n")
{
  return lldp_set_mgmt_address(argv[0], false);
}

/* Prints TLVs enabled for LLDP*/
void
lldp_show_tlv(struct ovsrec_open_vswitch *row)
{
  bool tlv_set = false;

  vty_out (vty, "%sTLVs advertised: %s", VTY_NEWLINE, VTY_NEWLINE);
  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Management Adress %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port description %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port VLAN-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_VLAN_ID_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port Protocol VLAN-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port VLAN Name %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port Protocol-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);

  if(tlv_set)
     vty_out (vty, "System capabilities %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "System description %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE,
                           OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "System name %s", VTY_NEWLINE);
}

DEFUN (cli_lldp_show_tlv,
       lldp_show_tlv_cmd,
       "show lldp tlv",
       "Show switch operation information. \n"
       "Show various LLDP settings.\n"
       "Show TLVs advertised by LLDP.\n")
{
  const struct ovsrec_open_vswitch *row = NULL;

  ovsdb_idl_run(idl);
  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
     return CMD_OVSDB_FAILURE;
  }

  lldp_show_tlv(row);

  return CMD_SUCCESS;
}

DEFUN (cli_lldp_show_intf_statistics,
       lldp_show_intf_statistics_cmd,
       "show lldp statistics IFNAME",
       "Show switch operation information. \n"
       "Show various LLDP settings.\n"
       "Show LLDP statistics.\n"
       "Specify the interface name.\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const char  *port_name = NULL;
  bool port_found = false;
  const struct ovsdb_datum *datum;
  static char *lldp_interface_statistics_keys [] = {
    INTERFACE_STATISTICS_LLDP_TX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_DISCARDED_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_UNRECOGNIZED_COUNT
  };

  unsigned int index;

  ovsdb_idl_run(idl);
  vty_out (vty, "LLDP statistics: %s%s", VTY_NEWLINE, VTY_NEWLINE);

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
     if(0 == strcmp(argv[0],ifrow->name))
     {
        union ovsdb_atom atom;

        port_found = true;
        vty_out (vty, "Port Name: %s%s", ifrow->name, VTY_NEWLINE);

        datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

        atom.string = lldp_interface_statistics_keys[0];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets transmitted :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys [1];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys[2];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received and discarded :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys[3];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received and unrecognized :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);
        break;
     }
  }

  if(!port_found)
  {
    vty_out (vty, "Error: Wrong interface name.%s", VTY_NEWLINE);
    return CMD_WARNING;
  }

  return CMD_SUCCESS;
}

DEFUN (cli_lldp_show_config,
       lldp_show_config_cmd,
       "show lldp configuration ",
       "Show switch operation information.\n"
       "Show various LLDP settings.\n"
       "Show LLDP configuration. \n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_open_vswitch *row = NULL;
  bool lldp_enabled = false;
  int tx_interval = 0;
  int hold_time = 0;
  lldp_intf_stats *intf_stats = NULL;
  lldp_intf_stats *new_intf_stats = NULL;
  lldp_intf_stats *temp = NULL, *current = NULL;

  ovsdb_idl_run(idl);
  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
     return CMD_OVSDB_FAILURE;
  }

  vty_out(vty, "LLDP Global Configuration:%s%s", VTY_NEWLINE, VTY_NEWLINE);

  lldp_enabled = smap_get_bool(&row->other_config,
                               OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_ENABLE,
                               OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_ENABLE_DEFAULT);

  tx_interval = smap_get_int(&row->other_config,
                               OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL,
                               OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT);

  hold_time = smap_get_int(&row->other_config,
                             OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD,
                             OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT);

  vty_out(vty, "LLDP Enabled :%s%s", lldp_enabled?"Yes":"No",VTY_NEWLINE);

  vty_out(vty, "LLDP Transmit Interval :%d%s", tx_interval, VTY_NEWLINE);

  vty_out(vty, "LLDP Hold time Multiplier :%d%s", hold_time, VTY_NEWLINE);

  lldp_show_tlv(row);

  vty_out(vty, "%sLLDP Port Configuration:%s%s", VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE);
  vty_out(vty, "%-6s","Port");
  vty_out(vty, "%-25s","Transmission-enabled");
  vty_out(vty, "%-25s","Receive-enabled");
  vty_out(vty, "%s", VTY_NEWLINE);

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    new_intf_stats = xcalloc(1, sizeof *new_intf_stats);
    char *lldp_enable_dir = smap_get(&ifrow->other_config , INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);

    memset(new_intf_stats->name, '\0', INTF_NAME_SIZE);
    strncpy(new_intf_stats->name, ifrow->name, INTF_NAME_SIZE);
    if(!lldp_enable_dir)
    {
      new_intf_stats->lldp_dir = LLDP_TXRX;
    }
    else if(strcmp(lldp_enable_dir, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF) == 0)
    {
      new_intf_stats->lldp_dir = LLDP_OFF;
    }
    else if(strcmp(lldp_enable_dir, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX) == 0)
    {
      new_intf_stats->lldp_dir = LLDP_TX;
    }
    else if(strcmp(lldp_enable_dir, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX) == 0)
    {
      new_intf_stats->lldp_dir = LLDP_RX;
    }
    if((intf_stats == NULL) || (strncmp(intf_stats->name, new_intf_stats->name, INTF_NAME_SIZE) > 0))
    {
      new_intf_stats->next = intf_stats;
      intf_stats = new_intf_stats;
    }
    else
    {
      current = intf_stats;

      while(current->next != NULL && (strncmp(current->next->name, new_intf_stats->name, INTF_NAME_SIZE) < 0))
      {
        current = current->next;
      }
      new_intf_stats->next = current->next;
      current->next = new_intf_stats;
    }
  }

  current = intf_stats;
  while(NULL != current)
  {
    vty_out (vty, "%-6s", current->name);

    if(current->lldp_dir == LLDP_OFF)
    {
      vty_out(vty, "%-25s", "No");
      vty_out(vty, "%-25s", "No");
    }
    else if(current->lldp_dir == LLDP_TX)
    {
      vty_out(vty, "%-25s", "Yes");
      vty_out(vty, "%-25s", "No");
    }
    else if(current->lldp_dir == LLDP_RX)
    {
      vty_out(vty, "%-25s", "No");
      vty_out(vty, "%-25s", "Yes");
    }
    else if(current->lldp_dir == LLDP_TXRX)
    {
      vty_out(vty, "%-25s", "Yes");
      vty_out(vty, "%-25s", "Yes");
    }
    printf("\n");
    current = current->next;
  }

  temp = intf_stats;
  current = intf_stats;
  while(NULL != current)
  {
    temp = current;
    current = current->next;
    free(temp);
  }

  return CMD_SUCCESS;
}

DEFUN (cli_lldp_show_statistics,
       lldp_show_statistics_cmd,
       "show lldp statistics",
       "Show switch operation information.\n"
       "Show various LLDP settings.\n"
       "Show LLDP statistics.\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_open_vswitch *row = NULL;
  bool lldp_enabled = false;
  int tx_interval = 0;
  int hold_time = 0;
  lldp_intf_stats *intf_stats = NULL;
  lldp_intf_stats *new_intf_stats = NULL;
  lldp_intf_stats *temp = NULL, *current = NULL;
  const struct ovsdb_datum *datum;
  static char *lldp_interface_statistics_keys [] = {
    INTERFACE_STATISTICS_LLDP_TX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_DISCARDED_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_UNRECOGNIZED_COUNT
  };
  unsigned int total_tx_packets = 0;
  unsigned int total_rx_packets = 0;
  unsigned int total_rx_discared = 0;
  unsigned int total_rx_unrecognized = 0;
  unsigned int index;

  ovsdb_idl_run(idl);
  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
     return CMD_OVSDB_FAILURE;
  }

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    union ovsdb_atom atom;
    new_intf_stats = xcalloc(1, sizeof *new_intf_stats);

    memset(new_intf_stats->name, '\0', INTF_NAME_SIZE);
    strncpy(new_intf_stats->name, ifrow->name, INTF_NAME_SIZE);

    datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

    atom.string = lldp_interface_statistics_keys[0];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_stats->tx_packets = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_statistics_keys [1];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_stats->rx_packets = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_statistics_keys[2];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_stats->rx_discared = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_statistics_keys[3];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_stats->rx_unrecognized = (index == UINT_MAX)? 0 : datum->values[index].integer;

    if((intf_stats == NULL) || (strncmp(intf_stats->name, new_intf_stats->name, INTF_NAME_SIZE) > 0))
    {
      new_intf_stats->next = intf_stats;
      intf_stats = new_intf_stats;
    }
    else
    {
      current = intf_stats;

      while(current->next != NULL && (strncmp(current->next->name, new_intf_stats->name, INTF_NAME_SIZE) < 0))
      {
        current = current->next;
      }
      new_intf_stats->next = current->next;
      current->next = new_intf_stats;
    }
  }

  current = intf_stats;
  while(NULL != current)
  {
    total_tx_packets += current->tx_packets;
    total_rx_packets += current->rx_packets;
    total_rx_discared += current->rx_discared;
    total_rx_unrecognized += current->rx_unrecognized;
    current = current->next;
  }

  vty_out(vty, "LLDP Global statistics:\n\n");
  vty_out(vty, "Total Packets transmitted : %lu\n",total_tx_packets);
  vty_out(vty, "Total Packets received : %lu\n",total_rx_packets);
  vty_out(vty, "Total Packet received and discarded : %lu\n",total_rx_discared);
  vty_out(vty, "Total TLVs unrecognized : %lu\n",total_rx_unrecognized);

  vty_out(vty, "LLDP Port Statistics:\n");
  vty_out(vty, "%-10s","Port-ID");
  vty_out(vty, "%-15s","Tx-Packets");
  vty_out(vty, "%-15s","Rx-packets");
  vty_out(vty, "%-20s","Rx-discarded");
  vty_out(vty, "%-20s","TLVs-Unknown");
  vty_out(vty, "%s", VTY_NEWLINE);

  current = intf_stats;
  while(NULL != current)
  {
    vty_out (vty, "%-10s", current->name);
    vty_out (vty, "%-15lu", current->tx_packets);
    vty_out (vty, "%-15lu", current->rx_packets);
    vty_out (vty, "%-20lu", current->rx_discared);
    vty_out (vty, "%-20lu", current->rx_unrecognized);
    printf("\n");
    current = current->next;
  }

  temp = intf_stats;
  current = intf_stats;
  while(NULL != current)
  {
    temp = current;
    current = current->next;
    free(temp);
  }

  return CMD_SUCCESS;
}

DEFUN (cli_lldp_show_neighbor_info,
       lldp_show_neighbor_info_cmd,
       "show lldp neighbor-info",
       "Show switch operation information.\n"
       "Show various LLDP settings.\n"
       "Show global LLDP neighbor information.\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_open_vswitch *row = NULL;
  lldp_neighbor_info *nbr_info = NULL;
  lldp_neighbor_info *new_intf_nbr_info = NULL;
  lldp_neighbor_info *temp = NULL, *current = NULL;
  const struct ovsdb_datum *datum;
  static char *lldp_interface_neighbor_info_keys [] = {
    INTERFACE_STATISTICS_LLDP_INSERT_COUNT,
    INTERFACE_STATISTICS_LLDP_DELETE_COUNT,
    INTERFACE_STATISTICS_LLDP_DROP_COUNT,
    INTERFACE_STATISTICS_LLDP_AGEOUT_COUNT,
    "chassis_id",
    "port_id",
    "chassis_ttl"
  };
  unsigned int total_insert_count = 0;
  unsigned int total_delete_count = 0;
  unsigned int total_drop_count = 0;
  unsigned int total_ageout_count = 0;
  unsigned int index = 0;

  ovsdb_idl_run(idl);
  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
     return CMD_OVSDB_FAILURE;
  }

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    union ovsdb_atom atom;
    new_intf_nbr_info = xcalloc(1, sizeof *new_intf_nbr_info);

    memset(new_intf_nbr_info->name, '\0', INTF_NAME_SIZE);
    strncpy(new_intf_nbr_info->name, ifrow->name, INTF_NAME_SIZE);

    datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

    atom.string = lldp_interface_neighbor_info_keys[0];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_nbr_info->insert_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_neighbor_info_keys[1];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_nbr_info->delete_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_neighbor_info_keys[2];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_nbr_info->drop_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;

    atom.string = lldp_interface_neighbor_info_keys[3];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    new_intf_nbr_info->ageout_count = (index == UINT_MAX)? 0 : datum->values[index].integer;

    datum = ovsrec_interface_get_lldp_neighbor_info(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_STRING);

    atom.string = lldp_interface_neighbor_info_keys[4];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
      strncpy(new_intf_nbr_info->chassis_id, datum->values[index].string, 256);

    atom.string = lldp_interface_neighbor_info_keys[5];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
      strncpy(new_intf_nbr_info->port_id, datum->values[index].string, 256);


    atom.string = lldp_interface_neighbor_info_keys[6];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
      strncpy(new_intf_nbr_info->chassis_ttl, datum->values[index].string, 256);

    /* Insert node in sorted order */
    if((nbr_info == NULL) || (strncmp(nbr_info->name, new_intf_nbr_info->name, INTF_NAME_SIZE) > 0))
    {
      new_intf_nbr_info->next = nbr_info;
      nbr_info = new_intf_nbr_info;
    }
    else
    {
      current = nbr_info;

      while(current->next != NULL && (strncmp(current->next->name, new_intf_nbr_info->name, INTF_NAME_SIZE) < 0))
      {
        current = current->next;
      }
      new_intf_nbr_info->next = current->next;
      current->next = new_intf_nbr_info;
    }
  }

  current = nbr_info;
  while(NULL != current)
  {
    total_insert_count += current->insert_count;
    total_delete_count += current->delete_count;
    total_drop_count += current->drop_count;
    total_ageout_count += current->ageout_count;
    current = current->next;
  }

  vty_out(vty, "\n");
  vty_out(vty, "Total neighbor entries : %lu\n", total_insert_count);
  vty_out(vty, "Total neighbor entries deleted : %lu\n", total_delete_count);
  vty_out(vty, "Total neighbor entries dropped : %lu\n", total_drop_count);
  vty_out(vty, "Total neighbor entries aged-out : %lu\n", total_ageout_count);

  vty_out(vty, "\n");
  vty_out(vty, "%-15s","Local Port");
  vty_out(vty, "%-25s","Neighbor Chassis-ID");
  vty_out(vty, "%-25s","Neighbor Port-ID");
  vty_out(vty, "%-10s","TTL");
  vty_out(vty, "%s", VTY_NEWLINE);

  current = nbr_info;
  while(NULL != current)
  {
    vty_out (vty, "%-15s", current->name);
    vty_out (vty, "%-25s", current->chassis_id);
    vty_out (vty, "%-25s", current->port_id);
    vty_out (vty, "%-10s", current->chassis_ttl);
    printf("\n");
    current = current->next;
  }

  temp = nbr_info;
  current = nbr_info;
  while(NULL != current)
  {
    temp = current;
    current = current->next;
    free(temp);
  }

  return CMD_SUCCESS;
}




DEFUN (cli_lldp_show_intf_neighbor_info,
       lldp_show_intf_neighbor_info_cmd,
       "show lldp neighbor-info IFNAME",
       "Show switch operation information.\n"
       "Show various LLDP settings.\n"
       "Show global LLDP neighbor information.\n"
       "Specify the interface name.")
{
  const struct ovsrec_interface *ifrow = NULL;
  const char  *port_name = NULL;
  bool port_found = false;
  const struct ovsdb_datum *datum;
  static char *lldp_interface_neighbor_info_keys [] = {
    INTERFACE_STATISTICS_LLDP_INSERT_COUNT,
    INTERFACE_STATISTICS_LLDP_DELETE_COUNT,
    INTERFACE_STATISTICS_LLDP_DROP_COUNT,
    INTERFACE_STATISTICS_LLDP_AGEOUT_COUNT,
    "chassis_id",
    "port_id",
    "chassis_ttl"
  };

  unsigned int index;

  ovsdb_idl_run(idl);

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
     if(0 == strcmp(argv[0],ifrow->name))
     {
        union ovsdb_atom atom;

        port_found = true;
        vty_out (vty, "Port: %s%s", ifrow->name, VTY_NEWLINE);

        datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);
        atom.string = lldp_interface_neighbor_info_keys[0];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[1];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries deleted :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[2];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries dropped :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[3];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries age-out :%d\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        datum = ovsrec_interface_get_lldp_neighbor_info(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_STRING);

        atom.string = lldp_interface_neighbor_info_keys[4];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Chassis-ID :%s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[5];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Port-ID :%s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[6];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "TTL :%s\n",(index == UINT_MAX)? "" : datum->values[index].string);
        break;
     }
  }

  if(!port_found)
  {
    vty_out (vty, "Error: Wrong interface name.%s", VTY_NEWLINE);
    return CMD_WARNING;
  }

  return CMD_SUCCESS;

}


int lldp_ovsdb_if_lldp_state(const char *ifvalue, const lldp_tx_rx state) {
  const struct ovsrec_interface * row = NULL;
  char *state_value;

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return 1;
  }

  row = ovsrec_interface_first(idl);
  if(!row)
  {
    VLOG_ERR("unable to fetch a row.");
    cli_do_config_abort();
    return 1;
  }

  OVSREC_INTERFACE_FOR_EACH(row, idl)
  {
    if(strcmp(row->name, ifvalue) == 0)
    {
      state_value = smap_get(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);
      if(state == LLDP_TX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF) == 0))
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX);
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX) == 0)
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX);
        }
      }
      else if (state == LLDP_RX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF) == 0))
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX);
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX) == 0)
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX);
        }
      }

      ovsrec_interface_set_other_config(row, &row->other_config);
      break;
    }
  }

  if(cli_do_config_finish())
    return 0;
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
  }

  return 1;
}

DEFUN (lldp_if_lldp_tx,
       lldp_if_lldp_tx_cmd,
       "lldp transmission",
       "Set the transmission\n"
       "Set the trans\n")
{
  if(lldp_ovsdb_if_lldp_state((char*)vty->index, LLDP_TX) != 0)
    VLOG_ERR("Failed to set lldp transmission in Interface context");

  return CMD_SUCCESS;
}

DEFUN (lldp_if_lldp_rx,
       lldp_if_lldp_rx_cmd,
       "lldp reception",
       "Set the reception\n"
       "Set the recv\n")
{
  if(lldp_ovsdb_if_lldp_state((char*)vty->index, LLDP_RX) != 0)
    VLOG_ERR("Failed to set lldp reception in Interface context");

  return CMD_SUCCESS;
}


int lldp_ovsdb_if_lldp_nodirstate(const char *ifvalue, const lldp_tx_rx state)
{
  const struct ovsrec_interface * row = NULL;
  char *state_value;
  boolean validstate = false, ifexists = false;

  if(!cli_do_config_start())
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    return 1;
  }

  row = ovsrec_interface_first(idl);
  if(!row)
  {
    VLOG_ERR("unable to fetch a row.");
    cli_do_config_abort();
    return 1;
  }

  OVSREC_INTERFACE_FOR_EACH(row, idl)
  {
    if(strcmp(row->name, ifvalue) == 0)
    {
      ifexists = true;
      state_value = smap_get(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);
      if(state == LLDP_TX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0))
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX);
          validstate = true;
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX) == 0)
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF);
          validstate = true;
        }
      }
      else if (state == LLDP_RX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0))
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX);
          validstate = true;
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX) == 0)
        {
          smap_replace(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF);
          validstate = true;
        }
      }

      if (true == validstate)
      {
        ovsrec_interface_set_other_config(row, &row->other_config);
      }
      break;
    }
  }

  if ((true == ifexists) && (true == validstate))
  {
    if(cli_do_config_finish())
      return 0;
    else
    {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    }
  }
  else
  {
    if (false == ifexists)
    {
      VLOG_ERR("unable to find the interface");
    }
    if ((true == ifexists) && (false == validstate))
    {
      VLOG_ERR("ifrow other_config has invalid lldp dir state");
    }

    cli_do_config_abort();
  }

  return 1;
}

DEFUN (lldp_if_no_lldp_tx,
       lldp_if_no_lldp_tx_cmd,
       "no lldp transmission",
       "Unset the transmission\n"
       "Unset the trans\n")
{
  if(lldp_ovsdb_if_lldp_nodirstate((char*)vty->index, LLDP_TX) != 0)
    VLOG_ERR("Failed to set lldp transmission in Interface context");

  return CMD_SUCCESS;
}

DEFUN (lldp_if_no_lldp_rx,
       lldp_if_no_lldp_rx_cmd,
       "no lldp reception",
       "Unset the reception\n"
       "Unset the recv\n")
{
  if(lldp_ovsdb_if_lldp_nodirstate((char*)vty->index, LLDP_RX) != 0)
    VLOG_ERR("Failed to set lldp reception in Interface context");

  return CMD_SUCCESS;
}
/* Install LLDP related vty commands. */
void
lldp_vty_init (void)
{

  install_element (CONFIG_NODE, &lldp_set_global_status_cmd);
  install_element (CONFIG_NODE, &lldp_no_set_global_status_cmd);
  install_element (CONFIG_NODE, &lldp_set_global_hold_time_cmd);
  install_element (CONFIG_NODE, &lldp_no_set_global_hold_time_cmd);
  install_element (CONFIG_NODE, &lldp_set_global_timer_cmd);
  install_element (CONFIG_NODE, &lldp_no_set_global_timer_cmd);
  install_element (CONFIG_NODE, &lldp_select_tlv_cmd);
  install_element (CONFIG_NODE, &lldp_no_select_tlv_cmd);
  install_element (CONFIG_NODE, &lldp_clear_counters_cmd);
  install_element (CONFIG_NODE, &lldp_clear_neighbors_cmd);
  install_element (CONFIG_NODE, &lldp_set_mgmt_address_cmd);
  install_element (CONFIG_NODE, &lldp_set_no_mgmt_address_cmd);
  install_element (ENABLE_NODE, &lldp_show_tlv_cmd);
  install_element (ENABLE_NODE, &lldp_show_intf_statistics_cmd);
  install_element (ENABLE_NODE, &lldp_show_intf_neighbor_info_cmd);
  install_element (ENABLE_NODE, &lldp_show_config_cmd);
  install_element (ENABLE_NODE, &lldp_show_statistics_cmd);
  install_element (INTERFACE_NODE, &lldp_if_lldp_tx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_lldp_rx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_no_lldp_tx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_no_lldp_rx_cmd);
  install_element (ENABLE_NODE, &lldp_show_neighbor_info_cmd);
}
