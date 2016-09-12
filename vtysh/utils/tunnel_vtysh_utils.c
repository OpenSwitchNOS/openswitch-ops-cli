/* Tunnel VTYSH utils
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tunnel_vtysh_utils.h"
#include "vswitch-idl.h"
#include "command.h"
#include "openvswitch/vlog.h"
#include "ovsdb-idl.h"
#include "vtysh.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh_user.h"

VLOG_DEFINE_THIS_MODULE(tunnel_vtysh_utils);

extern struct ovsdb_idl *idl;

/* Helper functions */
/*
const struct ovsrec_logical_switch *
get_matching_logical_switch(int64_t tunnel_key)
{
    const struct ovsrec_logical_switch *ls_row = NULL;
    OVSREC_LOGICAL_SWITCH_FOR_EACH(ls_row, idl)
    {
        if (ls_row->tunnel_key == tunnel_key)
            return ls_row;
    }
    return NULL;
}

const struct ovsrec_vlan *
get_matching_vlan(char *tunnel_name)
{
    const struct ovsrec_vlan *vlan_row = NULL;
    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (strcmp(vlan_row->name, tunnel_name) == 0)
            return vlan_row;
    }
    return NULL;
}
*/

char *
get_mode_from_line(const char line[])
{
  unsigned int count = 0, i = 0, j = 0, indx = 0;
  char *temp = malloc(sizeof(strlen(line)));

  for(i=0; i<strlen(line); i++)
  {
    if(line[i]==0x20)
      count += 1;

    if(count >= 3)
    {
      for(j=i+1; j<strlen(line); j++)
      {
        indx = j-i-1;
        temp[indx] = line[j];
      }
      temp[j-i] = '\0';
      break;
    }
  }
  return temp;
}

const struct ovsrec_interface *
get_intf_by_name(const char *name)
{
    const struct ovsrec_interface *if_row = NULL;
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if (!strcmp(if_row->name, name))
        {
            return if_row;
        }
    }

    return NULL;
}

bool
is_tunnel_intf_type(const struct ovsrec_interface *if_row,
                    const int intf_type)
{
    char *intf_type_str = NULL;

    switch(intf_type)
    {
        case INTERFACE_TYPE_VXLAN:
            intf_type_str = OVSREC_INTERFACE_TYPE_VXLAN;
            break;
        case INTERFACE_TYPE_GRE_IPV4:
            intf_type_str = OVSREC_INTERFACE_TYPE_GRE_IPV4;
            break;
        default:
            break;
    }

    return intf_type_str && !strcmp(if_row->type, intf_type_str);
}

const struct ovsrec_interface *
get_intf_by_name_and_type(const char *name,
                          int intf_type)
{
    const struct ovsrec_interface *intf_row = get_intf_by_name(name);
    return intf_row &&
           is_tunnel_intf_type(intf_row, intf_type) ? intf_row : NULL;
}

const struct ovsrec_port *
get_port_by_name(const char *name)
{
    const struct ovsrec_port *port_row = NULL;
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, name) == 0)
            return port_row;
    }
    return NULL;
}

int
txn_status_and_log(const int txn_status)
{
    int status = CMD_OVSDB_FAILURE;
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
        status = CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    }

    return status;
}

int
set_intf_tunnel_ip_addr_by_type(struct vty *vty,
                                const char *tunnel_name,
                                const int intf_type,
                                const char *new_ip)
{
    const struct ovsrec_interface *if_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    enum ovsdb_idl_txn_status txn_status;

    if_row = get_intf_by_name_and_type(tunnel_name, intf_type);
    if (!if_row)
    {
        vty_out(vty, "Invalid tunnel interface %s%s",
                tunnel_name, VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    // Set the IP address for the interface
    port_row = get_port_by_name(tunnel_name);
    if (!port_row)
    {
        vty_out(vty, "Port %s not found.%s",
                (char*)vty->index, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    struct ovsdb_idl_txn *txn = cli_do_config_start();
    if (txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_port_set_ip4_address(port_row, new_ip);
    txn_status = cli_do_config_finish(txn);

    return txn_status_and_log(txn_status);
}

int
set_intf_src_ip(struct vty *vty, const struct ovsrec_interface *if_row,
                const char *new_ip)
{
    const char *src_if = NULL;

    // Check if IP is supposed to be set by configured interface
    src_if = smap_get(&if_row->options,
                      OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_INTF);

    if (src_if)
    {
        vty_out(vty, "Source Interface IP %s is already set %s",
                src_if, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_IP,
                           new_ip);
}

int
unset_intf_src_ip(const struct ovsrec_interface *if_row)
{
    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_IP,
                           NULL);
}

int
set_intf_dest_ip(const struct ovsrec_interface *if_row, const char *new_ip)
{
    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_REMOTE_IP, new_ip);
}

int
unset_intf_dest_ip(const struct ovsrec_interface *if_row)
{
    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_REMOTE_IP, NULL);
}

int
set_src_intf(struct vty *vty, const struct ovsrec_interface *if_row,
             const char *new_if)
{
    const char *src_ip = NULL;

    // Check if IP is already configured
    src_ip = smap_get(&if_row->options,
                      OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_IP);

    if (src_ip)
    {
        vty_out(vty, "Source IP %s is already set %s", src_ip, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_INTF,
                           new_if);
}

int
unset_src_intf(const struct ovsrec_interface *if_row)
{
    return set_intf_option(if_row, OVSREC_INTERFACE_OPTIONS_TUNNEL_SOURCE_INTF,
                           NULL);
}

int
set_intf_option(const struct ovsrec_interface *if_row, const char *option,
                const char *new_value)
{
    enum ovsdb_idl_txn_status txn_status;

    // Update the option value if it is different
    const char *curr_value = smap_get(&if_row->options, option);

    // Value exists and is the same, or if it is already unset and trying to
    // unset again then skip configuration
    if ((!curr_value && !new_value) ||
        (curr_value && new_value && !strcmp(new_value, curr_value)))
    {
        VLOG_DBG("Skip configuration since option values are identical.");
        return CMD_SUCCESS;
    }

    struct ovsdb_idl_txn *txn = cli_do_config_start();
    if (txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
    }

    // Clone any existing options and update value for the option
    struct smap if_options;
    smap_clone(&if_options, &if_row->options);

    new_value ? smap_replace(&if_options, option, new_value) :
                smap_remove(&if_options, option);

    ovsrec_interface_set_options(if_row, &if_options);

    txn_status = cli_do_config_finish(txn);
    smap_destroy(&if_options);

    return txn_status_and_log(txn_status);
}
