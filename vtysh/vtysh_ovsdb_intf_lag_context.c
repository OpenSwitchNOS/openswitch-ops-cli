/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
*/
/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_intf_lag_context.c
 * Source for registering client callback with interface lag context.
 *
 ***************************************************************************/

#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_intf_context.h"
#include "vtysh_ovsdb_mgmt_intf_context.h"
#include "lacp_vty.h"
#include "vrf_vty.h"

char intflagcontextclientname[] = "vtysh_intf_lag_context_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_parse_vlan
| Responsibility : Used for VLAN related config
| Parameters :
|     const char *if_name           : Name of interface
|     vtysh_ovsdb_cbmsg_ptr p_msg   : Used for idl operations
|     bool interfaceNameWritten     : Check if "interface x" has already been
|                                     written
| Return : vtysh_ret_val
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_porttable_parse_vlan(const char *if_name,
                                 vtysh_ovsdb_cbmsg_ptr p_msg)
{
    const struct ovsrec_port *port_row;
    int i;

    port_row = port_lookup(if_name, p_msg->idl);
    if (port_row == NULL)
    {
        return e_vtysh_ok;
    }

    if (port_row->vlan_mode == NULL)
    {
        return e_vtysh_ok;
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan access ",
            *port_row->tag);
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) == 0)
    {
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
        }
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) == 0)
    {
        if (port_row->n_tag == 1)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk native ",
                *port_row->tag);
        }
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
        }
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) == 0)
    {
        if (port_row->n_tag == 1)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk native ",
                *port_row->tag);
        }
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "vlan trunk native tag");
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
        }
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_intf_lag_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private : void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_intf_lag_context_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const char *data = NULL;
  const struct ovsrec_port *port_row = NULL;

  OVSREC_PORT_FOR_EACH(port_row, p_msg->idl)
  {
    if(strncmp(port_row->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
    {
      /* Print the LAG port name because lag port is present. */
      vtysh_ovsdb_cli_print(p_msg, "interface lag %d", atoi(&port_row->name[LAG_PORT_NAME_PREFIX_LENGTH]));
      data = smap_get(&port_row->other_config, "lacp");
      if (check_port_in_bridge(port_row->name))
      {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no routing");
          vtysh_ovsdb_porttable_parse_vlan(port_row->name, p_msg);
      }
      if(data)
      {
        vtysh_ovsdb_cli_print(p_msg, "%4slacp mode %s"," ",data);
      }
      data = NULL;
      data = smap_get(&port_row->other_config, "bond_mode");
      if(data)
      {
        vtysh_ovsdb_cli_print(p_msg, "%4shash %s"," ",data);
      }
      data = NULL;
      data = smap_get(&port_row->other_config, "lacp-fallback-ab");
      if(data)
      {
        if (VTYSH_STR_EQ(data, "true"))
        {
          vtysh_ovsdb_cli_print(p_msg, "%4slacp fallback"," ");
        }
      }
      data = NULL;
      data = smap_get(&port_row->other_config, "lacp-time");
      if(data)
      {
        vtysh_ovsdb_cli_print(p_msg, "%4slacp rate %s"," ",data);
      }

    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_intf_lag_context_clients
| Responsibility : Registers the client callback routines for interface lag context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_intf_lag_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = intflagcontextclientname;
  client.client_id = e_vtysh_interface_lag_context_config;
  client.p_callback = &vtysh_intf_lag_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_interface_lag_context, e_vtysh_interface_lag_context_config, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "Interface LAG context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
