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
 * @file vtysh_ovsdb_ovstable.c
 * Source for registering client callback with openvswitch table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_config_context.h"
#include "fan_vty.h"
#include "aaa_vty.h"
#include "logrotate_vty.h"
#include "openhalon-dflt.h"

#define DEFAULT_LED_STATE OVSREC_LED_STATE_OFF

char globalconfigclientname[] = "vtysh_config_context_global_clientcallback";
char vrfconfigclientname[]= "vtysh_config_context_vrf_clientcallback";
char fanconfigclientname[]= "vtysh_config_context_fan_clientcallback";
char ledconfigclientname[]= "vtysh_config_context_led_clientcallback";
char staticrouteconfigclientname[]= "vtysh_config_context_staticroute_clientcallback";

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
      vtysh_ovsdb_cli_print(p_msg, "lldp timer %d", transmit_interval);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv management-address");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_VLAN_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-protocol-vlan-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-protocol-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-vlan-name");
    }
  }



  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-vlan-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-capabilities");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-name");
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
| Function : vtysh_ovsdb_ovstable_parse_lacpcfg
| Responsibility : parse lacp_config in open_vswitch table
| Parameters :
|    ifrow_config : lacp_config object pointer
|    fp : file pointer
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_lacpcfg(const struct smap *lacp_config, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL;

  if(NULL == lacp_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(lacp_config,  PORT_OTHER_CONFIG_MAP_LACP_SYSTEM_PRIORITY);
  if (data)
  {
    if (DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY != atoi(data))
    {
      vtysh_ovsdb_cli_print(p_msg, "lacp system-priority %d", atoi(data));
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_alias
| Responsibility : parse alias column in open_vswitch table
| Parameters :
|    row : idl row object pointer
|    fp : file pointer
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_alias(vtysh_ovsdb_cbmsg *p_msg)
{
  struct ovsrec_cli_alias *alias_row = NULL;
  OVSREC_CLI_ALIAS_FOR_EACH (alias_row, p_msg->idl)
  {
     vtysh_ovsdb_cli_print(p_msg, "alias %s %s",
         alias_row->alias_name, alias_row->alias_definition);
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_radiusservertable_parse_options
| Responsibility : parse column in radius server table
| Parameters :
|    row : idl row object pointer
|    fp : file pointer
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_radiusservertable_parse_options(const struct ovsrec_radius_server *row, vtysh_ovsdb_cbmsg *p_msg)
{
    int64_t local_retries = 1;
    char ip[1000]={0}, *ipaddr=NULL,*udp_port=NULL,*timeout=NULL,*passkey=NULL;
    char file_name[]="/etc/raddb/server";
    FILE *fp=NULL;

    fp = fopen(file_name,"r");
    if (fp == NULL) {
        vtysh_ovsdb_cli_print(p_msg, "Unable to open radius server configuration file");
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

       local_retries = *(row->retries);
       row = ovsrec_radius_server_next(row);
    }

    if (local_retries != RADIUS_SERVER_DEFAULT_RETRIES) {
        vtysh_ovsdb_cli_print(p_msg, "radius-server retries %ld", local_retries);
    }

    if (atoi(timeout) != RADIUS_SERVER_DEFAULT_TIMEOUT) {
        vtysh_ovsdb_cli_print(p_msg, "radius-server timeout %d", atoi(timeout));
    }

    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_display_radiusservertable_commands
| Responsibility : display radius server table commands
| scope : static
| Parameters :
|    row : idl row object pointer
|    fp : file pointer
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_display_radiusservertable_commands(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  const struct ovsrec_radius_server *row;
  int server_count = 0;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_radiusservertable_clientcallback entered");
  row = ovsrec_radius_server_first(p_msg->idl);

  if( row!= NULL )
  {
    /* parse radius server param */
    vtysh_ovsdb_radiusservertable_parse_options(row, p_msg);
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_logrotate_cfg
| Responsibility : parse logrotate_config in open_vswitch table
| Parameters :
|    ifrow_config : logrotate_config object pointer
|    pmsg         : callback arguments from show running config handler|
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_logrotate_cfg(const struct smap *ifrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL, *uri = NULL;
  int maxSize = 0;

  if(NULL == ifrow_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT))
    {
      vtysh_ovsdb_cli_print(p_msg, "logrotate period %s",data);
    }
  }

  data = smap_get(ifrow_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT))
    {
      vtysh_ovsdb_cli_print(p_msg, "logrotate maxsize %s", data);
    }
  }

  data = smap_get(ifrow_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "logrotate target %s",data);
  }

  return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_aaa_cfg
| Responsibility : parse aaa column in open_vswitch table
| Parameters :
|    ifrow_aaa   : aaa column object pointer
|    pmsg        : callback arguments from show running config handler|
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_aaa_cfg(const struct smap *ifrow_aaa, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL;

  if(NULL == ifrow_aaa)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_aaa, OPEN_VSWITCH_AAA_RADIUS);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, HALON_FALSE_STR))
    {
      vtysh_ovsdb_cli_print(p_msg, "aaa authentication login radius");
    }
  }

  data = smap_get(ifrow_aaa, OPEN_VSWITCH_AAA_FALLBACK);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, HALON_TRUE_STR))
    {
      vtysh_ovsdb_cli_print(p_msg, "no aaa authentication login fallback error local");
    }
  }

  data = smap_get(ifrow_aaa, SSH_PASSWORD_AUTHENTICATION);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SSH_AUTH_ENABLE))
        vtysh_ovsdb_cli_print(p_msg, "no ssh password-authentication");
  }

  data = smap_get(ifrow_aaa, SSH_PUBLICKEY_AUTHENTICATION);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SSH_AUTH_ENABLE))
        vtysh_ovsdb_cli_print(p_msg, "no ssh public-key-authentication");
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_config_context_global_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_global_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  const struct ovsrec_open_vswitch *vswrow;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_global_clientcallback entered");
  vswrow = ovsrec_open_vswitch_first(p_msg->idl);

  if(vswrow)
  {
    if (vswrow->hostname[0] != '\0')
    {
      vtysh_ovsdb_cli_print(p_msg, "hostname \"%s\"", vswrow->hostname);
    }

    /* parse the alias coumn */
    vtysh_ovsdb_ovstable_parse_alias(p_msg);

    /* parse other config param */
    vtysh_ovsdb_ovstable_parse_othercfg(&vswrow->other_config, p_msg);

    /* parse lacp config param */
    vtysh_ovsdb_ovstable_parse_lacpcfg(&vswrow->lacp_config, p_msg);

    /* parse logrotate config param */
    vtysh_ovsdb_ovstable_parse_logrotate_cfg(&vswrow->logrotate_config, p_msg);

    /* parse aaa config param */
    vtysh_ovsdb_ovstable_parse_aaa_cfg(&vswrow->aaa, p_msg);
  }

  /* display radius server commands */
  vtysh_display_radiusservertable_commands(p_private);

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_config_context_vrf_clientcallback
| Responsibility : vrf client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_vrf_clientcallback(void *p_private)
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
| Function : vtysh_ovsdb_subsystemtable_parse_othercfg
| Responsibility : parse subsystem table
| scope: Static
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_subsystemtable_parse_othercfg(const struct smap *subsystemrow_config,
                                          vtysh_ovsdb_cbmsg *p_msg)
{
    const char *data = NULL;
    if(NULL == subsystemrow_config)
    {
        return e_vtysh_error;
    }
    data = smap_get(subsystemrow_config, FAN_SPEED_OVERRIDE_STR);
    if(data)
    {
        if(!(VTYSH_STR_EQ(data, "normal")))
            vtysh_ovsdb_cli_print(p_msg, "fan-speed %s",data);
    }
    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_config_context_fan_clientcallback
| Responsibility : fan config client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_fan_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_subsystem *subsysrow;
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_fan_clientcallback entered");
    subsysrow = ovsrec_subsystem_first(p_msg->idl);
    if(subsysrow)
    {
    /* parse other config param */
        vtysh_ovsdb_subsystemtable_parse_othercfg(&subsysrow->other_config, p_msg);
    }
    return e_vtysh_ok;
}

