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
 * @file vtysh_ovsdb_ovstable.c
 * Source for registering client callback with openvswitch table.
 *
 ***************************************************************************/

#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_config_context.h"
#include "fan_vty.h"
#include "aaa_vty.h"
#include "logrotate_vty.h"
#include "openswitch-dflt.h"
#include "ecmp_vty.h"

#define DEFAULT_LED_STATE OVSREC_LED_STATE_OFF

char globalconfigclientname[] = "vtysh_config_context_global_clientcallback";
char vrfconfigclientname[]= "vtysh_config_context_vrf_clientcallback";
char fanconfigclientname[]= "vtysh_config_context_fan_clientcallback";
char ledconfigclientname[]= "vtysh_config_context_led_clientcallback";
char staticrouteconfigclientname[]= "vtysh_config_context_staticroute_clientcallback";
char ecmpconfigclientname[] = "vtysh_config_context_ecmp_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_othercfg
| Responsibility : parse other_config in system table
| Parameters :
|    ifrow_config : other_config object pointer
|    fp : file pointer
| Return : void
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_othercfg(const struct smap *ifrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL;
  int hold_time = 0, transmit_interval = 0, reinit_time = 0;

  if(NULL == ifrow_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "true"))
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp enable");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD );
  if (data)
  {
    hold_time = atoi(data);
    if ( SYSTEM_OTHER_CONFIG_MAP_LLDP_HOLD_DEFAULT != hold_time)
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp holdtime %d", hold_time);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL);
  if (data)
  {
    transmit_interval = atoi(data);
    if ( SYSTEM_OTHER_CONFIG_MAP_LLDP_TX_INTERVAL_DEFAULT != hold_time)
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp timer %d", transmit_interval);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_REINIT);
  if (data)
  {
    reinit_time = atoi(data);
    if (SYSTEM_OTHER_CONFIG_MAP_LLDP_REINIT_DEFAULT != reinit_time)
    {
      vtysh_ovsdb_cli_print(p_msg, "lldp reinit %d", reinit_time);
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_MGMT_ADDR_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv management-address");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_VLAN_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-protocol-vlan-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_PROTO_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-protocol-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_NAME_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-vlan-name");
    }
  }



  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_PORT_VLAN_ID_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv port-vlan-id");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_CAP_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-capabilities");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_DESC_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-description");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_TLV_SYS_NAME_ENABLE);
  if (data)
  {
    if (VTYSH_STR_EQ(data, "false"))
    {
      vtysh_ovsdb_cli_print(p_msg, "no lldp select-tlv system-name");
    }
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_LLDP_MGMT_ADDR);
  if (data)
  {
    vtysh_ovsdb_cli_print(p_msg, "lldp management-address %s", data);
  }

  data = NULL;
  data = smap_get(ifrow_config, SYSTEM_OTHER_CONFIG_MAP_CLI_SESSION_TIMEOUT);
  if (data && (atoi(data) != DEFAULT_SESSION_TIMEOUT_PERIOD))
  {
    vtysh_ovsdb_cli_print(p_msg, "session-timeout %d", atoi(data));
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_lacpcfg
| Responsibility : parse lacp_config in system table
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
    if (DFLT_SYSTEM_LACP_CONFIG_SYSTEM_PRIORITY != atoi(data))
    {
      vtysh_ovsdb_cli_print(p_msg, "lacp system-priority %d", atoi(data));
    }
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_alias
| Responsibility : parse alias column in system table
| Parameters :
|    row : idl row object pointer
|    fp : file pointer
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_alias(vtysh_ovsdb_cbmsg *p_msg)
{
  const struct ovsrec_cli_alias *alias_row = NULL;
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
|    temp :  Double pointer to RADIUS information
|    count : Total number of RADIUS server entries.
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_radiusservertable_parse_options(char **temp, int count, vtysh_ovsdb_cbmsg *p_msg)
{
    int64_t local_count = 0;
    char *ipaddr, *udp_port, *timeout, *passkey, *retries;

    while (count--)
    {
       ipaddr=strtok(temp[local_count],":");
       udp_port=strtok(NULL," ");
       passkey=strtok(NULL," ");
       retries=strtok(NULL, " ");
       timeout=strtok(NULL, " ");

       if (!strcmp(passkey, RADIUS_SERVER_DEFAULT_PASSKEY) && (atoi(udp_port) == RADIUS_SERVER_DEFAULT_PORT) ) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s", ipaddr);
       }

       if (strcmp(passkey, RADIUS_SERVER_DEFAULT_PASSKEY)) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s key %s", ipaddr, passkey);
       }

       if (atoi(udp_port) != RADIUS_SERVER_DEFAULT_PORT) {
           vtysh_ovsdb_cli_print(p_msg, "radius-server host %s auth_port %s", ipaddr, udp_port);
       }
       local_count += 1;
    }

    if (atoi(retries) != RADIUS_SERVER_DEFAULT_RETRIES) {
        vtysh_ovsdb_cli_print(p_msg, "radius-server retries %d", atoi(retries));
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
  char *temp[64];
  int count = 0;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_radiusservertable_clientcallback entered");
  if (!ovsrec_radius_server_first(p_msg->idl))
  {
      return e_vtysh_ok;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, p_msg->idl)
  {
      /* Array buff max size is 60, since it should accomodate a string
       * in below format, where IP address max lenght is 15, port max
       * length is 5, passkey/shared secret max length is 32, retries
       * max length is 1 and timeout max length is 2.
       * {"<ipaddress>:<port> <passkey> <retries> <timeout> "}
       */
      char buff[60]= {0};

      sprintf(buff, "%s:%ld %s %d %d ", row->ip_address, *(row->udp_port), \
                            row->passkey, *(row->retries), *(row->timeout));
      temp[row->priority - 1] = (char *)malloc(strlen(buff));
      strncpy(temp[row->priority - 1],buff,strlen(buff));
      count += 1;
  }
  /* parse radius server param */
  vtysh_ovsdb_radiusservertable_parse_options(temp, count, p_msg);
  while(count)
  {
      count--;
      free(temp[count]);
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_logrotate_cfg
| Responsibility : parse logrotate_config in system table
| Parameters :
|    ifrow_config : logrotate_config object pointer
|    pmsg         : callback arguments from show running config handler|
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_logrotate_cfg(const struct smap *ifrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
  const char *data = NULL;

  if(NULL == ifrow_config)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_config, SYSTEM_LOGROTATE_CONFIG_MAP_PERIOD);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SYSTEM_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT))
    {
      vtysh_ovsdb_cli_print(p_msg, "logrotate period %s",data);
    }
  }

  data = smap_get(ifrow_config, SYSTEM_LOGROTATE_CONFIG_MAP_MAXSIZE);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SYSTEM_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT))
    {
      vtysh_ovsdb_cli_print(p_msg, "logrotate maxsize %s", data);
    }
  }

  data = smap_get(ifrow_config, SYSTEM_LOGROTATE_CONFIG_MAP_TARGET);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SYSTEM_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT))
        vtysh_ovsdb_cli_print(p_msg, "logrotate target %s",data);
  }

  return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_aaa_cfg
