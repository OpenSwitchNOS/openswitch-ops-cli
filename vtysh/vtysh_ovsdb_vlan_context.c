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
 * @file vtysh_ovsdb_vlan_context.c
 * Source for registering client callback with vlan context.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_vlan_context.h"

char vlancontextclientname[] = "vtysh_vlan_context_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_vlan_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_vlan_context_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_vlan *vlan_row;
  int printed_vlan_name = false;

  vlan_row = ovsrec_vlan_first(p_msg->idl);
  if (vlan_row == NULL)
  {
      return e_vtysh_ok;
  }

  OVSREC_VLAN_FOR_EACH(vlan_row, p_msg->idl)
  {
      vtysh_ovsdb_cli_print(p_msg, "%s %d", "vlan", vlan_row->id);

      if (strcmp(vlan_row->admin, OVSREC_VLAN_ADMIN_UP) == 0)
      {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no shutdown");
      }

      if (vlan_row->description != NULL)
      {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "description ", vlan_row->description);
      }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_vlan_contextclients
| Responsibility : Registers the client callback routines for vlancontext
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_vlan_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = vlancontextclientname;
  client.client_id = e_vtysh_vlan_context_config;
  client.p_callback = &vtysh_vlan_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_vlan_context, e_vtysh_vlan_context_config, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "vlan context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
