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
typedef enum vtysh_context_idenum
{
  e_vtysh_context_id_first = 0,
  e_vtysh_config_context = 0,
  e_vtysh_router_context,
  e_vtysh_vlan_context,
  e_vtysh_interface_context,
  e_vtysh_mgmt_interface_context,
  e_vtysh_interface_lag_context,
  e_vtysh_dependent_config,
  e_vtysh_context_id_max
} vtysh_contextid;

/* Config Context Client ID type */
typedef enum vtysh_config_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_config_context_client_id_first = 0,
  e_vtysh_config_context_global,
  e_vtysh_config_context_vrf,
  e_vtysh_config_context_fan,
  e_vtysh_config_context_led,
  e_vtysh_config_context_staticroute,
  e_vtysh_config_context_ecmp,
  e_vtysh_config_context_client_id_max
} vtysh_config_context_clientid;

/* Router Context Client ID type */
typedef enum vtysh_router_context_client_idenum
{
  e_vtysh_router_context_client_id_first = 0,
  e_vtysh_router_context_bgp,
  e_vtysh_router_context_ospf,
  e_vtysh_router_context_client_id_max
} vtysh_router_context_clientid;

/* Interface Context client-id type */
typedef enum vtysh_interface_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_interface_context_client_id_first = 0,
  e_vtysh_interface_context_config,
  e_vtysh_interface_context_client_id_max
} vtysh_interface_context_clientid;

/* Mgmt Interface Context client-id type */
typedef enum vtysh_mgmt_interface_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_mgmt_interface_context_client_id_first = 0,
  e_vtysh_mgmt_interface_context_config,
  e_vtysh_mgmt_interface_context_client_id_max
} vtysh_mgmt_interface_context_clientid;

/* Mgmt Interface Context client-id type */
typedef enum vtysh_interface_lag_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_interface_lag_context_client_id_first = 0,
  e_vtysh_interface_lag_context_config,
  e_vtysh_interface_lag_context_client_id_max
} vtysh_interface_lag_context_clientid;

/* Vlan Context client-id type */
typedef enum vtysh_vlan_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_vlan_context_client_id_first = 0,
  e_vtysh_vlan_context_config,
  e_vtysh_vlan_context_client_id_max
} vtysh_vlan_context_clientid;

/* Dependent Config Client ID type */
typedef enum vtysh_dependent_config_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_dependent_config_client_id_first = 0,
  e_vtysh_dependent_config_staticroute,
  e_vtysh_dependent_config_client_id_max
} vtysh_dependent_config_clientid;

typedef struct vtysh_ovsdb_cbmsg_struct
{
  FILE *fp;
  struct ovsdb_idl *idl;
  vtysh_contextid contextid;
  int clientid;
}vtysh_ovsdb_cbmsg;

typedef struct vtysh_ovsdb_cbmsg_struct *vtysh_ovsdb_cbmsg_ptr;

/* callback prototype for context client callback */
typedef vtysh_ret_val (*vtysh_context_client_callback_ptr)(void* p_private);

/* vtysh ovsdb client type */
typedef struct vtysh_context_client_struct
{
  const char* p_client_name; /* client callback function name */
  int client_id;            /* client id */
  vtysh_context_client_callback_ptr p_callback; /* function call back */
} vtysh_context_client;

/* vtysh ovsdb table type */
typedef struct vtysh_context_list_struct
{
  const char *name; /* context name */
  vtysh_contextid contextid; /* context id */
  vtysh_context_client (*clientlist)[]; /* client list */
} vtysh_context_list;

#define is_valid_vtysh_contextid(contextid) \
        ((contextid >= e_vtysh_context_id_first) && (contextid < e_vtysh_context_id_max))

#define VTYSH_STR_EQ(s1, s2)      ((strlen((s1)) == strlen((s2))) && (!strncmp((s1), (s2), strlen((s2)))))

#define VTYSH_OVSDB_CONFIG_ERR 1
#define VTYSH_OVSDB_CONFIG_WARN 2
#define VTYSH_OVSDB_CONFIG_INFO 3
#define VTYSH_OVSDB_CONFIG_DBG 4

extern struct ovsdb_idl *vtysh_ovsdb_idl;
extern unsigned int vtysh_ovsdb_idl_seqno;

vtysh_ret_val vtysh_context_addclient(vtysh_contextid contextid,
                                      int clientid,
                                      vtysh_context_client *p_client);
vtysh_ret_val vtysh_context_removeclient(vtysh_contextid contextid,
                                         int clientid,
                                         vtysh_context_client *p_client);
vtysh_ret_val vtysh_context_iterateoverclients(vtysh_contextid contextid,
                                               vtysh_ovsdb_cbmsg *p_msg);

int vtysh_context_get_maxclientid(vtysh_contextid contextid);
int vtysh_context_get_minclientid(vtysh_contextid contextid);

void vtysh_ovsdb_config_init(const char *db_path);
void vtysh_ovsdb_read_config(FILE *fp);
void vtysh_context_table_list_clients(struct vty *vty);
void vtysh_ovsdb_init_clients();

vtysh_ret_val vtysh_ovsdb_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...);

/* All log/debug/err logging functions */
void vtysh_ovsdb_config_logmsg(int loglevel, char *fmt,  ...);

struct ovsdb_idl_txn * cli_do_config_start(void);

enum ovsdb_idl_txn_status cli_do_config_finish(struct ovsdb_idl_txn* txn);

void cli_do_config_abort(struct ovsdb_idl_txn* txn);

#endif /* VTYSH_OVSDB_CONFIG_H */
