/*
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * File: ecmp_vty.c
 *
 * Purpose:  To add ECMP CLI configuration and display commands.
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file ecmp_vty.c
 * Allow enabling/disable ECMP and hash function based on
 *  [dst-ip|dst-port|src-ip|src-port]
 *
 ***************************************************************************/

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
#include "ecmp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"

VLOG_DEFINE_THIS_MODULE ( vtysh_ecmp_cli);
extern struct ovsdb_idl *idl;

static int
ecmp_config_set_status (bool status, const char * field)
{
  const struct ovsrec_system *ovs_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start ();
  bool rc = false;
  struct smap smap_ecmp_config;

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  /* Need to set ecmp_config status */
  ovs_row = ovsrec_system_first (idl);

  if (!ovs_row)
    {
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  rc = smap_get_bool (&ovs_row->ecmp_config, field,
                      SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT);

  if (rc != status)
    {
      smap_clone (&smap_ecmp_config, &ovs_row->ecmp_config);
      smap_replace (&smap_ecmp_config, field, status ? "true" : "false");
      VLOG_DBG ("%s Set the ecmp config to status = %s old state = %s",
                __func__, status ? "enabled" : "disabled",
                rc ? "enabled" : "disabled");
      ovsrec_system_set_ecmp_config (ovs_row, &smap_ecmp_config);
      smap_destroy (&smap_ecmp_config);
    }

  txn_status = cli_do_config_finish (status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

DEFUN (ip_ecmp_status,
    ip_ecmp_status_cmd,
    "ip ecmp disable",
    IP_STR
    ECMP_STR
    ECMP_CONFIG_DISABLE_STR)
  {
    VLOG_DBG("We are seting status of the ecmp");
    return ecmp_config_set_status(false, SYSTEM_ECMP_CONFIG_STATUS);
  }

DEFUN (ip_ecmp_load_balance_src_ip,
    ip_ecmp_load_balance_src_ip_cmd,
    "ip ecmp load-balance src-ip disable",
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by source IP\n"
    "Disable load balancing by source IP\n")
  {
    VLOG_DBG("Add hash option Source IP");
    return ecmp_config_set_status(false, SYSTEM_ECMP_CONFIG_HASH_SRC_IP);
  }

DEFUN (ip_ecmp_load_balance_dst_ip,
    ip_ecmp_load_balance_dst_ip_cmd,
    "ip ecmp load-balance dst-ip disable",
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by destination IP\n"
    "Disable load balancing by destination IP\n")
  {
    VLOG_DBG("Add hash option Destination IP");
    return ecmp_config_set_status(false, SYSTEM_ECMP_CONFIG_HASH_DST_IP);
  }

DEFUN (ip_ecmp_load_balance_dst_port,
    ip_ecmp_load_balance_dst_port_cmd,
    "ip ecmp load-balance dst-port disable",
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by destination port\n"
    "Disable load balancing by destination port\n")
  {
    VLOG_DBG("Add hash option Destination Port");
    return ecmp_config_set_status(false, SYSTEM_ECMP_CONFIG_HASH_DST_PORT);
  }

DEFUN (ip_ecmp_load_balance_src_port,
    ip_ecmp_load_balance_src_port_cmd,
    "ip ecmp load-balance src-port disable",
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by source port\n"
    "Disable load balancing by source port\n")
  {
    VLOG_DBG("Add hash option Source Port");
    return ecmp_config_set_status(false, SYSTEM_ECMP_CONFIG_HASH_SRC_PORT);
  }

DEFUN (no_ip_ecmp_status,
    no_ip_ecmp_status_cmd,
    "no ip ecmp disable",
    NO_STR
    IP_STR
    ECMP_STR
    ECMP_CONFIG_DISABLE_STR)
  {
    VLOG_DBG("We are setting the ecmp state");
    return ecmp_config_set_status(true, SYSTEM_ECMP_CONFIG_STATUS);
  }

DEFUN (no_ip_ecmp_load_balance_src_ip,
    no_ip_ecmp_load_balance_src_ip_cmd,
    "no ip ecmp load-balance src-ip disable",
    NO_STR
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by source IP\n"
    "Disable load balancing by source IP\n")
  {
    VLOG_DBG("Delete hash option Source IP");
    return ecmp_config_set_status(true, SYSTEM_ECMP_CONFIG_HASH_SRC_IP);
  }

DEFUN (no_ip_ecmp_load_balance_dst_ip,
    no_ip_ecmp_load_balance_dst_ip_cmd,
    "no ip ecmp load-balance dst-ip disable",
    NO_STR
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by destination IP\n"
    "Disable load balancing by destination IP\n")
  {
    VLOG_DBG("Add hash option Destination IP");
    return ecmp_config_set_status(true, SYSTEM_ECMP_CONFIG_HASH_DST_IP);
  }

DEFUN (no_ip_ecmp_load_balance_dst_port,
    no_ip_ecmp_load_balance_dst_port_cmd,
    "no ip ecmp load-balance dst-port disable",
    NO_STR
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by destination port\n"
    "Disable load balancing by destination port\n")
  {
    VLOG_DBG("Add hash option Destination Port");
    return ecmp_config_set_status(true, SYSTEM_ECMP_CONFIG_HASH_DST_PORT);
  }

DEFUN (no_ip_ecmp_load_balance_src_port,
    no_ip_ecmp_load_balance_src_port_cmd,
    "no ip ecmp load-balance src-port disable",
    NO_STR
    IP_STR
    ECMP_STR
    LOAD_BAL_STR
    "Load balancing by source port\n"
    "Disable load balancing by source port\n")
  {
    VLOG_DBG("Add hash option Source Port");
    return ecmp_config_set_status(true, SYSTEM_ECMP_CONFIG_HASH_SRC_PORT);
  }

DEFUN (show_ip_ecmp,
    show_ip_ecmp_cmd,
    "show ip ecmp",
    SHOW_STR
    IP_STR
    "ECMP Configuration\n")
  {
    const struct ovsrec_system *ovs_row = NULL;
    ovs_row = ovsrec_system_first(idl);

    if(!ovs_row)
      {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
      }

    vty_out(vty, "\nECMP Configuration\n");
    vty_out(vty, "---------------------\n\n");
    vty_out(vty, "ECMP Status        : %s%s",
        GET_ECMP_CONFIG_STATUS(ovs_row)?"Enabled":"Disabled",
        VTY_NEWLINE);
    vty_out(vty, "\nECMP Load Balancing by\n");
    vty_out(vty, "------------------------\n");
    vty_out(vty, "Source IP          : %s%s",
        GET_ECMP_CONFIG_HASH_SRC_IP_STATUS(ovs_row)?"Enabled":"Disabled",
        VTY_NEWLINE);
    vty_out(vty, "Destination IP     : %s%s",
        GET_ECMP_CONFIG_HASH_DST_IP_STATUS(ovs_row)?"Enabled":"Disabled",
        VTY_NEWLINE);
    vty_out(vty, "Source Port        : %s%s",
        GET_ECMP_CONFIG_HASH_SRC_PORT_STATUS(ovs_row)?"Enabled":"Disabled",
        VTY_NEWLINE);
    vty_out(vty, "Destination Port   : %s%s\n",
        GET_ECMP_CONFIG_HASH_DST_PORT_STATUS(ovs_row)?"Enabled":"Disabled",
        VTY_NEWLINE);
    return CMD_SUCCESS;
  }

/* Install ECMP related vty commands. */
void
ecmp_vty_init (void)
{

  install_element (CONFIG_NODE, &ip_ecmp_status_cmd);
  install_element (CONFIG_NODE, &ip_ecmp_load_balance_src_ip_cmd);
  install_element (CONFIG_NODE, &ip_ecmp_load_balance_src_port_cmd);
  install_element (CONFIG_NODE, &ip_ecmp_load_balance_dst_ip_cmd);
  install_element (CONFIG_NODE, &ip_ecmp_load_balance_dst_port_cmd);
  install_element (ENABLE_NODE, &show_ip_ecmp_cmd);
  install_element (CONFIG_NODE, &no_ip_ecmp_status_cmd);
  install_element (CONFIG_NODE, &no_ip_ecmp_load_balance_src_ip_cmd);
  install_element (CONFIG_NODE, &no_ip_ecmp_load_balance_src_port_cmd);
  install_element (CONFIG_NODE, &no_ip_ecmp_load_balance_dst_ip_cmd);
  install_element (CONFIG_NODE, &no_ip_ecmp_load_balance_dst_port_cmd);
}
