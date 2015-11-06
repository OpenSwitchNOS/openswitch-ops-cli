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
 * @file sub_intf_vty.c
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
#include "sub_intf_vty.h"
#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vlan_vty.h"
#include "vrf_vty.h"


VLOG_DEFINE_THIS_MODULE (vtysh_sub_intf_cli);
extern struct ovsdb_idl *idl;

/*
 * This function is used to configure an IP address for a port
 * which is attached to a sub interface.
 */
static int
sub_intf_config_ip (const char *if_name, const char *ip4, bool secondary)
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
      return CMD_SUCCESS;
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
 * which is attached to a sub interface.
 */
static int
sub_intf_del_ipv4 (const char *if_name, const char *ip4, bool secondary)
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
 * which is attached to a sub interface.
 */
static int
sub_intf_config_ipv6 (const char *if_name, const char *ipv6, bool secondary)
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
      VLOG_DBG ("%s Interface \"%s\" is not attached to any Sub Interface. "
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
sub_intf_del_ipv6 (const char *if_name, const char *ipv6, bool secondary)
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
remove_intf_from_sub_intf(const char *if_name)
{
   const struct ovsrec_interface *row = NULL;
   const struct ovsrec_interface *interface_row = NULL;
   const struct ovsrec_interface *if_row = NULL;
   struct ovsdb_idl_txn* status_txn = NULL;
   enum ovsdb_idl_txn_status status;
   char sub_intf_name[8]={0};
   bool port_found = false;
   struct ovsrec_interface **interfaces;
   const struct ovsrec_port *sub_intf_port = NULL;
   int i=0, n=0, k=0;
   bool interface_found = false;

   /* Check if the sub interface is present in DB. */
   OVSREC_PORT_FOR_EACH(sub_intf_port, idl)
   {
     if (strcmp(sub_intf_port->name, if_name) == 0)
     {
       for (k = 0; k < sub_intf_port->n_interfaces; k++)
       {
         if_row = sub_intf_port->interfaces[k];
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
     vty_out(vty, "Specified sub interface port doesn't exist.\n");
     return CMD_SUCCESS;
   }
   if(!interface_found)
   {
     vty_out(vty, "Interface %s is not part of %s.\n", if_name, if_name);
     return CMD_SUCCESS;
   }

   //Check weather interface is sub-interface or loopback interface
   //return if intreface is not sub-interface or loopback interface
   if ((strcmp(if_row->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
      || (strcmp(if_row->type, OVSREC_INTERFACE_TYPE_LOOPBACK) != 0) == 0)
   {
	return CMD_SUCCESS;
   }

   status_txn = cli_do_config_start();
   if(status_txn == NULL)
   {
      VLOG_ERR(SUB_IF_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   /* check for interface row and copy the instance in variable */
   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, if_name) == 0)
      {
         interface_row = row;
         break;
      }
   }

   /* Unlink the interface from the Port row found*/
   interfaces = xmalloc(sizeof *sub_intf_port->interfaces * (sub_intf_port->n_interfaces-1));
   for(i = n = 0; i < sub_intf_port->n_interfaces; i++)
   {
      if(sub_intf_port->interfaces[i] != interface_row)
      {
         interfaces[n++] = sub_intf_port->interfaces[i];
      }
   }
   ovsrec_interface_delete(interface_row);
   ovsrec_port_set_interfaces(sub_intf_port, interfaces, n);
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
}

int
delete_sub_intf(const char *sub_intf_name)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  const struct ovsrec_bridge *bridge_row = NULL;
  const struct ovsrec_port *sub_intf_port_row = NULL;
  bool sub_intf_to_vrf = false;
  bool sub_intf_to_bridge = false;
  struct ovsrec_port **ports;
  int k=0, n=0, i=0;
  struct ovsdb_idl_txn* status_txn = NULL;
  enum ovsdb_idl_txn_status status;
  bool port_found = false;
  int ret;

  ret = remove_intf_from_sub_intf(sub_intf_name);
  if(ret == CMD_OVSDB_FAILURE){
	vty_out(vty, "Unable to remove interface from subinterface port. %s",VTY_NEWLINE);
  }

  /* Return if sub interface port doesn't exit */
  OVSREC_PORT_FOR_EACH(sub_intf_port_row, idl)
  {
    if (strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
    {
       port_found = true;
    }
  }

  if(!port_found)
  {
    vty_out(vty, "Specified sub interface port doesn't exist.%s",VTY_NEWLINE);
    return CMD_SUCCESS;
  }

  /* Check if the given sub interface port is part of VRF */
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
  {
    for (k = 0; k < vrf_row->n_ports; k++)
    {
       sub_intf_port_row = vrf_row->ports[k];
       if(strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
       {
          sub_intf_to_vrf = true;
          goto port_attached;
       }
    }
  }

   /* Check if the given sub interface port is part of bridge */
  OVSREC_BRIDGE_FOR_EACH (bridge_row, idl)
  {
    for (k = 0; k < bridge_row->n_ports; k++)
    {
       sub_intf_port_row = bridge_row->ports[k];
       if(strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
       {
          sub_intf_to_bridge = true;
          goto port_attached;
       }
    }
  }

port_attached:
  if(sub_intf_to_vrf || sub_intf_to_bridge)
  {
    /* sub interface port is attached to either VRF or bridge.
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
    /* assert if the sub interface port is not part of either VRF or bridge */
    assert(0);
    VLOG_ERR("Port table entry not found either in VRF or in bridge.Function=%s, Line=%d", __func__,__LINE__);
    return CMD_OVSDB_FAILURE;
  }

  if(sub_intf_to_vrf)
  {
    /* Delete the sub interface port reference from VRF */
    ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
    for(i = n = 0; i < vrf_row->n_ports; i++)
    {
       if(vrf_row->ports[i] != sub_intf_port_row)
       {
          ports[n++] = vrf_row->ports[i];
       }
    }
    ovsrec_vrf_set_ports(vrf_row, ports, n);
    free(ports);
  }

  ovsrec_port_delete(sub_intf_port_row);

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

/*vlan envapsulation configuration*/
DEFUN  (cli_encapsulation_dot1Q_vlan,
        cli_encapsulation_dot1Q_vlan_cmd,
        "encapsulation dot1Q <1-4094>",
        "Set encapsulation type for an interface\n"
	"IEEE 802.1Q virtual LAN\n"
        "VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (NULL == status_txn)
    {
        VLOG_ERR("Failed to create transaction. Function:%s, Line:%d", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

   /* port_row = ovsrec_port_first(idl);
    if (NULL == port_row)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN, %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }*/

    if (NULL == vlan_port_row )
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (NULL == vlan_port_row->vlan_mode)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The interface is in access mode.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }

    int64_t* trunks = NULL;
    trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks + 1));
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        trunks[i] = vlan_port_row->trunks[i];
    }
    trunks[vlan_port_row->n_trunks] = vlan_id;
    int trunk_count = vlan_port_row->n_trunks + 1;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);

    status = cli_do_config_finish(status_txn);
    free(trunks);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN  (cli_no_encapsulation_dot1Q_vlan,
        cli_no_encapsulation_dot1Q_vlan_cmd,
        "no encapsulation dot1Q <1-4094>",
         NO_STR 
        "Set encapsulation type for an interface\n"
        "IEEE 802.1Q virtual LAN\n"
        "VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, n = 0;

    if (NULL == status_txn)
    {
        VLOG_ERR("Failed to create transaction. Function:%s, Line:%d", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, "Failed to remove trunk VLAN%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

  /*  port_row = ovsrec_port_first(idl);
    if (NULL == port_row)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }*/

    if (NULL == vlan_port_row)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        vty_out(vty, "The interface is not in trunk mode.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    int64_t* trunks = NULL;
    int trunk_count = vlan_port_row->n_trunks;
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        if (vlan_id == vlan_port_row->trunks[i])
        {
            trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks - 1));
            for (i = n = 0; i < vlan_port_row->n_trunks; i++)
            {
                if (vlan_id != vlan_port_row->trunks[i])
                {
                    trunks[n++] = vlan_port_row->trunks[i];
                }
            }
            trunk_count = vlan_port_row->n_trunks - 1;
            ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
            break;
        }
    }

    if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) == 0)
    {
        if (0 == trunk_count)
        {
            ovsrec_port_set_vlan_mode(vlan_port_row, NULL);
        }
    }

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove trunk VLAN. Function:%s, Line:%d", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
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



DEFUN (vtysh_sub_interface,
       vtysh_sub_interface_cmd,
       "interface A.B",
      "Select a sub-interface to configure\n"
      "Sub Interface phy_if.sub_intf name\n")
{
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_interface *intf_row;
  struct ovsrec_interface **iface_list;
  bool port_found = false;
  bool intf_found = false;
  struct ovsdb_idl_txn *txn = NULL;
  enum ovsdb_idl_txn_status status_txn;
  static char ifnumber[SUB_INTF_NAME_LENGTH]={0};
  const struct ovsrec_vrf *default_vrf_row = NULL;
  const struct ovsrec_vrf *vrf_row = NULL;
  int i=0;
  struct ovsrec_port **ports = NULL;
  char phy_intf[SUB_INTERFACE_NAME_LENGTH];
  char sub_intf[SUB_INTERFACE_NAME_LENGTH];
  int phy_intf_number;
  int sub_intf_number;


  if (strlen(argv[0]) < 10)
    memcpy(ifnumber, argv[0], strlen(argv));
  vty->node = SUB_INTERFACE_NODE;

  phy_intf_number = atof(argv[0]);
  sub_intf_number = atoi(strchr(argv[0],'.') + 1);

  if((phy_intf_number < MIN_PHY_INTF) || (phy_intf_number > MAX_PHY_INTF)){
    fprintf (stdout, "Physical Interface Range <%d %d>\n", MIN_PHY_INTF, MAX_PHY_INTF);
    return CMD_ERR_NOTHING_TODO;
  }

  if((sub_intf_number < MIN_SUB_INTF) || (sub_intf_number > MAX_SUB_INTF)){
    fprintf (stdout, "Sub Interface Range <%d %d>\n", MIN_SUB_INTF, MAX_SUB_INTF);
    return CMD_ERR_NOTHING_TODO;
  }
  memcpy(phy_intf, argv[0], strlen(argv[0]) - strlen(strchr(argv[0],'.')));
  memcpy(sub_intf, strchr(argv[0],'.') + 1, strlen(strchr(argv[0],'.') + 1));

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
    ovsrec_interface_set_type(intf_row, OVSREC_INTERFACE_TYPE_VLANSUBINT);

    /*Create port table entry*/
    port_row = ovsrec_port_insert(txn);
    ovsrec_port_set_name(port_row, ifnumber);

    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
            default_vrf_row = vrf_row;
            break;
        }
    }

    /* Adding a port to the corresponding interface*/
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
      vty->index = ifnumber;
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
    vty->index = ifnumber;
    return CMD_SUCCESS;
  }

  return CMD_SUCCESS;
}

DEFUN_NO_FORM (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n");

DEFUN (cli_sub_intf_config_ipv4,
    cli_sub_intf_config_ipv4_cmd,
    "ip address A.B.C.D/M {secondary}",
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return sub_intf_config_ip((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_sub_intf_del_ipv4,
    cli_sub_intf_del_ipv4_cmd,
    "no ip address A.B.C.D/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return sub_intf_del_ipv4((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_sub_intf_config_ipv6,
    cli_sub_intf_config_ipv6_cmd,
    "ipv6 address X:X::X:X/M {secondary}",
    IPV6_STR
    "Set IP address\n"
    "Interface IPv6 address\n"
    "Set as secondary IPv6 address\n")
{
  return sub_intf_config_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_sub_intf_del_ipv6,
    cli_sub_intf_del_ipv6_cmd,
    "no ipv6 address X:X::X:X/M {secondary}",
    NO_STR
    IP_STR
    "Set IP address\n"
    "Interface IP address\n"
    "Set as secondary IP address\n")
{
  return sub_intf_del_ipv6((char*) vty->index, argv[0],
      (argv[1] != NULL) ? true : false);
}

DEFUN (cli_del_sub_intf,
    cli_del_sub_intf_cmd,
    "no interface A.B",
    "Delete sub_interface L3_intf.sub_intf\n"
    "Remove sub-interface entry from db\n")
{
  return delete_sub_intf(argv[0]);
}


/* Install sub interface related vty commands. */
void
sub_intf_vty_init (void)
{
  install_element (CONFIG_NODE,		&vtysh_sub_interface_cmd);
  install_element (CONFIG_NODE, 	&cli_del_sub_intf_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ipv4_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ipv4_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ipv6_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ipv6_cmd);
  install_element (SUB_INTERFACE_NODE, &cli_sub_intf_shutdown_cmd);
  install_element (SUB_INTERFACE_NODE, &no_cli_sub_intf_shutdown_cmd);

}

void encapsulation_vty_init(void) {
    install_element (SUB_INTERFACE_NODE,&cli_encapsulation_dot1Q_vlan_cmd);
    install_element (SUB_INTERFACE_NODE,&cli_no_encapsulation_dot1Q_vlan_cmd);

}
