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
 * @file loopback_intf_vty.c
 * LOOPBACK CLI Commands
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
#include "loopback_vty.h"

VLOG_DEFINE_THIS_MODULE (vtysh_loopback_if_cli);
extern struct ovsdb_idl *idl;


static int
create_if(char *ifnumber)
{
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_interface *intf_row;
  struct ovsrec_interface **iface_list;
  bool port_found = false;
  bool intf_found = false;
  struct ovsdb_idl_txn *txn = NULL;
  enum ovsdb_idl_txn_status status_txn;
  const struct ovsrec_vrf *default_vrf_row = NULL;
  const struct ovsrec_vrf *vrf_row = NULL;
  int i=0;
  struct ovsrec_port **ports = NULL;
  int loopback_number;


  loopback_number = atoi(ifnumber + 1);
  if((loopback_number < MIN_LOOP_INTF) || (loopback_number > MAX_LOOP_INTF))
  {
	vty_out(vty, "invalid loopback range <0-1024>.%s",VTY_NEWLINE);
    	return CMD_ERR_NOTHING_TODO;
  }

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, ifnumber) == 0)
    {
      port_found = true;
      break;
    }
  }
  if(!port_found)
  {
    txn = cli_do_config_start();
    if (txn == NULL)
    {
      VLOG_DBG("Transaction creation failed by %s. Function=%s, Line=%d",
               " cli_do_config_start()", __func__, __LINE__);
          cli_do_config_abort(txn);
          return CMD_OVSDB_FAILURE;
    }

    /* adding an interface */
    intf_row = ovsrec_interface_insert(txn);
    ovsrec_interface_set_name(intf_row, ifnumber);

    ovsrec_interface_set_type(intf_row, OVSREC_INTERFACE_TYPE_LOOPBACK);

   /*creating port table entry*/
    port_row = ovsrec_port_insert(txn);
    ovsrec_port_set_name(port_row, ifnumber);

    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
            default_vrf_row = vrf_row;
            break;
        }
    }

    /* Adding a interface to the corresponding port*/
    iface_list = xmalloc(sizeof(struct ovsrec_interface));
    iface_list[0] = (struct ovsrec_interface *)intf_row;
    ovsrec_port_set_interfaces(port_row, iface_list, 1);

    if(default_vrf_row == NULL)
    {
      assert(0);
      VLOG_DBG("Couldn't fetch default VRF row. Function=%s, Line=%d",
                __func__, __LINE__);
      cli_do_config_abort(txn);
      return CMD_OVSDB_FAILURE;
    }

    ports = xmalloc(sizeof *default_vrf_row->ports *
                   (default_vrf_row->n_ports + 1));
    for (i = 0; i < default_vrf_row->n_ports; i++)
    {
      ports[i] = default_vrf_row->ports[i];
    }
    ports[default_vrf_row->n_ports] = CONST_CAST(struct ovsrec_port*,port_row);
    ovsrec_vrf_set_ports(default_vrf_row, ports,
                         default_vrf_row->n_ports + 1);
    free(ports);

    status_txn = cli_do_config_finish(txn);
    if(status_txn == TXN_SUCCESS || status_txn == TXN_UNCHANGED)
    {
      return CMD_SUCCESS;
    }
    else
    {
      VLOG_ERR("Transaction commit failed in function=%s, line=%d",__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
    }
  }
  else
  {
    return CMD_SUCCESS;
  }

  return CMD_SUCCESS;
}


/*
 * This function is used to configure an IP address for a port
 * which is attached to a SUB_IF.
 */
static int
loopback_if_config_ip (const char *if_name, const char *ip4, bool secondary)
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
loopback_if_del_ipv4 (const char *if_name, const char *ip4, bool secondary)
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
 * This function is used to configure an IPv6 address for a port
 * which is attached to a SUB_IF.
 */
