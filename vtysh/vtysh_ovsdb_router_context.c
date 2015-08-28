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
/*vtysh_ret_val
vtysh_router_context_bgp_clientcallback(void *p_private)
{
  /* HALON-TODO /
  return e_vtysh_ok;
}*/


/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_neighbor_callback
| Responsibility : Neighbor commands
| Parameters :
|     vtysh_ovsdb_cbmsg_ptr p_msg: struct vtysh_ovsdb_cbmsg_struct *
| Return : void
-----------------------------------------------------------------------------*/

void vtysh_router_context_bgp_neighbor_callback(vtysh_ovsdb_cbmsg_ptr p_msg)
{
   struct ovsrec_bgp_neighbor *ovs_bgp_neighbor=NULL;
   int i=0;

    OVSREC_BGP_NEIGHBOR_FOR_EACH(ovs_bgp_neighbor, p_msg->idl)
    {
       vtysh_ovsdb_cli_print(p_msg,"    neighbor %s remote-as %d",ovs_bgp_neighbor->name,*(ovs_bgp_neighbor->remote_as));

       if(ovs_bgp_neighbor->description)
         vtysh_ovsdb_cli_print(p_msg,"    neighbor %s description %s",ovs_bgp_neighbor->name,ovs_bgp_neighbor->description);

       if(ovs_bgp_neighbor->password)
         vtysh_ovsdb_cli_print(p_msg,"    neighbor %s password %s",ovs_bgp_neighbor->name,ovs_bgp_neighbor->password);

       if(ovs_bgp_neighbor->n_timers > 0)
         vtysh_ovsdb_cli_print(p_msg,"    neighbor %s timers %d %d",ovs_bgp_neighbor->name,ovs_bgp_neighbor->value_timers[0],ovs_bgp_neighbor->value_timers[1]);

       i=0;
       while(i< ovs_bgp_neighbor->n_route_maps)
       {
          vtysh_ovsdb_cli_print(p_msg,"    neighbor %s route-map %s %s",ovs_bgp_neighbor->name,ovs_bgp_neighbor->value_route_maps[i]->name,ovs_bgp_neighbor->key_route_maps[i]);
          i++;
       }

       if(ovs_bgp_neighbor->n_allow_as_in)
         vtysh_ovsdb_cli_print(p_msg,"    neighbor %s allowas-in %d",ovs_bgp_neighbor->name,*(ovs_bgp_neighbor->allow_as_in));

       if(ovs_bgp_neighbor->n_remove_private_as)
         vtysh_ovsdb_cli_print(p_msg,"    neighbor %s remove-private-AS",ovs_bgp_neighbor->name);
    }
}


/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_clientcallback
| Responsibility : client callback routine
| Parameters :
     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_bgp_clientcallback(void *p_private)
{
   struct ovsrec_bgp_router *bgp_router_context=NULL;
   int i=0;

   vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

   vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_context_router_bgp_clientcallback entered");


   OVSREC_BGP_ROUTER_FOR_EACH(bgp_router_context,p_msg->idl)
   {
      vtysh_ovsdb_cli_print(p_msg,"%s %d","router bgp ",bgp_router_context->asn);

      if(bgp_router_context->router_id)
        vtysh_ovsdb_cli_print(p_msg,"%s %s","    bgp router-id ",bgp_router_context->router_id);

      while(i < bgp_router_context->n_networks)
      {
        vtysh_ovsdb_cli_print(p_msg,"%s %s","    network ",bgp_router_context->networks[i]);
        i++;
      }

      if(bgp_router_context->n_maximum_paths)
        vtysh_ovsdb_cli_print(p_msg,"    maximum-paths %d",*(bgp_router_context->maximum_paths));
   }

    vtysh_router_context_bgp_neighbor_callback(p_msg);

    vtysh_ovsdb_cli_print(p_msg,"!");

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
