/* SFLOW CLI commands
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
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
 *
 * File: sflow_vty.c
 *
 * Purpose:  To add SFLOW CLI configuration and display commands.
 */
#include <inttypes.h>
#include <stdio.h>
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sflow_vty.h"
#include <lib/version.h>
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "lldp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include <stdlib.h>

VLOG_DEFINE_THIS_MODULE (vtysh_sflow_cli);
extern struct ovsdb_idl *idl;

bool
check_ipaddr_family(const char *add)
  {
      struct addrinfo hint, *res = NULL;
      memset(&hint, '\0', sizeof hint);

      hint.ai_family = PF_UNSPEC;
      hint.ai_flags = AI_NUMERICHOST;
      int ret = getaddrinfo(add, NULL, &hint, &res);
        if (ret) {
           puts("Invalid address");
           puts(gai_strerror(ret));
           return 1;
          }
      if (res->ai_family == AF_INET) {
          freeaddrinfo(res);
          return true;
      }
      else {
          freeaddrinfo(res);
          return false;
      }
  }



static int sflow_show(void)
{
    const struct ovsrec_system *ovs_row = NULL;
    const struct ovsrec_sflow *sflow_row = NULL;
    size_t i=0;
    sflow_row = ovsrec_sflow_first(idl);
    if (!sflow_row) {
        vty_out(vty, "sFlow Disabled.%s",
                     VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    ovs_row = ovsrec_system_first (idl);

    if (ovs_row->sflow != NULL)
      {
        vty_out(vty, "%ssFlow Configuration %s",
                  VTY_NEWLINE,VTY_NEWLINE);
        vty_out(vty, "-----------------------------------------%s",VTY_NEWLINE);
        vty_out(vty, "sFlow                         Enabled%s", VTY_NEWLINE);
        if(sflow_row->n_targets > 0)
          {
            vty_out(vty, "Collector ip : Port : Vrf     %s%s", sflow_row->targets[i], VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Collector ip : Port : Vrf     NOT SET%s", VTY_NEWLINE);
          }
        for(i = 1; i < sflow_row->n_targets; i++)
          {
            vty_out(vty, "                              %s%s", sflow_row->targets[i], VTY_NEWLINE);
          }
        if(sflow_row->agent != NULL)
          {
            vty_out(vty, "Agent Interface               %s%s", sflow_row->agent, VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Agent Interface               NOT SET%s", VTY_NEWLINE);
          }
        if(sflow_row->agent_addr_family != NULL)
          {
            vty_out(vty, "Agent Address Family          %s%s", sflow_row->agent_addr_family, VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Agent Address Family          NOT SET%s", VTY_NEWLINE);
          }
        if(sflow_row->sampling != NULL)
          {
            vty_out(vty, "Sampling Rate                 %lld%s", *(sflow_row->sampling), VTY_NEWLINE);
            //vty_out(vty, "%" PRId64 "\n", sflow_row->sampling);
          }
        else
          {
            vty_out(vty, "Sampling Rate                 %d%s",OVSDB_SFLOW_SAMPLING_RATE, VTY_NEWLINE);
          }
        if(sflow_row->polling != NULL)
          {
            vty_out(vty, "Polling Interval              %lld%s", sflow_row->polling, VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Polling Interval              %d%s", OVSDB_SFLOW_POLLING_INTERVAL, VTY_NEWLINE);
          }
        if(sflow_row->header != NULL)
          {
            vty_out(vty, "Header Size                   %lld%s", sflow_row->header, VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Header Size                   %d%s",OVSDB_SFLOW_HEADER_SIZE, VTY_NEWLINE);
          }
        if(sflow_row->max_datagram != NULL)
          {
            vty_out(vty, "Max Datagram Size             %lld%s", sflow_row->max_datagram, VTY_NEWLINE);
          }
        else
          {
            vty_out(vty, "Max Datagram Size             %d%s", OVSDB_SFLOW_MAX_DATAGRAM_SIZE, VTY_NEWLINE);
          }
        vty_out(vty, "Number of Samples             0%s", VTY_NEWLINE);
      }
    else
      {
        vty_out(vty, "sFlow Disabled.%s", VTY_NEWLINE);
      }
    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;

}

static int sflow_set_global_status(const char *status)
{
    const struct ovsrec_system *ovs_row = NULL;
    const struct ovsrec_sflow *sflow_row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    const char *sflow_name = "global";

    if (status_txn == NULL)
      {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
      }

  ovs_row = ovsrec_system_first (idl);
    if (!ovs_row)
      {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
       }

  sflow_row = ovsrec_sflow_first (idl);

    if(strcmp("true", status) == 0)
      {
        if(ovs_row->sflow != NULL)
          {
            vty_out (vty, "Sflow already enabled.%s", VTY_NEWLINE);
            cli_do_config_abort (status_txn);
            return CMD_SUCCESS;
          }
        else
          {
            if(!sflow_row)
              {
                sflow_row = ovsrec_sflow_insert (status_txn);
                ovsrec_sflow_set_name (sflow_row, sflow_name);
              }
            ovsrec_system_set_sflow (ovs_row, sflow_row);
          }
       }
     else
       {
         if(sflow_row)
           {
             ovsrec_system_set_sflow (ovs_row, NULL);
           }
         else
           {
             vty_out (vty, "sFlow already disabled.%s", VTY_NEWLINE);
             cli_do_config_abort (status_txn);
             return CMD_SUCCESS;
           }
       }
  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command succeeded and SFLOW was added successfully");
      return CMD_SUCCESS;
    }
  else
    {
       VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
       return CMD_OVSDB_FAILURE;
    }
}

static int
sflow_set_sampling_rate( int64_t *rate )
{
  const struct ovsrec_system *ovs_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  const struct ovsrec_sflow *sflow_row = NULL;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

  ovs_row = ovsrec_system_first (idl);
  if (!ovs_row)
    {
        VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }
  sflow_row = ovsrec_sflow_first (idl);
  if(!sflow_row && *rate != 0)
    {
      sflow_row = ovsrec_sflow_insert (status_txn);
    }
  if(!sflow_row && *rate == 0)
    {
      vty_out (vty, "sFlow sampling not set%s", VTY_NEWLINE);
    }
  if(*rate != 0)
    {
      ovsrec_sflow_set_sampling(sflow_row, rate, 1);
    }
  else
    {
      ovsrec_sflow_set_sampling(sflow_row, NULL, 0);
     }
  txn_status = cli_do_config_finish(status_txn);
  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command succeeded and SFLOW was added successfully");
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }

}



static int sflow_set_collector(const char *ip, const char *port, const char *vrf, const char *set)
{
  const struct ovsrec_system *ovs_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  const struct ovsrec_sflow *sflow_row = NULL;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  char **target;
  size_t i=0,n=0,m=0;
  char *temp;
  bool sflow_collector_match = false;
  if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

  ovs_row = ovsrec_system_first (idl);
  if (!ovs_row)
    {
        VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

  if(vrf != NULL)
    {
        if (strcmp(vrf, DEFAULT_VRF_NAME) != 0)
          {
              vty_out (vty, "Only default_vrf is permitted.%s", VTY_NEWLINE);
              cli_do_config_abort (status_txn);
              return CMD_SUCCESS;
          }
    }

  sflow_row = ovsrec_sflow_first (idl);
  if(!sflow_row && (strcmp(set, "true")==0))
    {
      sflow_row = ovsrec_sflow_insert (status_txn);
    }
  if(!sflow_row && (strcmp(set, "false")==0))
    {
      vty_out (vty, "No sFlow collector present.%s", VTY_NEWLINE);
    }

  size_t size = sflow_row->n_targets;
  if((strcmp(set, "true")==0) && size >= 3)
    {
      vty_out (vty, "Only 3 collector entries permitted.%s", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if (check_ipaddr_family(ip))
    {
      if (port == NULL && vrf == NULL)
        {
            temp = malloc(IP_ADDRESS_LENGTH+1);
            strcpy(temp, ip);
        }
      else if(port != NULL && vrf == NULL)
        {
            temp = malloc(IP_ADDRESS_LENGTH+MAX_PORT_SIZE+2);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, port);
        }
      else if(port == NULL && vrf != NULL)
        {
            temp = malloc(IP_ADDRESS_LENGTH+sizeof(DEFAULT_VRF_NAME)+2);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, vrf);

        }
      else
        {
            temp = malloc(IP_ADDRESS_LENGTH+MAX_PORT_SIZE+sizeof(DEFAULT_VRF_NAME)+3);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, port);
            strcat(temp, "/");
            strcat(temp, vrf);
        }
    }
  else
    {
      if (port == NULL && vrf == NULL)
        {
            temp = malloc(IPV6_ADDRESS_LENGTH+1);
            strcpy(temp, ip);
        }
      else if(port != NULL && vrf == NULL)
        {
            temp = malloc(IPV6_ADDRESS_LENGTH+MAX_PORT_SIZE+2);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, port);
        }
      else if(port == NULL && vrf != NULL)
        {
            temp = malloc(IPV6_ADDRESS_LENGTH+sizeof(DEFAULT_VRF_NAME)+2);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, vrf);
        }
      else
        {
            temp = malloc(IP_ADDRESS_LENGTH+MAX_PORT_SIZE+sizeof(DEFAULT_VRF_NAME)+3);
            strcpy(temp, ip);
            strcat(temp, "/");
            strcat(temp, port);
            strcat(temp, "/");
            strcat(temp, vrf);
        }
      }
  if(strcmp(set, "true") == 0)
    {
      for (m = 0; m < sflow_row->n_targets; m++)
        {
          if (strcmp (temp, sflow_row->targets[m]) == 0)
            {
               vty_out (vty, "Duplicate collector,Cannot add.%s", VTY_NEWLINE);
               cli_do_config_abort (status_txn);
               return CMD_SUCCESS;
            }
        }
      if(sflow_row->n_targets==1)
        {
          target = xmalloc(sizeof(temp)+sizeof(sflow_row->targets[0]));
        }
      if(sflow_row->n_targets==2)
        {
          target = xmalloc(sizeof(temp)+sizeof(sflow_row->targets[0])+sizeof(sflow_row->targets[1]));
        }
      else
        {
          target = xmalloc(sizeof(temp));
        }
      for (i = 0; i <sflow_row->n_targets; i++)
        {
          target[i] = sflow_row->targets[i];
        }
      target[sflow_row->n_targets] = temp;
      if(sflow_row)
        {
            ovsrec_sflow_set_targets(sflow_row, target, sflow_row->n_targets + 1);
        }
      else
        {
            vty_out (vty, "sFlow disabled.%s", VTY_NEWLINE);
            cli_do_config_abort (status_txn);
            return CMD_SUCCESS;
        }
      free (temp);
      free (target);
    }
  else
    {
      if(sflow_row->n_targets==0)
        {
          vty_out (vty, "No collector present.%s", VTY_NEWLINE);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      for (i = 0; i < sflow_row->n_targets; i++)
        {
            if (strcmp (temp, sflow_row->targets[i]) == 0)
              {
                //vty_out (vty, "values are:%s%s", sflow_row->targets[i], VTY_NEWLINE);
                //vty_out (vty, "temp match %s", temp);
                sflow_collector_match = true;
                //vty_out (vty, "match_value %d", sflow_collector_match);
                break;
              }
        }
       if (!sflow_collector_match)
         {
             //vty_out (vty, "match_value %d", sflow_collector_match);
             vty_out (vty, "Collector not found.%s", VTY_NEWLINE);
             cli_do_config_abort (status_txn);
             return CMD_SUCCESS;
         }
       if(sflow_row->n_targets==3)
         {
           target = xmalloc(sizeof(sflow_row->targets[0])+sizeof(sflow_row->targets[1])+sizeof(sflow_row->targets[2])-sizeof(sflow_row->targets[i]));
         }
       if(sflow_row->n_targets==2)
         {
           target = xmalloc(sizeof(sflow_row->targets[0])+sizeof(sflow_row->targets[1])-sizeof(sflow_row->targets[i]));
         }
       for (i = n = 0; i < sflow_row->n_targets; i++)
         {
           if (strcmp (temp, sflow_row->targets[i])!= 0)
             target[n++] = sflow_row->targets[i];
         }
       ovsrec_sflow_set_targets(sflow_row, target, n);
       free (target);
       free (temp);
    }

  txn_status = cli_do_config_finish(status_txn);
  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command succeeded and SFLOW was added successfully");
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

static int sflow_set_agent_interface(const char *interface, const char *family)
{
    const struct ovsrec_system *ovs_row = NULL;
    const struct ovsrec_sflow *sflow_row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();

    if (status_txn == NULL)
      {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
      }

   ovs_row = ovsrec_system_first (idl);
    if (!ovs_row)
      {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
       }

   sflow_row = ovsrec_sflow_first (idl);
   if(!sflow_row && interface != NULL)
    {
      sflow_row = ovsrec_sflow_insert (status_txn);
    }
   if(!sflow_row && interface == NULL)
    {
      vty_out (vty, "sFlow agent not present.%s", VTY_NEWLINE);
    }
    if(sflow_row)
      {
        if (family == NULL && interface == NULL)
          {
              if(sflow_row->agent == NULL)
                {
                  vty_out (vty, "Sflow agent interface not present.%s", VTY_NEWLINE);
                  cli_do_config_abort (status_txn);
                  return CMD_SUCCESS;
                }
              ovsrec_sflow_set_agent(sflow_row, interface);
              ovsrec_sflow_set_agent_addr_family(sflow_row, family);
          }
        if (interface != NULL)
          {
              ovsrec_sflow_set_agent(sflow_row, interface);
          }
        if (family != NULL)
          {
              ovsrec_sflow_set_agent_addr_family(sflow_row, family);
          }
      }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      VLOG_DBG ("%s The command succeeded and SFLOW was added successfully");
      return CMD_SUCCESS;
    }
  else
    {
       VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
       return CMD_OVSDB_FAILURE;
    }
}


DEFUN (cli_sflow_set_global_status,
       cli_sflow_set_global_status_cmd,
       "sflow enable",
       SFLOW_STR
       "Enable/Disable sflow feature\n")
{
    return sflow_set_global_status("true");
}

DEFUN (cli_sflow_no_set_global_status,
       cli_sflow_no_set_global_status_cmd,
       "no sflow enable",
        NO_STR
        SFLOW_STR
        "Enable/Disable sflow feature\n")
{
    return sflow_set_global_status("false");
}

DEFUN (cli_flow_set_sampling_rate,
       cli_sflow_set_sampling_rate_cmd,
       "sflow sampling <1-1000000000>",
       SFLOW_STR
       "set sampling rate for corresponding interface\n")
{
    int64_t s_rate = (int64_t) atoi(argv[0]);
    return sflow_set_sampling_rate(&s_rate);
}

DEFUN (cli_sflow_no_set_sampling_rate,
       cli_sflow_no_set_sampling_rate_cmd,
       "no sflow sampling",
       NO_STR
       SFLOW_STR
       "resets sampling rate to default\n")
{
    int64_t rate_sample = 0;
    return sflow_set_sampling_rate(&rate_sample);
    //free(rate_sample);
}


DEFUN (cli_sflow_set_collector,
       cli_sflow_set_collector_cmd,
       "sflow collector (A.B.C.D|X:X::X:X)"
       " {port <0-65535> | vrf VRF_NAME}",
       SFLOW_STR
       "Set collector ip address configuration\n"
       "Enter IPv4 address\n"
       "Enter IPv6 address\n"
       "Port for IP address\n"
       "Enter port number\n"
       "Assign a vrf\n"
       "VRF name\n")
{
    return sflow_set_collector(argv[0], argv[1], argv[2], "true");
}

DEFUN (cli_sflow_no_set_collector,
       cli_sflow_no_set_collector_cmd,
       "no sflow collector (A.B.C.D|X:X::X:X)"
       " {port <0-65535> | vrf VRF_NAME}",
       NO_STR
       SFLOW_STR
       "removes sflow collector\n")
{
    return sflow_set_collector(argv[0], argv[1], argv[2], "false");
}
DEFUN (cli_sflow_set_agent_interface,
       cli_sflow_set_agent_interface_cmd,
       "sflow agent-interface INTERFACE"
       " {agent-address-family (ipv4|ipv6)}",
       SFLOW_STR
       "Set agent interface\n"
       "Enter interface number\n"
       "Agent address family\n"
       "Enter whether IPV4\n"
       "Enter whether IPV6\n")
{
    return sflow_set_agent_interface(argv[0], argv[1]);
}

DEFUN (cli_sflow_no_set_agent_interface,
       cli_sflow_no_set_agent_interface_cmd,
       "no sflow agent-interface",
       SFLOW_STR
       "Removes set agent interface\n")
{
    return sflow_set_agent_interface(NULL, NULL);
}
/*
DEFUN (cli_sflow_set_header_size,
       cli_sflow_set_header_size_cmd,
       "sflow header-size <64-256>",
       SFLOW_STR
       "set header size\n")
{
    int ret_code;
    sflow_params_t params;
    if (atoi(argv[0]) < 256 && atoi(argv[0]) > 64) {
        params.header = atoi(argv[0]);
    }else {
        vty_out(vty,"%s is invalid%s",argv[0],VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    ret_code =
    return (ret_code);
}
*/
DEFUN (cli_sflow_show,
       cli_sflow_show_cmd,
       "show sflow",
       SHOW_STR
       SFLOW_STR
       "show sflow config\n")
{
        return sflow_show();
}

/* Install SFLOW related vty commands. */
void
sflow_vty_init (void)
{
  install_element (CONFIG_NODE, &cli_sflow_set_global_status_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_set_global_status_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_sampling_rate_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_set_sampling_rate_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_collector_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_set_collector_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_agent_interface_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_set_agent_interface_cmd);
  install_element (ENABLE_NODE, &cli_sflow_show_cmd);
}
/*  install_element (CONFIG_NODE, &cli_sflow_set_header_size_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_header_size_cmd);
  install_element (ENABLE_NODE, &cli_sflow_config_show_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_agent_interface_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_agent_interface_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_collector_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_collector_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_sampling_rate_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_sampling_rate_cmd);
  install_element (CONFIG_NODE, &cli_sflow_set_global_status_cmd);
  install_element (CONFIG_NODE, &cli_sflow_no_set_global_status_cmd);
  install_element (INTERFACE_NODE, &cli_sflow_enable_cmd);
  install_element (INTERFACE_NODE, &cli_sflow_no_enable_cmd);*/
