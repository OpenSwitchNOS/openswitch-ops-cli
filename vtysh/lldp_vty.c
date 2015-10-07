/* LLDP CLI commands
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
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "lldp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"

VLOG_DEFINE_THIS_MODULE(vtysh_lldp_cli);
extern struct ovsdb_idl *idl;

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

typedef struct
{
  char name[INTF_NAME_SIZE];
  int insert_count;
  int delete_count;
  int drop_count;
  int ageout_count;
  char  chassis_id[LLDP_MAX_BUF_SIZE];
  char  port_id[LLDP_MAX_BUF_SIZE];
  char  chassis_ttl[LLDP_STR_CHASSIS_TLV_LENGTH];
}lldp_neighbor_info;


/* Sets the LLDP global status.
   Takes true/false argument   */
static int lldp_set_global_status(const char *status)
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);

  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_other_config, &row->other_config);

  if(strcmp("true",status) == 0)
    smap_replace(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE, status);
  else
    smap_remove(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE);

  ovsrec_system_set_other_config(row, &smap_other_config);

  txn_status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);
  if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_lldp_set_global_status,
       lldp_set_global_status_cmd,
       "feature lldp",
       "Enables or disables the selected feature\n"
       CONFIG_LLDP_STR)
{
  return lldp_set_global_status("true");
}

DEFUN (cli_lldp_no_set_global_status,
       lldp_no_set_global_status_cmd,
       "no feature lldp",
        NO_STR
       "Enables or disables the selected feature\n"
       CONFIG_LLDP_STR)
{
  return lldp_set_global_status("false");
}

/* Sets LLDP hold time if the hold time passed is non-default.
   If the holdtime is default then deletes the key from the column. */
static int set_global_hold_time(const char *hold_time)
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn* status_txn = cli_do_config_start();
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_other_config, &row->other_config);
  if( SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT != atoi(hold_time))
    smap_replace(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD, hold_time);
  else
    smap_remove(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD);

  ovsrec_system_set_other_config(row, &smap_other_config);

  status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lldp_set_hold_time,
       lldp_set_global_hold_time_cmd,
       "lldp holdtime <2-10>",
       CONFIG_LLDP_STR
       "Set hold time multiplier, total hold time is calculated by multiplying it with LLDP timer\n"
       "Hold time multiplier(Default:4)\n")
{
  return set_global_hold_time(argv[0]);
}

DEFUN (cli_lldp_no_set_hold_time,
       lldp_no_set_global_hold_time_cmd,
       "no lldp holdtime [<2-10>]",
        NO_STR
       CONFIG_LLDP_STR
       "Set hold time multiplier, total hold time is calculated by multiplying it with LLDP timer\n"
       "Hold time multiplier(Default:4)\n")
{
  char def_holdtime[LLDP_TIMER_MAX_STRING_LENGTH]={0};
  snprintf(def_holdtime,LLDP_TIMER_MAX_STRING_LENGTH, "%d",SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT);
  return set_global_hold_time(def_holdtime);
}

/* Sets LLDP transmission time if the transmision time passed is non-default.
   If the transmission time is default then deletes the key from the column. */
static int lldp_set_global_timer(const char *timer)
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_other_config, &row->other_config);
  if(SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT != atoi(timer))
    smap_replace(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL, timer);
  else
    smap_remove(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL);

  ovsrec_system_set_other_config(row, &smap_other_config);

  status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
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
       CONFIG_LLDP_STR
       "The interval(in seconds) at which LLDP status updates are transmitted to neighbors\n"
        "The range is 5 to 32768 seconds (Default:30 seconds)\n")
{
  return lldp_set_global_timer(argv[0]);
}

DEFUN (cli_no_lldp_set_timer,
       lldp_no_set_global_timer_cmd,
       "no lldp timer [<5-32768>]",
       NO_STR
       CONFIG_LLDP_STR
       "The interval(in seconds) at which LLDP status updates are transmitted to neighbors\n"
       "The range is 5 to 32768 seconds (Default:30 seconds)\n")
{
  char def_global_time[LLDP_TIMER_MAX_STRING_LENGTH]={0};
  snprintf(def_global_time,LLDP_TIMER_MAX_STRING_LENGTH, "%d",SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT);
  return lldp_set_global_timer(def_global_time);
}

