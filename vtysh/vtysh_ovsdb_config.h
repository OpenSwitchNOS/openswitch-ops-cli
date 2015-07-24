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
 * @ingroup  quagga
 *
 * @file vtysh_ovsdb_config.h
 * Source for config infra to walkthrough the ovsdb tables.
 *
 ***************************************************************************/

#ifndef VTYSH_OVSDB_CONFIG_H
#define VTYSH_OVSDB_CONFIG_H

#include "lib/vty.h"

/* general vtysh return type */
typedef enum vtysh_ret_val_enum
{
  e_vtysh_error = -1,
  e_vtysh_ok = 0
} vtysh_ret_val;

/* vtysh ovsdb table_id type */
typedef enum vtysh_ovsdb_table_idenum
{
  e_vtysh_table_id_first = 0,
  e_open_vswitch_table = 0,
  e_interface_table,
  e_vlan_table,
  e_port_table,
  e_bridge_table,
  e_power_supply_table,
  e_temp_sensor_table,
  e_fan_table,
  e_led_table,
  e_route_table,
  e_vtysh_table_id_max
} vtysh_ovsdb_tableid;

/* Open_v_switch Table Client ID type */
typedef enum vtysh_ovsdb_open_vswitch_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_open_vswitch_table_client_id_first = 0,
  e_vtysh_open_vswitch_table_config,
  e_vtysh_open_vswitch_table_client_id_max
} vtysh_ovsdb_openvswicth_table_clientid;

/* Interface Table client-id type */
typedef enum vtysh_ovsdb_interface_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_interface_table_client_id_first = 0,
  e_vtysh_interface_table_config,
  e_vtysh_interface_table_client_id_max
} vtysh_ovsdb_interface_table_clientid;

/* Vlan Table client-id type */
typedef enum vtysh_ovsdb_vlan_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_vlan_table_client_id_first = 0,
  e_vtysh_vlan_table_client_id_max
} vtysh_ovsdb_vlan_table_clientid;

/* Port Table client-id type */
typedef enum vtysh_port_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_port_table_client_id_first = 0,
  e_vtysh_port_table_client_id_max
} vtysh_ovsdb_port_table_clientid;

/* Bridge Table client-id type */
typedef enum vtysh_bridge_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_bridge_table_client_id_first = 0,
  e_vtysh_bridge_table_client_id_max
} vtysh_ovsdb_bridge_table_clientid;

/* Powersupply Table client-id type */
typedef enum vtysh_ovsdb_power_supply_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_power_supply_table_client_id_first = 0,
  e_vtysh_power_supply_table_client_id_max
} vtysh_ovsdb_powersupply_table_clientid;

/* Tempsensor Table client-id type */
typedef enum vtysh_ovsdb_temp_sensor_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_temp_sensor_table_client_id_first = 0,
  e_vtysh_temp_sensor_table_client_id_max
} vtysh_ovsdb_tempsensor_table_clientid;

/* Fan Table client-id type */
typedef enum vtysh_ovsdb_fan_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_fan_table_client_id_first = 0,
  e_vtysh_fan_table_client_id_max
} vtysh_ovsdb_fan_table_clientid;

/* Led Table client-id type */
typedef enum vtysh_ovsdb_led_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_led_table_client_id_first = 0,
  e_vtysh_led_table_config,              //client id for LED configuration
  e_vtysh_led_table_client_id_max
} vtysh_ovsdb_led_table_clientid;

/* Route Table client-id type */
typedef enum vtysh_ovsdb_route_table_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_route_table_client_id_first = 0,
  e_vtysh_route_table_config,
  e_vtysh_route_table_client_id_max
} vtysh_ovsdb_route_table_clientid;

typedef struct vtysh_ovsdb_cbmsg_struct
{
  FILE *fp;
  struct ovsdb_idl *idl;
  vtysh_ovsdb_tableid tableid;
  int clientid;
}vtysh_ovsdb_cbmsg;

typedef struct vtysh_ovsdb_cbmsg_struct *vtysh_ovsdb_cbmsg_ptr;

/* callback prototype for ovsdb table client callback */
typedef vtysh_ret_val (*vtysh_ovsdb_table_client_callback_ptr)(void* p_private);

/* vtysh ovsdb client type */
typedef struct vtysh_ovsdb_client_struct
{
  const char* p_client_name; /* client callback function name */
  int client_id;            /* client id */
  vtysh_ovsdb_table_client_callback_ptr p_callback; /* function call back */
} vtysh_ovsdb_client;

/* vtysh ovsdb table type */
typedef struct vtysh_ovsdb_table_list_struct
{
  const char *name; /* table name */
  vtysh_ovsdb_tableid tableid; /* table id */
  vtysh_ovsdb_client (*clientlist)[]; /* client list */
} vtysh_ovsdb_table_list;

#define is_valid_vtysh_ovsdb_tableid(tableid) \
        ((tableid >= e_vtysh_table_id_first) && (tableid < e_vtysh_table_id_max))

#define VTYSH_STR_EQ(s1, s2)      ((strlen((s1)) == strlen((s2))) && (!strncmp((s1), (s2), strlen((s2)))))

#define VTYSH_OVSDB_CONFIG_ERR 1
#define VTYSH_OVSDB_CONFIG_WARN 2
#define VTYSH_OVSDB_CONFIG_INFO 3
#define VTYSH_OVSDB_CONFIG_DBG 4

extern struct ovsdb_idl *vtysh_ovsdb_idl;
extern unsigned int vtysh_ovsdb_idl_seqno;

vtysh_ret_val vtysh_ovsdbtable_addclient(vtysh_ovsdb_tableid tableid,
                               int clientid,
                               vtysh_ovsdb_client *p_client);
vtysh_ret_val vtysh_ovsdbtable_removeclient(vtysh_ovsdb_tableid tableid,
                                  int clientid,
                                  vtysh_ovsdb_client *p_client);
vtysh_ret_val vtysh_ovsdbtable_iterateoverclients(vtysh_ovsdb_tableid tableid, vtysh_ovsdb_cbmsg *p_msg);

int vtysh_ovsdb_table_get_maxclientid(vtysh_ovsdb_tableid tableid);
int vtysh_ovsdb_table_get_minclientid(vtysh_ovsdb_tableid tableid);

void vtysh_ovsdb_config_init(const char *db_path);
void vtysh_ovsdb_read_config(FILE *fp);
void vtysh_ovsdb_table_list_clients(struct vty *vty);
void vtysh_ovsdb_init_clients();

vtysh_ret_val vtysh_ovsdb_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...);

/* All log/debug/err logging functions */
void vtysh_ovsdb_config_logmsg(int loglevel, char *fmt,  ...);

#endif /* VTYSH_OVSDB_CONFIG_H */
