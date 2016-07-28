/*
 * Copyright (C) 2000 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
*/
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file vrf_vty.c
 * VRF CLI Commands
 *
 ***************************************************************************/

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "vrf_vty.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "smap.h"
#include "openswitch-dflt.h"
#include "vtysh/utils/vlan_vtysh_utils.h"
#include "vtysh/vtysh_ovsdb_vrf_context.h"
#include "vtysh/utils/vrf_vtysh_utils.h"
#include "vtysh/utils/l3_vtysh_utils.h"
#include "vrf-utils.h"

VLOG_DEFINE_THIS_MODULE (vtysh_vrf_cli);
extern struct ovsdb_idl *idl;

/*
 * Check for split validations on a specific interface.
 * 1. If interface has split capability(split parent interface)
 *    - Check if split.
 *      Dont allow layer 3 configurations if split.
 * 2. If interface is a split child interface
 *    - Check if parent is split.
 *      Dont allow layer 3 configurations if parent is not split.
 */
bool
check_split_iface_conditions (const char *ifname)
{
  const struct ovsrec_interface *if_row, *next, *parent_iface;
  const char *lanes_split_value = NULL;
  const char *split_value = NULL;
  bool allowed = true;

  OVSREC_INTERFACE_FOR_EACH_SAFE(if_row, next, idl)
    {
      if (strcmp(ifname, if_row->name) == 0)
        break;
    }

  if (!if_row)
    {
      /* Interface row is not present */
      return false;
    }

  split_value = smap_get(&if_row->hw_intf_info,
          INTERFACE_HW_INTF_INFO_MAP_SPLIT_4);
  lanes_split_value = smap_get(&if_row->user_config,
          INTERFACE_USER_CONFIG_MAP_LANE_SPLIT);
  /* Check for split_4 attribute */
  if ((split_value != NULL) &&
      (strcmp(split_value,
               INTERFACE_HW_INTF_INFO_MAP_SPLIT_4_TRUE) == 0))
    {
      /* Must be a split parent interface */
      if ((lanes_split_value != NULL) &&
          (strcmp(lanes_split_value,
                   INTERFACE_USER_CONFIG_MAP_LANE_SPLIT_SPLIT) == 0))
        {
          vty_out(vty,
                  "This interface has been split. Operation"
                  " not allowed%s", VTY_NEWLINE);
          allowed = false;
        }
    }
  else
    {
      parent_iface = if_row->split_parent;
      if (parent_iface != NULL)
        {
          lanes_split_value = smap_get(&parent_iface->user_config,
                                       INTERFACE_USER_CONFIG_MAP_LANE_SPLIT);
          if ((lanes_split_value == NULL) ||
              (strcmp(lanes_split_value,
                      INTERFACE_USER_CONFIG_MAP_LANE_SPLIT_SPLIT) != 0))
            {
              /* Parent interface is not split.*/
              vty_out(vty,
                      "This is a QSFP child interface whose"
                      " parent interface has not been split."
                      " Operation not allowed%s", VTY_NEWLINE);
              allowed = false;
            }
        }
    }

  return allowed;
}

/*
 * Adds a new VRF to the VRF table.
 * Takes VRF name as an argument.
 */
