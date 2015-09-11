/* LACP CLI commands
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
 * File: lacp_vty.c
 *
 * Purpose:  To add LACP CLI configuration and display commands.
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
#include "lacp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "openhalon-dflt.h"
#include "vrf_vty.h"

VLOG_DEFINE_THIS_MODULE(vtysh_lacp_cli);
extern struct ovsdb_idl *idl;
int maximum_lag_interfaces = 0;

static int
delete_lag(const char *lag_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_bridge *bridge_row = NULL;
  const struct ovsrec_port *lag_port_row = NULL;
  bool lag_to_vrf = false;
  bool lag_to_bridge = false;
  struct ovsrec_port **ports;
  int k=0, n=0, i=0;
  struct ovsdb_idl_txn* status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  bool port_found = false;

  /* Return if LAG port doesn't exit */
  OVSREC_PORT_FOR_EACH(lag_port_row, idl)
  {
    if (strcmp(lag_port_row->name, lag_name) == 0)
    {
       port_found = true;
    }
  }

  if(!port_found)
  {
    vty_out(vty, "Specified LAG port doesn't exist.%s",VTY_NEWLINE);
    return CMD_SUCCESS;
  }

  /* Check if the given LAG port is part of VRF */
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
  {
    for (k = 0; k < vrf_row->n_ports; k++)
    {
       lag_port_row = vrf_row->ports[k];
       if(strcmp(lag_port_row->name, lag_name) == 0)
       {
          lag_to_vrf = true;
          goto port_attached;
       }
    }
  }

   /* Check if the given LAG port is part of bridge */
  OVSREC_BRIDGE_FOR_EACH (bridge_row, idl)
  {
    for (k = 0; k < bridge_row->n_ports; k++)
    {
       lag_port_row = bridge_row->ports[k];
       if(strcmp(lag_port_row->name, lag_name) == 0)
       {
          lag_to_bridge = true;
          goto port_attached;
       }
    }
  }

port_attached:
  if(lag_to_vrf || lag_to_bridge)
  {
    /* LAG port is attached to either VRF or bridge.
     * So create transaction.                    */
    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
    }
  }
  else
  {
    /* assert if the LAG port is not part of either VRF or bridge */
    assert(0);
    VLOG_ERR("Port table entry not found either in VRF or in bridge.Function=%s, Line=%d", __func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }

  if(lag_to_vrf)
  {
    /* Delete the LAG port reference from VRF */
    ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
    for(i = n = 0; i < vrf_row->n_ports; i++)
    {
       if(vrf_row->ports[i] != lag_port_row)
       {
          ports[n++] = vrf_row->ports[i];
       }
    }
    ovsrec_vrf_set_ports(vrf_row, ports, n);
    free(ports);
  }
  else if(lag_to_bridge)
  {
    /* Delete the LAG port reference from Bridge */
    ports = xmalloc(sizeof *bridge_row->ports * (bridge_row->n_ports-1));
    for(i = n = 0; i < bridge_row->n_ports; i++)
    {
       if(bridge_row->ports[i] != lag_port_row)
       {
          ports[n++] = bridge_row->ports[i];
       }
    }
    ovsrec_bridge_set_ports(bridge_row, ports, n);
    free(ports);
  }

  /* Delete the port as we cleared references to it from VRF or bridge*/
  ovsrec_port_delete(lag_port_row);

  status = cli_do_config_finish(status_txn);

  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
     return CMD_SUCCESS;
  }
  else
  {
     VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
     return CMD_OVSDB_FAILURE;
  }
}

DEFUN (vtysh_remove_lag,
       vtysh_remove_lag_cmd,
       "no interface lag <1-2000>",
        NO_STR
       "Select an interface to configure.\n"
       "Configure link-aggregation parameters.\n"
       "LAG number ranges from 1 to 2000.\n")
{
  char lag_number[LAG_NAME_LENGTH]={0};
  /* Form lag name in the form of lag1,lag2,etc */
  snprintf(lag_number, LAG_NAME_LENGTH, "%s%s",LAG_PORT_NAME_PREFIX, argv[0]);
  return delete_lag(lag_number);
}


