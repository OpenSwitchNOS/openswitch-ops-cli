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
 * @file vtysh_ovsdb_intf_context.c
 * Source for registering client callback with interface context.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_intf_context.h"
#include "lacp_vty.h"

#define PRINT_INTERFACE_NAME(name_written, p_msg, if_name)\
  if (!(name_written))\
  {\
    vtysh_ovsdb_cli_print(p_msg, "interface %s", if_name);\
    name_written = true;\
  }


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

char intfcontextclientname[] = "vtysh_intf_context_clientcallback";
static vtysh_ret_val
vtysh_ovsdb_intftable_parse_l3config(const char *if_name,
                                     vtysh_ovsdb_cbmsg_ptr p_msg,
                                     bool interfaceNameWritten);

/*-----------------------------------------------------------------------------
| Function : port_vrf_match
| Responsibility : Lookup VRF to which interface is connected
| Parameters :
|    const struct ovsdb_idl *idl : idl for vtysh
|    const struct ovsrec_port *port_row: pointer to port_row for looking up VRF
| Return : pointer to VRF row
-----------------------------------------------------------------------------*/
struct ovsrec_vrf* port_vrf_match(const struct ovsdb_idl *idl,
                                  const struct ovsrec_port *port_row)
{
    struct ovsrec_vrf *vrf_row = NULL;
    size_t i;
    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
      for (i = 0; i < vrf_row->n_ports; i++) {
        if (vrf_row->ports[i] == port_row) {
          return vrf_row;
        }
      }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function : port_lookup
| Responsibility : Lookup port table entry for interface name
| Parameters :
|   const char *if_name : Interface name
|   const struct ovsdb_idl *idl : IDL for vtysh
| Return : bool : returns true/false
-----------------------------------------------------------------------------*/
struct ovsrec_port* port_lookup(const char *if_name,
                                const struct ovsdb_idl *idl)
{
    struct ovsrec_port *port_row = NULL;
    size_t i;
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
      if (strcmp(port_row->name, if_name) == 0) {
        return port_row;
      }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_parse_lacp_othercfg
| Responsibility : parse other_config in intf table to display lacp config
| Parameters :
|              ifrow_config : other_config object pointer
|                     p_msg : vtysh_ovsdb_cbmsg_ptr data
|                  intf_cfg : pointer of type vtysh_ovsdb_intf_cfg
|                   if_name : interface name
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_intftable_parse_lacp_othercfg(const struct smap *ifrow_config,
                                          vtysh_ovsdb_cbmsg_ptr p_msg,
                                          vtysh_ovsdb_intf_cfg *intf_cfg,
                                          const char *if_name)
{
  const char *data = NULL;

  data = smap_get(ifrow_config, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_ID);
  if (data)
  {
    PRINT_INTERFACE_NAME(intf_cfg->disp_intf_cfg, p_msg, if_name)
    vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "lacp port-id", atoi(data));
  }
  data = NULL;
  data = smap_get(ifrow_config, INTERFACE_OTHER_CONFIG_MAP_LACP_PORT_PRIORITY);
  if (data)
  {
    PRINT_INTERFACE_NAME(intf_cfg->disp_intf_cfg, p_msg, if_name)
    vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "lacp port-priority", atoi(data));
  }
  data = NULL;
  data = smap_get(ifrow_config, INTERFACE_OTHER_CONFIG_MAP_LACP_AGGREGATION_KEY);
  if (data)
  {
    PRINT_INTERFACE_NAME(intf_cfg->disp_intf_cfg, p_msg, if_name)
    vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "lacp aggregation-key", atoi(data));
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_print_lag
| Responsibility : print lag information of an interface
| Parameters :
|                     p_msg : vtysh_ovsdb_cbmsg_ptr data
|                  intf_cfg : pointer of type vtysh_ovsdb_intf_cfg
|                   if_name : interface name
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_intftable_print_lag(vtysh_ovsdb_cbmsg_ptr p_msg,
                                vtysh_ovsdb_intf_cfg *intf_cfg,
                                const char *if_name)
{
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_interface *if_row = NULL;
  int k=0;

   OVSREC_PORT_FOR_EACH(port_row, p_msg->idl)
   {
     if (strncmp(port_row->name, LAG_PORT_NAME_PREFIX, LAG_PORT_NAME_PREFIX_LENGTH) == 0)
     {
        for (k = 0; k < port_row->n_interfaces; k++)
        {
           if_row = port_row->interfaces[k];
           if(strcmp(if_name, if_row->name) == 0)
           {
             PRINT_INTERFACE_NAME(intf_cfg->disp_intf_cfg, p_msg, if_name)
             vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", " ", "lag", atoi(&port_row->name[LAG_PORT_NAME_PREFIX_LENGTH]));
           }
        }
     }
   }

  return e_vtysh_ok;
}

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
| Function : display_l3_info
| Responsibility : Decide if L3 info needs to be printed
| Parameters :
|   const struct ovsrec_interface *if_row : Interface row data
|   const struct ovsrec_vrf *vrf_row : VRF row data
| Return : bool : returns true/false
-----------------------------------------------------------------------------*/
bool
display_l3_info(const struct ovsrec_port *port_row,
                const struct ovsrec_vrf *vrf_row)
{
   if (port_row->ip4_address || (port_row->n_ip4_address_secondary > 0)
        || port_row->ip6_address || (port_row->n_ip6_address_secondary > 0)
        || (strcmp(vrf_row->name, DEFAULT_VRF_NAME) != 0)) {
     return true;
   }
   return false;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_intf_context_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private1, *p_private2: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_intf_context_clientcallback(void *p_private)
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
      vtysh_ovsdb_intftable_parse_lacp_othercfg(&ifrow->other_config, p_msg,
                                                &intfcfg, ifrow->name);
      vtysh_ovsdb_intftable_print_lag(p_msg, &intfcfg, ifrow->name);
      vtysh_ovsdb_intftable_parse_l3config(ifrow->name, p_msg, intfcfg.disp_intf_cfg);
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_parse_l3config
| Responsibility : Used for VRF related config
| Parameters :
|     const char *if_name           : Name of interface
|     vtysh_ovsdb_cbmsg_ptr p_msg   : Used for idl operations
|     bool interfaceNameWritten     : Check if "interface x" has already been
|                                     written
| Return : vtysh_ret_val
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_intftable_parse_l3config(const char *if_name,
                                     vtysh_ovsdb_cbmsg_ptr p_msg,
                                     bool interfaceNameWritten)
{
  struct ovsrec_port *port_row;
  struct ovsrec_vrf *vrf_row;
  bool displayL3Info = false;
  size_t i;

  port_row = port_lookup(if_name, p_msg->idl);
  if (!port_row) {
    return e_vtysh_ok;
  }
  if (check_iface_in_bridge(if_name)) {
    if (!interfaceNameWritten) {
      vtysh_ovsdb_cli_print(p_msg, "interface %s", if_name);
    }
    vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no routing");
  }
  if (check_iface_in_vrf(if_name)) {
    vrf_row = port_vrf_match(p_msg->idl, port_row);
    if (display_l3_info(port_row, vrf_row)) {
      if (!interfaceNameWritten) {
        vtysh_ovsdb_cli_print(p_msg, "interface %s", if_name);
      }
      if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) != 0) {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "vrf attach ", vrf_row->name);
      }
      if (port_row->ip4_address) {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "ip address ", port_row->ip4_address);
      }
      for (i = 0; i < port_row->n_ip4_address_secondary; i++) {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%s%s", "", "ip address ",
                port_row->ip4_address_secondary[i], " secondary");
      }
      if (port_row->ip6_address) {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "", "ipv6 address ", port_row->ip6_address);
      }
      for (i = 0; i < port_row->n_ip6_address_secondary; i++) {
        vtysh_ovsdb_cli_print(p_msg, "%4s%s%s%s", "", "ipv6 address ",
                port_row->ip6_address_secondary[i], " secondary");
      }
    }
  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_intf_context_clients
| Responsibility : Registers the client callback routines for interface context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_intf_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = intfcontextclientname;
  client.client_id = e_vtysh_interface_context_config;
  client.p_callback = &vtysh_intf_context_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_interface_context, e_vtysh_interface_context_config, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "interface context unable to add config callback");
    assert(0);
    return retval;
  }
  return e_vtysh_ok;
}