/* Sets or increments lldp_num_clear_counters_requested value. */
DEFUN (cli_lldp_clear_counters,
       lldp_clear_counters_cmd,
       "lldp clear counters",
       CONFIG_LLDP_STR
       "Clear LLDP information\n"
       "Clear LLDP counters\n")
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn* status_txn = cli_do_config_start();
  int clear_counter = 0;
  char buffer[10] = {0};
  struct smap smap_status;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);
  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  clear_counter = smap_get_int(&row->status, "lldp_num_clear_counters_requested", 0);
  clear_counter++;

  sprintf(buffer, "%d", clear_counter);
  smap_clone(&smap_status, &row->status);
  smap_replace(&smap_status, "lldp_num_clear_counters_requested", buffer);
  ovsrec_system_set_status(row, &smap_status);

  status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_status);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
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
       CONFIG_LLDP_STR
       "Clear LLDP information\n"
       "Clear LLDP neighbor entries\n")
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  int clear_neighbor_table = 0;
  char buffer[10] = {0};
  struct smap smap_status;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);
  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  clear_neighbor_table = smap_get_int(&row->status, "lldp_num_clear_table_requested", 0);
  clear_neighbor_table++;

  sprintf(buffer, "%d", clear_neighbor_table);
  smap_clone(&smap_status, &row->status);
  smap_replace(&smap_status, "lldp_num_clear_table_requested", buffer);
  ovsrec_system_set_status(row, &smap_status);

  status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_status);

  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
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
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn* status_txn = cli_do_config_start();
  char tlv[50]={0};
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);

  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  if(0 == strcmp(tlv_name, "management-address"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE);
  else if (0 == strcmp(tlv_name, "port-description"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE);
  else if (0 == strcmp(tlv_name, "port-vlan-id"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "port-vlan-name"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE);
  else if(0 == strcmp(tlv_name, "port-protocol-vlan-id"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_VLAN_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "port-protocol-id"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE);
  else if(0 == strcmp(tlv_name, "system-capabilities"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE);
  else if(0 == strcmp(tlv_name, "system-description"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE);
  else if(0 == strcmp(tlv_name, "system-name"))
    strcpy(tlv, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE);

  smap_clone(&smap_other_config, &row->other_config);
  if(strcmp("false",status) == 0)
    smap_replace(&smap_other_config, tlv, status);
  else
    smap_remove(&smap_other_config, tlv);

  ovsrec_system_set_other_config(row, &smap_other_config);

  txn_status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);
  if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_lldp_select_tlv,
       lldp_select_tlv_cmd,
       "lldp select-tlv (management-address | port-description | port-vlan-id |\
                        port-vlan-name | port-protocol-vlan-id |\
                        port-protocol-id | system-capabilities |\
                        system-description | system-name)",
       CONFIG_LLDP_STR
       "Specifies the TLVs to send and receive in LLDP packets\n"
       "Select management-address TLV\n"
       "Select port-description TLV\n"
       "Select port-vlan-id TLV\n"
       "Select port-vlan-name TLV\n"
       "Select port-protocol-vlan-id TLV\n"
       "Select port-protocol-id TLV\n"
       "Select system-capabilities TLV\n"
       "Select system-description TLV\n"
       "Select system-name TLV\n")
{
  return lldp_set_tlv(argv[0],"true");
}

DEFUN (cli_no_lldp_select_tlv,
       lldp_no_select_tlv_cmd,
       "no lldp select-tlv (management-address | port-description | port-vlan-id |\
                        port-vlan-name | port-protocol-vlan-id |\
                        port-protocol-id | system-capabilities |\
                        system-description | system-name)",
       NO_STR
       CONFIG_LLDP_STR
       "Specifies the TLVs to send and receive in LLDP packets\n"
       "Select management-address TLV\n"
       "Select port-description TLV\n"
       "Select port-vlan-id TLV\n"
       "Select port-vlan-name TLV\n"
       "Select port-protocol-vlan-id TLV\n"
       "Select port-protocol-id TLV\n"
       "Select system-capabilities TLV\n"
       "Select system-description TLV\n"
       "Select system-name TLV\n")
{
  return lldp_set_tlv(argv[0],"false");
}

/* Sets management address to be advertised by LLDP. */
static int lldp_set_mgmt_address(const char *status, boolean set)
{
  const struct ovsrec_system *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_system_first(idl);
  if(!row)
  {
    VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_other_config, &row->other_config);
  if(set)
    smap_replace(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR, status);
  else
    smap_remove(&smap_other_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR);

  ovsrec_system_set_other_config(row, &smap_other_config);

  txn_status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);

  if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lldp_set_mgmt_address,
       lldp_set_mgmt_address_cmd,
       "lldp management-address (A.B.C.D | X:X::X:X)",
       CONFIG_LLDP_STR
       "LLDP management IP address to be sent in TLV\n"
       "LLDP management IPv4 address\n"
       "LLDP management IPv6 address\n")
{
  return lldp_set_mgmt_address(argv[0], true);
}

DEFUN (cli_lldp_set_no_mgmt_address,
       lldp_set_no_mgmt_address_cmd,
       "no lldp management-address",
       NO_STR
       CONFIG_LLDP_STR
       "LLDP management IP address to be sent in TLV\n")
{
  return lldp_set_mgmt_address(argv[0], false);
}

DEFUN (cli_lldp_set_no_mgmt_address_arg,
       lldp_set_no_mgmt_address_cmd_arg,
       "no lldp management-address (A.B.C.D | X:X::X:X)",
       NO_STR
       CONFIG_LLDP_STR
       "LLDP management IP address to be sent in TLV\n"
       "LLDP management IPv4 address\n"
       "LLDP management IPv6 address\n")
{
  return lldp_set_mgmt_address(argv[0], false);
}


/* Prints TLVs enabled for LLDP*/
void
lldp_show_tlv(const struct ovsrec_system *row)
{
  bool tlv_set = false;

  vty_out (vty, "%sTLVs advertised: %s", VTY_NEWLINE, VTY_NEWLINE);
  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Management Address %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port description %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port VLAN-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_VLAN_ID_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port Protocol VLAN-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port VLAN Name %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "Port Protocol-ID %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);

  if(tlv_set)
     vty_out (vty, "System capabilities %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "System description %s", VTY_NEWLINE);

  tlv_set = smap_get_bool(&row->other_config,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE,
                           SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_DEFAULT);
  if(tlv_set)
     vty_out (vty, "System name %s", VTY_NEWLINE);
}

DEFUN (cli_lldp_show_tlv,
       lldp_show_tlv_cmd,
       "show lldp tlv",
       SHOW_STR
       SHOW_LLDP_STR
       "Show TLVs advertised by LLDP\n")
{
  const struct ovsrec_system *row = NULL;

  row = ovsrec_system_first(idl);

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
       SHOW_STR
       SHOW_LLDP_STR
       "Show LLDP statistics\n"
       "Specify the interface name\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  bool port_found = false;
  const struct ovsdb_datum *datum;
  static char *lldp_interface_statistics_keys [] = {
    INTERFACE_STATISTICS_LLDP_TX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_DISCARDED_COUNT,
    INTERFACE_STATISTICS_LLDP_RX_UNRECOGNIZED_COUNT
  };

  unsigned int index;

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
        vty_out(vty, "Packets transmitted :%ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys [1];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received :%ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys[2];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received and discarded :%ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_statistics_keys[3];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Packets received and unrecognized :%ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);
        break;
     }
  }

  if(!port_found)
  {
    vty_out (vty, "Wrong interface name%s", VTY_NEWLINE);
    return CMD_WARNING;
  }

  return CMD_SUCCESS;
}

DEFUN (cli_lldp_show_config,
       lldp_show_config_cmd,
       "show lldp configuration ",
       SHOW_STR
       SHOW_LLDP_STR
       "Show LLDP configuration\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_system *row = NULL;
  bool lldp_enabled = false;
  int tx_interval = 0;
  int hold_time = 0;
  lldp_intf_stats *intf_stats = NULL;
  lldp_intf_stats *new_intf_stats = NULL;
  lldp_intf_stats *temp = NULL, *current = NULL;

  row = ovsrec_system_first(idl);

  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     return CMD_OVSDB_FAILURE;
  }

  vty_out(vty, "LLDP Global Configuration:%s%s", VTY_NEWLINE, VTY_NEWLINE);

  lldp_enabled = smap_get_bool(&row->other_config,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE_DEFAULT);

  tx_interval = smap_get_int(&row->other_config,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT);

  hold_time = smap_get_int(&row->other_config,
                             SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD,
                             SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT);

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
    if(ifrow && (0 != strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_SYSTEM)))
    {
       /* Skipping internal interfaces */
       continue;
    }
    new_intf_stats = xcalloc(1, sizeof *new_intf_stats);
    const char *lldp_enable_dir = smap_get(&ifrow->other_config , INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);

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
    else if(strcmp(lldp_enable_dir, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0)
    {
      new_intf_stats->lldp_dir = LLDP_TXRX;
    }
    if((NULL == intf_stats) || (strncmp(intf_stats->name, new_intf_stats->name, INTF_NAME_SIZE) > 0))
    {
      new_intf_stats->next = intf_stats;
      intf_stats = new_intf_stats;
    }
    else
    {
      current = intf_stats;

      while(NULL != current->next && (strncmp(current->next->name, new_intf_stats->name, INTF_NAME_SIZE) < 0))
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

DEFUN (cli_lldp_show_config_if,
       lldp_show_config_cmd_if,
       "show lldp configuration IFNAME",
       SHOW_STR
       SHOW_LLDP_STR
       "Show LLDP configuration\n"
       "Specify the interface name\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_system *row = NULL;
  bool lldp_enabled = false;
  int tx_interval = 0;
  int hold_time = 0;
  lldp_intf_stats *new_intf_stats = NULL;

  row = ovsrec_system_first(idl);

  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     return CMD_OVSDB_FAILURE;
  }

  vty_out(vty, "%sLLDP Global Configuration:%s", VTY_NEWLINE,VTY_NEWLINE);
  vty_out(vty,"--------------------------%s",VTY_NEWLINE);

  lldp_enabled = smap_get_bool(&row->other_config,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE_DEFAULT);

  tx_interval = smap_get_int(&row->other_config,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL,
                               SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT);

  hold_time = smap_get_int(&row->other_config,
                             SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD,
                             SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT);

  vty_out(vty, "LLDP Enabled              :%s%s", lldp_enabled?"Yes":"No",VTY_NEWLINE);

  vty_out(vty, "LLDP Transmit Interval    :%d%s", tx_interval, VTY_NEWLINE);

  vty_out(vty, "LLDP Hold time Multiplier :%d%s", hold_time, VTY_NEWLINE);


  vty_out(vty, "%sLLDP Port Configuration:%s", VTY_NEWLINE, VTY_NEWLINE);
  vty_out(vty,"--------------------------%s",VTY_NEWLINE);
  vty_out(vty, "%-6s","Port");
  vty_out(vty, "%-25s","Transmission-enabled");
  vty_out(vty, "%-25s","Reception-enabled");
  vty_out(vty, "%s", VTY_NEWLINE);

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    if(ifrow && (0 != strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_SYSTEM)))
    {
       /* Skipping internal interfaces */
       continue;
    }

    if (0 == strcmp(ifrow->name, argv[0]))
    {
      new_intf_stats = xcalloc(1, sizeof *new_intf_stats);
      if(new_intf_stats)
      {
            const char *lldp_enable_dir = smap_get(&ifrow->other_config , INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);

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
            else if(strcmp(lldp_enable_dir, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0)
            {
              new_intf_stats->lldp_dir = LLDP_TXRX;
            }
      }
      break;
    }
  }
  vty_out (vty, "%-6s", new_intf_stats->name);
  if(new_intf_stats->lldp_dir == LLDP_OFF)
  {
    vty_out(vty, "%-25s", "No");
    vty_out(vty, "%-25s", "No");
  }
  else if(new_intf_stats->lldp_dir == LLDP_TX)
  {
    vty_out(vty, "%-25s", "Yes");
    vty_out(vty, "%-25s", "No");
  }
  else if(new_intf_stats->lldp_dir == LLDP_RX)
  {
    vty_out(vty, "%-25s", "No");
    vty_out(vty, "%-25s", "Yes");
  }
  else if(new_intf_stats->lldp_dir == LLDP_TXRX)
  {
    vty_out(vty, "%-25s", "Yes");
    vty_out(vty, "%-25s", "Yes");
  }
  vty_out(vty, "%s%s", VTY_NEWLINE,VTY_NEWLINE);

  if(new_intf_stats)
    free(new_intf_stats);

  return CMD_SUCCESS;
}


DEFUN (cli_lldp_show_statistics,
       lldp_show_statistics_cmd,
       "show lldp statistics",
       SHOW_STR
       SHOW_LLDP_STR
       "Show LLDP statistics\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_system *row = NULL;
  lldp_intf_stats *intf_stats = NULL;
  lldp_intf_stats *new_intf_stats = NULL;
  lldp_intf_stats *temp = NULL, *current = NULL;
  const struct ovsdb_datum *datum = NULL;
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

  row = ovsrec_system_first(idl);

  if(!row)
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     return CMD_OVSDB_FAILURE;
  }

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    if(ifrow && (0 != strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_SYSTEM)))
    {
       /* Skipping internal interfaces */
       continue;
    }
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

    if((NULL == intf_stats) || (strncmp(intf_stats->name, new_intf_stats->name, INTF_NAME_SIZE) > 0))
    {
      new_intf_stats->next = intf_stats;
      intf_stats = new_intf_stats;
    }
    else
    {
      current = intf_stats;

      while(NULL != current->next && (strncmp(current->next->name, new_intf_stats->name, INTF_NAME_SIZE) < 0))
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
  vty_out(vty, "Total Packets transmitted : %u\n",total_tx_packets);
  vty_out(vty, "Total Packets received : %u\n",total_rx_packets);
  vty_out(vty, "Total Packet received and discarded : %u\n",total_rx_discared);
  vty_out(vty, "Total TLVs unrecognized : %u\n",total_rx_unrecognized);

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
    vty_out (vty, "%-15d", current->tx_packets);
    vty_out (vty, "%-15d", current->rx_packets);
    vty_out (vty, "%-20d", current->rx_discared);
    vty_out (vty, "%-20d", current->rx_unrecognized);
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

/* qsort comparator function.
 * This may need to be modified depending on the format of interface name
 */
static inline int compare_intf(const void*a, const void* b)
{
    lldp_neighbor_info* s1 = (lldp_neighbor_info*)a;
    lldp_neighbor_info* s2 = (lldp_neighbor_info*)b;
    uint i1=0,i2=0,i3=0,i4=0;

    /* For sorting number with 21-1 etc. name */
    sscanf(s1->name,"%d-%d",&i1,&i2);
    sscanf(s2->name,"%d-%d",&i3,&i4);

    if(i1 == i3)
    {
        if(i2 == i4)
            return 0;
        else if(i2 < i4)
            return -1;
        else
            return 1;
    }
    else if (i1 < i3)
        return -1;
    else
        return 1;

    return 0;
}

DEFUN (cli_lldp_show_neighbor_info,
       lldp_show_neighbor_info_cmd,
       "show lldp neighbor-info",
       SHOW_STR
       SHOW_LLDP_STR
       "Show global LLDP neighbor information\n")
{
  const struct ovsrec_interface *ifrow = NULL;
  const struct ovsrec_subsystem *row = NULL;
  lldp_neighbor_info *nbr_info = NULL;
  uint  iter = 0, nIntf = 0;
  const struct ovsdb_datum *datum = NULL;
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

  row = ovsrec_subsystem_first(idl);

  if(row)
  {
     nIntf = row->n_interfaces;
     if(!nIntf)
     {
        VLOG_ERR(OVSDB_LLDP_INTF_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
     }
  }
  else
  {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_OVSDB_FAILURE;
  }

  nbr_info = xcalloc(nIntf, sizeof (lldp_neighbor_info));

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
    union ovsdb_atom atom;

   if(ifrow && (0 != strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_SYSTEM)))
   {
      /* Skipping internal interfaces */
      continue;
   }
  /*
   * If for some reason the n_interfaces doesnt comply with
   * OVSREC_INTERFACE_FOR_EACH rows.
  */
   if(iter >= nIntf)
    break;

    strncpy(nbr_info[iter].name, ifrow->name, INTF_NAME_SIZE);
    datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

    atom.string = lldp_interface_neighbor_info_keys[0];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    nbr_info[iter].insert_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;
    total_insert_count += nbr_info[iter].insert_count;

    atom.string = lldp_interface_neighbor_info_keys[1];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    nbr_info[iter].delete_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;
    total_delete_count += nbr_info[iter].delete_count;

    atom.string = lldp_interface_neighbor_info_keys[2];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    nbr_info[iter].drop_count = (index == UINT_MAX)? 0 : datum->values[index].integer ;
    total_drop_count += nbr_info[iter].drop_count;

    atom.string = lldp_interface_neighbor_info_keys[3];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    nbr_info[iter].ageout_count = (index == UINT_MAX)? 0 : datum->values[index].integer;
    total_ageout_count += nbr_info[iter].ageout_count;

    datum = ovsrec_interface_get_lldp_neighbor_info(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_STRING);

    atom.string = lldp_interface_neighbor_info_keys[4];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
       strncpy(nbr_info[iter].chassis_id, datum->values[index].string, 256);

    atom.string = lldp_interface_neighbor_info_keys[5];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
       strncpy(nbr_info[iter].port_id, datum->values[index].string, 256);

    atom.string = lldp_interface_neighbor_info_keys[6];
    index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
    if(index != UINT_MAX)
      strncpy(nbr_info[iter].chassis_ttl, datum->values[index].string, 256);

    iter++;
  }

  vty_out(vty, "\n");
  vty_out(vty, "Total neighbor entries : %u\n", total_insert_count);
  vty_out(vty, "Total neighbor entries deleted : %u\n", total_delete_count);
  vty_out(vty, "Total neighbor entries dropped : %u\n", total_drop_count);
  vty_out(vty, "Total neighbor entries aged-out : %u\n", total_ageout_count);

  vty_out(vty, "\n");
  vty_out(vty, "%-15s","Local Port");
  vty_out(vty, "%-25s","Neighbor Chassis-ID");
  vty_out(vty, "%-25s","Neighbor Port-ID");
  vty_out(vty, "%-10s","TTL");
  vty_out(vty, "%s", VTY_NEWLINE);

  qsort((void*)nbr_info,nIntf,sizeof(lldp_neighbor_info),compare_intf);

  iter = 0;
  while(iter < nIntf)
  {
    vty_out (vty, "%-15s", nbr_info[iter].name);
    vty_out (vty, "%-25s", nbr_info[iter].chassis_id);
    vty_out (vty, "%-25s", nbr_info[iter].port_id);
    vty_out (vty, "%-10s", nbr_info[iter].chassis_ttl);
    printf("\n");
    iter++;
  }

  if(nbr_info)
  {
    free(nbr_info);
    nbr_info = NULL;
  }

  return CMD_SUCCESS;
}




DEFUN (cli_lldp_show_intf_neighbor_info,
       lldp_show_intf_neighbor_info_cmd,
       "show lldp neighbor-info IFNAME",
       SHOW_STR
       SHOW_LLDP_STR
       "Show global LLDP neighbor information\n"
       "Specify the interface name")
{
  const struct ovsrec_interface *ifrow = NULL;
  bool port_found = false;
  const struct ovsdb_datum *datum = NULL;
  static char *lldp_interface_neighbor_info_keys [] = {
    INTERFACE_STATISTICS_LLDP_INSERT_COUNT,
    INTERFACE_STATISTICS_LLDP_DELETE_COUNT,
    INTERFACE_STATISTICS_LLDP_DROP_COUNT,
    INTERFACE_STATISTICS_LLDP_AGEOUT_COUNT,
    "chassis_id",
    "port_id",
    "chassis_ttl",
    "chassis_capability_available",
    "chassis_capability_enabled",
    "chassis_name",
    "chassis_description"
  };

  unsigned int index;

  OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
  {
     if(0 == strcmp(argv[0],ifrow->name))
     {
        union ovsdb_atom atom;

        port_found = true;
        vty_out (vty, "Port                           : %s%s", ifrow->name, VTY_NEWLINE);

        datum = ovsrec_interface_get_lldp_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);
        atom.string = lldp_interface_neighbor_info_keys[0];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries               : %ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[1];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries deleted       : %ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[2];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries dropped       : %ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        atom.string = lldp_interface_neighbor_info_keys[3];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor entries age-out       : %ld\n",(index == UINT_MAX)? 0 : datum->values[index].integer);

        datum = ovsrec_interface_get_lldp_neighbor_info(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_STRING);

        atom.string = lldp_interface_neighbor_info_keys[9];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Chassis-Name          : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[10];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Chassis-Description   : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[4];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Chassis-ID            : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[7];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Chassis Capabilities Available : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[8];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Chassis Capabilities Enabled   : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[5];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "Neighbor Port-ID               : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);

        atom.string = lldp_interface_neighbor_info_keys[6];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "TTL                            : %s\n",(index == UINT_MAX)? "" : datum->values[index].string);
        break;
     }
  }

  if(!port_found)
  {
    VLOG_ERR("Wrong interface name");
    return CMD_WARNING;
  }

  return CMD_SUCCESS;

}