static int
lacp_set_mode(const char *lag_name, const char *mode_to_set, const char *present_mode)
{
  const struct ovsrec_port *port_row = NULL;
  bool port_found = false;
  struct smap smap = SMAP_INITIALIZER(&smap);
  struct ovsdb_idl_txn* txn = NULL;
  enum ovsdb_idl_txn_status status;
  const char *mode_in_db = NULL;

  txn = cli_do_config_start();
  if(txn == NULL)
  {
    VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, lag_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found)
  {
    /* assert - as LAG port should be present in DB. */
    assert(0);
    VLOG_ERR("Port table entry not found in DB.Function=%s, Line=%d", __func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap, &port_row->other_config);

  if(strcmp("off", mode_to_set) == 0)
  {
    mode_in_db = smap_get(&smap, "lacp");
    if(strcmp(present_mode, mode_in_db) == 0)
    {
       smap_remove(&smap, "lacp");
    }
    else
    {
       vty_out(vty, "Enter the configured LACP mode.\n");
       cli_do_config_abort(txn);
       return CMD_SUCCESS;
    }
  }
  else
  {
    smap_replace(&smap, "lacp", mode_to_set);
  }

  ovsrec_port_set_other_config(port_row, &smap);
  smap_destroy(&smap);
  status = cli_do_config_finish(txn);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR("Transaction commit failed.Function=%s Line=%d", __func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lacp_set_mode,
       lacp_set_mode_cmd,
       "lacp mode (active | passive)",
        LACP_STR
       "Configure LACP mode.\n"
       "Sets an interface as LACP active.\n"
       "Sets an interface as LACP passive.\n")
{
  return lacp_set_mode((char*) vty->index, argv[0], "");
}

DEFUN (cli_lacp_set_no_mode,
       lacp_set_mode_no_cmd,
       "no lacp mode (active | passive)",
       NO_STR
       LACP_STR
       "Configure LACP mode.\n"
       "Sets an interface as LACP active.\n"
       "Sets an interface as LACP passive.\n")
{
  return lacp_set_mode((char*) vty->index, "off", argv[0]);
}

static int
lacp_set_hash(const char *lag_name, const char *hash)
{
  const struct ovsrec_port *port_row = NULL;
  bool port_found = false;
  struct smap smap = SMAP_INITIALIZER(&smap);
  struct ovsdb_idl_txn* txn = NULL;
  enum ovsdb_idl_txn_status status;

  txn = cli_do_config_start();
  if(txn == NULL)
  {
    VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }
  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, lag_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found)
  {
    /* assert - as LAG port should be present in DB. */
    assert(0);
    cli_do_config_abort(txn);
    VLOG_ERR("Port table entry not found in DB.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
  smap_clone(&smap, &port_row->other_config);

  if(strcmp("l3-src-dst", hash) == 0)
    smap_remove(&smap, "bond_mode");
  else
    smap_replace(&smap, "bond_mode", hash);

  ovsrec_port_set_other_config(port_row, &smap);
  smap_destroy(&smap);
  status = cli_do_config_finish(txn);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR("Transaction commit failed.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lacp_set_hash,
       lacp_set_hash_cmd,
       "hash l2-src-dst",
       "The type of hash algorithm used for aggregated port.\n"
       "Base the hash on l2-src-dst. The default is l3-src-dst.\n")
{
  return lacp_set_hash((char*) vty->index, "l2-src-dst");
}

DEFUN (cli_lacp_set_no_hash,
       lacp_set_no_hash_cmd,
       "no hash l2-src-dst",
       NO_STR
       "The type of hash algorithm used for aggregated port.\n"
       "Base the hash on l2-src-dst. The default is l3-src-dst.\n")
{
  return lacp_set_hash((char*) vty->index, "l3-src-dst");
}

DEFUN (cli_lacp_set_no_hash_shortform,
       lacp_set_no_hash_shortform_cmd,
       "no hash",
       NO_STR
       "The type of hash algorithm used for aggregated port.\n")
{
  return lacp_set_hash((char*) vty->index, "l3-src-dst");
}

static int
lacp_set_fallback(const char *lag_name, const char *fallback_status)
{
  const struct ovsrec_port *port_row = NULL;
  bool port_found = false;
  struct smap smap = SMAP_INITIALIZER(&smap);
  struct ovsdb_idl_txn* txn = NULL;
  enum ovsdb_idl_txn_status status;

  txn = cli_do_config_start();
  if(txn == NULL)
  {
    VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, lag_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found)
  {
    /* assert - as LAG port should be present in DB. */
    assert(0);
    VLOG_ERR("Port table entry not found in DB.Function=%s Line=%d",__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap, &port_row->other_config);

  if(strcmp("false", fallback_status) == 0)
  {
    smap_remove(&smap, "lacp-fallback-ab");
  }
  else
  {
    smap_replace(&smap, "lacp-fallback-ab", fallback_status);
  }

  ovsrec_port_set_other_config(port_row, &smap);
  smap_destroy(&smap);
  status = cli_do_config_finish(txn);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR("Transaction commit failed.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lacp_set_fallback,
       lacp_set_fallback_cmd,
       "lacp fallback",
       LACP_STR
       "Enable LACP fallback mode.\n")
{
  return lacp_set_fallback((char*) vty->index, "true");
}

DEFUN (cli_lacp_set_no_fallback,
       lacp_set_no_fallback_cmd,
       "no lacp fallback",
       NO_STR
       LACP_STR
       "Enable LACP fallback mode.\n")
{
  return lacp_set_fallback((char*) vty->index, "false");
}

static int
lacp_set_heartbeat_rate(const char *lag_name, const char *rate)
{
  const struct ovsrec_port *port_row = NULL;
  bool port_found = false;
  struct smap smap = SMAP_INITIALIZER(&smap);
  struct ovsdb_idl_txn* txn = NULL;
  enum ovsdb_idl_txn_status status;

  txn = cli_do_config_start();
  if(txn == NULL)
  {
    VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, lag_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found)
  {
    /* assert - as LAG port should be present in DB. */
    assert(0);
    cli_do_config_abort(txn);
    VLOG_ERR("Port table entry not found in DB.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap, &port_row->other_config);

  if(strcmp(PORT_OTHER_CONFIG_LACP_TIME_SLOW, rate) == 0)
  {
    smap_remove(&smap, PORT_OTHER_CONFIG_MAP_LACP_TIME);
  }
  else
  {
    smap_replace(&smap, PORT_OTHER_CONFIG_MAP_LACP_TIME, rate);
  }

  ovsrec_port_set_other_config(port_row, &smap);
  smap_destroy(&smap);
  status = cli_do_config_finish(txn);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR("Transaction commit failed.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lacp_set_heartbeat_rate,
       lacp_set_heartbeat_rate_cmd,
       "lacp rate fast",
       LACP_STR
       "Set LACP heartbeat request time. Default is slow which is once every 30seconds.\n"
       "LACP heartbeats are requested at the rate of one per second.\n")
{
  return lacp_set_heartbeat_rate((char*) vty->index, PORT_OTHER_CONFIG_LACP_TIME_FAST);
}
DEFUN (cli_lacp_set_no_heartbeat_rate,
       lacp_set_no_heartbeat_rate_cmd,
       "no lacp rate",
       NO_STR
       LACP_STR
       "Set LACP heartbeat request time. Default is slow which is once every 30seconds.\n")
{
  return lacp_set_heartbeat_rate((char*) vty->index, PORT_OTHER_CONFIG_LACP_TIME_SLOW);
}

DEFUN (cli_lacp_set_no_heartbeat_rate_fast,
       lacp_set_no_heartbeat_rate_fast_cmd,
       "no lacp rate fast",
       NO_STR
       LACP_STR
       "Set LACP heartbeat request time. Default is slow which is once every 30seconds.\n"
       "LACP heartbeats are requested at the rate of one per second.\n")
{
  return lacp_set_heartbeat_rate((char*) vty->index, PORT_OTHER_CONFIG_LACP_TIME_SLOW);
}


static int
lacp_set_global_sys_priority(const char *priority)
{
  const struct ovsrec_open_vswitch *row = NULL;
  struct smap smap = SMAP_INITIALIZER(&smap);
  struct ovsdb_idl_txn* txn = NULL;
  enum ovsdb_idl_txn_status status;

  txn = cli_do_config_start();
  if(txn == NULL)
  {
    VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if(!row)
  {
    VLOG_ERR(LACP_OVSDB_ROW_FETCH_ERROR,__func__,__LINE__);
    cli_do_config_abort(txn);
    return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap, &row->lacp_config);

  if(DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY == atoi(priority))
    smap_remove(&smap, PORT_OTHER_CONFIG_MAP_LACP_SYSTEM_PRIORITY);
  else
    smap_replace(&smap, PORT_OTHER_CONFIG_MAP_LACP_SYSTEM_PRIORITY, priority);

  ovsrec_open_vswitch_set_lacp_config(row, &smap);
  smap_destroy(&smap);
  status = cli_do_config_finish(txn);
  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
    return CMD_SUCCESS;
  }
  else
  {
    VLOG_ERR("Transaction commit failed.Function=%s Line=%d",__func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_lacp_set_global_sys_priority,
       lacp_set_global_sys_priority_cmd,
       "lacp system-priority <0-65535>",
       LACP_STR
       "Set LACP system priority.\n"
       "The range is 0 to 65535; the default is 65534.\n")
{
  return lacp_set_global_sys_priority(argv[0]);
}

DEFUN (cli_lacp_set_no_global_sys_priority,
       lacp_set_no_global_sys_priority_cmd,
       "no lacp system-priority <0-65535>",
       NO_STR
       LACP_STR
       "Set LACP system priority.\n"
       "The range is 0 to 65535; the default is 65534.\n")
{
  char def_sys_priority[LACP_DEFAULT_SYS_PRIORITY_LENGTH]={0};
  snprintf(def_sys_priority, LACP_DEFAULT_SYS_PRIORITY_LENGTH, "%d", DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY);
  return lacp_set_global_sys_priority(def_sys_priority);
}

DEFUN (cli_lacp_set_no_global_sys_priority_shortform,
       lacp_set_no_global_sys_priority_shortform_cmd,
       "no lacp system-priority",
       NO_STR
       LACP_STR
       "Set LACP system priority.\n")
{
  char def_sys_priority[LACP_DEFAULT_SYS_PRIORITY_LENGTH]={0};
  snprintf(def_sys_priority, LACP_DEFAULT_SYS_PRIORITY_LENGTH, "%d", DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY);
  return lacp_set_global_sys_priority(def_sys_priority);
}

static int
lacp_intf_set_port_id(const char *if_name, const char *port_id_val)
{
   const struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   struct smap smap = SMAP_INITIALIZER(&smap);

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         smap_clone(&smap, &row->other_config);
         smap_replace(&smap, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_ID, port_id_val);

         ovsrec_interface_set_other_config(row, &smap);
         smap_destroy(&smap);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR, __func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
}

DEFUN (cli_lacp_intf_set_port_id,
      cli_lacp_intf_set_port_id_cmd,
      "lacp port-id <1-65535>",
      LACP_STR
      "Set port ID used in LACP negotiation.\n."
      "The range is 1 to 65535.\n")
{
  return lacp_intf_set_port_id((char*)vty->index, argv[0]);
}

static int
lacp_intf_set_port_priority(const char *if_name, const char *port_priority_val)
{
   const struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   struct smap smap = SMAP_INITIALIZER(&smap);

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         smap_clone(&smap, &row->other_config);
         smap_replace(&smap, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_PRIORITY, port_priority_val);

         ovsrec_interface_set_other_config(row, &smap);
         smap_destroy(&smap);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
}

DEFUN (cli_lacp_intf_set_port_priority,
      cli_lacp_intf_set_port_priority_cmd,
      "lacp port-priority <1-65535>",
      LACP_STR
      "Set port priority is used in LACP negotiation.\n."
      "The range is 1 to 65535.\n")
{
  return lacp_intf_set_port_priority((char*)vty->index, argv[0]);
}

static int
lacp_intf_set_aggregation_key(const char *if_name, const char *agg_key)
{
   const struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   struct smap smap = SMAP_INITIALIZER(&smap);

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         smap_clone(&smap, &row->other_config);
         smap_replace(&smap,INTERFACE_OTHER_CONFIG_MAP_LACP_AGGREGATION_KEY , agg_key);

         ovsrec_interface_set_other_config(row, &smap);
         smap_destroy(&smap);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
}

DEFUN (cli_lacp_intf_set_aggregation_key,
      cli_lacp_intf_set_aggregation_key_cmd,
      "lacp aggregation-key <1-65535>",
      LACP_STR
      "Set aggregation-key is used in LACP negotiation.\n."
      "The range is 1 to 65535.\n")
{
  return lacp_intf_set_aggregation_key((char*)vty->index, argv[0]);
}

static int
lacp_add_intf_to_lag(const char *if_name, const char *lag_number)
{
   const struct ovsrec_interface *row = NULL;
   const struct ovsrec_interface *interface_row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   char lag_name[LAG_NAME_LENGTH]={0};
   const struct ovsrec_port *port_row = NULL;
   bool port_found = false;
   const struct ovsrec_interface *if_row = NULL;
   const struct ovsrec_port *port_row_found = NULL;
   struct ovsrec_interface **interfaces;
   const struct ovsrec_port *lag_port = NULL;
   int i=0, k=0, n=0;

   snprintf(lag_name, LAG_NAME_LENGTH, "%s%s", LAG_PORT_NAME_PREFIX, lag_number);

   /* Check if the LAG port is present or not. */
   OVSREC_PORT_FOR_EACH(lag_port, idl)
   {
     if (strcmp(lag_port->name, lag_name) == 0)
     {
       port_found = true;
       if(lag_port->n_interfaces == MAX_INTF_TO_LAG)
       {
         vty_out(vty, "Cannot add more interfaces to LAG. Maximum interface count is reached.\n");
         return CMD_SUCCESS;
       }
       break;
     }
   }
   if(!port_found)
   {
     vty_out(vty, "Specified LAG port doesn't exist.\n");
     return CMD_SUCCESS;
   }

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   /* Delete the port entry of interface if already exists.
    * This can happen if the interface is attached to VLAN.
    */
   OVSREC_PORT_FOR_EACH(port_row, idl)
   {
     if(strcmp(port_row->name, if_name) == 0)
     {
        ovsrec_port_delete(port_row);
        break;
     }
   }

   /* Fetch the interface row to "interface_row" variable. */
   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         interface_row = row;
         break;
      }
   }

   /* Search if the interface is part of any LAG or not.
    * If it is part of LAG port which is is specified in CLI then
    * return with SUCCESS.
    * If it is part of any other LAG then exit and remove the
    * reference from that LAG port.*/
   OVSREC_PORT_FOR_EACH(port_row, idl)
   {
     if (strncmp(port_row->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
     {
        for (k = 0; k < port_row->n_interfaces; k++)
        {
           if_row = port_row->interfaces[k];
           if(strcmp(if_name, if_row->name) == 0)
           {
              if (strcmp(port_row->name, lag_name) == 0)
              {
                 vty_out(vty, "Interface %s is already part of %s\n", if_name, port_row->name);
                 cli_do_config_abort(status_txn);
                 return CMD_SUCCESS;
              }
              else
              {
                 /* Unlink interface from "port_row_found" port entry. */
                 port_row_found = port_row;
                 goto exit_loop;
              }
           }
        }
     }
   }

exit_loop:
   if(port_row_found)
   {
       /* Unlink the interface from the Port row found*/
      interfaces = xmalloc(sizeof *port_row_found->interfaces * (port_row_found->n_interfaces-1));
      for(i = n = 0; i < port_row_found->n_interfaces; i++)
      {
         if(port_row_found->interfaces[i] != interface_row)
         {
            interfaces[n++] = port_row_found->interfaces[i];
         }
      }
      ovsrec_port_set_interfaces(port_row_found, interfaces, n);
      free(interfaces);
   }

   /* Link the interface to the LAG port specified. */
   interfaces = xmalloc(sizeof *lag_port->interfaces * (lag_port->n_interfaces + 1));
   for(i = 0; i < lag_port->n_interfaces; i++)
   {
      interfaces[i] = lag_port->interfaces[i];
   }
   interfaces[lag_port->n_interfaces] = (struct ovsrec_interface *)interface_row;
   ovsrec_port_set_interfaces(lag_port, interfaces, lag_port->n_interfaces + 1);
   free(interfaces);

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
}

DEFUN (cli_lacp_add_intf_to_lag,
      cli_lacp_add_intf_to_lag_cmd,
      "lag <1-2000>",
      "Add the current interface to link aggregation.\n"
      "LAG number ranges from 1 to 2000.\n")
{
  return lacp_add_intf_to_lag((char*)vty->index, argv[0]);
}

static int
lacp_remove_intf_from_lag(const char *if_name, const char *lag_number)
{
   const struct ovsrec_interface *row = NULL;
   const struct ovsrec_interface *interface_row = NULL;
   const struct ovsrec_interface *if_row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   char lag_name[8]={0};
   bool port_found = false;
   struct ovsrec_interface **interfaces;
   const struct ovsrec_port *lag_port = NULL;
   int i=0, n=0, k=0;
   bool interface_found = false;

   /* LAG name should be in the format of lag1,lag2,etc  */
   snprintf(lag_name, LAG_NAME_LENGTH, "%s%s", LAG_PORT_NAME_PREFIX, lag_number);

   /* Check if the LAG port is present in DB. */
   OVSREC_PORT_FOR_EACH(lag_port, idl)
   {
     if (strcmp(lag_port->name, lag_name) == 0)
     {
       for (k = 0; k < lag_port->n_interfaces; k++)
       {
         if_row = lag_port->interfaces[k];
         if(strcmp(if_name, if_row->name) == 0)
         {
           interface_found = true;
         }
       }
       port_found = true;
       break;
     }
   }

   if(!port_found)
   {
     vty_out(vty, "Specified LAG port doesn't exist.\n");
     return CMD_SUCCESS;
   }
   if(!interface_found)
   {
     vty_out(vty, "Interface %s is not part of %s\n", if_name, lag_name);
     return CMD_SUCCESS;
   }

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(LACP_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   /* Fetch the interface row to "interface_row" */
   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         interface_row = row;
         break;
      }
   }

   /* Unlink the interface from the Port row found*/
   interfaces = xmalloc(sizeof *lag_port->interfaces * (lag_port->n_interfaces-1));
   for(i = n = 0; i < lag_port->n_interfaces; i++)
   {
      if(lag_port->interfaces[i] != interface_row)
      {
         interfaces[n++] = lag_port->interfaces[i];
      }
   }
   ovsrec_port_set_interfaces(lag_port, interfaces, n);
   free(interfaces);

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(LACP_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
}



DEFUN (cli_lacp_remove_intf_from_lag,
      cli_lacp_remove_intf_from_lag_cmd,
      "no lag <1-2000>",
      NO_STR
      "Add the current interface to link aggregation.\n"
      "LAG number ranges from 1 to 2000.\n")
{
  return lacp_remove_intf_from_lag((char*)vty->index, argv[0]);
}

static int
lacp_show_configuration()
{
   const struct ovsrec_open_vswitch *row = NULL;
   const char *system_id = NULL;
   const char *system_priority = NULL;

   row = ovsrec_open_vswitch_first(idl);
   if(!row)
   {
      return CMD_OVSDB_FAILURE;
   }
   system_id = smap_get(&row->lacp_config, PORT_OTHER_CONFIG_MAP_LACP_SYSTEM_ID);
   if(NULL == system_id)
       vty_out(vty,"System-id       : %s", row->system_mac);
   else
       vty_out(vty,"System-id       : %s", system_id);
   vty_out(vty,"%s",VTY_NEWLINE);
   system_priority = smap_get(&row->lacp_config, PORT_OTHER_CONFIG_MAP_LACP_SYSTEM_PRIORITY);
   if(NULL == system_priority)
      vty_out(vty, "System-priority : %d", DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY);
   else
      vty_out(vty, "System-priority : %s", system_priority);
   vty_out(vty,"%s",VTY_NEWLINE);
   return CMD_SUCCESS;
}

DEFUN (cli_lacp_show_configuration,
      cli_lacp_show_configuration_cmd,
      "show lacp configuration",
      SHOW_STR
      "Show various LACP settings.\n"
      "Show LACP system-wide configuration.\n")
{
  return lacp_show_configuration();
}

static int
lacp_show_aggregates(const char *lag_name)
{
   const struct ovsrec_port *lag_port = NULL;
   const struct ovsrec_interface *if_row = NULL;
   const char *heartbeat_rate = NULL;
   bool fallback = false;
   const char *aggregate_mode = NULL;
   const char *hash = NULL;
   bool show_all = false;
   bool port_found = false;
   int k = 0;

   if(strncmp("all", lag_name, 3) == 0)
   {
      show_all = true;
   }

   OVSREC_PORT_FOR_EACH(lag_port, idl)
   {
      if(((strncmp(lag_port->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0) && show_all)
          || (strcmp(lag_port->name, lag_name) == 0))
      {
         vty_out(vty, "%s", VTY_NEWLINE);
         vty_out(vty, "%s%s%s","Aggregate-name        : ", lag_port->name, VTY_NEWLINE);
         vty_out(vty, "%s","Aggregated-interfaces : ");
         for (k = 0; k < lag_port->n_interfaces; k++)
         {
            if_row = lag_port->interfaces[k];
            vty_out(vty, "%s ", if_row->name);
         }
         vty_out(vty, "%s", VTY_NEWLINE);
         heartbeat_rate = smap_get(&lag_port->other_config, "lacp-time");
         if(heartbeat_rate)
            vty_out(vty, "%s%s%s", "Heartbeat rate        : ",heartbeat_rate, VTY_NEWLINE);
         else
            vty_out(vty, "%s%s%s", "Heartbeat rate        : ",PORT_OTHER_CONFIG_LACP_TIME_SLOW, VTY_NEWLINE);

         fallback = smap_get_bool(&lag_port->other_config, "lacp-fallback-ab", false);
         vty_out(vty, "%s%s%s", "Fallback              : ",(fallback)?"true":"false", VTY_NEWLINE);

         hash = smap_get(&lag_port->other_config, "bond_mode");
         if(hash)
            vty_out(vty, "%s%s%s", "Hash                  : ",hash, VTY_NEWLINE);
         else
            vty_out(vty, "%s%s%s", "Hash                  : ","l3-src-dst", VTY_NEWLINE);

         aggregate_mode = smap_get(&lag_port->other_config, "lacp");
         if(aggregate_mode)
            vty_out(vty, "%s%s%s", "Aggregate mode        : ",aggregate_mode, VTY_NEWLINE);
         else
            vty_out(vty, "%s%s%s", "Aggregate mode        : ","off", VTY_NEWLINE);
         vty_out(vty, "%s", VTY_NEWLINE);

         if(!show_all)
         {
            port_found = true;
            break;
         }
      }
   }

   if(!show_all && !port_found)
      vty_out(vty, "Specified LAG port doesn't exist.\n");

   return CMD_SUCCESS;
}

DEFUN (cli_lacp_show_all_aggregates,
      cli_lacp_show_all_aggregates_cmd,
      "show lacp aggregates",
      SHOW_STR
      "Show various LACP settings.\n"
      "Show LACP aggregates.\n")
{
  return lacp_show_aggregates("all");
}

DEFUN (cli_lacp_show_aggregates,
      cli_lacp_show_aggregates_cmd,
      "show lacp aggregates WORD",
      SHOW_STR
      "Show various LACP settings.\n"
      "Show LACP aggregates.\n"
      "Link-aggregate name.\n")
{
  return lacp_show_aggregates(argv[0]);
}

static char *
get_lacp_state(const char *state)
{
   static char ret_state[8]={0};
   int n = 0;

   memset(ret_state, 0, 8);
   if(state == NULL)
   {
     return ret_state;
   }
   if(state[0])
      ret_state[n++]='A';
   else
      ret_state[n++]='P';

   if(state[1])
      ret_state[n++]='L';
   else
      ret_state[n++]='S';

   if(state[2])
      ret_state[n++]='F';
   else
      ret_state[n++]='I';

   if(state[3])
      ret_state[n++]='N';
   else
      ret_state[n++]='O';

   if(state[4])
      ret_state[n++]='C';

   if(state[5])
      ret_state[n++]='D';

   if(state[6])
      ret_state[n++]='E';

   if(state[7])
      ret_state[n++]='X';

   return ret_state;
}

static int
lacp_show_interfaces_all()
{
   const struct ovsrec_port *lag_port = NULL;
   const struct ovsrec_interface *if_row = NULL;
   int k = 0;
   const char *lacp_state = NULL;
   const char *key = NULL, *port_priority = NULL, *port_id = NULL;

   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "State abbreviations :%s", VTY_NEWLINE);
   vty_out(vty, "A - Active        P - Passive      F - Aggregable I - Individual");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "S - Short-timeout L - Long-timeout N - InSync     O - OutofSync");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "C - Collecting    D - Distributing ");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "X - State m/c expired              E - Default neighbor state");
   vty_out(vty,"%s%s", VTY_NEWLINE, VTY_NEWLINE);

   vty_out(vty, "Actor details of all interfaces:%s",VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "%-12s %-6s %-10s %-10s", "Intf-name", "Key", "Priority", "State");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------");
   vty_out(vty,"%s", VTY_NEWLINE);

   OVSREC_PORT_FOR_EACH(lag_port, idl)
   {
      if(strncmp(lag_port->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
      {
         vty_out(vty, "Aggregate-name : %s%s", lag_port->name, VTY_NEWLINE);

         for (k = 0; k < lag_port->n_interfaces; k++)
         {
            if_row = lag_port->interfaces[k];
            lacp_state = get_lacp_state(smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_STATE));
            key = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_KEY);
            port_priority = smap_get(&if_row->other_config, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_PRIORITY);
            vty_out(vty, "%-12s %-6s %-10s %-10s",
                       if_row->name,
                       key ? key: " ",
                       port_priority ? port_priority : " ",
                       lacp_state?lacp_state:" ");
            vty_out(vty,"%s", VTY_NEWLINE);
         }
         if(k == 0)
           vty_out(vty, "No interfaces are attached to %s%s", lag_port->name, VTY_NEWLINE);
      }
   }
   vty_out(vty,"%s%s", VTY_NEWLINE, VTY_NEWLINE);

   vty_out(vty, "Partner details of all interfaces:%s",VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------------");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "%-12s %-8s %-6s %-10s %-10s", "Intf-name", "Partner", "Key", "Priority", "State");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "%-12s %-8s"," ","port-id");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------------");
   vty_out(vty,"%s", VTY_NEWLINE);

   OVSREC_PORT_FOR_EACH(lag_port, idl)
   {
      if(strncmp(lag_port->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
      {
         vty_out(vty, "Aggregate-name : %s%s", lag_port->name, VTY_NEWLINE);

         for (k = 0; k < lag_port->n_interfaces; k++)
         {
            if_row = lag_port->interfaces[k];
            lacp_state = get_lacp_state(smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_STATE));
            key = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_KEY);
            port_id = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_PORT_ID);
            port_priority = smap_get(&if_row->other_config, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_PRIORITY);
            vty_out(vty, "%-12s %-8s %-6s %-10s %-10s",
                       if_row->name,
                       port_id ? port_id : " ",
                       key ? key : "",
                       port_priority ? port_priority : " ",
                       lacp_state?lacp_state:"");
            vty_out(vty,"%s", VTY_NEWLINE);
         }
         if(k == 0)
           vty_out(vty, "No interfaces are attached to %s%s", lag_port->name, VTY_NEWLINE);
      }
   }

   return CMD_SUCCESS;
}

DEFUN (cli_lacp_show_all_interfaces,
      cli_lacp_show_all_interfaces_cmd,
      "show lacp interfaces",
      SHOW_STR
      "Show various LACP settings.\n"
      "Show LACP interfaces.\n")
{
  return lacp_show_interfaces_all();
}

static int
lacp_show_interfaces(const char *if_name)
{
   const struct ovsrec_port *port_row = NULL;
   const struct ovsrec_interface *if_row = NULL;
   int k = 0;
   const char *a_lacp_state = NULL, *p_lacp_state = NULL;
   const char *a_key = NULL, *a_system_id = NULL, *a_port_id = NULL;
   const char *p_key = NULL, *p_system_id = NULL, *p_port_id = NULL;
   bool port_row_round = false;

   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "State abbreviations :%s", VTY_NEWLINE);
   vty_out(vty, "A - Active        P - Passive      F - Aggregable I - Individual");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "S - Short-timeout L - Long-timeout N - InSync     O - OutofSync");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "C - Collecting    D - Distributing ");
   vty_out(vty,"%s", VTY_NEWLINE);
   vty_out(vty, "X - State m/c expired              E - Default neighbor state");
   vty_out(vty,"%s%s", VTY_NEWLINE, VTY_NEWLINE);

   OVSREC_PORT_FOR_EACH(port_row, idl)
   {
     if (strncmp(port_row->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
     {
        for (k = 0; k < port_row->n_interfaces; k++)
        {
           if_row = port_row->interfaces[k];
           if(strcmp(if_name, if_row->name) == 0)
           {
             a_lacp_state = get_lacp_state(smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_STATE));
             a_key = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_KEY);
             a_port_id = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_PORT_ID);
             a_system_id = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_ACTOR_SYSTEM_ID);

             p_lacp_state = get_lacp_state(smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_STATE));
             p_key = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_KEY);
             p_port_id = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_PORT_ID);
             p_system_id = smap_get(&if_row->lacp_status, INTERFACE_LACP_STATUS_MAP_PARTNER_SYSTEM_ID);
             port_row_round = true;
             goto Exit;
           }
        }
     }
   }
