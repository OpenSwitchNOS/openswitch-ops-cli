/*
 * Copyright (C) 1997 Kunihiro Ishiguro
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
/************************************************************************//**
 * @ingroup quagga
 *
 * @file vtysh_ovsdb_config.c
 * Source for config infra to walkthrough ovsdb tables.
 *
 ***************************************************************************/

#include "openvswitch/vlog.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"
#include "lib/vty.h"
#include "vtysh_ovsdb_config_context.h"
#include "vtysh_ovsdb_intf_context.h"
#include "vtysh_ovsdb_vlan_context.h"
#include "vtysh_ovsdb_router_context.h"
#include "vtysh_ovsdb_intf_lag_context.h"
#include "vtysh_ovsdb_mgmt_intf_context.h"
/* Intialize the module "vtysh_ovsdb_config" used for log macros */
VLOG_DEFINE_THIS_MODULE(vtysh_ovsdb_config);

#define MAX_WAIT_LOOPCNT 1000
extern struct ovsdb_idl *idl;

/* vtysh context client list defintions */
vtysh_context_client vtysh_config_context_client_list[e_vtysh_config_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_router_context_client_list[e_vtysh_router_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_vlan_context_client_list[e_vtysh_vlan_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_interface_context_client_list[e_vtysh_interface_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_mgmt_interface_context_client_list[e_vtysh_mgmt_interface_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_interface_lag_context_client_list[e_vtysh_interface_lag_context_client_id_max] = {{NULL}};
vtysh_context_client vtysh_dependent_config_client_list[e_vtysh_dependent_config_client_id_max] = {{NULL}};
vtysh_context_client vtysh_dhcp_tftp_context_client_list[e_vtysh_dhcp_tftp_context_client_id_max] = {{NULL}};

/* static array of vtysh context lists
   context traversal order as shown below.
   addition of new context should be done in correct order.
*/
vtysh_context_list vtysh_context_table[e_vtysh_context_id_max] =
{
  { "Config Context",     e_vtysh_config_context,    &vtysh_config_context_client_list},
  { "Router Context",     e_vtysh_router_context,    &vtysh_router_context_client_list},
  { "Vlan Context",       e_vtysh_vlan_context,      &vtysh_vlan_context_client_list},
  { "Interface Context",  e_vtysh_interface_context, &vtysh_interface_context_client_list},
  { "Mgmt Interface Context",  e_vtysh_mgmt_interface_context, &vtysh_mgmt_interface_context_client_list},
  { "Interface LAG Context",  e_vtysh_interface_lag_context, &vtysh_interface_lag_context_client_list},
  { "Dependent Config",   e_vtysh_dependent_config,  &vtysh_dependent_config_client_list},
  { "dhcp tftp Config",   e_vtysh_dhcp_tftp_context,  &vtysh_dhcp_tftp_context_client_list},
};

/*-----------------------------------------------------------------------------
| Function: vtysh_context_get_maxclientid
| Responsibility : get the max client-id value for requested contextid
| Parameters:
|           vtysh_contextid contextid : contextid value
| Return: int : returns max client id
------------------------------------------------------------------------------*/
int
vtysh_context_get_maxclientid(vtysh_contextid contextid)
{
  int ret_val = 0;

  if(!is_valid_vtysh_contextid(contextid))
  {
    return e_vtysh_error;
  }

  switch(contextid)
  {
    case e_vtysh_config_context:
         ret_val = e_vtysh_config_context_client_id_max;
         break;
    case e_vtysh_router_context:
         ret_val = e_vtysh_router_context_client_id_max;
         break;
    case e_vtysh_vlan_context:
         ret_val = e_vtysh_vlan_context_client_id_max;
         break;
    case e_vtysh_interface_context:
         ret_val = e_vtysh_interface_context_client_id_max;
         break;
    case e_vtysh_mgmt_interface_context:
         ret_val = e_vtysh_mgmt_interface_context_client_id_max;
         break;
    case e_vtysh_interface_lag_context:
         ret_val = e_vtysh_interface_lag_context_client_id_max;
         break;
    case e_vtysh_dependent_config:
         ret_val = e_vtysh_dependent_config_client_id_max;
         break;
    case e_vtysh_dhcp_tftp_context:
         ret_val = e_vtysh_dhcp_tftp_context_client_id_max;
         break;
    default:
         ret_val = e_vtysh_error;
         break;
  }

  return ret_val;

}

/*-----------------------------------------------------------------------------
| Function: vtysh_context_get_minclientid
| Responsibility : get the min client-id value for requested contextid
| Parameters:
|           vtysh_contextid contextid : contextid value
| Return: int : returns min client id
------------------------------------------------------------------------------*/
int
vtysh_context_get_minclientid(vtysh_contextid contextid)
{
  int ret_val = 0;

  if(!is_valid_vtysh_contextid(contextid))
  {
    return e_vtysh_error;
  }

  switch(contextid)
  {
    case e_vtysh_config_context:
         ret_val = e_vtysh_config_context_client_id_first;
         break;
    case e_vtysh_router_context:
         ret_val = e_vtysh_router_context_client_id_first;
         break;
    case e_vtysh_vlan_context:
         ret_val = e_vtysh_vlan_context_client_id_first;
         break;
    case e_vtysh_interface_context:
         ret_val = e_vtysh_interface_context_client_id_first;
         break;
    case e_vtysh_mgmt_interface_context:
         ret_val = e_vtysh_mgmt_interface_context_client_id_first;
         break;
    case e_vtysh_interface_lag_context:
         ret_val = e_vtysh_interface_lag_context_client_id_first;
         break;
    case e_vtysh_dependent_config:
         ret_val = e_vtysh_dependent_config_client_id_first;
         break;
    case e_vtysh_dhcp_tftp_context:
         ret_val = e_vtysh_dhcp_tftp_context_client_id_first;
         break;
    default:
         ret_val = e_vtysh_error;
         break;
  }

  return ret_val;

}

/*-----------------------------------------------------------------------------
| Function: vtysh_isvalid_contextclientid
| Responsibility : calidates the client-id for the given contextid
| Parameters:
|           vtysh_contextid contextid : contextid value
|           int clientid: clientid value
| Return: int : returns e_vtysh_ok if clientid is valid else e_vtysh_error
------------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_isvalid_contextclientid(vtysh_contextid contextid, int clientid)
{
  int maxclientid=0, minclientid=0;

  minclientid = vtysh_context_get_minclientid(contextid);
  maxclientid = vtysh_context_get_maxclientid(contextid);

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
| Function: vtysh_context_addclient
| Responsibility : Add client callback to the given context
| Parameters:
|           vtysh_contextid contextid : contextid value
|           int clientid: clientid value
|           vtysh_context_client *p_client: client param
| Return: int : returns e_vtysh_ok if client callabck added successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_context_addclient(vtysh_contextid contextid,
                        int clientid,
                        vtysh_context_client *p_client)
{
  vtysh_context_client *povs_client = NULL;

  VLOG_DBG("vtysh_context_addclient called with context id %d clientid %d", contextid,clientid);

  if(NULL == p_client)
  {
    VLOG_ERR("add_client: NULL Client callback for contextid %d, client id %d",contextid, clientid);
    return e_vtysh_error;
  }

  if (e_vtysh_ok != vtysh_isvalid_contextclientid(contextid, clientid))
  {
    VLOG_ERR("add_client:Invalid client id %d for given context id %d", clientid, contextid);
    return e_vtysh_error;
  }

  povs_client = &(*vtysh_context_table[contextid].clientlist)[clientid-1];
  if (NULL == povs_client->p_callback)
  {
    /* add client call back */
    povs_client->p_client_name = p_client->p_client_name;
    povs_client->client_id = p_client->client_id;
    povs_client->p_callback = p_client->p_callback;
    VLOG_DBG("add_client: Client id %d callback successfully registered with context id %d",clientid, contextid);
  }
  else
  {
    /* client callback is already registered  */
    VLOG_ERR("add_client: Client callback exists for client id %d in context id %d", clientid, contextid);
    return e_vtysh_error;
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_context_removeclient
| Responsibility : Remove client callback to the given context
| Parameters:
|           vtysh_contextid contextid : contextid value
|           int clientid: clientid value
|           vtysh_context_client *p_client: client param
| Return: int : returns e_vtysh_ok if client callback removed successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_context_removeclient(vtysh_contextid contextid,
                           int clientid,
                           vtysh_context_client *p_client)
{
  vtysh_context_client *povs_client = NULL;

  if (NULL == p_client)
  {
    VLOG_ERR("remove_client: Inavlid client callback param");
    return e_vtysh_error;
  }

  if (e_vtysh_ok != vtysh_isvalid_contextclientid(contextid, clientid))
  {
    VLOG_ERR("remove_client: Invalid client id %d for given contextid %d", clientid, contextid);
    return e_vtysh_error;
  }

  povs_client = &(*vtysh_context_table[contextid].clientlist)[clientid-1];
  if (povs_client->p_callback == p_client->p_callback)
  {
    /*client callback matches with registered callback,
      proceed with unregistering client callback */
    povs_client->p_client_name= NULL;
    povs_client->client_id = 0;
    povs_client->p_callback = NULL;
    VLOG_DBG("remove_client: clientid %d callback successfully unregistered for contextid %d", clientid, contextid);
  }
  else
  {
    /* client registered details unmatched */
    VLOG_DBG("remove_client: clientid %d callback param unmatched with registered callback param in contextid %d", clientid, contextid);
    return e_vtysh_error;
  }

  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_context_iterateoverclients
| Responsibility : iterates over the client callback for given contextid
| Parameters:
|           vtysh_contextid contextid : contextid value
|           vtysh_ovsdb_cbmsg *p_msg: client param
| Return: int : returns e_vtysh_ok if client callback invoked successfully
|               else e_vtysh_error
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_context_iterateoverclients(vtysh_contextid contextid, vtysh_ovsdb_cbmsg *p_msg)
{
  int maxclientid = 0, i = 0;
  vtysh_context_client *povs_client = NULL;

  maxclientid = vtysh_context_get_maxclientid(contextid);
  if (e_vtysh_error == maxclientid )
  {
    return e_vtysh_error;
  }

  for (i = 0; i < maxclientid-1; i++)
  {
    povs_client = &(*vtysh_context_table[contextid].clientlist)[i];
    if (NULL != povs_client->p_callback)
    {
      p_msg->clientid = i+1;
      if(e_vtysh_ok != (*(povs_client->p_callback))(p_msg))
      {
        /* log debug msg */
        VLOG_ERR("iteration error for context-id %d, client-id %d", contextid,
                 p_msg->clientid);
        assert(0);
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
  vtysh_contextid contextid=0;
  vtysh_ovsdb_cbmsg msg;

  VLOG_DBG("readconfig:before- idl 0x%p seq no %d", idl, ovsdb_idl_get_seqno(idl));

  msg.fp = fp;
  msg.idl = idl;
  msg.contextid = 0;
  msg.clientid = 0;

  VLOG_DBG("readconfig:after idl 0x%p seq no %d", idl, ovsdb_idl_get_seqno(idl));
  fprintf(fp, "Current configuration:\n");
  fprintf(fp, "!\n");

  for(contextid = 0; contextid < e_vtysh_context_id_max; contextid++)
  {
    msg.contextid = contextid;
    msg.clientid = 0;
    vtysh_context_iterateoverclients(contextid, &msg);
  }
}


/*-----------------------------------------------------------------------------
| Function: vtysh_context_table_list_clients
| Responsibility : list the registered client callback for all config contexts
| Parameters:
|           vty - pointer to object type struct vty
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_context_table_list_clients(struct vty *vty)
{
  vtysh_contextid contextid=0;
  int maxclientid = 0, i =0, minclientid = 0;
  vtysh_context_client *povs_client = NULL;

  if(NULL == vty)
  {
    return;
  }

  for (contextid = 0; contextid < e_vtysh_context_id_max; contextid++)
  {
    vty_out(vty, "%s:%s", vtysh_context_table[contextid].name, VTY_NEWLINE);

    maxclientid = vtysh_context_get_maxclientid(contextid);
    if (e_vtysh_error == maxclientid )
    {
      return;
    }

    minclientid = vtysh_context_get_minclientid(contextid);
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
      povs_client = &(*vtysh_context_table[contextid].clientlist)[i];
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
  return e_vtysh_ok;
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
vtysh_ovsdb_init_clients(void)
{
  /* register vtysh context table client callbacks */
  vtysh_init_config_context_clients();
  vtysh_init_router_context_clients();
  vtysh_init_vlan_context_clients();
  vtysh_init_intf_context_clients();
  vtysh_init_mgmt_intf_context_clients();
  vtysh_init_intf_lag_context_clients();
  vtysh_init_dhcp_tftp_context_clients();
}