/***************************************************************************
* @function      : vtysh_config_context_led_clientcallback
* @detail    : client callback routine for LED configuration
* @parame[in]
*   p_private: Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
*          structure object
* @return : e_vtysh_ok on success
***************************************************************************/
vtysh_ret_val
vtysh_config_context_led_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    struct ovsrec_led *pLedRow = NULL;

    OVSREC_LED_FOR_EACH(pLedRow,p_msg->idl)
    {
        if(pLedRow)
        {
            /* Assuming there is no misconfiguration, state can be on|off|flashing */
            if(0 != strcasecmp(pLedRow->state,DEFAULT_LED_STATE))
            {
                vtysh_ovsdb_cli_print(p_msg,"%s %s %s", "led",pLedRow->id,pLedRow->state);
            }
        }
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_config_context_staticroute_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_staticroute_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  const struct ovsrec_route *row_route;
  char str_temp[80];
  int ipv4_flag = 0;
  int ipv6_flag = 0;
  int len = 0;
  char str[50];
  int i;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_staticroute_clientcallback entered");

  OVSREC_ROUTE_FOR_EACH(row_route, p_msg->idl) {
      ipv4_flag = 0;
      ipv6_flag = 0;
      if (row_route->address_family != NULL) {
          if (!strcmp(row_route->address_family, "ipv4")) {
              ipv4_flag = 1;
          } else if (!strcmp(row_route->address_family, "ipv6")) {
              ipv6_flag = 1;
          }
      } else {
          break;
      }

      if (ipv4_flag == 1 || ipv6_flag == 1) {
          for (i = 0; i < row_route->n_nexthops; i++) {
              if (row_route->prefix) {
                  memset(str, 0, sizeof(str));
                  len = 0;
                  len = snprintf(str, sizeof(str), "%s", row_route->prefix);
                  if (ipv4_flag == 1 && ipv6_flag == 0) {
                      snprintf(str_temp, sizeof(str_temp), "ip route %s", str);
                  }
                  else {
                      snprintf(str_temp, sizeof(str_temp), "ipv6 route %s", str);
                  }
              } else {
                  return e_vtysh_error;
              }

              if (row_route->distance != NULL) {
                if (row_route->n_nexthops && row_route->nexthops[i]->ip_address &&
                    row_route->distance) {
                    if (*row_route->distance == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"%s %s", str_temp,
                            row_route->nexthops[i]->ip_address);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"%s %s %d", str_temp,
                            row_route->nexthops[i]->ip_address, *row_route->distance);
                    }

                } else if (row_route->n_nexthops && row_route->nexthops[i]->ports
                    && row_route->distance) {
                    if (*row_route->distance == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"%s %s", str_temp,
                            row_route->nexthops[i]->ports[0]->name);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"%s %s %d", str_temp,
                            row_route->nexthops[i]->ports[0]->name, *row_route->distance);
                    }
                } else {
                    return e_vtysh_error;
                }
            }
         }
      }
  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_config_context_clients
