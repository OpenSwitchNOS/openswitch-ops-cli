/*
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
#include "logrotate_vty.h"
#include "openswitch-dflt.h"
#include "ecmp_vty.h"
#include "vtysh/vtysh_utils.h"
#include "utils/system_vtysh_utils.h"
#include "vtysh.h"

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

      sprintf(buff, "%s:%ld %s %lu %lu ", row->ip_address, *(row->udp_port), \
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
  char msg_str[VTYSH_CONSOLE_LENGTH];

  if(NULL == ifrow_aaa)
  {
    return e_vtysh_error;
  }

  data = smap_get(ifrow_aaa, SYSTEM_AAA_RADIUS);
  if (data)
  {
    if (!VTYSH_STR_EQ(data, OPS_FALSE_STR))
    {
        snprintf(msg_str, sizeof(msg_str),"%s", "aaa authentication"
                 " login radius");

        data = smap_get(ifrow_aaa, SYSTEM_AAA_RADIUS_AUTH);
        if (data)
        {
          if (!VTYSH_STR_EQ(data, RADIUS_PAP))
          {
            snprintf(msg_str, sizeof(msg_str), "%s", "aaa authentication"
                     " login radius radius-auth chap");
          }
        }
        vtysh_ovsdb_cli_print(p_msg, msg_str);
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
| Function : vtysh_ovsdb_ovstable_parse_ntp_cfg
| Responsibility : parse ntp_config in system table
| Parameters :
|    ifrow_config : ntp_config object pointer
|    pmsg         : callback arguments from show running config handler|
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_ntp_cfg(const struct smap *ifrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
    bool status = false;

    status = smap_get_bool(ifrow_config, SYSTEM_NTP_CONFIG_AUTHENTICATION_ENABLE, false);
    if (status != SYSTEM_NTP_CONFIG_AUTHENTICATION_DEFAULT) {
        vtysh_ovsdb_cli_print(p_msg, "ntp authentication enable");
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
  const char *data = NULL;

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_global_clientcallback entered");
  vswrow = ovsrec_system_first(p_msg->idl);

  if(vswrow)
  {
    if (vswrow->hostname[0] != '\0')
    {
      vtysh_ovsdb_cli_print(p_msg, "hostname %s", vswrow->hostname);
    }
    if ((vswrow->domain_name != NULL) && (vswrow->domain_name[0] != '\0'))
    {
      vtysh_ovsdb_cli_print(p_msg, "domain-name %s", vswrow->domain_name);
    }

    data = smap_get(&vswrow->other_config,
                    SYSTEM_OTHER_CONFIG_MAP_CLI_SESSION_TIMEOUT);
    if (data && (atoi(data) != DEFAULT_SESSION_TIMEOUT_PERIOD))
    {
      vtysh_ovsdb_cli_print(p_msg, "session-timeout %d", atoi(data));
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

    /* parse ntp config param */
    vtysh_ovsdb_ovstable_parse_ntp_cfg(&vswrow->ntp_config, p_msg);
  }

  /* display radius server commands */
  vtysh_display_radiusservertable_commands(p_private);

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
| Function : vtysh_config_context_sflow_clientcallback
| Responsibility : sflow client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_config_context_sflow_clientcallback(void *p_private)
{
  extern struct ovsdb_idl *idl;
  int i=0;
  char *ptr=NULL, *ip=NULL, *port=NULL, *vrf=NULL, *copy=NULL;
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
  const struct ovsrec_system *ovs_row = NULL;
  ovs_row = ovsrec_system_first (idl);
  const struct ovsrec_sflow *sflow_row = NULL;
  const char delim[2] = "/";

  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_config_context_sflow_clientcallback entered");

  OVSREC_SFLOW_FOR_EACH(sflow_row, p_msg->idl){
    if (ovs_row->sflow != NULL)
      {
        vtysh_ovsdb_cli_print(p_msg, "sflow enable");
      }
      if (sflow_row->n_targets > 0)
        {
          for (i = 0; i < sflow_row->n_targets; i++)
            {
              copy = xstrdup(sflow_row->targets[i]);
              ptr = strtok(copy, delim);
              if (ptr != NULL)
                {
                  ip = xstrdup(ptr);
                  ptr = strtok(NULL, delim);
                }
              if (ptr != NULL)
                {
                  port = xstrdup(ptr);
                  ptr = strtok(NULL, delim);
                }
              if (ptr != NULL)
                {
                  vrf = xstrdup(ptr);
                }
              if (port == NULL)
                {
                  vtysh_ovsdb_cli_print(p_msg, "sflow collector %s",ip);
                  free (ip);
                  free (copy);
                }
              if (port != NULL && vrf == NULL)
                {
                  vtysh_ovsdb_cli_print(p_msg, "sflow collector %s port %s",
                                        ip, port);
                  free (ip);
                  free (port);
                  free (copy);
                }
              if (port != NULL && vrf != NULL)
                {
                  vtysh_ovsdb_cli_print(p_msg, "sflow collector %s port %s vrf %s",
                                        ip, port, vrf);
                  free (ip);
                  free (port);
                  free (vrf);
                  free (copy);
                }
            }
        }
      if (sflow_row->agent != NULL && sflow_row->agent_addr_family == NULL)
        {
          vtysh_ovsdb_cli_print(p_msg, "sflow agent-interface %s", sflow_row->agent);
        }
      else if (sflow_row->agent != NULL && sflow_row->agent_addr_family != NULL)
        {
           vtysh_ovsdb_cli_print(p_msg, "sflow agent-interface %s %s",
                                 sflow_row->agent,
                                 sflow_row->agent_addr_family);
        }
      if (sflow_row->sampling != NULL)
        {
          vtysh_ovsdb_cli_print(p_msg, "sflow sampling %lld", *(sflow_row->sampling));

        }
      if (sflow_row->header != NULL)
        {
          vtysh_ovsdb_cli_print(p_msg, "sflow header-size %lld", *(sflow_row->header));
        }
      if (sflow_row->max_datagram != NULL)
        {
          vtysh_ovsdb_cli_print(p_msg, "sflow max-datagram-size %lld", *(sflow_row->max_datagram));
        }
      if (sflow_row->polling != NULL)
        {
          vtysh_ovsdb_cli_print(p_msg, "sflow polling %lld", *(sflow_row->polling));
        }
  return e_vtysh_ok;
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
  vtysh_ret_val retval = e_vtysh_error;

  retval = install_show_run_config_context(e_vtysh_config_context,
                                  &vtysh_config_context_global_clientcallback,
                                  NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                        "config context unable to add global client callback");
    assert(0);
    return retval;
  }

  retval = install_show_run_config_context(e_vtysh_dependent_config,
                                  NULL, NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                    "config context unable to add dependent config  callback");
    assert(0);
    return retval;
  }

  retval = e_vtysh_error;
  retval = install_show_run_config_subcontext(e_vtysh_config_context,
                                     e_vtysh_config_context_ecmp,
                                     &vtysh_config_context_ecmp_clientcallback,
                                     NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                          "config context unable to add ecmp client callback");
    assert(0);
    return retval;
  }

  retval = e_vtysh_error;
  retval = install_show_run_config_subcontext(e_vtysh_config_context,
                                     e_vtysh_config_context_sflow,
                                     &vtysh_config_context_sflow_clientcallback,
                                     NULL, NULL);
  if(e_vtysh_ok != retval)
  {
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                          "config context unable to add sflow client callback");
    assert(0);
    return retval;
  }

  return e_vtysh_ok;
}