static int
vrf_add (const char *vrf_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  const struct ovsrec_system *ovs_row = NULL;
  struct ovsrec_vrf **vrfs;
  size_t i;
  enum ovsdb_idl_txn_status status;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  if (!strcmp (vrf_name, DEFAULT_VRF_NAME))
    {
      vty_out (vty, "Default VRF already exists.%s", VTY_NEWLINE);
      VLOG_DBG ("%s The default VRF is"
                " created as part of system bootup.",
                __func__);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /* OPS_TODO: In case multiple vrfs. */
#ifdef VRF_ENABLE
  vrf_row = vrf_lookup(idl, vrf_name);
  if (vrf_row)
    {
      vty_out (vty, "VRF already exists.%s", VTY_NEWLINE);
      VLOG_DBG("%s Trying to add a VRF which is already present. Check if"
          " VRF name \"%s\" is already present.", __func__, vrf_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }
#else
  vrf_row = ovsrec_vrf_first (idl);
  if (vrf_row)
    {
      vty_out (vty, "Non-default VRFs not supported%s", VTY_NEWLINE);
      VLOG_DBG ("%s Only default VRF is allowed right now. This default VRF is"
                " created as part of system bootup.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
#endif

  if (strlen (vrf_name) > VRF_NAME_MAX_LENGTH)
    {
      vty_out (vty, "VRF name can only be upto 32 characters long.%s",
               VTY_NEWLINE);
      VLOG_DBG ("%s VRF Name can only be 32 characters. Check the VRF name"
                " \"%s\" and try again!",
                __func__, vrf_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  ovs_row = ovsrec_system_first (idl);
  if (!ovs_row)
    {
      vty_out (vty, "Could not fetch System data.%s", VTY_NEWLINE);
      VLOG_DBG ("%s System table did not have any rows. Ideally it"
                " should have just one entry.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  vrf_row = ovsrec_vrf_insert (status_txn);
  ovsrec_vrf_set_name (vrf_row, vrf_name);

  vrfs = xmalloc (sizeof *ovs_row->vrfs * (ovs_row->n_vrfs + 1));

  for (i = 0; i < ovs_row->n_vrfs; i++)
    vrfs[i] = ovs_row->vrfs[i];

  struct ovsrec_vrf
  *temp_vrf_row = CONST_CAST(struct ovsrec_vrf*, vrf_row);
  vrfs[ovs_row->n_vrfs] = temp_vrf_row;
  ovsrec_system_set_vrfs (ovs_row, vrfs, ovs_row->n_vrfs + 1);
  free (vrfs);

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and VRF \"%s\" was added "
                "successfully",
                __func__, vrf_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if VRF \"%s\" "
                "is already present",
                __func__, vrf_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete a VRF and all ports linked to it.
 */
static int
vrf_delete (const char *vrf_name)
{
  /*
   *  OPS_TODO: For now we will only move ports to default VRF.
   */

  const struct ovsrec_vrf *vrf_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  const struct ovsrec_system *ovs_row = NULL;
  const struct ovsrec_port *port_row = NULL;
  struct ovsrec_port **ports;
  enum ovsdb_idl_txn_status status;
  size_t i, n;
  char *port_name;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  if (!strcmp (vrf_name, DEFAULT_VRF_NAME))
    {
      vty_out (vty, "Cannot delete default VRF.%s", VTY_NEWLINE);
      VLOG_DBG ("%s Cannot delete default VRF.", __func__);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /*
   * OPS_TODO: In case of multiple VRFs.
   */
#ifdef VRF_ENABLE
  vrf_row = vrf_lookup(idl, vrf_name);
  if (!vrf_row)
    {
      vty_out(vty, "VRF %s not found.%s", vrf_name, VTY_NEWLINE);
      VLOG_DBG("%s VRF \"%s\" is not found.", __func__, vrf_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }
#else
  vrf_row = ovsrec_vrf_first (idl);
  if (vrf_row)
    {
      vty_out (vty, "Non-default VRFs not supported%s", VTY_NEWLINE);
      VLOG_DBG ("%s All L3 ports are part of default VRF."
                "Cannot attach to any other VRF.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
#endif

  while (vrf_row->n_ports > 0)
    {
      port_name = xstrdup (vrf_row->ports[0]->name);
      port_row = port_check_and_add (port_name, false, false, status_txn);
      ports = xmalloc (sizeof *vrf_row->ports * (vrf_row->n_ports - 1));
      for (i = n = 0; i < vrf_row->n_ports; i++)
      {
          if (vrf_row->ports[i] != port_row)
              ports[n++] = vrf_row->ports[i];
      }
      ovsrec_vrf_set_ports (vrf_row, ports, n);
      ovsrec_port_delete (port_row);
      port_check_and_add (port_name, true, true, status_txn);
      free (ports);
      free (port_name);
    }

  ovs_row = ovsrec_system_first (idl);
  if (!ovs_row)
    {
      vty_out (vty, "Could not fetch System data.%s", VTY_NEWLINE);
      VLOG_DBG ("%s System table did not have any rows. Ideally it"
                " should have just one entry.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /* OPS_TODO: In case multiple vrfs. */
#ifdef VRF_ENABLE
  struct ovsrec_vrf **vrfs;
  vrfs = xmalloc(sizeof *ovs_row->vrfs * (ovs_row->n_vrfs - 1));
  for (i = n = 0; i < ovs_row->n_vrfs; i++)
    {
      if (strcmp(ovs_row->vrfs[i]->name,vrf_name) != 0)
          vrfs[n++] = ovs_row->vrfs[i];
    }
  ovsrec_system_set_vrfs(ovs_row, vrfs, n);
  free(vrfs);
#else
  ovsrec_system_set_vrfs (ovs_row, NULL, 0);
#endif

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and VRF \"%s\" was deleted", __func__,
                vrf_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if VRF \"%s\" "
                "has already been deleted",
                __func__, vrf_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * Adds an interface/port to a VRF.
 * Takes interface name and VRF name as arguments.
 */
int
vrf_add_port (const char *if_name, const char *vrf_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_vrf *unlink_vrf_row = NULL;
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsrec_port **ports;
  size_t i = 0, n = 0;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  /* Check for spit interface conditions */
  if (!check_split_iface_conditions (if_name))
    {
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  port_row = port_check_and_add (if_name, true, true, status_txn);
  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /*
   * OPS_TODO: In case of multiple VRFs, change the error message below.
   * Error message should be "To attach to default VRF, use the no
   * vrf attach command."
   */
  if (!strcmp (vrf_name, DEFAULT_VRF_NAME))
    {
      vty_out (vty, "Already attached to default VRF.%s", VTY_NEWLINE);
      VLOG_DBG ("%s Already attached to default VRF.", __func__);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /*
   * OPS_TODO: In case of multiple VRFs.
   */
#ifdef VRF_ENABLE
  vrf_row = vrf_lookup(idl, vrf_name);
  if (!vrf_row)
    {
      vty_out(vty, "VRF %s not found.%s", vrf_name, VTY_NEWLINE);
      VLOG_DBG("%s VRF \"%s\" is not found.", __func__, vrf_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }
#else
  vrf_row = ovsrec_vrf_first (idl);
  if (vrf_row)
    {
      vty_out (vty, "Non-default VRFs not supported%s", VTY_NEWLINE);
      VLOG_DBG ("%s All L3 ports are part of default VRF."
                "Cannot attach to any other VRF.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
#endif

  unlink_vrf_row = port_vrf_lookup (port_row);
  if (unlink_vrf_row == vrf_row)
    {
      vty_out (vty, "Interface %s is already part of VRF %s.%s", if_name,
               vrf_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is already attached to VRF. \"%s\"",
                __func__, if_name, vrf_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  ports = xmalloc (
      sizeof *unlink_vrf_row->ports * (unlink_vrf_row->n_ports - 1));
  for (i = n = 0; i < unlink_vrf_row->n_ports; i++)
    {
      if (unlink_vrf_row->ports[i] != port_row)
        ports[n++] = unlink_vrf_row->ports[i];
    }
  ovsrec_vrf_set_ports (unlink_vrf_row, ports, n);
  free(ports);

  ovsrec_port_delete (port_row);
  port_row = port_check_and_add (if_name, true, false, status_txn);

  ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports + 1));
  for (i = 0; i < vrf_row->n_ports; i++)
    ports[i] = vrf_row->ports[i];

  struct ovsrec_port
  *temp_port_row = CONST_CAST(struct ovsrec_port*, port_row);
  ports[vrf_row->n_ports] = temp_port_row;
  ovsrec_vrf_set_ports (vrf_row, ports, vrf_row->n_ports + 1);
  free (ports);

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" was attached"
                " to VRF \"%s\"",
                __func__, if_name, vrf_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if "
                "interface \"%s\" is already attached to VRF \"%s\" ",
                __func__, if_name, vrf_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete a port linked to a VRF.
 */
int
vrf_del_port (const char *if_name, const char *vrf_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsrec_port **ports;
  size_t i, n;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  if (!strcmp (vrf_name, DEFAULT_VRF_NAME))
    {
      vty_out (vty, "Cannot detach from default VRF.%s", VTY_NEWLINE);
      VLOG_DBG ("%s Cannot detach from default VRF.", __func__);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /*
   * OPS_TODO: In case of multiple VRFs.
   */
#ifdef VRF_ENABLE
  vrf_row = vrf_lookup(idl, vrf_name);
  if (!vrf_row)
    {
      vty_out(vty, "VRF %s not found.%s", vrf_name, VTY_NEWLINE);
      VLOG_DBG("%s VRF \"%s\" is not found.", __func__, vrf_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }
#else
  vrf_row = get_default_vrf(idl);
  if (vrf_row)
    {
      vty_out (vty, "Non-default VRFs not supported%s", VTY_NEWLINE);
      VLOG_DBG ("%s All L3 ports are part of default VRF."
                "Cannot attach to any other VRF.",
                __func__);
      VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
#endif

  port_row = port_check_and_add (if_name, false, false, status_txn);
  if (!port_row)
    {
      VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  port_row = NULL;
  for (i = 0; i < vrf_row->n_ports; i++)
    {
      if (strcmp (vrf_row->ports[i]->name, if_name) == 0)
        {
          port_row = vrf_row->ports[i];
          break;
        }
    }

  if (!port_row)
    {
      vty_out (vty, "Interface %s not attached to given VRF.%s", if_name,
               VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to VRF \"%s\".", __func__,
                if_name, vrf_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  ports = xmalloc (sizeof *vrf_row->ports * (vrf_row->n_ports - 1));
  for (i = n = 0; i < vrf_row->n_ports; i++)
    {
      if (vrf_row->ports[i] != port_row)
        ports[n++] = vrf_row->ports[i];
    }
  ovsrec_vrf_set_ports (vrf_row, ports, n);
  ovsrec_port_delete (port_row);
  port_row = port_check_and_add (if_name, true, true, status_txn);
  free (ports);

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface %s was detached from"
                " VRF %s",
                __func__, if_name, vrf_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" is already detached from VRF \"%s\"",
                __func__, if_name, vrf_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to make an interface L3.
 * It attaches the port to the default VRF.
 */
static int
vrf_routing (const char *if_name)
{
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_bridge *default_bridge_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsrec_port **ports;
  size_t i, n;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  /* Check for spit interface conditions */
  if (!check_split_iface_conditions (if_name))
    {
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /* Check for acl configuration only if we are changing the port from non-vrf
   * to vrf
   */
  if (!check_iface_in_vrf(if_name) && check_acl_configuration (if_name))
    {
      vty_out(vty, "acl is configured on the interface %s,"
        "Please remove the acl configuration before enabling routing.%s",
        if_name, VTY_NEWLINE);
      VLOG_DBG("%s acl is configured on the interface %s,"
        "Please remove the acl configuration before enabling routing.",
                __func__, if_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }

  port_row = port_check_and_add (if_name, false, false, status_txn);
  if (!port_row)
    {
      VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_vrf(if_name)) {
      vty_out(vty, "Interface %s is already L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG("%s Interface \"%s\" is already L3. No change required.",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_lag (if_name)) {
      vty_out(vty, "Interface %s is associate to Lag.%s", if_name, VTY_NEWLINE);
      VLOG_DBG("%s Interface \"%s\" is associate to Lag. ",
                __func__, if_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
  }

  default_bridge_row = ovsrec_bridge_first (idl);
  ports = xmalloc (
      sizeof *default_bridge_row->ports * (default_bridge_row->n_ports - 1));
  for (i = n = 0; i < default_bridge_row->n_ports; i++)
    {
      if (default_bridge_row->ports[i] != port_row)
        ports[n++] = default_bridge_row->ports[i];
    }
  ovsrec_bridge_set_ports (default_bridge_row, ports, n);
  ovsrec_port_delete (port_row);
  port_row = port_check_and_add (if_name, true, true, status_txn);
  free (ports);

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" is now L3"
                " and attached to default VRF",
                __func__, if_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" is already L3",
                __func__, if_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to make an interface L2.
 * It attaches the port to the default VRF.
 * It also removes all L3 related configuration like IP addresses.
 */
static int
vrf_no_routing (const char *if_name)
{
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_bridge *default_bridge_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  struct ovsrec_port **vrf_ports;
  struct ovsrec_port **bridge_ports;
  size_t i;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  if (check_iface_in_lag(if_name)) {
      vty_out(vty, "Interface %s is associate to Lag.%s", if_name, VTY_NEWLINE);
      VLOG_DBG("%s Interface \"%s\" is associate to Lag. ",
                __func__, if_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
  }

  /* Check for spit interface conditions */
  if (!check_split_iface_conditions (if_name))
    {
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  /* Check for acl configuration only if we are changing the port from
   * non-bridge to bridge
   */
  if (!check_iface_in_bridge(if_name) && check_acl_configuration (if_name))
    {
      vty_out(vty, "acl is configured on the interface %s,"
        "Please remove the acl configuration before disbling routing.%s",
        if_name, VTY_NEWLINE);
      VLOG_DBG("%s acl is configured on the interface %s,"
        "Please remove the acl configuration before disabling routing.",
                __func__, if_name);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
    }


  port_row = port_check_and_add (if_name, true, false, status_txn);
  if (check_iface_in_bridge (if_name))
    {
      VLOG_DBG ("%s Interface \"%s\" is already L2. No change required.",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  else if ((vrf_row = port_vrf_lookup (port_row)) != NULL)
    {
        /* Delete subinterfaces configured, if any. */
        const struct ovsrec_port *tmp_port_row = NULL;
        const struct ovsrec_vrf *tmp_vrf_row = NULL;
        const struct ovsrec_interface *tmp_intf_row = NULL;
        const struct ovsrec_interface *tmp_parent_intf_row = NULL;
        struct ovsrec_port **ports;
        int k=0, n=0, i=0;

        tmp_parent_intf_row = port_row->interfaces[0];

        OVSREC_PORT_FOR_EACH(tmp_port_row, idl)
        {
            if (tmp_port_row->interfaces == NULL) continue;

            tmp_intf_row = tmp_port_row->interfaces[0];
            if (tmp_intf_row->n_subintf_parent > 0)
            {
                if (tmp_intf_row->value_subintf_parent[0] == tmp_parent_intf_row)
                {
                    /* This is a subinterface created for the parent
                       being configured as L2 interface, need to remove. */
                    ovsrec_interface_delete(tmp_intf_row);

                    OVSREC_VRF_FOR_EACH (tmp_vrf_row, idl)
                    {
                        for (k = 0; k < tmp_vrf_row->n_ports; k++)
                        {
                            if (tmp_port_row == tmp_vrf_row->ports[k])
                            {
                                ports = xmalloc(sizeof *tmp_vrf_row->ports
                                        * (tmp_vrf_row->n_ports-1));
                                if (ports != NULL)
                                {
                                   for (i = n = 0; i < tmp_vrf_row->n_ports; i++)
                                   {
                                       if (tmp_vrf_row->ports[i] != tmp_port_row)
                                       {
                                           ports[n++] = tmp_vrf_row->ports[i];
                                       }
                                   }
                                   ovsrec_vrf_set_ports(tmp_vrf_row, ports, n);
                                   free(ports);
                                }
                                break;
                            }
                        }
                    }
                    ovsrec_port_delete(tmp_port_row);
                }
            }
        }
     vrf_ports = xmalloc (sizeof *vrf_row->ports * (vrf_row->n_ports - 1));
      for (i = n = 0; i < vrf_row->n_ports; i++)
        {
          if (vrf_row->ports[i] != port_row)
            vrf_ports[n++] = vrf_row->ports[i];
        }
      ovsrec_vrf_set_ports (vrf_row, vrf_ports, n);
      free (vrf_ports);
      ovsrec_port_delete (port_row);
      port_row = port_check_and_add (if_name, true, false, status_txn);
    }

  ovsrec_port_set_vlan_mode(port_row, OVSREC_PORT_VLAN_MODE_ACCESS);
  ops_port_set_tag(DEFAULT_VLAN, port_row, idl);
  ops_port_set_trunks(NULL, 0, port_row, idl);

  default_bridge_row = ovsrec_bridge_first (idl);
  bridge_ports = xmalloc (
      sizeof *default_bridge_row->ports * (default_bridge_row->n_ports + 1));
  for (i = 0; i < default_bridge_row->n_ports; i++)
    bridge_ports[i] = default_bridge_row->ports[i];

  struct ovsrec_port
  *temp_port_row = CONST_CAST(struct ovsrec_port*, port_row);
  bridge_ports[default_bridge_row->n_ports] = temp_port_row;
  ovsrec_bridge_set_ports (default_bridge_row, bridge_ports,
                           default_bridge_row->n_ports + 1);
  free (bridge_ports);

  status = cli_do_config_finish (status_txn);
  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" is now L2"
                " and attached to default bridge",
                __func__, if_name);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" is already L2",
                __func__, if_name);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }

}

/*
 * This function is used to configure an IP address for a port
 * which is attached to a VRF.
 */
static int
vrf_config_ip (const char *if_name, const char *ip4, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ip4_addresses;
  size_t i;

  if (!is_valid_ip_address(ip4)) {
      vty_out(vty, "  %s %s", OVSDB_INVALID_IPV4_IPV6_ERROR, VTY_NEWLINE);
      return CMD_SUCCESS;
  }

  status_txn = cli_do_config_start ();
  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  /* Check for spit interface conditions */
  if (!check_split_iface_conditions (if_name))
    {
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  port_row = port_check_and_add (if_name, true, true, status_txn);

  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (!is_ip_configurable(vty, ip4, if_name, AF_INET, secondary))
    {
      VLOG_DBG("%s  An interface with the same IP address or "
               "subnet or an overlapping network%s"
               "%s already exists.",
               __func__, VTY_NEWLINE, ip4);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  if (!secondary)
    ovsrec_port_set_ip4_address (port_row, ip4);
  else
    {
      /*
       * Duplicate entries are taken care of of set function.
       * Refer to ovsdb_datum_sort_unique() in vswitch-idl.c
       */
      secondary_ip4_addresses = xmalloc (
          IP_ADDRESS_LENGTH * (port_row->n_ip4_address_secondary + 1));
      for (i = 0; i < port_row->n_ip4_address_secondary; i++)
        secondary_ip4_addresses[i] = port_row->ip4_address_secondary[i];

      secondary_ip4_addresses[port_row->n_ip4_address_secondary] = (char *) ip4;
      ovsrec_port_set_ip4_address_secondary (
          port_row, secondary_ip4_addresses,
          port_row->n_ip4_address_secondary + 1);
      free (secondary_ip4_addresses);
    }

  status = cli_do_config_finish (status_txn);
  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" was configured"
                " with IP address \"%s\"",
                __func__, if_name, ip4);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" is already configured with IP \"%s\"",
                __func__, if_name, ip4);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete an IP address assigned for a port
 * which is attached to a VRF.
 */
static int
vrf_del_ip (const char *if_name, const char *ip4, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ip4_addresses;
  size_t i, n;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  port_row = port_check_and_add (if_name, false, false, status_txn);

  if (!port_row)
    {
      VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (!secondary)
    {
      if (!port_row->ip4_address)
        {
          vty_out (vty, "No IP address configured on interface %s.%s", if_name,
                   VTY_NEWLINE);
          VLOG_DBG ("%s No IP address configured on interface \"%s\".",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }

      if (port_row->n_ip4_address_secondary)
        {
          vty_out (vty, "Delete all secondary IP addresses before deleting"
                   " primary.%s",
                   VTY_NEWLINE);
          VLOG_DBG ("%s Interface \"%s\" has secondary IP addresses"
                    " assigned to it. Delete them before deleting primary.",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }

      if (strcmp (port_row->ip4_address, ip4) != 0)
        {
          vty_out (vty, "IP address %s not found.%s", ip4, VTY_NEWLINE);
          VLOG_DBG ("%s IP address \"%s\" not configured on interface "
                    "\"%s\".",
                    __func__, ip4, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      ovsrec_port_set_ip4_address (port_row, NULL);
    }
  else
    {
      if (!port_row->n_ip4_address_secondary)
        {
          vty_out (vty, "No secondary IP address configured on"
                   " interface %s.%s",
                   if_name, VTY_NEWLINE);
          VLOG_DBG ("%s No secondary IP address configured on interface"
                    " \"%s\".",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      bool ip4_address_match = false;
      for (i = 0; i < port_row->n_ip4_address_secondary; i++)
        {
          if (strcmp (ip4, port_row->ip4_address_secondary[i]) == 0)
            {
              ip4_address_match = true;
              break;
            }
        }

      if (!ip4_address_match)
        {
          vty_out (vty, "IP address %s not found.%s", ip4, VTY_NEWLINE);
          VLOG_DBG ("%s IP address \"%s\" not configured on interface"
                    " \"%s\".",
                    __func__, ip4, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      secondary_ip4_addresses = xmalloc (
          IP_ADDRESS_LENGTH * (port_row->n_ip4_address_secondary - 1));
      for (i = n = 0; i < port_row->n_ip4_address_secondary; i++)
        {
          if (strcmp (ip4, port_row->ip4_address_secondary[i]) != 0)
            secondary_ip4_addresses[n++] = port_row->ip4_address_secondary[i];
        }
      ovsrec_port_set_ip4_address_secondary (port_row, secondary_ip4_addresses,
                                             n);
      free (secondary_ip4_addresses);
    }

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" no longer has"
                " the IP address \"%s\"",
                __func__, if_name, ip4);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" has the IP \"%s\"",
                __func__, if_name, ip4);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to configure an IPv6 address for a port
 * which is attached to a VRF.
 */
static int
vrf_config_ipv6 (const char *if_name, const char *ipv6, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ipv6_addresses;
  size_t i;

  if (!is_valid_ip_address(ipv6)) {
      vty_out(vty, "  %s %s", OVSDB_INVALID_IPV4_IPV6_ERROR, VTY_NEWLINE);
      return CMD_SUCCESS;
  }

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  /* Check for spit interface conditions */
  if (!check_split_iface_conditions (if_name))
    {
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }
  port_row = port_check_and_add (if_name, true, true, status_txn);

  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (!is_ip_configurable(vty, ipv6, if_name, AF_INET6, secondary))
    {
      VLOG_DBG("%s  An interface with the same IP address or "
               "subnet or an overlapping network%s"
               "%s already exists.",
               __func__, VTY_NEWLINE, ipv6);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (!secondary)
    ovsrec_port_set_ip6_address (port_row, ipv6);
  else
    {
      /*
       * Duplicate entries are taken care of of set function.
       * Refer to ovsdb_datum_sort_unique() in vswitch-idl.c
       */
      secondary_ipv6_addresses = xmalloc (
          IPV6_ADDRESS_LENGTH * (port_row->n_ip6_address_secondary + 1));
      for (i = 0; i < port_row->n_ip6_address_secondary; i++)
        secondary_ipv6_addresses[i] = port_row->ip6_address_secondary[i];

      secondary_ipv6_addresses[port_row->n_ip6_address_secondary] =
          (char *) ipv6;
      ovsrec_port_set_ip6_address_secondary (
          port_row, secondary_ipv6_addresses,
          port_row->n_ip6_address_secondary + 1);
      free (secondary_ipv6_addresses);
    }

  status = cli_do_config_finish (status_txn);
  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" was configured"
                " with IPv6 address \"%s\"",
                __func__, if_name, ipv6);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" already has IPv6 \"%s\"",
                __func__, if_name, ipv6);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete an IPv6 address assigned for a port
 * which is attached to a VRF.
 */
static int
vrf_del_ipv6 (const char *if_name, const char *ipv6, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ipv6_addresses;
  size_t i, n;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  port_row = port_check_and_add (if_name, false, false, status_txn);

  if (!port_row)
    {
      VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
      vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (!secondary)
    {
      if (!port_row->ip6_address)
        {
          vty_out (vty, "No IPv6 address configured on interface"
                   " %s.%s",
                   if_name, VTY_NEWLINE);
          VLOG_DBG ("%s No IPv6 address configured on interface"
                    " \"%s\".",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }

      if (port_row->n_ip6_address_secondary)
        {
          vty_out (vty, "Delete all secondary IP addresses before deleting"
                   " primary.%s",
                   VTY_NEWLINE);
          VLOG_DBG ("%s Interface \"%s\" has secondary IP addresses"
                    " assigned to it. Delete them before deleting primary.",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }

      if (strcmp (port_row->ip6_address, ipv6) != 0)
        {
          vty_out (vty, "IPv6 address %s not found.%s", ipv6, VTY_NEWLINE);
          VLOG_DBG ("%s IPv6 address \"%s\" not configured on interface"
                    " \"%s\".",
                    __func__, ipv6, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      ovsrec_port_set_ip6_address (port_row, NULL);
    }
  else
    {
      if (!port_row->n_ip6_address_secondary)
        {
          vty_out (vty, "No secondary IPv6 address configured on interface"
                   " %s.%s",
                   if_name, VTY_NEWLINE);
          VLOG_DBG ("%s No secondary IPv6 address configured on interface"
                    " \"%s\".",
                    __func__, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      bool ipv6_address_match = false;
      for (i = 0; i < port_row->n_ip6_address_secondary; i++)
        {
          if (strcmp (ipv6, port_row->ip6_address_secondary[i]) == 0)
            {
              ipv6_address_match = true;
              break;
            }
        }

      if (!ipv6_address_match)
        {
          vty_out (vty, "IPv6 address %s not found.%s", ipv6, VTY_NEWLINE);
          VLOG_DBG ("%s IPv6 address \"%s\" not configured on interface"
                    " \"%s\".",
                    __func__, ipv6, if_name);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      secondary_ipv6_addresses = xmalloc (
          IPV6_ADDRESS_LENGTH * (port_row->n_ip6_address_secondary - 1));
      for (i = n = 0; i < port_row->n_ip6_address_secondary; i++)
        {
          if (strcmp (ipv6, port_row->ip6_address_secondary[i]) != 0)
            secondary_ipv6_addresses[n++] = port_row->ip6_address_secondary[i];

        }
      ovsrec_port_set_ip6_address_secondary (port_row, secondary_ipv6_addresses,
                                             n);
      free (secondary_ipv6_addresses);
    }

  status = cli_do_config_finish (status_txn);

  if (status == TXN_SUCCESS)
    {
      VLOG_DBG ("%s The command succeeded and interface \"%s\" no longer"
                " has IPv6 address \"%s\"",
                __func__, if_name, ipv6);
      return CMD_SUCCESS;
    }
  else if (status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command resulted in no change. Check if"
                " interface \"%s\" has IPv6 \"%s\"",
                __func__, if_name, ipv6);
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to print the sorted interface list
 * that are attached to VRF and the VRF table_id and status.
 */
static void
print_vrf_info (const struct ovsrec_vrf *vrf_row)
{
    const struct ovsrec_port *port_row = NULL;
    const struct shash_node **nodes;
    struct shash sorted_ports;
    uint16_t count;
    size_t i;

    vty_out (vty, "VRF Name   : %s%s", vrf_row->name, VTY_NEWLINE);
    if (vrf_is_ready(idl, vrf_row->name))
    {
        vty_out (vty, "VRF Status : %s%s", "UP", VTY_NEWLINE);
        vty_out (vty, "table_id   : %" PRId64 "%s", *(vrf_row->table_id), VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "VRF Status : %s%s", "DOWN", VTY_NEWLINE);
        vty_out (vty, "table_id   : %s%s", "", VTY_NEWLINE);
    }
    vty_out (vty, "\t%-20s   %s%s", "Interfaces", "Status", VTY_NEWLINE);
    vty_out (vty, "\t-----------------------------%s", VTY_NEWLINE);

    shash_init(&sorted_ports);
    for (i = 0; i < vrf_row->n_ports; i++)
    {
        shash_add(&sorted_ports, vrf_row->ports[i]->name,
                  (void *)vrf_row->ports[i]);
    }

    count = shash_count(&sorted_ports);
    if (count)
    {
        nodes = xmalloc(count * sizeof *nodes);
        if (nodes)
        {
            ops_sort(&sorted_ports, compare_interface_nodes_vrf, nodes);
            for (i = 0; i < count; i++)
            {
                port_row = (const struct ovsrec_port *)nodes[i]->data;
                if (smap_get(&port_row->status, PORT_STATUS_MAP_ERROR) == NULL)
                {
                    vty_out (vty, "\t%-20s     %s%s", port_row->name,
                             PORT_STATUS_MAP_ERROR_DEFAULT, VTY_NEWLINE);
                }
                else
                {
                    vty_out (vty, "\t%-20s     error : %-8s%s", port_row->name,
                             smap_get(&port_row->status, PORT_STATUS_MAP_ERROR),
                             VTY_NEWLINE);
                }
            }
            free(nodes);
        }
    }
    shash_destroy(&sorted_ports);
    vty_out (vty, "%s", VTY_NEWLINE);
    return;
}

/*
 * This function is used to show the VRF information.
 * Currently, it shows the interfaces attached to each VRF.
 */
static int
show_vrf_info (char* vrf_name)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_vrf vrf;
    extern struct ovsdb_idl_index_cursor vrf_cursor;
    extern bool is_vrf_cursor_initialized;

    /** Sample output **
     * VRF Configuration:
     * ------------------
     * VRF Name : vrf_default
     * VRF Status : UP
     * table_id   : 0
     *         Interfaces :     Status :
     *         -------------------------
     *         1                up
     *         2                error: no_internal_vlan
     *
     */

    if (!is_vrf_cursor_initialized)
    {
        return CMD_SUCCESS;
    }
    else
    {
        vty_out (vty, "VRF Configuration:%s", VTY_NEWLINE);
        vty_out (vty, "------------------%s", VTY_NEWLINE);
        if (vrf_name != NULL)
        {
            vrf.name = strdup(vrf_name);
            vrf_row = ovsrec_vrf_index_find(&vrf_cursor, &vrf);
            if (vrf_row == NULL)
            {
                vty_out(vty, "VRF %s not found.%s", vrf_name, VTY_NEWLINE);
                VLOG_DBG("%s VRF \"%s\" not found.", __func__, vrf_name);
                free(vrf.name);
                return CMD_SUCCESS;
            }
            print_vrf_info(vrf_row);
            free(vrf.name);
        }
        else
        {
            /* Print vrf_default */
            vrf.name = strdup("vrf_default");
            vrf_row = ovsrec_vrf_index_find(&vrf_cursor, &vrf);
            print_vrf_info(vrf_row);

            /* Print the rest of the VRFs */
            OVSREC_VRF_FOR_EACH_BYINDEX (vrf_row, &vrf_cursor)
            {
                /* Skip default VRF */
                if(strncmp("vrf_default", vrf_row->name, strlen("vrf_default") ) == 0)
                    continue;
                print_vrf_info(vrf_row);
            }
            free(vrf.name);
        }
        return CMD_SUCCESS;
    }
}

/*-----------------------------------------------------------------------------
 | Function :vrf_proxy_arp_toggle_state
 | Responsibility : To enable/disable proxy-arp on an interface
 | Parameters:
 |              if_name : Interface name.
 |              value   : The new status of proxy-arp.
 |
 | Return : CMD_SUCCESS for success , CMD_OVSDB_FAILURE for failure
 -----------------------------------------------------------------------------*/
static int vrf_proxy_arp_toggle_state(const char *if_name, const char* value)
{
    const struct ovsrec_port *port_row;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    struct smap smap_other_config;

    if (status_txn == NULL) {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Verify the port is not part of a bridge */
    if (check_iface_in_bridge(if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0)) {
        vty_out(vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge", __func__, if_name);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    /* Check for spit interface conditions */
    if (!check_split_iface_conditions(if_name)) {
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    /* Fetch the port record for the given interface name. */
    OVSREC_PORT_FOR_EACH(port_row, idl) {
        if (strcmp(port_row->name, if_name) == 0) {
            break;
        }
    }

    /* Return error if we didn't find a port-record with the
     * given interface name. */
    if (!port_row) {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Cache the current state of 'other-config' column for this port record */
    smap_clone(&smap_other_config, &port_row->other_config);

    /* Based on the new value passed, add or remove the key to the column */
    if (value == NULL) {
        /* Remove proxy-arp {key,value} pair for this port */
        smap_remove(&smap_other_config,
                PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED);
    } else if (!strcmp(value, PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED_TRUE)
            && !smap_get(&smap_other_config,
            PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED)) {
        /* Add proxy-arp {key,value} pair for this port */
        smap_add(&smap_other_config, PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED,
                value);
    }

    /* Put back the new value of 'other config' column into the port record */
    ovsrec_port_set_other_config(port_row, &smap_other_config);
    smap_destroy(&smap_other_config);
    txn_status = cli_do_config_finish(status_txn);

    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        vty_out(vty, "Status failure Proxy-ARP%s", VTY_NEWLINE);
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}
/*-----------------------------------------------------------------------------
 | Function : vrf_local_proxy_arp_toggle_state
 | Responsibility : To enable/disable local-proxy-arp on an interface
 | Parameters:
 |              if_name : Interface name.
 |              value   : The new status of local-proxy-arp.
 |
 | Return : CMD_SUCCESS for success , CMD_OVSDB_FAILURE for failure
 -----------------------------------------------------------------------------*/
static int
vrf_local_proxy_arp_toggle_state(const char *if_name, const char* value)
{
    const struct ovsrec_port *port_row;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    struct smap smap_other_config;

    if (status_txn == NULL) {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Verify the port is not part of a bridge */
    if (check_iface_in_bridge(if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out(vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge", __func__, if_name);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    /* Check for split interface conditions */
    if (!check_split_iface_conditions(if_name)) {
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    /* Fetch the port record for the given interface name. */
    OVSREC_PORT_FOR_EACH(port_row, idl) {
        if (strcmp(port_row->name, if_name) == 0) {
            break;
        }
    }

    /* Return error if we didn't find a port-record with the
     * given interface name. */
    if (!port_row) {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Cache the current state of 'other-config' column for this port record */
    smap_clone(&smap_other_config, &port_row->other_config);

    /* Based on the new value passed, add or remove the key to the column */
    if (value == NULL) {
        /* Remove local-proxy-arp {key,value} pair for this port */
        smap_remove(&smap_other_config,
                PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED);
    } else if (!strcmp(value, PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED_TRUE)
            && !smap_get(&smap_other_config,
            PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED)) {
        /* Add local-proxy-arp {key,value} pair for this port */
        smap_add(&smap_other_config, PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED,
                value);
    }

    /* Put back the new value of 'other config' column into the port record */
    ovsrec_port_set_other_config(port_row, &smap_other_config);
    smap_destroy(&smap_other_config);
    txn_status = cli_do_config_finish(status_txn);

    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        vty_out(vty, "Status failure Local-Proxy-ARP%s", VTY_NEWLINE);
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN (cli_vrf_add,
    cli_vrf_add_cmd,
    "vrf VRF_NAME",
    VRF_STR
    "VRF name\n")
{
  if (!strcmp(argv[0], "swns")) {
      vty_out(vty, "Cannot create vrf %s, as %s namespace already present.%s",
                     argv[0], argv[0], VTY_NEWLINE);
      return CMD_SUCCESS;
  }
  else if (!strcmp(argv[0], "nonet")) {
      vty_out(vty, "Cannot create vrf %s, as %s namespace already present.%s",
                     argv[0], argv[0], VTY_NEWLINE);
      return CMD_SUCCESS;
  }
  return vrf_add(argv[0]);
}

DEFUN (cli_vrf_delete,
    cli_vrf_delete_cmd,
    "no vrf VRF_NAME",
    NO_STR
    VRF_STR
    "VRF name\n")
{
  return vrf_delete(argv[0]);
}

DEFUN (cli_vrf_add_port,
    cli_vrf_add_port_cmd,
    "vrf attach VRF_NAME",
    VRF_STR
    "Attach interface to VRF\n"
    "VRF name\n")
{
  return vrf_add_port((char*) vty->index, argv[0]);
}

DEFUN (cli_vrf_del_port,
    cli_vrf_del_port_cmd,
    "no vrf attach VRF_NAME",
    NO_STR
    VRF_STR
    "Detach interface from VRF\n"
    "VRF name\n")
{
  return vrf_del_port((char*) vty->index, argv[0]);
}

DEFUN (cli_vrf_routing,
    cli_vrf_routing_cmd,
    "routing",
    "Configure interface as L3\n")
{
  return vrf_routing((char*) vty->index);
}

DEFUN (cli_vrf_no_routing,
    cli_vrf_no_routing_cmd,
    "no routing",
    NO_STR
    "Configure interface as L3\n")
{
    return vrf_no_routing((char*) vty->index);
}

DEFUN (cli_vrf_config_ip,
    cli_vrf_config_ip_cmd,
    "ip address A.B.C.D/M {secondary}",
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return vrf_config_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_vrf_del_ip,
    cli_vrf_del_ip_cmd,
    "no ip address A.B.C.D/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return vrf_del_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_vrf_config_ipv6,
    cli_vrf_config_ipv6_cmd,
    "ipv6 address X:X::X:X/M {secondary}",
    IPV6_STR
    "Set IP address\n"
    "Interface IPv6 address\n"
    "Set as secondary IPv6 address\n")
{
  return vrf_config_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_vrf_del_ipv6,
    cli_vrf_del_ipv6_cmd,
    "no ipv6 address X:X::X:X/M {secondary}",
    NO_STR
    IPV6_STR
    "Set IP Address\n"
    "Interface IPv6 address\n"
    "Set as secondary IPv6  address\n")
{
  return vrf_del_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_vrf_show_vrf_name,
       cli_vrf_show_vrf_name_cmd,
       "show vrf VRF",
       SHOW_STR
       VRF_STR
       "VRF name\n")
{
  return show_vrf_info((char*) argv[0]);
}

DEFUN (cli_vrf_show,
       cli_vrf_show_cmd,
       "show vrf",
       SHOW_STR
       VRF_STR)
{
  return show_vrf_info(NULL);
}

#ifdef FTR_PROXY_ARP
  DEFUN (cli_vrf_proxy_arp_enable,
         cli_vrf_proxy_arp_enable_cmd,
         "ip proxy-arp",
         IP_STR
         "Enable proxy ARP\n")
  {
       return vrf_proxy_arp_toggle_state((char *) vty->index,
               PORT_OTHER_CONFIG_MAP_PROXY_ARP_ENABLED_TRUE);
  }

  DEFUN (cli_vrf_proxy_arp_disable,
         cli_vrf_proxy_arp_disable_cmd,
         "no ip proxy-arp",
         NO_STR
         IP_STR
         "Disable proxy ARP\n")
  {
      return vrf_proxy_arp_toggle_state((char *) vty->index, NULL);
  }
#endif /* FTR_PROXY_ARP */

#ifdef FTR_LOCAL_PROXY_ARP
  DEFUN (cli_vrf_local_proxy_arp_enable,
         cli_vrf_local_proxy_arp_enable_cmd,
         "ip local-proxy-arp",
         IP_STR
         "Enable local proxy ARP\n")
  {
      return vrf_local_proxy_arp_toggle_state((char *) vty->index,
              PORT_OTHER_CONFIG_MAP_LOCAL_PROXY_ARP_ENABLED_TRUE);
  }

  DEFUN (cli_vrf_local_proxy_arp_disable,
         cli_vrf_local_proxy_arp_disable_cmd,
         "no ip local-proxy-arp",
         NO_STR
         IP_STR
         "Disable local proxy ARP\n")
  {
      return vrf_local_proxy_arp_toggle_state((char *) vty->index, NULL);
  }
#endif /* FTR_LOCAL_PROXY_ARP */

/* Install VRF related vty commands. */
void
vrf_vty_init (void)
{
  vtysh_ret_val retval = e_vtysh_error;

  install_element (CONFIG_NODE, &cli_vrf_add_cmd);
  install_element (CONFIG_NODE, &cli_vrf_delete_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_add_port_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_del_port_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_config_ip_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_config_ipv6_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_del_ip_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_del_ipv6_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_routing_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_no_routing_cmd);
#ifdef FTR_PROXY_ARP
  install_element (INTERFACE_NODE, &cli_vrf_proxy_arp_enable_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_proxy_arp_disable_cmd);
#endif /* FTR_PROXY_ARP */

#ifdef FTR_LOCAL_PROXY_ARP
  install_element (INTERFACE_NODE, &cli_vrf_local_proxy_arp_enable_cmd);
  install_element (INTERFACE_NODE, &cli_vrf_local_proxy_arp_disable_cmd);
#endif /* FTR_LOCAL_PROXY_ARP */

  install_element (ENABLE_NODE, &cli_vrf_show_cmd);
  install_element (ENABLE_NODE, &cli_vrf_show_vrf_name_cmd);

  install_element (VLAN_INTERFACE_NODE, &cli_vrf_add_port_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_port_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_config_ip_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_config_ipv6_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_ip_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_ipv6_cmd);
#ifdef FTR_PROXY_ARP
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_proxy_arp_enable_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_proxy_arp_disable_cmd);
#endif /* FTR_PROXY_ARP */

#ifdef FTR_LOCAL_PROXY_ARP
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_local_proxy_arp_enable_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_local_proxy_arp_disable_cmd);
#endif /* FTR_LOCAL_PROXY_ARP */

  retval = e_vtysh_error;
  retval = install_show_run_config_subcontext(e_vtysh_interface_context,
                                     e_vtysh_interface_context_vrf,
                                     &vtysh_intf_context_vrf_clientcallback,
                                     NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                           "Interface context unable to add vrf client callback");
    assert(0);
    return;
  }

  retval = e_vtysh_error;
  retval = install_show_run_config_subcontext(e_vtysh_config_context,
                                     e_vtysh_config_context_vrf,
                                     &vtysh_config_context_vrf_clientcallback,
                                     NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                           "Config context unable to add vrf client callback");
    assert(0);
    return;
  }

}