Exit:
   vty_out(vty,"%s",VTY_NEWLINE);
   vty_out(vty, "Aggregate-name : %s%s", port_row_round?port_row->name:" ", VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------------");
   vty_out(vty,"%s",VTY_NEWLINE);
   vty_out(vty, "                   Actor             Partner");
   vty_out(vty,"%s",VTY_NEWLINE);
   vty_out(vty, "-------------------------------------------------");
   vty_out(vty,"%s",VTY_NEWLINE);
   vty_out(vty,"%-10s | %-18s | %-18s %s",
               "System-id",a_system_id?a_system_id:" ", p_system_id?p_system_id:" ", VTY_NEWLINE);
   vty_out(vty,"%-10s | %-18s | %-18s %s",
               "Port-id", a_port_id?a_port_id:" ", p_port_id?p_port_id:" ", VTY_NEWLINE);
   vty_out(vty,"%-10s | %-18s | %-18s %s",
               "Key", a_key?a_key:" ", p_key?p_key:" ", VTY_NEWLINE);
   vty_out(vty,"%-10s | %-18s | %-18s %s",
               "State", a_lacp_state?a_lacp_state:" ", p_lacp_state?p_lacp_state:" ", VTY_NEWLINE);
   vty_out(vty,"%s",VTY_NEWLINE);

   return CMD_SUCCESS;
}