/* set LLDP interface state as TX, RX or both */
int lldp_ovsdb_if_lldp_state(const char *ifvalue, const lldp_tx_rx state) {
  const struct ovsrec_interface * row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn* status_txn = cli_do_config_start();
  const char *state_value;
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return 1;
  }

  row = ovsrec_interface_first(idl);
  if(!row)
  {
    VLOG_ERR("unable to fetch row");
    cli_do_config_abort(status_txn);
    return 1;
  }

  OVSREC_INTERFACE_FOR_EACH(row, idl)
  {
    if(strcmp(row->name, ifvalue) == 0)
    {
      state_value = smap_get(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);
      smap_clone(&smap_other_config, &row->other_config);
      if(state == LLDP_TX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF) == 0))
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX);
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX) == 0)
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX);
        }
      }
      else if (state == LLDP_RX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF) == 0))
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX);
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX) == 0)
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX);
        }
      }

      ovsrec_interface_set_other_config(row, &smap_other_config);
      break;
    }
  }

  status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_other_config);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return 0;
  }
  else
  {
    VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
  }

  return 1;
}

DEFUN (lldp_if_lldp_tx,
       lldp_if_lldp_tx_cmd,
       "lldp transmission",
       INTF_LLDP_STR
       "Set the transmission\n")
{
  if(lldp_ovsdb_if_lldp_state((char*)vty->index, LLDP_TX) != 0)
    VLOG_ERR("Failed to set lldp transmission");

  return CMD_SUCCESS;
}

