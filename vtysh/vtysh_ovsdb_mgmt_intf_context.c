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
 * @file vtysh_ovsdb_mgmt_intf_context.c
 * Source for registering client callback with management interface context.
 *
 ***************************************************************************/

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_mgmt_intf_context.h"

char mgmtintfcontextclientname[] = "vtysh_mgmt_intf_context_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_mgmt_intf_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private : void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_mgmt_intf_context_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const char *data = NULL;
    const char *ip = NULL;
    const char *subnet = NULL;
    const struct ovsrec_open_vswitch *vswrow;
    vswrow = ovsrec_open_vswitch_first(p_msg->idl);
    if(!vswrow)
    {
        return e_vtysh_error;
    }

    data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if (data)
    {
        if (VTYSH_STR_EQ(data, OPEN_VSWITCH_MGMT_INTF_MAP_MODE_STATIC))
        {
            vtysh_ovsdb_cli_print(p_msg, "interface mgmt");
            ip = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_IP);
            subnet = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
            if (ip && subnet )
                vtysh_ovsdb_cli_print(p_msg, "%4sip static %s %s","",ip,subnet);
        }
        else
            return;
    }
    data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if (data)
    {
        vtysh_ovsdb_cli_print(p_msg, "%4sdefault-gateway %s","",data);
    }
    data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if (data)
    {
        vtysh_ovsdb_cli_print(p_msg, "%4snameserver1 %s","", data);
    }
    data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (data)
    {
        vtysh_ovsdb_cli_print(p_msg, "%4snameserver2 %s","", data);
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_mgmt_intf_context_clients
| Responsibility : Registers the client callback routines for mgmt interface context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_mgmt_intf_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = mgmtintfcontextclientname;
  client.client_id = e_vtysh_mgmt_interface_context_config;
  client.p_callback = &vtysh_mgmt_intf_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_mgmt_interface_context, e_vtysh_mgmt_interface_context_config, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "Mgmt interface context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
