/*
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_intf_context.c
 * Source for registering client callback with interface context.
 *
 ***************************************************************************/

#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_intf_context.h"
#include "intf_vty.h"
#include "lacp_vty.h"
#include "vrf_vty.h"

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
const struct ovsrec_vrf* port_vrf_match(const struct ovsdb_idl *idl,
                                  const struct ovsrec_port *port_row)
{
    const struct ovsrec_vrf *vrf_row = NULL;
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
const struct ovsrec_port* port_lookup(const char *if_name,
                                const struct ovsdb_idl *idl)
{
    const struct ovsrec_port *port_row = NULL;
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
    if (atoi(data) != LACP_DEFAULT_PORT_PRIORITY) {
       PRINT_INTERFACE_NAME(intf_cfg->disp_intf_cfg, p_msg, if_name)
       vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "lacp port-priority", atoi(data));
    }
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
    }
    else if (VTYSH_STR_EQ(data, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_RX))
    {
      p_msg->lldptxrx_state = e_lldp_dir_rx;
    }
    else if (VTYSH_STR_EQ(data, INTERFACE_OTHER_CONFIG_MAP_LLDP_ENABLE_DIR_OFF))
    {
      p_msg->lldptxrx_state = e_lldp_dir_off;
    }
    else
    {
      p_msg->lldptxrx_state = e_lldp_dir_tx_rx;
    }
  }

  return e_vtysh_ok;
}
#if 0
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
#endif
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
| Function : is_parent_interface_split
| Responsibility : Check if parent interface has been split
| Parameters :
|   const struct ovsrec_interface *parent_iface : Parent Interface row data
|                                                 for the specific child
| Return : bool : returns true/false
-----------------------------------------------------------------------------*/
static bool
is_parent_interface_split(const struct ovsrec_interface *parent_iface)
{
    char *lanes_split_value = NULL;
    bool is_split = false;

    lanes_split_value = smap_get(&parent_iface->user_config,
                               INTERFACE_USER_CONFIG_MAP_LANE_SPLIT);
    if ((lanes_split_value != NULL) &&
        (strcmp(lanes_split_value,
                INTERFACE_USER_CONFIG_MAP_LANE_SPLIT_SPLIT) == 0))
      {
        /* Parent interface is split.
         * Display child interface configurations. */
        is_split = true;
      }
    return is_split;
}