DEFUN (lldp_if_lldp_rx,
       lldp_if_lldp_rx_cmd,
       "lldp reception",
       INTF_LLDP_STR
       "Set the reception\n")
{
  if(lldp_ovsdb_if_lldp_state((char*)vty->index, LLDP_RX) != 0)
    VLOG_ERR("Failed to set lldp reception");

  return CMD_SUCCESS;
}

/* set LLDP interface state to default */
int lldp_ovsdb_if_lldp_nodirstate(const char *ifvalue, const lldp_tx_rx state)
{
  const struct ovsrec_interface * row = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn* status_txn = cli_do_config_start();
  const char *state_value;
  boolean validstate = false, ifexists = false;
  struct smap smap_other_config;

  if(NULL == status_txn)
  {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return 1;
  }

  row = ovsrec_interface_first(idl);
  if(!row)
  {
    VLOG_ERR("unable to fetch a row");
    cli_do_config_abort(status_txn);
    return 1;
  }

  OVSREC_INTERFACE_FOR_EACH(row, idl)
  {
    if(strcmp(row->name, ifvalue) == 0)
    {
      ifexists = true;
      state_value = smap_get(&row->other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);
      smap_clone(&smap_other_config, &row->other_config);
      if(state == LLDP_TX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0))
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX);
          validstate = true;
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX) == 0)
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF);
          validstate = true;
        }
      }
      else if (state == LLDP_RX)
      {
        if((NULL == state_value) || (strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RXTX) == 0))
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX);
          validstate = true;
        }
        else if(strcmp(state_value, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX) == 0)
        {
          smap_replace(&smap_other_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR,
                        INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF);
          validstate = true;
        }
      }

      if (true == validstate)
      {
        ovsrec_interface_set_other_config(row, &smap_other_config);
      }
      break;
    }
  }

  if ((true == ifexists) && (true == validstate))
  {
    status = cli_do_config_finish(status_txn);
    smap_destroy(&smap_other_config);
    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
      return 0;
    }
    else
    {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    }
  }
  else
  {
    smap_destroy(&smap_other_config);
    if (false == ifexists)
    {
      VLOG_ERR("unable to find the interface");
    }
    if ((true == ifexists) && (false == validstate))
    {
      VLOG_ERR("ifrow other_config has invalid LLDP dir state");
    }

    cli_do_config_abort(status_txn);
  }

  return 1;
}

