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
 * @file vtysh_ovsdb_ecmp_context.c
 * Source for registering client callback with management interface context.
 *
 ***************************************************************************/

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_ecmp_context.h"
#include "ecmp_vty.h"

char ecmpcontextclientname[] = "vtysh_ecmp_context_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ecmp_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private : void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ecmp_context_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_open_vswitch *ovs_row;

    ovs_row = ovsrec_open_vswitch_first(p_msg->idl);
    if(!ovs_row)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_INFO,
                            "vtysh_ecmp_context_clientcallback: error ovs_row");
        return e_vtysh_error;
    }

    if(!GET_ECMP_CONFIG_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp disable");
    }
    if(!GET_ECMP_CONFIG_HASH_SRC_IP_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance src-ip disable");
    }
    if(!GET_ECMP_CONFIG_HASH_SRC_PORT_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance src-port disable");
    }
    if (!GET_ECMP_CONFIG_HASH_DST_IP_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance dst-ip disable");
    }
    if(!GET_ECMP_CONFIG_HASH_DST_PORT_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance dst-port disable");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_ecmp_context_clients
| Responsibility : Registers the client callback routines for ecmp context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_ecmp_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = ecmpcontextclientname;
  client.client_id = e_vtysh_ecmp_context_config;
  client.p_callback = &vtysh_ecmp_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_ecmp_context, e_vtysh_ecmp_context_config, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "ecmp context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
