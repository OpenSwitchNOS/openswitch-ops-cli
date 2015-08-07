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
 * @file vtysh_ovsdb_router_context.c
 * Source for registering client callback with router context.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_router_context.h"

char routercontextbgpclientname[] = "vtysh_router_context_bgp_clientcallback";
char routercontextospfclientname[] = "vtysh_router_context_ospf_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_bgp_clientcallback(void *p_private)
{
  /* HALON-TODO */
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_ospf_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_ospf_clientcallback(void *p_private)
{
  /* HALON-TODO */
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_router_contextclients
| Responsibility : Registers the client callback routines for router context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_router_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = routercontextbgpclientname;
  client.client_id = e_vtysh_router_context_bgp;
  client.p_callback = &vtysh_router_context_bgp_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_router_context, e_vtysh_router_context_bgp, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "router context unable to add bgp callback");
    assert(0);
    return retval;
  }

  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = routercontextospfclientname;
  client.client_id = e_vtysh_router_context_ospf;
  client.p_callback = &vtysh_router_context_ospf_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_router_context, e_vtysh_router_context_ospf, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "router context unable to add ospf callback");
    assert(0);
    return retval;
  }

  return e_vtysh_ok;
}