DEFUN (lldp_if_no_lldp_tx,
       lldp_if_no_lldp_tx_cmd,
       "no lldp transmission",
       NO_STR
       INTF_LLDP_STR
       "Set the transmission\n")
{
  if(lldp_ovsdb_if_lldp_nodirstate((char*)vty->index, LLDP_TX) != 0)
    VLOG_ERR("Failed to set lldp transmission");

  return CMD_SUCCESS;
}

DEFUN (lldp_if_no_lldp_rx,
       lldp_if_no_lldp_rx_cmd,
       "no lldp reception",
       NO_STR
       INTF_LLDP_STR
       "Set the reception\n")
{
  if(lldp_ovsdb_if_lldp_nodirstate((char*)vty->index, LLDP_RX) != 0)
    VLOG_ERR("Failed to set lldp reception");

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
  install_element (CONFIG_NODE, &lldp_set_no_mgmt_address_cmd_arg);
  install_element (ENABLE_NODE, &lldp_show_tlv_cmd);
  install_element (ENABLE_NODE, &lldp_show_intf_statistics_cmd);
  install_element (ENABLE_NODE, &lldp_show_intf_neighbor_info_cmd);
  install_element (ENABLE_NODE, &lldp_show_config_cmd);
  install_element (ENABLE_NODE, &lldp_show_config_cmd_if);
  install_element (ENABLE_NODE, &lldp_show_statistics_cmd);
  install_element (INTERFACE_NODE, &lldp_if_lldp_tx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_lldp_rx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_no_lldp_tx_cmd);
  install_element (INTERFACE_NODE, &lldp_if_no_lldp_rx_cmd);
  install_element (ENABLE_NODE, &lldp_show_neighbor_info_cmd);
}
