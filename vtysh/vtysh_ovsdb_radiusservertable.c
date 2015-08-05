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
 * @file vtysh_ovsdb_radiusservertable.c
 * Source for registering client callback with radius server table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_radiusservertable.h"
#include "aaa_vty.h"

char radiusserverclientname[] = "vtysh_ovsdb_radiusservertable_clientcallback";

static vtysh_ret_val vtysh_ovsdb_radiusservertable_parse_options(struct ovsrec_radius_server *row, vtysh_ovsdb_cbmsg *p_msg)
{
    int timeout_check = 1;
    char ip[1000]={0}, *ipaddr=NULL,*udp_port=NULL,*timeout=NULL,*passkey=NULL;
    char file_name[]="/etc/raddb/server";
    FILE *fp=NULL;

    fp = fopen(file_name,"r");
    if (fp == NULL) {
        vtysh_ovsdb_cli_print(p_msg, "Error while opening the Data Base file");
    }

    while (fgets(ip,100 ,fp) != NULL)
    {
       ipaddr=strtok(ip,":");
       udp_port=strtok(NULL," ");
       passkey=strtok(NULL," ");
       timeout=strtok(NULL, " ");

       if (row == NULL) {
           break;
       }

       if (!strcmp(passkey, RADIUS_SERVER_DEFAULT_PASSKEY) && (atoi(udp_port) == RADIUS_SERVER_DEFAULT_PORT) ) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s", ipaddr);
       }

       if (strcmp(passkey, RADIUS_SERVER_DEFAULT_PASSKEY)) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s key %s", ipaddr, passkey);
       }

       if (atoi(udp_port) != RADIUS_SERVER_DEFAULT_PORT) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s auth_port %s", ipaddr, udp_port);
       }

       if (*(row->retries) != RADIUS_SERVER_DEFAULT_RETRIES) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server retries %lld", *(row->retries));
       }

       if (atoi(timeout) != RADIUS_SERVER_DEFAULT_TIMEOUT) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server timeout %d", atoi(timeout));
       }

       row = ovsrec_radius_server_next(row);
    }
}

vtysh_ret_val vtysh_ovsdb_radiusservertable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  struct ovsrec_radius_server *row;
  int server_count = 0;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_radiusservertable_clientcallback entered");
  row = ovsrec_radius_server_first(p_msg->idl);

  if( row!= NULL )
  {
    /* parse radius server param */
    vtysh_ovsdb_radiusservertable_parse_options(row, p_msg);
  }
}

int vtysh_ovsdb_init_radiusservertableclients()
{
  vtysh_ovsdb_client client;

  client.p_client_name = radiusserverclientname;
  client.client_id = e_vtysh_radius_server_table_config;
  client.p_callback = &vtysh_ovsdb_radiusservertable_clientcallback;

  vtysh_ovsdbtable_addclient(e_radius_server_table, e_vtysh_radius_server_table_config, &client);
  return e_vtysh_ok;
}