DEFUN (cli_lacp_show_interfaces,
      cli_lacp_show_interfaces_cmd,
      "show lacp interfaces IFNAME",
      SHOW_STR
      "Show various LACP settings.\n"
      "Show LACP interfaces.\n"
      "Interface's name.\n")
{
  return lacp_show_interfaces(argv[0]);
}

/*
* This function is used to make an LAG L3.
* It attaches the port to the default VRF.
*/
static int lag_routing(const char *port_name)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_vrf *default_vrf_row = NULL;
    const struct ovsrec_bridge *default_bridge_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_port **ports;
    size_t i, n;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_DBG(
            "%s Got an error when trying to create a transaction using"
            " cli_do_config_start()", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    port_row = port_check_and_add(port_name, false, false, status_txn);

    if (check_port_in_vrf(port_name)) {
        VLOG_DBG(
            "%s Interface \"%s\" is already L3. No change required.",
            __func__, port_name);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    default_bridge_row = ovsrec_bridge_first(idl);
    ports = xmalloc(sizeof *default_bridge_row->ports *
        (default_bridge_row->n_ports - 1));
    for (i = n = 0; i < default_bridge_row->n_ports; i++) {
        if (default_bridge_row->ports[i] != port_row) {
            ports[n++] = default_bridge_row->ports[i];
        }
    }
    ovsrec_bridge_set_ports(default_bridge_row, ports, n);

    default_vrf_row = vrf_lookup(DEFAULT_VRF_NAME);
    xrealloc(ports, sizeof *default_vrf_row->ports *
        (default_vrf_row->n_ports + 1));
    for (i = 0; i < default_vrf_row->n_ports; i++) {
        ports[i] = default_vrf_row->ports[i];
    }
    struct ovsrec_port *temp_port_row = CONST_CAST(struct ovsrec_port*, port_row);
    ports[default_vrf_row->n_ports] = temp_port_row;
    ovsrec_vrf_set_ports(default_vrf_row, ports, default_vrf_row->n_ports + 1);
    free(ports);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
            "%s The command succeeded and interface \"%s\" is now L3"
            " and attached to default VRF", __func__, port_name);
        return CMD_SUCCESS;
    }
    else if (status == TXN_UNCHANGED) {
        VLOG_DBG(
            "%s The command resulted in no change. Check if"
            " LAG \"%s\" is already L3",
            __func__, port_name);
        return CMD_SUCCESS;
    }
    else {
        VLOG_DBG(
            "%s While trying to commit transaction to DB, got a status"
            " response : %s", __func__,
            ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
* This function is used to make an LAG L2.
* It attaches the port to the default VRF.
* It also removes all L3 related configuration like IP addresses.
*/
static int lag_no_routing(const char *port_name)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_bridge *default_bridge_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_port **vrf_ports;
    struct ovsrec_port **bridge_ports;
    size_t i, n;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_DBG(
            "%s Got an error when trying to create a transaction using"
            " cli_do_config_start()", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    port_row = port_check_and_add(port_name, true, false, status_txn);
    if (check_port_in_bridge(port_name)) {
        VLOG_DBG(
            "%s Interface \"%s\" is already L2. No change required.",
            __func__, port_name);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if ((vrf_row = port_vrf_lookup(port_row)) != NULL) {
        vrf_ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports - 1));
        for (i = n = 0; i < vrf_row->n_ports; i++){
            if (vrf_row->ports[i] != port_row) {
                vrf_ports[n++] = vrf_row->ports[i];
            }
        }
        ovsrec_vrf_set_ports(vrf_row, vrf_ports, n);
        free(vrf_ports);
    }
    default_bridge_row = ovsrec_bridge_first(idl);
    bridge_ports = xmalloc(sizeof *default_bridge_row->ports
        * (default_bridge_row->n_ports + 1));
    for (i = 0; i < default_bridge_row->n_ports; i++) {
        bridge_ports[i] = default_bridge_row->ports[i];
    }
    struct ovsrec_port *temp_port_row = CONST_CAST(struct ovsrec_port*, port_row);
    bridge_ports[default_bridge_row->n_ports] = temp_port_row;
    ovsrec_bridge_set_ports(default_bridge_row, bridge_ports,
        default_bridge_row->n_ports + 1);
    free(bridge_ports);
    ovsrec_port_set_ip4_address(port_row, NULL);
    ovsrec_port_set_ip4_address_secondary(port_row, NULL, 0);
    ovsrec_port_set_ip6_address(port_row, NULL);
    ovsrec_port_set_ip6_address_secondary(port_row, NULL, 0);

    status = cli_do_config_finish(status_txn);
    if (status == TXN_SUCCESS) {
        VLOG_DBG(
            "%s The command succeeded and interface \"%s\" is now L2"
            " and attached to default bridge", __func__, port_name);
        return CMD_SUCCESS;
    }
    else if (status == TXN_UNCHANGED) {
        VLOG_DBG(
            "%s The command resulted in no change. Check if"
            " interface \"%s\" is already L2",
            __func__, port_name);
        return CMD_SUCCESS;
    }
    else {
        VLOG_DBG(
            "%s While trying to commit transaction to DB, got a status"
            " response : %s", __func__,
            ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN(cli_lag_routing,
    cli_lag_routing_cmd,
    "routing",
    "Configure LAG as L3.\n")
{
    return lag_routing((char*) vty->index);
}

DEFUN(cli_lag_no_routing,
    cli_lag_no_routing_cmd,
    "no routing",
    NO_STR
    "Configure LAG as L3.\n")
{
    return lag_no_routing((char*) vty->index);
}

/* Install LACP related vty commands. */
void
lacp_vty_init (void)
{
  install_element (LINK_AGGREGATION_NODE, &lacp_set_mode_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_mode_no_cmd);
  install_element (LINK_AGGREGATION_NODE, &cli_lag_routing_cmd);
  install_element (LINK_AGGREGATION_NODE, &cli_lag_no_routing_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_hash_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_no_hash_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_no_hash_shortform_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_fallback_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_no_fallback_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_heartbeat_rate_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_no_heartbeat_rate_cmd);
  install_element (LINK_AGGREGATION_NODE, &lacp_set_no_heartbeat_rate_fast_cmd);
  install_element (CONFIG_NODE, &lacp_set_global_sys_priority_cmd);
  install_element (CONFIG_NODE, &lacp_set_no_global_sys_priority_cmd);
  install_element (CONFIG_NODE, &lacp_set_no_global_sys_priority_shortform_cmd);
  install_element (CONFIG_NODE, &vtysh_remove_lag_cmd);
  install_element (INTERFACE_NODE, &cli_lacp_intf_set_port_id_cmd);
  install_element (INTERFACE_NODE, &cli_lacp_intf_set_port_priority_cmd);
  install_element (INTERFACE_NODE, &cli_lacp_intf_set_aggregation_key_cmd);
  install_element (INTERFACE_NODE, &cli_lacp_add_intf_to_lag_cmd);
  install_element (INTERFACE_NODE, &cli_lacp_remove_intf_from_lag_cmd);
  install_element (ENABLE_NODE, &cli_lacp_show_configuration_cmd);
  install_element (ENABLE_NODE, &cli_lacp_show_all_aggregates_cmd);
  install_element (ENABLE_NODE, &cli_lacp_show_aggregates_cmd);
  install_element (ENABLE_NODE, &cli_lacp_show_all_interfaces_cmd);
  install_element (ENABLE_NODE, &cli_lacp_show_interfaces_cmd);
}