| Responsibility : registers the client callbacks for config context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_config_context_clients()
{
  vtysh_context_client client;
  vtysh_ret_val retval = e_vtysh_error;

  client.p_client_name = globalconfigclientname;
  client.client_id = e_vtysh_config_context_global;
  client.p_callback = &vtysh_config_context_global_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_config_context, e_vtysh_config_context_global, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "config context unable to add global client callback");
    assert(0);
    return retval;
  }

  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = vrfconfigclientname;
  client.client_id = e_vtysh_config_context_vrf;
  client.p_callback = &vtysh_config_context_vrf_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_config_context, e_vtysh_config_context_vrf, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "config context unable to add vrf client callback");
    assert(0);
    return retval;
  }

  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = fanconfigclientname;
  client.client_id = e_vtysh_config_context_fan;
  client.p_callback = &vtysh_config_context_fan_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_config_context, e_vtysh_config_context_fan, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "config context unable to add fan client callback");
    assert(0);
    return retval;
  }


  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = ledconfigclientname;
  client.client_id = e_vtysh_config_context_led;
  client.p_callback = &vtysh_config_context_led_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_config_context, e_vtysh_config_context_led, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "config context unable to add led client callback");
    assert(0);
    return retval;
  }


  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = staticrouteconfigclientname;
  client.client_id = e_vtysh_dependent_config_staticroute;
  client.p_callback = &vtysh_config_context_staticroute_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_dependent_config, e_vtysh_dependent_config_staticroute, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "dependent config unable to add static route client callback");
    assert(0);
    return retval;
  }

  return e_vtysh_ok;
}
