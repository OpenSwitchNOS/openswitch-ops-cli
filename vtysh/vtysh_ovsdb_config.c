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
/************************************************************************//**
 * @ingroup quagga
 *
 * @file vtysh_ovsdb_config.c
 * Source for config infra to walkthrough ovsdb tables.
 *
 ***************************************************************************/

#include "openvswitch/vlog.h"
#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"
#include "lib/vty.h"

/* Intialize the module "vtysh_ovsdb_config" used for log macros */
VLOG_DEFINE_THIS_MODULE(vtysh_ovsdb_config);

#define MAX_WAIT_LOOPCNT 1000
extern struct ovsdb_idl *idl;

/* vtysh ovsdb client list defintions */
vtysh_ovsdb_client vtysh_open_vswitch_table_client_list[e_vtysh_open_vswitch_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_vrf_table_client_list[e_vtysh_vrf_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_interface_table_client_list[e_vtysh_interface_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_vlan_table_client_list[e_vtysh_vlan_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_port_table_client_list[e_vtysh_port_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_bridge_table_client_list[e_vtysh_bridge_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_power_supply_table_client_list[e_vtysh_power_supply_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_temp_sensor_table_client_list[e_vtysh_temp_sensor_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_fan_table_client_list[e_vtysh_fan_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_led_table_client_list[e_vtysh_led_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_route_table_client_list[e_vtysh_route_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_subsystem_table_client_list[e_vtysh_subsystem_table_client_id_max] = {NULL};
vtysh_ovsdb_client vtysh_radius_server_table_client_list[e_vtysh_radius_server_table_client_id_max] = {NULL};

/* static array of vtysh ovsdb tables
   table traversal order as shown below.
   addition of new table should be done in correct order.
*/
vtysh_ovsdb_table_list vtysh_ovsdb_table[e_vtysh_table_id_max] =
{
  { "Open_v_switch Table", e_open_vswitch_table, &vtysh_open_vswitch_table_client_list},
  { "VRF Table",           e_vrf_table,          &vtysh_vrf_table_client_list},
  { "Interface Table",     e_interface_table,    &vtysh_interface_table_client_list},
  { "Vlan Table",          e_vlan_table,         &vtysh_vlan_table_client_list},
  { "Port Table",          e_port_table,         &vtysh_port_table_client_list},
  { "Bridge Table",        e_bridge_table,       &vtysh_bridge_table_client_list},
  { "power_supply Table",  e_power_supply_table, &vtysh_power_supply_table_client_list},
  { "temp_sensor Table",   e_temp_sensor_table,  &vtysh_temp_sensor_table_client_list},
  { "Fan Table",           e_fan_table,          &vtysh_fan_table_client_list},
  { "Led Table",           e_led_table,          &vtysh_led_table_client_list},
  { "Route Table",         e_route_table,        &vtysh_route_table_client_list},
  { "Subsystem Table",     e_subsystem_table,    &vtysh_subsystem_table_client_list},
  { "Radius_Server Table", e_radius_server_table,&vtysh_radius_server_table_client_list},
};

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_table_get_maxclientid
| Responsibility : get the max client-id value for requested tableid
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
| Return: int : returns max client id
------------------------------------------------------------------------------*/
int
vtysh_ovsdb_table_get_maxclientid(vtysh_ovsdb_tableid tableid)
{
  int ret_val = 0;

  if(!is_valid_vtysh_ovsdb_tableid(tableid))
  {
    return e_vtysh_error;
  }

  switch(tableid)
  {
    case e_open_vswitch_table:
         ret_val = e_vtysh_open_vswitch_table_client_id_max;
         break;
    case e_vrf_table:
         ret_val = e_vtysh_vrf_table_client_id_max;
         break;
    case e_interface_table:
         ret_val = e_vtysh_interface_table_client_id_max;
         break;
    case e_vlan_table:
         ret_val = e_vtysh_vlan_table_client_id_max;
         break;
    case e_port_table:
         ret_val = e_vtysh_port_table_client_id_max;
         break;
    case e_bridge_table:
         ret_val = e_vtysh_bridge_table_client_id_max;
         break;
    case e_power_supply_table:
         ret_val = e_vtysh_power_supply_table_client_id_max;
         break;
    case e_temp_sensor_table:
         ret_val = e_vtysh_temp_sensor_table_client_id_max;
         break;
    case e_fan_table:
         ret_val = e_vtysh_fan_table_client_id_max;
         break;
    case e_led_table:
         ret_val = e_vtysh_led_table_client_id_max;
         break;
    case e_route_table:
         ret_val = e_vtysh_route_table_client_id_max;
         break;
    case e_subsystem_table:
         ret_val = e_vtysh_subsystem_table_client_id_max;
         break;
    case e_radius_server_table:
         ret_val = e_vtysh_radius_server_table_client_id_max;
         break;
    default:
         ret_val = e_vtysh_error;
         break;
  }

  return ret_val;

}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_table_get_minclientid
| Responsibility : get the min client-id value for requested tableid
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
| Return: int : returns min client id
------------------------------------------------------------------------------*/
int
vtysh_ovsdb_table_get_minclientid(vtysh_ovsdb_tableid tableid)
{
  int ret_val = 0;

  if(!is_valid_vtysh_ovsdb_tableid(tableid))
  {
    return e_vtysh_error;
  }

  switch(tableid)
  {
    case e_open_vswitch_table:
         ret_val = e_vtysh_open_vswitch_table_client_id_first;
         break;
    case e_vrf_table:
         ret_val = e_vtysh_vrf_table_client_id_first;
         break;
    case e_interface_table:
         ret_val = e_vtysh_interface_table_client_id_first;
         break;
    case e_vlan_table:
         ret_val = e_vtysh_vlan_table_client_id_first;
         break;
    case e_port_table:
         ret_val = e_vtysh_port_table_client_id_first;
         break;
    case e_bridge_table:
         ret_val = e_vtysh_bridge_table_client_id_first;
         break;
    case e_power_supply_table:
         ret_val = e_vtysh_power_supply_table_client_id_first;
         break;
    case e_temp_sensor_table:
         ret_val = e_vtysh_temp_sensor_table_client_id_first;
         break;
    case e_fan_table:
         ret_val = e_vtysh_fan_table_client_id_first;
         break;
    case e_led_table:
         ret_val = e_vtysh_led_table_client_id_first;
         break;
    case e_route_table:
         ret_val = e_vtysh_route_table_client_id_first;
         break;
    case e_subsystem_table:
         ret_val = e_vtysh_subsystem_table_client_id_first;
         break;
    case e_radius_server_table:
         ret_val = e_vtysh_radius_server_table_client_id_first;
         break;
    default:
         ret_val = e_vtysh_error;
         break;
  }

  return ret_val;

}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_isvalid_tableclientid
| Responsibility : calidates the client-id for the given tableid
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
|           int clientid: clientid value
| Return: int : returns e_vtysh_ok if clientid is valid else e_vtysh_error
------------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_isvalid_tableclientid(vtysh_ovsdb_tableid tableid, int clientid)
{
  int maxclientid=0, minclientid=0;

  minclientid = vtysh_ovsdb_table_get_minclientid(tableid);
  maxclientid = vtysh_ovsdb_table_get_maxclientid(tableid);

  if ((e_vtysh_error == minclientid) || (e_vtysh_error == maxclientid))
  {
    return e_vtysh_error;
  }

  if ((clientid > minclientid) && (clientid < maxclientid))
  {
    return e_vtysh_ok;
  }
  else
  {
    return e_vtysh_error;
  }

}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdbtable_addclient
| Responsibility : Add client callback to the given table
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
|           int clientid: clientid value
|           vtysh_ovsdb_client *p_client: client param
| Return: int : returns e_vtysh_ok if client callabck added successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdbtable_addclient(vtysh_ovsdb_tableid tableid,
                           int clientid,
                           vtysh_ovsdb_client *p_client)
{
  vtysh_ovsdb_client *povs_client = NULL;

  VLOG_DBG("vtysh_ovsdbtable_addclient called with table id %d clientid %d", tableid,clientid);

  if(NULL == p_client)
  {
    VLOG_ERR("add_client: NULL Client callback for tableid %d, client id",tableid, clientid);
    return e_vtysh_error;
  }

  if (e_vtysh_ok != vtysh_ovsdb_isvalid_tableclientid(tableid, clientid))
  {
    VLOG_ERR("add_client:Invalid client id %d for given table id %d", clientid, tableid);
    return e_vtysh_error;
  }

  povs_client = &(*vtysh_ovsdb_table[tableid].clientlist)[clientid-1];
  if (NULL == povs_client->p_callback)
  {
    /* add client call back */
    povs_client->p_client_name = p_client->p_client_name;
    povs_client->client_id = p_client->client_id;
    povs_client->p_callback = p_client->p_callback;
    VLOG_DBG("add_client: Client id %d callback successfully registered with table id %d",clientid, tableid);
  }
  else
  {
    /* client callback is already registered  */
    VLOG_ERR("add_client: Client callback exists for client id %d in table id", clientid, tableid);
    return e_vtysh_error;
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdbtable_removeclient
| Responsibility : Remove client callback to the given table
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
|           int clientid: clientid value
|           vtysh_ovsdb_client *p_client: client param
| Return: int : returns e_vtysh_ok if client callback removed successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdbtable_removeclient(vtysh_ovsdb_tableid tableid,
                              int clientid,
                              vtysh_ovsdb_client *p_client)
{
  vtysh_ovsdb_client *povs_client = NULL;

  if (NULL == p_client)
  {
    VLOG_ERR("remove_client: Inavlid client callback param");
    return e_vtysh_error;
  }

  if (e_vtysh_ok != vtysh_ovsdb_isvalid_tableclientid(tableid, clientid))
  {
    VLOG_ERR("remove_client: Invalid client id %d for given tableid %d", clientid, tableid);
    return e_vtysh_error;
  }

  povs_client = &(*vtysh_ovsdb_table[tableid].clientlist)[clientid-1];
  if (povs_client->p_callback == p_client->p_callback)
  {
    /*client callback matches with registered callback,
      proceed with unregistering client callback */
    povs_client->p_client_name= NULL;
    povs_client->client_id = 0;
    povs_client->p_callback = NULL;
    VLOG_DBG("remove_client: clientid %d callback successfully unregistered for tableid %d", clientid, tableid);
  }
  else
  {
    /* client registered details unmatched */
    VLOG_DBG("remove_client: clientid %d callback param unmatched with registered callback param in tabledid %d", clientid, tableid);
    return e_vtysh_error;
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdbtable_iterateoverclients
| Responsibility : iterates over the client callback for given tableid
| Parameters:
|           vtysh_ovsdb_tableid tableid : tableid value
|           vtysh_ovsdb_openvswicth_table_clientid clientid: clientid value
|           vtysh_ovsdb_client *p_client: client param
| Return: int : returns e_vtysh_ok if client callback invoked successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdbtable_iterateoverclients(vtysh_ovsdb_tableid tableid, vtysh_ovsdb_cbmsg *p_msg)
{
  int maxclientid = 0, i = 0;
  vtysh_ovsdb_client *povs_client = NULL;

  maxclientid = vtysh_ovsdb_table_get_maxclientid(tableid);
  if (e_vtysh_error == maxclientid )
  {
    return e_vtysh_error;
  }

  for (i = 0; i < maxclientid-1; i++)
  {
    povs_client = &(*vtysh_ovsdb_table[tableid].clientlist)[i];
    if (NULL != povs_client->p_callback)
    {
      p_msg->clientid = i+1;
      if(e_vtysh_ok != (*(povs_client->p_callback))(p_msg))
      {
        /* log debug msg */
        break;
      }
    }
  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_read_config
| Responsibility : reads ovsdb config by traversing the vtysh_ovsdb_tables
| Parameters:
|           FILE *fp : file pointer to write data to
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_read_config(FILE *fp)
{
  vtysh_ovsdb_tableid tableid=0;
  vtysh_ovsdb_cbmsg msg;
  int loopcnt = 0;

  VLOG_DBG("readconfig:before- idl 0x%x seq no 0x%x", idl, ovsdb_idl_get_seqno(idl));

  msg.fp = fp;
  msg.idl = idl;
  msg.tableid = 0;
  msg.clientid = 0;

  VLOG_DBG("readconfig:after idl 0x%x seq no 0x%x", idl, ovsdb_idl_get_seqno(idl));
  fprintf(fp, "Current configuration:\n");
  fprintf(fp, "!\n");

  for(tableid = 0; tableid < e_vtysh_table_id_max; tableid++)
  {
    msg.tableid = tableid;
    msg.clientid = 0;
    vtysh_ovsdbtable_iterateoverclients(tableid, &msg);
  }
}


/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_table_list_clients
| Responsibility : list the registered client callback for all the ovsdb tables
| Parameters:
|           vty - pointer to object type struct vty
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_table_list_clients(struct vty *vty)
{
  vtysh_ovsdb_tableid tableid=0;
  int maxclientid = 0, i =0, minclientid = 0;
  vtysh_ovsdb_client *povs_client = NULL;

  if(NULL == vty)
  {
    return;
  }

  for (tableid = 0; tableid < e_vtysh_table_id_max; tableid++)
  {
    vty_out(vty, "%s:%s", vtysh_ovsdb_table[tableid].name, VTY_NEWLINE);

    maxclientid = vtysh_ovsdb_table_get_maxclientid(tableid);
    if (e_vtysh_error == maxclientid )
    {
      return;
    }

    minclientid = vtysh_ovsdb_table_get_minclientid(tableid);
    if (minclientid == (maxclientid -1))
    {
      vty_out(vty, "%8s%s%s", "", "No clients registered", VTY_NEWLINE);
      continue;
    }
    else
    {
      vty_out(vty, "%8s%s: %d %s", "", "clients registered", maxclientid -1, VTY_NEWLINE);
    }

    for (i = 0; i < maxclientid-1; i++)
    {
      povs_client = &(*vtysh_ovsdb_table[tableid].clientlist)[i];
      if (NULL != povs_client->p_callback)
      {
        vty_out(vty, "%8s%s%s", "", povs_client->p_client_name, VTY_NEWLINE);
      }
    }
  }
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_cli_print
| Responsibility : prints the command in given format
| Parameters:
|           p_msg - pointer to object type vtysh_ovsdb_cbmsg
|           fmt - print cli format
|           elipses - vraiable args
| Return: returns e_vtysh_ok if it prints cli else e_vtysh_error.
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...)
{
  va_list args;

  if ((NULL == p_msg) || (NULL == p_msg))
  {
    return e_vtysh_error;
  }

  va_start(args, fmt);

  vfprintf(p_msg->fp, fmt, args);
  fprintf(p_msg->fp, "\n");
  fflush(p_msg->fp);

  va_end(args);
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_config_logmsg
| Responsibility : logs info/dbg/err/warn level message
| Parameters: loglevel - logging level INFO/DBG/ERR/WARN
|             fmt - log message foramt
|             elipses - variable args
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_config_logmsg(int loglevel, char *fmt, ...)
{

  va_list args;
  va_start(args, fmt);

  switch (loglevel) {

    case VTYSH_OVSDB_CONFIG_ERR:
          VLOG_ERR(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_WARN:
          VLOG_WARN(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_INFO:
          VLOG_INFO(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_DBG:
          VLOG_DBG(fmt, args);
          break;
    default :
          break;
  }
  va_end(args);
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_init_clients
| Responsibility :   initializes the ovsdb config table client callbacks
| Parameters: void
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_init_clients()
{
  /* register vtysh ovsdb table client callbacks */
  vtysh_ovsdb_init_ovstableclients();
  vtysh_ovsdb_init_vrftableclients();
  vtysh_ovsdb_init_intftableclients();
  vtysh_ovsdb_init_routetableclients();
  /* Register Callback for LED configuration */
  vtysh_ovsdb_init_ledtableclients();
  vtysh_ovsdb_init_subsystemtableclients();
  vtysh_ovsdb_init_vlantableclients();
  vtysh_ovsdb_init_radiusservertableclients();
}
