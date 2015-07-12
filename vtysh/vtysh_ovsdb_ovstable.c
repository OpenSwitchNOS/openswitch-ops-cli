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
 * @file vtysh_ovsdb_ovstable.c
 * Source for registering client callback with openvswitch table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_ovstable.h"

char clientname[] = "vtysh_ovsdb_ovstable_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_othercfg
| Responsibility : parse other_config in open_vswitch table
| Parameters :
|    ifrow_config : other_config object pointer
|    fp : file pointer
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_othercfg(const struct smap *ifrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL;
  int hold_time = 0, transmit_interval = 0;

  if(NULL == ifrow_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "true"))
    {
      vtysh_ovsdb_cli_print(p_msg, "feature lldp");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD );
  if (data)
  {
    hold_time = atoi(data);
    if ( OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT != hold_time)
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp holdtime %d", hold_time);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL);
  if (data)
  {
    transmit_interval = atoi(data);
    if ( OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT != hold_time)
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp transmit-time %d", transmit_interval);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv management_address");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port_description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv Port-vlan_id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system_capabilites");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system_description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system_name");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR);
  if (data)
  {
    vtysh_ovsdb_cli_print(p_msg, "lldp management-address %s", data);
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_ovstable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  const struct ovsrec_open_vswitch *vswrow;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_ovstable_clientcallback entered");
  vswrow = ovsrec_open_vswitch_first(p_msg->idl);

  if(vswrow)
  {
    if (vswrow->hostname[0] != '\0')
    {
      vtysh_ovsdb_cli_print(p_msg, "hostname \"%s\"", vswrow->hostname);
    }

    /* parse other config param */
    vtysh_ovsdb_ovstable_parse_othercfg(&vswrow->other_config, p_msg);
  }
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_init_ovstableclients
| Responsibility : registers the client callbacks for ovstable
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_ovsdb_init_ovstableclients()
{
  vtysh_ovsdb_client client;

  client.p_client_name = clientname;
  client.client_id = e_vtysh_open_vswitch_table_config;
  client.p_callback = &vtysh_ovsdb_ovstable_clientcallback;

  vtysh_ovsdbtable_addclient(e_open_vswitch_table, e_vtysh_open_vswitch_table_config, &client);

  return e_vtysh_ok;
}