#define PRINT_INT_HEADER_IN_SHOW_RUN if(!intfcfg.disp_intf_cfg) \
{ \
   intfcfg.disp_intf_cfg = true;\
   vtysh_ovsdb_cli_print(p_msg, "interface %s ", ifrow->name);\
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
   const char *cur_state =NULL;
   struct shash sorted_interfaces;
   const struct shash_node **nodes;
   int idx, count;

   shash_init(&sorted_interfaces);

   OVSREC_INTERFACE_FOR_EACH(ifrow, p_msg->idl)
   {
       shash_add(&sorted_interfaces, ifrow->name, (void *)ifrow);
   }

   nodes = sort_interface(&sorted_interfaces);
   count = shash_count(&sorted_interfaces);

   for (idx = 0; idx < count; idx++)
   {
      vtysh_ovsdb_intf_cfg intfcfg;

      /* set to default values */
      intfcfg.admin_state = false;
      intfcfg.disp_intf_cfg = false;
      intfcfg.lldptxrx_state = e_lldp_dir_tx_rx;

      ifrow = (const struct ovsrec_interface *)nodes[idx]->data;

      if (ifrow && !strcmp(ifrow->name, DEFAULT_BRIDGE_NAME)) {
          continue;
      }

      if (ifrow->split_parent != NULL &&
              !is_parent_interface_split(ifrow->split_parent)) {
          /* Parent is not split. Don't display child interfaces. */
          continue;
      }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_ADMIN);
     if ((NULL != cur_state)
           && (strcmp(cur_state, OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP) == 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no shutdown");
     }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_LANE_SPLIT);

     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_LANE_SPLIT_SPLIT) == 0))
     {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "split");
     }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_SPEEDS);
     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT) != 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "", "speed", cur_state);
     }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_MTU);
     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT) != 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "", "mtu", cur_state);
     }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_DUPLEX);
     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_DUPLEX_FULL) != 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "", "duplex", cur_state);
     }


     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_PAUSE);
     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) != 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0)
        {
           vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "flowcontrol receive on");
        }
        else if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0)
        {
           vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "flowcontrol send on");
        }
        else
        {
           vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "flowcontrol receive on");
           vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "flowcontrol send on");
        }
     }

     cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG);
     if ((NULL != cur_state)
           && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_AUTONEG_DEFAULT) != 0))
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "", "autonegotiation", cur_state);
     }

     /* parse interface other config */
     vtysh_ovsdb_intftable_parse_othercfg(&ifrow->other_config, &intfcfg);

     /* display interface config */

     if (e_lldp_dir_tx == intfcfg.lldptxrx_state)
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp reception");
     }
     else if (e_lldp_dir_rx == intfcfg.lldptxrx_state)
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp transmission");
     }
     else if (e_lldp_dir_off == intfcfg.lldptxrx_state)
     {
        PRINT_INT_HEADER_IN_SHOW_RUN;
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp transmission");
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no lldp reception");
     }
     vtysh_ovsdb_intftable_parse_lacp_othercfg(&ifrow->other_config, p_msg,
                                               &intfcfg, ifrow->name);
     vtysh_ovsdb_intftable_print_lag(p_msg, &intfcfg, ifrow->name);
     vtysh_ovsdb_intftable_parse_l3config(ifrow->name, p_msg, intfcfg.disp_intf_cfg);
   }

   shash_destroy(&sorted_interfaces);
   free(nodes);

   return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_intftable_parse_vlan
| Responsibility : Used for VLAN related config
| Parameters :
|     const char *if_name           : Name of interface
|     vtysh_ovsdb_cbmsg_ptr p_msg   : Used for idl operations
| Return : vtysh_ret_val
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_intftable_parse_vlan(const char *if_name,
                                 vtysh_ovsdb_cbmsg_ptr p_msg)
{
    const struct ovsrec_port *port_row;
    int i;

    port_row = port_lookup(if_name, p_msg->idl);
    if (port_row == NULL)
    {
        return e_vtysh_ok;
    }

    if (port_row->vlan_mode == NULL)
    {
        return e_vtysh_ok;
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        if(port_row->n_tag == 1)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan access ",
                *port_row->tag);
        }
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) == 0)
    {
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
        }
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) == 0)
    {
        if (port_row->n_tag == 1)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk native ",
                *port_row->tag);
        }
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
        }
    }
    else if (strcmp(port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) == 0)
    {
        if (port_row->n_tag == 1)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk native ",
                *port_row->tag);
        }
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "vlan trunk native tag");
        for (i = 0; i < port_row->n_trunks; i++)
        {
            vtysh_ovsdb_cli_print(p_msg, "%4s%s%d", "", "vlan trunk allowed ",
                port_row->trunks[i]);
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
  const struct ovsrec_port *port_row;
  const struct ovsrec_vrf *vrf_row;
  size_t i;

  port_row = port_lookup(if_name, p_msg->idl);
  if (!port_row) {
    return e_vtysh_ok;
  }
  if (!check_iface_in_vrf(if_name)) {
    if (!interfaceNameWritten) {
      vtysh_ovsdb_cli_print(p_msg, "interface %s", if_name);
    }
    vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "no routing");
    vtysh_ovsdb_intftable_parse_vlan(if_name, p_msg);
  }
  if (check_iface_in_vrf(if_name)) {
    vrf_row = port_vrf_match(p_msg->idl, port_row);
    if (NULL != vrf_row) {
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