| Responsibility : parse aaa column in system table
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

  data = smap_get(ifrow_aaa, SYSTEM_AAA_RADIUS);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPS_FALSE_STR))
    {
      vtysh_ovsdb_cli_print(p_msg, "aaa authentication login radius");
    }
  }

  data = smap_get(ifrow_aaa, SYSTEM_AAA_FALLBACK);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPS_TRUE_STR))
    {
      vtysh_ovsdb_cli_print(p_msg, "no aaa authentication login fallback error local");
    }
  }

  data = smap_get(ifrow_aaa, SSH_PASSWORD_AUTHENTICATION_ENABLE);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, SSH_AUTH_ENABLE))
        vtysh_ovsdb_cli_print(p_msg, "no ssh password-authentication");
  }

  data = smap_get(ifrow_aaa, SSH_PUBLICKEY_AUTHENTICATION_ENABLE);
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

  const struct ovsrec_system *vswrow;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_global_clientcallback entered");
  vswrow = ovsrec_system_first(p_msg->idl);

  if(vswrow)
  {
    if (vswrow->hostname[0] != '\0')
    {
      vtysh_ovsdb_cli_print(p_msg, "hostname %s", vswrow->hostname);
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
    const struct ovsrec_led *pLedRow = NULL;

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
  char str[50];
  int i;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_staticroute_clientcallback entered");

  OVSREC_ROUTE_FOR_EACH(row_route, p_msg->idl) {
      ipv4_flag = 0;
      ipv6_flag = 0;
      if (strcmp(row_route->from, OVSREC_ROUTE_FROM_STATIC)) {
          continue;
      }

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
                  snprintf(str, sizeof(str), "%s", row_route->prefix);
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
| Function : vtysh_config_context_ecmp_clientcallback
| Responsibility : ecmp config client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_ecmp_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_system *ovs_row;

    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                              "vtysh_config_context_ecmp_clientcallback entered");

    ovs_row = ovsrec_system_first(p_msg->idl);
    if (!ovs_row)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                            "vtysh_config_context_ecmp_clientcallback: error ovs_row");
        return e_vtysh_error;
    }

    if (!GET_ECMP_CONFIG_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp disable");
    }
    if (!GET_ECMP_CONFIG_HASH_SRC_IP_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance src-ip disable");
    }
    if (!GET_ECMP_CONFIG_HASH_SRC_PORT_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance src-port disable");
    }
    if (!GET_ECMP_CONFIG_HASH_DST_IP_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance dst-ip disable");
    }
    if (!GET_ECMP_CONFIG_HASH_DST_PORT_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance dst-port disable");
    }
    if (!GET_ECMP_CONFIG_HASH_RESILIENT_STATUS(ovs_row))
    {
        vtysh_ovsdb_cli_print(p_msg, "ip ecmp load-balance resilient disable");
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

  retval = e_vtysh_error;
  memset(&client, 0, sizeof(vtysh_context_client));
  client.p_client_name = ecmpconfigclientname;
  client.client_id = e_vtysh_config_context_ecmp;
  client.p_callback = &vtysh_config_context_ecmp_clientcallback;
  retval = vtysh_context_addclient(e_vtysh_config_context, e_vtysh_config_context_ecmp, &client);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                              "config context unable to add ecmp client callback");
    assert(0);
    return retval;
  }

  return e_vtysh_ok;
}
