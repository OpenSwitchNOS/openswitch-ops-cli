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
 * @ingroup cli/vtysh
 *
 * @file vtysh_ovsdb_vrftable.c
 * Source for registering client callback with VRF table.
 *
 ***************************************************************************/

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_vrftable.h"

char vrfclientname[] = "vtysh_ovsdb_vrftable_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_vrftable_clientcallback
| Responsibility : vrf client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_vrftable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_vrf *vrf_row = NULL;
  OVSREC_VRF_FOR_EACH(vrf_row, p_msg->idl){
    if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
      continue;
    }
    vtysh_ovsdb_cli_print(p_msg, "vrf %s", vrf_row->name);
  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_init_vrftableclients
| Responsibility : registers the client callbacks for VRF table
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_ovsdb_init_vrftableclients()
{
  vtysh_ovsdb_client client;

  client.p_client_name = vrfclientname;
  client.client_id = e_vtysh_vrf_table_config;
  client.p_callback = &vtysh_ovsdb_vrftable_clientcallback;

  vtysh_ovsdbtable_addclient(e_vrf_table, e_vtysh_vrf_table_config, &client);

  return e_vtysh_ok;
}
