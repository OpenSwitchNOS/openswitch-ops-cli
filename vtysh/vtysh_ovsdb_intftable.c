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
 * @file vtysh_ovsdb_intftable.c
 * Source for registering client callback with interface table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_intftable.h"

typedef enum vtysh_ovsdb_intf_lldptxrx_enum
{
  e_lldp_dir_tx = 0,
  e_lldp_dir_rx,
  e_lldp_dir_tx_rx,
  e_lldp_dir_off,
} vtysh_ovsdb_intf_lldptxrx;

typedef struct vtysh_ovsdb_intf_cfg_struct
{
  bool disp_intf_cfg;
  bool admin_state;
  vtysh_ovsdb_intf_lldptxrx lldptxrx_state;
} vtysh_ovsdb_intf_cfg;

char intfclientname[] = "vtysh_ovsdb_intftable_clientcallback";
/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_parse_othercfg
| Responsibility : parse other_config in intf table
| Parameters :
|    ifrow_config : other_config object pointer
|    p_msg: pointer to object type vtysh_ovsdb_intf_cfg
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_intftable_parse_othercfg(const struct smap *ifrow_config, vtysh_ovsdb_intf_cfg *p_msg)
{
  const char *data = NULL;

  if(NULL == ifrow_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_config, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR);
  if (data)
  {
    if (VTYSH_STR_EQ(data, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_TX))
    {
      p_msg->lldptxrx_state = e_lldp_dir_tx;
      p_msg->disp_intf_cfg = true;
    }
    else if (VTYSH_STR_EQ(data, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX))
    {
      p_msg->lldptxrx_state = e_lldp_dir_rx;
      p_msg->disp_intf_cfg = true;
    }
    else if (VTYSH_STR_EQ(data, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF))
    {
      p_msg->lldptxrx_state = e_lldp_dir_off;
      p_msg->disp_intf_cfg = true;
    }
    else
    {
      p_msg->lldptxrx_state = e_lldp_dir_tx_rx;
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : intfd_get_user_cfg_adminstate
| Responsibility : get teh admin state form user_config column in specific row
| Parameters :
|   const struct smap *ifrow_config : specific row config
| Return : bool *adminstate : returns the admin state
-----------------------------------------------------------------------------*/
static void
intfd_get_user_cfg_adminstate(const struct smap *ifrow_config,
                              bool *adminstate)
{
  const char *data = NULL;

  data = smap_get(ifrow_config, INTERFACE_USER_CONFIG_MAP_ADMIN);

  if (data && (VTYSH_STR_EQ(data, OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP)))
  {
    *adminstate = true;
  }
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private1, *p_private2: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_intftable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_interface *ifrow;
  bool adminstate = false;


  OVSREC_INTERFACE_FOR_EACH(ifrow, p_msg->idl)
  {
    vtysh_ovsdb_intf_cfg intfcfg;

    /* set to default values */
    intfcfg.admin_state = false;
    intfcfg.disp_intf_cfg = false;
    intfcfg.lldptxrx_state = e_lldp_dir_tx_rx;
    if (ifrow)
    {
      adminstate = false;
      intfd_get_user_cfg_adminstate(&ifrow->user_config, &adminstate);
      if (true == adminstate)
      {
        intfcfg.admin_state = true;
        intfcfg.disp_intf_cfg = true;
      }

      /* parse interface other config */
      vtysh_ovsdb_intftable_parse_othercfg(& ifrow->other_config, &intfcfg);

      /* display interface config */
      if (true == intfcfg.disp_intf_cfg)
      {
        vtysh_ovsdb_cli_print(p_msg, "interface %s", ifrow->name);
        if (e_lldp_dir_tx == intfcfg.lldptxrx_state)
        {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp reception");
        }
        else if (e_lldp_dir_rx == intfcfg.lldptxrx_state)
        {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp transmission");
        }
        else if (e_lldp_dir_off == intfcfg.lldptxrx_state)
        {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp transmission");
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp reception");
        }

        if (true == intfcfg.admin_state)
        {
          vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no shutdown");
        }
      }
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_init_intftableclients
| Responsibility : Registers the client callback routines for interface table
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_ovsdb_init_intftableclients()
{
  vtysh_ovsdb_client client;

  client.p_client_name = intfclientname;
  client.client_id = e_vtysh_interface_table_config;
  client.p_callback = &vtysh_ovsdb_intftable_clientcallback;

  vtysh_ovsdbtable_addclient(e_interface_table, e_vtysh_interface_table_config, &client);
}