static int
loopback_if_config_ipv6 (const char *if_name, const char *ipv6, bool secondary)
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
      VLOG_DBG ("%s Interface \"%s\" is not attached to any SUB_IF. "
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
loopback_if_del_ipv6 (const char *if_name, const char *ipv6, bool secondary)
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


static int
cli_show_loopback_interface_exec (struct cmd_element *self, struct vty *vty,
        int flags, int argc, const char *argv[], bool brief)
{
    const struct ovsrec_interface *ifrow = NULL;
    const char *cur_state =NULL;
    struct shash sorted_interfaces;
    const struct shash_node **nodes;
    int idx, count;

    const struct ovsdb_datum *datum;
    static char *interface_statistics_keys [] = {
        "rx_packets",
        "rx_bytes",
        "tx_packets",
        "tx_bytes",
        "rx_dropped",
        "rx_frame_err",
        "rx_over_err",
        "rx_crc_err",
        "rx_errors",
        "tx_dropped",
        "collisions",
        "tx_errors"
    };

    unsigned int index;
    int64_t intVal = 0;

    if (brief)
    {
        /* Display the brief information */
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
        vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason                   Speed     Port%s", VTY_NEWLINE);
        vty_out(vty, "Interface                                                          (Mb/s)    Ch#%s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "%s", VTY_NEWLINE);
    }

    shash_init(&sorted_interfaces);

    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        shash_add(&sorted_interfaces, ifrow->name, (void *)ifrow);
    }

    nodes = shash_sort(&sorted_interfaces);

    count = shash_count(&sorted_interfaces);

    for (idx = 0; idx < count; idx++)
    {
        union ovsdb_atom atom;

        ifrow = (const struct ovsrec_interface *)nodes[idx]->data;

        if ((NULL != argv[0]) && (0 != strcmp(argv[0],ifrow->name)))
        {
            continue;
        }

        if ((strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_INTERNAL) == 0) ||
              (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_SYSTEM) == 0) ||
              (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) == 0))
        {
            /* Skipping internal interfaces */
            continue;
        }

        if (brief)
        {
            /* Display the brief information */
            vty_out (vty, " %-12s ", ifrow->name);
            vty_out(vty, "--      "); /*vVLAN */
            vty_out(vty, "eth  "); /*type */
            vty_out(vty, "--     "); /* mode - routed or not */

            vty_out (vty, "%-6s ", ifrow->link_state);

            if ((NULL != ifrow->admin_state) &&
                    (strcmp(ifrow->admin_state,
                            OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0))
            {
                vty_out (vty, "Administratively down    ");
            }
            else
            {
                vty_out (vty, "                         ");
            }
            intVal = 0;
            datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
            if ((NULL!=datum) && (datum->n >0))
            {
                intVal = datum->keys[0].integer;
            }

            if(intVal == 0)
            {
                vty_out(vty, " %-6s", "auto");
            }
            else
            {
                vty_out(vty, " %-6ld", intVal/1000000);
            }
            vty_out(vty, "   -- ");  /* Port channel */
            vty_out (vty, "%s", VTY_NEWLINE);
        }
        else
        {
            intVal = 0;

            vty_out (vty, "Interface %s is %s ", ifrow->name, ifrow->link_state);
            if ((NULL != ifrow->admin_state)
                    && strcmp(ifrow->admin_state,
                            OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0)
            {
                vty_out (vty, "(Administratively down) %s", VTY_NEWLINE);
                vty_out (vty, " Admin state is down%s",
                        VTY_NEWLINE);
            }
            else
            {
                vty_out (vty, "%s", VTY_NEWLINE);
                vty_out (vty, " Admin state is up%s", VTY_NEWLINE);
            }

            if (ifrow->error != NULL)
            {
                vty_out (vty, " State information: %s%s",
                        ifrow->error, VTY_NEWLINE);
            }

            //Check for sub interface
            char parent_intf[PARENT_INTERFACE_NAME_LENGTH];
            int len=0;
            if (strcmp(ifrow->type,OVSREC_INTERFACE_TYPE_VLANSUBINT) == 0)
	    {
            	len = strlen(ifrow->name) - strlen(strchr(ifrow->name,'.'));
	    	if(len)
		{
            		strncpy(parent_intf,ifrow->name,len);
			parent_intf[len]=NULL;
            		vty_out (vty, " parent interface is : %s %s",
                			parent_intf, VTY_NEWLINE);
	    	}
	   }

            parse_vlan(ifrow->name, vty);
            vty_out (vty, " Hardware: Ethernet, MAC Address: %s %s",
                    ifrow->mac_in_use, VTY_NEWLINE);

            /* Displaying ipv4 and ipv6 primary and secondary addresses*/
            show_ip_addresses(ifrow->name, vty);

            datum = ovsrec_interface_get_mtu(ifrow, OVSDB_TYPE_INTEGER);
            if ((NULL!=datum) && (datum->n >0))
            {
                intVal = datum->keys[0].integer;
            }

            vty_out(vty, " MTU %ld %s", intVal, VTY_NEWLINE);

            if ((NULL != ifrow->duplex) &&
                    (strcmp(ifrow->duplex, "half") == 0))
            {
                vty_out(vty, " Half-duplex %s", VTY_NEWLINE);
            }
            else
            {
                vty_out(vty, " Full-duplex %s", VTY_NEWLINE);
            }

            intVal = 0;
            datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
            if ((NULL!=datum) && (datum->n >0))
            {
                intVal = datum->keys[0].integer;
            }
            vty_out(vty, " Speed %ld Mb/s %s",intVal/1000000 , VTY_NEWLINE);

            cur_state = smap_get(&ifrow->user_config,
                    INTERFACE_USER_CONFIG_MAP_AUTONEG);
            if ((NULL == cur_state) ||
                    strcmp(cur_state, "off") !=0)
            {
                vty_out(vty, " Auto-Negotiation is turned on %s", VTY_NEWLINE);
            }
            else
            {
                vty_out(vty, " Auto-Negotiation is turned off %s",
                        VTY_NEWLINE);
            }

            cur_state = ifrow->pause;
            if (NULL != cur_state)
            {
                if (strcmp(cur_state,
                        INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) == 0)

                {
                    vty_out(vty, " Input flow-control is off, "
                            "output flow-control is off%s",VTY_NEWLINE);
                }
                else if (strcmp(cur_state,
                        INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0)
                {
                    vty_out(vty, " Input flow-control is on, "
                            "output flow-control is off%s",VTY_NEWLINE);
                }
                else if (strcmp(cur_state,
                        INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0)
                {
                    vty_out(vty, " Input flow-control is off, "
                            "output flow-control is on%s",VTY_NEWLINE);
                }
                else
                {
                    vty_out(vty, " Input flow-control is on, "
                            "output flow-control is on%s",VTY_NEWLINE);
                }
            }
            else
            {
                vty_out(vty, " Input flow-control is off, "
                        "output flow-control is off%s",VTY_NEWLINE);
            }

            datum = ovsrec_interface_get_statistics(ifrow,
                    OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

            if (NULL==datum) continue;

            vty_out(vty, " RX%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[0];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld input packets  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            atom.string = interface_statistics_keys[1];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld bytes  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[8];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld input error    ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            atom.string = interface_statistics_keys[4];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld dropped  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[7];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld CRC/FCS  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            vty_out(vty, " TX%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[2];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld output packets ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            atom.string = interface_statistics_keys[3];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld bytes  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[11];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld input error    ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            atom.string = interface_statistics_keys[9];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld dropped  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            atom.string = interface_statistics_keys[10];
            index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
            vty_out(vty, "   %10ld collision  ",
                    (index == UINT_MAX)? 0 : datum->values[index].integer);
            vty_out(vty, "%s", VTY_NEWLINE);

            vty_out(vty, "%s", VTY_NEWLINE);

            if (NULL != argv[0])
            {
                break;
            }
        }
    }

    shash_destroy(&sorted_interfaces);
    free(nodes);

    return CMD_SUCCESS;
}


static int
remove_intf_from_loopback_if(const char *if_name)
{
   const struct ovsrec_interface *row = NULL;
   const struct ovsrec_interface *interface_row = NULL;
   const struct ovsrec_interface *if_row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   char loopback_if_name[8]={0};
   bool port_found = false;
   struct ovsrec_interface **interfaces;
   const struct ovsrec_port *loopback_if_port = NULL;
   int i=0, n=0, k=0;
   bool interface_found = false;

#if 1
   /* Check if the LOOPBACK_INTERFACE port is present in DB. */
   OVSREC_PORT_FOR_EACH(loopback_if_port, idl)
   {
     if (strcmp(loopback_if_port->name, if_name) == 0)
     {
       for (k = 0; k < loopback_if_port->n_interfaces; k++)
       {
         if_row = loopback_if_port->interfaces[k];
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
     vty_out(vty, "Specified LOOPBACK_INTERFACE port doesn't exist.\n");
     return CMD_SUCCESS;
   }
   if(!interface_found)
   {
     vty_out(vty, "Interface %s is not part of %s.\n", if_name, if_name);
     return CMD_SUCCESS;
   }

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(SUB_IF_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
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
   interfaces = xmalloc(sizeof *loopback_if_port->interfaces * (loopback_if_port->n_interfaces-1));
   for(i = n = 0; i < loopback_if_port->n_interfaces; i++)
   {
      if(loopback_if_port->interfaces[i] != interface_row)
      {
         interfaces[n++] = loopback_if_port->interfaces[i];
      }
   }
   ovsrec_interface_delete(interface_row);
   ovsrec_port_set_interfaces(loopback_if_port, interfaces, n);
   free(interfaces);
   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(SUB_IF_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
   }
#endif
}

static int
delete_loopback_if(const char *loopback_if_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_bridge *bridge_row = NULL;
  const struct ovsrec_port *loopback_if_port_row = NULL;
  bool loopback_if_to_vrf = false;
  bool loopback_if_to_bridge = false;
  struct ovsrec_port **ports;
  int k=0, n=0, i=0;
  struct ovsdb_idl_txn* status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  bool port_found = false;
  int ret;

  ret = remove_intf_from_loopback_if(loopback_if_name);
  if(ret == CMD_OVSDB_FAILURE){
	vty_out(vty, "Unable to remove interface from loopback interface port. %s",VTY_NEWLINE);
  }

  /* Return if LOOPBACK_INTERFACE port doesn't exit */
  OVSREC_PORT_FOR_EACH(loopback_if_port_row, idl)
  {
    if (strcmp(loopback_if_port_row->name, loopback_if_name) == 0)
    {
       port_found = true;
    }
  }

  if(!port_found)
  {
    vty_out(vty, "Specified LOOPBACK_INTERFACE port doesn't exist.%s",VTY_NEWLINE);
    return CMD_SUCCESS;
  }

  /* Check if the given LOOPBACK_INTERFACE port is part of VRF */
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
  {
    for (k = 0; k < vrf_row->n_ports; k++)
    {
       loopback_if_port_row = vrf_row->ports[k];
       if(strcmp(loopback_if_port_row->name, loopback_if_name) == 0)
       {
          loopback_if_to_vrf = true;
          goto port_attached;
       }
    }
  }

   /* Check if the given LOOPBACK_INTERFACE port is part of bridge */
  OVSREC_BRIDGE_FOR_EACH (bridge_row, idl)
  {
    for (k = 0; k < bridge_row->n_ports; k++)
    {
       loopback_if_port_row = bridge_row->ports[k];
       if(strcmp(loopback_if_port_row->name, loopback_if_name) == 0)
       {
          loopback_if_to_bridge = true;
          goto port_attached;
       }
    }
  }

port_attached:
  if(loopback_if_to_vrf || loopback_if_to_bridge)
  {
    /* LOOPBACK_INTERFACE port is attached to either VRF or bridge.
     * So create transaction.                    */
    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
      VLOG_ERR(SUB_IF_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
    }
  }
  else
  {
    /* assert if the LOOPBACK_INTERFACE port is not part of either VRF or bridge */
    assert(0);
    VLOG_ERR("Port table entry not found either in VRF or in bridge.Function=%s, Line=%d", __func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }

  if(loopback_if_to_vrf)
  {
    /* Delete the LOOPBACK_INTERFACE port reference from VRF */
    ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
    for(i = n = 0; i < vrf_row->n_ports; i++)
    {
       if(vrf_row->ports[i] != loopback_if_port_row)
       {
          ports[n++] = vrf_row->ports[i];
       }
    }
    ovsrec_vrf_set_ports(vrf_row, ports, n);
    free(ports);
  }
  else if(loopback_if_to_bridge)
  {
    /* Delete the LOOPBACK_INTERFACE port reference from Bridge */
    ports = xmalloc(sizeof *bridge_row->ports * (bridge_row->n_ports-1));
    for(i = n = 0; i < bridge_row->n_ports; i++)
    {
       if(bridge_row->ports[i] != loopback_if_port_row)
       {
          ports[n++] = bridge_row->ports[i];
       }
    }
    ovsrec_bridge_set_ports(bridge_row, ports, n);
    free(ports);
  }

  /* Delete the port as we cleared references to it from VRF or bridge*/
  ovsrec_port_delete(loopback_if_port_row);

  status = cli_do_config_finish(status_txn);

  if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
  {
     return CMD_SUCCESS;
  }
  else
  {
     VLOG_ERR(SUB_IF_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
     return CMD_OVSDB_FAILURE;
  }
}

/*
 * CLI "shutdown"
 * default : enabled
 */
DEFUN (cli_loopback_intf_shutdown,
        cli_loopback_intf_shutdown_cmd,
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

DEFUN (cli_intf_show_intferface_loopback,
        cli_intf_show_intferface_loopback_cmd,
        "show interface loopback {brief|transceiver}",
        SHOW_STR
        INTERFACE_STR
        IFNAME_STR
        "Show brief info of interface\n"
        "Show transceiver info for interface\n")
{
    bool brief = false;
    bool transceiver = false;
    int rc = CMD_SUCCESS;

    if ((NULL != argv[0]) && (strcmp(argv[0], "brief") == 0))
    {
        brief = true;
    }
    if ((NULL != argv[1]) && (strcmp(argv[1], "transceiver") == 0))
    {
        transceiver = true;
    }
    argv[0] = NULL;

    if (transceiver)
    {
        rc = cli_show_xvr_exec (self, vty, vty_flags, argc, argv, brief);
    }
    else
    {
        rc = cli_show_loopback_interface_exec (self, vty, vty_flags, argc, argv, brief);
    }
    return rc;
}

DEFUN (cli_intf_show_intferface_loopback_instance,
        cli_intf_show_intferface_loopback_instance_cmd,
        "show interface loopback IFNAME {brief|transceiver}",
        SHOW_STR
        INTERFACE_STR
        IFNAME_STR
        "Show brief info of interface\n"
        "Show transceiver info for interface\n")
{
    bool brief = false;
    bool transceiver = false;
    int rc = CMD_SUCCESS;
    static char ifname[SUB_INTF_NAME_LENGTH]={0};
    snprintf(ifname,SUB_INTF_NAME_LENGTH,"%s%s","l",argv[0]);
    vty->index = ifname;
    strcpy(argv[0],ifname);

    if ((NULL != argv[1]) && (strcmp(argv[1], "brief") == 0))
    {
        brief = true;
    }
    if ((NULL != argv[2]) && (strcmp(argv[2], "transceiver") == 0))
    {
        transceiver = true;
    }
    if (transceiver)
    {
        rc = cli_show_xvr_exec (self, vty, vty_flags, argc, argv, brief);
    }
    else
    {
        rc = cli_show_interface_exec (self, vty, vty_flags, argc, argv, brief);
    }

    return rc;
}

DEFUN_NO_FORM (cli_loopback_intf_shutdown,
        cli_loopback_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n");

DEFUN (cli_loopback_if_config_ipv4,
    cli_loopback_if_config_ipv4_cmd,
    "ip address A.B.C.D/M {secondary}",
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return loopback_if_config_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_loopback_if_del_ipv4,
    cli_loopback_if_del_ipv4_cmd,
    "no ip address A.B.C.D/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return loopback_if_del_ipv4((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_loopback_if_config_ipv6,
    cli_loopback_if_config_ipv6_cmd,
    "ipv6 address X:X::X:X/M {secondary}",
    IPV6_STR
    "Set IP address\n"
    "Interface IPv6 address\n"
    "Set as secondary IPv6 address\n")
{
  return loopback_if_config_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_loopback_if_del_ipv6,
    cli_loopback_if_del_ipv6_cmd,
    "no ipv6 address X:X::X:X/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return loopback_if_del_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (vtysh_del_loopback_interface,
       vtysh_del_loopback_interface_cmd,
       "no interface loopback IFNAME",
      "Select a loopback to interface\n"
      "loopback interface (l<number>) name\n")
{
  static char ifname[SUB_INTF_NAME_LENGTH]={0};
  snprintf(ifname,SUB_INTF_NAME_LENGTH,"%s%s","l",argv[0]);
  return delete_loopback_if(ifname);
}

DEFUN (vtysh_loopback_interface,
       vtysh_loopback_interface_cmd,
       "interface loopback IFNAME",
      "Select a loopback to configure\n"
      "loopback interfce (l<number>) name\n")
{
  static char ifname[SUB_INTF_NAME_LENGTH]={0};
  snprintf(ifname,SUB_INTF_NAME_LENGTH,"%s%s","l",argv[0]);
  vty->index = ifname;
  vty->node = LOOPBACK_INTERFACE_NODE;
  return create_if(ifname);
}

/* Install SUB_IF related vty commands. */
void
loopback_intf_vty_init (void)
{
  /*Configurations at CONFIG_NODE*/
  install_element (CONFIG_NODE,	       	    &vtysh_loopback_interface_cmd);
  install_element (CONFIG_NODE,	       	    &vtysh_del_loopback_interface_cmd);

  /*Configurations at LOOPBACK_INTERFACE_NODE*/
  install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_config_ipv4_cmd);
  install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_del_ipv4_cmd);
  install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_config_ipv6_cmd);
  install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_del_ipv6_cmd);
  install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_intf_shutdown_cmd);
  install_element (LOOPBACK_INTERFACE_NODE, &no_cli_loopback_intf_shutdown_cmd);

  /*show cammands at enable node */
  install_element (ENABLE_NODE, &cli_intf_show_intferface_loopback_instance_cmd);
  install_element (ENABLE_NODE, &cli_intf_show_intferface_loopback_cmd);

}
