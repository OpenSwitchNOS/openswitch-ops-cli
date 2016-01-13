/*
 * Copyright (C) 2000 Kunihiro Ishiguro
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
#include "intf_vty.h"
#include "smap.h"
#include "openswitch-dflt.h"

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
  char *lanes_split_value = NULL;
  char *split_value = NULL;
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
 * Check if port is part of any VRF and return the VRF row.
 */
const struct ovsrec_vrf*
port_vrf_lookup (const struct ovsrec_port *port_row)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  size_t i;
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
      for (i = 0; i < vrf_row->n_ports; i++)
        {
          if (vrf_row->ports[i] == port_row)
            return vrf_row;
        }
    }
  return NULL;
}

/*
 * This function will check if a given IPv4/IPv6 address
 * is already present as a primary or secondary IPv4/IPv6 address.
 */
bool
check_ip_addr_duplicate (const char *ip_address,
                         const struct ovsrec_port *port_row, bool ipv6,
                         bool *secondary)
{
  size_t i;
  if (ipv6)
    {
      if (port_row->ip6_address && !strcmp (port_row->ip6_address, ip_address))
        {
          *secondary = false;
          return true;
        }
      for (i = 0; i < port_row->n_ip6_address_secondary; i++)
        {
          if (!strcmp (port_row->ip6_address_secondary[i], ip_address))
            {
              *secondary = true;
              return true;
            }
        }
    }
  else
    {
      if (port_row->ip4_address && !strcmp (port_row->ip4_address, ip_address))
        {
          *secondary = false;
          return true;
        }
      for (i = 0; i < port_row->n_ip4_address_secondary; i++)
        {
          if (!strcmp (port_row->ip4_address_secondary[i], ip_address))
            {
              *secondary = true;
              return true;
            }
        }
    }
  return false;
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
#ifndef ENABLE_VRF
  vrf_row = vrf_lookup(vrf_name);
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
  enum ovsdb_idl_txn_status status;
  size_t i;
  int n;
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
#ifndef ENABLE_VRF
  vrf_row = vrf_lookup(vrf_name);
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

  for (i = 0; i < vrf_row->n_ports; i++)
    {
      port_name = xstrdup (vrf_row->ports[i]->name);
      ovsrec_port_delete (vrf_row->ports[i]);
      port_check_and_add (port_name, true, true, status_txn);
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
#ifndef ENABLE_VRF
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

  ovsrec_vrf_delete (vrf_row);
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
static int
vrf_add_port (const char *if_name, const char *vrf_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_vrf *unlink_vrf_row = NULL;
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
#ifndef ENABLE_VRF
  vrf_row = vrf_lookup(vrf_name);
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
  ovsrec_port_delete (port_row);
  port_row = port_check_and_add (if_name, true, false, status_txn);

  xrealloc (ports, sizeof *vrf_row->ports * (vrf_row->n_ports + 1));
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
static int
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
#ifndef ENABLE_VRF
  vrf_row = vrf_lookup(vrf_name);
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
  port_row = port_check_and_add (if_name, false, false, status_txn);
  if (!port_row)
    {
      VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_iface_in_vrf (if_name))
    {
      VLOG_DBG ("%s Interface \"%s\" is already L3. No change required.",
                __func__, if_name);
      cli_do_config_abort (status_txn);
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
  bool is_secondary;

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
  if (check_ip_addr_duplicate (ip4, port_row, false, &is_secondary))
    {
      vty_out (vty, "IP address is already assigned to interface %s"
               " as %s.%s",
               if_name, is_secondary ? "secondary" : "primary", VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" already has the IP address \"%s\""
                " assigned to it as \"%s\".",
                __func__, if_name, ip4, is_secondary ? "secondary" : "primary");
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
  bool is_secondary;

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

  if (check_ip_addr_duplicate (ipv6, port_row, true, &is_secondary))
    {
      vty_out (vty, "IP address is already assigned to interface %s"
               " as %s.%s",
               if_name, is_secondary ? "secondary" : "primary", VTY_NEWLINE);
      VLOG_DBG ("%s Interface \"%s\" already has the IP address \"%s\""
                " assigned to it as \"%s\".",
                __func__, if_name, ipv6,
                is_secondary ? "secondary" : "primary");
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
 * This function is used to show the VRF information.
 * Currently, it shows the interfaces attached to each VRF.
 */
static int
show_vrf_info ()
{
  const struct ovsrec_vrf *vrf_row = NULL;
  size_t i;

  vrf_row = ovsrec_vrf_first (idl);
  if (!vrf_row)
    {
      vty_out (vty, "No VRF found.%s", VTY_NEWLINE);
      return CMD_SUCCESS;
    }
  /** Sample output **
   * VRF Configuration:
   * ------------------
   * VRF Name : vrf_default
   *
   *         Interfaces :     Status :
   *         -------------------------
   *         1                up
   *         2                error: no_internal_vlan
   *
   */

  vty_out (vty, "VRF Configuration:%s", VTY_NEWLINE);
  vty_out (vty, "------------------%s", VTY_NEWLINE);
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
      vty_out (vty, "VRF Name : %s%s\n", vrf_row->name, VTY_NEWLINE);
      vty_out (vty, "\tInterfaces :     Status : %s", VTY_NEWLINE);
      vty_out (vty, "\t-------------------------%s", VTY_NEWLINE);
      for (i = 0; i < vrf_row->n_ports; i++)
        {
        if (smap_get(&vrf_row->ports[i]->status, PORT_STATUS_MAP_ERROR) == NULL)
          {
            vty_out (vty, "\t%s                %s%s", vrf_row->ports[i]->name,
                     PORT_STATUS_MAP_ERROR_DEFAULT, VTY_NEWLINE);
          }
        else
          {
            vty_out (vty, "\t%s                error: %s%s", vrf_row->ports[i]->name,
                     smap_get(&vrf_row->ports[i]->status, PORT_STATUS_MAP_ERROR),
                     VTY_NEWLINE);
          }
        }
    }
  return CMD_SUCCESS;

}

DEFUN (cli_vrf_add,
    cli_vrf_add_cmd,
    "vrf VRF_NAME",
    VRF_STR
    "VRF name\n")
{
  if (!strcmp(argv[0], "swns")) {
      vty_out(vty, "Cannot create vrf %s, as %s namespace already present\n",
                     argv[0], argv[0]);
      return CMD_SUCCESS;
  }
  else if (!strcmp(argv[0], "nonet")) {
      vty_out(vty, "Cannot create vrf %s, as %s namespace already present\n",
                     argv[0], argv[0]);
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

DEFUN (cli_vrf_show,
    cli_vrf_show_cmd,
    "show vrf",
    SHOW_STR
    VRF_STR)
{
  return show_vrf_info();
}

/* Install VRF related vty commands. */
void
vrf_vty_init (void)
{
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
  install_element (ENABLE_NODE, &cli_vrf_show_cmd);

  install_element (VLAN_INTERFACE_NODE, &cli_vrf_add_port_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_port_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_config_ip_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_config_ipv6_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_ip_cmd);
  install_element (VLAN_INTERFACE_NODE, &cli_vrf_del_ipv6_cmd);
}
