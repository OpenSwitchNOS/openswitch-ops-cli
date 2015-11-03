/*
 * Copyright (C) 2000 Kunihiro Ishiguro
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
*/
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file sub_if_vty.c
 * SUB_IF CLI Commands
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
#include "intf_vty.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "sub_if_vty.h"

VLOG_DEFINE_THIS_MODULE (vtysh_sub_if_cli);
extern struct ovsdb_idl *idl;
int maximum_sub_interfaces;

/*
 * This function is used to configure an IP address for a port
 * which is attached to a SUB_IF.
 */
static int
sub_if_config_ip (const char *if_name, const char *ip4, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ip4_addresses;
  size_t i;
  bool is_secondary;
  bool port_found;

  status_txn = cli_do_config_start ();
  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, if_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found){
      vty_out(vty,"Port %s not Found\n",if_name);
      return CMD_OVSDB_FAILURE;
  }

  ovsrec_port_set_ip4_address(port_row, ip4);

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
 * which is attached to a SUB_IF.
 */
static int
sub_if_del_ip (const char *if_name, const char *ip4, bool secondary)
{
  const struct ovsrec_port *port_row = NULL;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  char **secondary_ip4_addresses;
  size_t i, n;
  bool port_found;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, if_name) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found){
      vty_out(vty,"Port %s not Found\n",if_name);
      return CMD_OVSDB_FAILURE;
  }


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
      VLOG_DBG ("%s Interface \"%s\" is not attached to any SUB_IF. "
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
    return CMD_SUCCESS;
}

/*
 * CLI "shutdown"
 * default : enabled
 */
DEFUN (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n")
{
    const struct ovsrec_interface * row = NULL;
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn* status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    struct smap smap_user_config;

    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_interface_first(idl);
    if (!row)
    {
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_INTERFACE_FOR_EACH(row, idl)
    {
        if (strcmp(row->name, (char*)vty->index) == 0)
        {
            smap_clone(&smap_user_config, &row->user_config);

            if (vty_flags & CMD_FLAG_NO_CMD)
            {
                smap_replace(&smap_user_config,
                        INTERFACE_USER_CONFIG_MAP_ADMIN,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);
            }
            else
            {
                smap_remove(&smap_user_config,
                        INTERFACE_USER_CONFIG_MAP_ADMIN);
            }
            ovsrec_interface_set_user_config(row, &smap_user_config);
            break;
        }
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if(strcmp(port_row->name, (char*)vty->index) == 0)
        {
            if(vty_flags & CMD_FLAG_NO_CMD)
            {
                ovsrec_port_set_admin(port_row,
                        OVSREC_INTERFACE_ADMIN_STATE_UP);
            }
            else
            {
                ovsrec_port_set_admin(port_row,
                        OVSREC_INTERFACE_ADMIN_STATE_DOWN);
            }
            break;
        }
    }

    status = cli_do_config_finish(status_txn);

    smap_destroy(&smap_user_config);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    }

    return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n");

DEFUN (cli_sub_if_config_ip,
    cli_sub_if_config_ip_cmd,
    "ip address A.B.C.D/M {secondary}",
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return sub_if_config_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_sub_if_del_ip,
    cli_sub_if_del_ip_cmd,
    "no ip address A.B.C.D/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return sub_if_del_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}


/* Install SUB_IF related vty commands. */
void
sub_if_vty_init (void)
{
  install_element (SUB_INTERFACE_NODE, &cli_sub_if_config_ip_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_if_del_ip_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_shutdown_cmd);
  install_element (SUB_INTERFACE_NODE, &no_cli_sub_intf_shutdown_cmd);
}
