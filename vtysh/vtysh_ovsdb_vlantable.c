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
 * @ingroup quagga
 *
 * @file vtysh_ovsdb_vlantable.c
 * Source for registering client callback with vlan table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_vlantable.h"

char vlanclientname[] = "vtysh_ovsdb_vlantable_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_vlantable_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_vlantable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_vlan *ifrow;
  int count = 0;

  OVSREC_VLAN_FOR_EACH(ifrow, p_msg->idl)
  {
    if (ifrow)
    {
      count++;
    }
  }

  /* for testing max vlan temporarily added below statement
     need to cleanup when we implement vlan commands
   */
  if(count > 0)
  {
    vtysh_ovsdb_cli_print(p_msg, "Total vlans retrieved from db %d", count);
  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_init_vlantableclients
| Responsibility : Registers the client callback routines for vlan table
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_ovsdb_init_vlantableclients()
{
  vtysh_ovsdb_client client;

  client.p_client_name = vlanclientname;
  client.client_id = e_vtysh_vlan_table_config;
  client.p_callback = &vtysh_ovsdb_vlantable_clientcallback;

  vtysh_ovsdbtable_addclient(e_vlan_table, e_vtysh_vlan_table_config, &client);
}
