/* SFLOW CLI commands
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include <stdlib.h>
#include "vrf_vty.h"
#include "sflow.h"
#include "prefix.h"

VLOG_DEFINE_THIS_MODULE (vtysh_sflow_cli);
extern struct ovsdb_idl *idl;

/* This function displays the sflow configuration set in the sFlow table */
static int sflow_show(void)
{
  const struct ovsrec_system *system_row = NULL;
  const struct ovsrec_sflow *sflow_row = NULL;
  size_t i=0;
  uint64_t MAX_CMD_LEN = IPV6_ADDRESS_LENGTH + MAX_PORT_SIZE +
            VRF_NAME_MAX_LENGTH + 3;
  char temp_ip[MAX_CMD_LEN];
  char *collector_ip = NULL;
  char *collector_port = NULL;
  char *collector_vrf = NULL;

  sflow_row = ovsrec_sflow_first(idl);
  if (!sflow_row)
    {
      vty_out(vty, "\nsFlow not yet configured.%s\n",
                     VTY_NEWLINE);
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_SUCCESS;
    }
  system_row = ovsrec_system_first (idl);
  if (!system_row)
    {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_SUCCESS;
    }

  vty_out(vty, "%ssFlow Configuration %s",
            VTY_NEWLINE,VTY_NEWLINE);
  vty_out(vty, "-----------------------------------------%s",
            VTY_NEWLINE);
  if (system_row->sflow != NULL)
    {
      vty_out(vty, "sFlow                         enabled%s",
              VTY_NEWLINE);
    }
  else
    {
      vty_out(vty, "sFlow                         disabled%s",
              VTY_NEWLINE);
    }

  if(sflow_row->n_targets > 0)
    vty_out(vty, "Collector IP/Port/Vrf");
  else
    vty_out(vty, "Collector IP/Port/Vrf         NOT SET%s",
            VTY_NEWLINE);

  for(i = 0; i < sflow_row->n_targets; i++)
    {
    memset(temp_ip, 0, sizeof(temp_ip));
    strcpy(temp_ip, sflow_row->targets[i]);
    collector_ip = strtok(temp_ip, "/");
    collector_port = strtok(NULL, "/");
    collector_vrf = strtok(NULL, "\0");
    if (!collector_vrf && collector_port)
      {
        if (!atoi(collector_port))
          {
            collector_vrf = collector_port;
            collector_port = NULL;
          }
      }

    if (collector_ip && collector_port && collector_vrf)
      {
        if (i == 0)
          vty_out(vty, "         %s%s",
                  sflow_row->targets[i], VTY_NEWLINE);
        else
          vty_out(vty, "                              %s%s",
                  sflow_row->targets[i], VTY_NEWLINE);
      }
    if (collector_ip && collector_port && !collector_vrf)
      {
        if (i == 0)
          vty_out(vty, "         %s/%s%s",
                  sflow_row->targets[i], DEFAULT_VRF_NAME, VTY_NEWLINE);
        else
          vty_out(vty, "                              %s/%s%s",
                  sflow_row->targets[i], DEFAULT_VRF_NAME, VTY_NEWLINE);
      }
    if (collector_ip && !collector_port && collector_vrf)
      {
        if (i == 0)
          vty_out(vty, "         %s/%d/%s%s",
                 collector_ip, SFL_DEFAULT_COLLECTOR_PORT, collector_vrf,
                 VTY_NEWLINE);
        else
          vty_out(vty, "                              %s/%d/%s%s",
                 collector_ip, SFL_DEFAULT_COLLECTOR_PORT, collector_vrf,
                 VTY_NEWLINE);
      }
    if (collector_ip && !collector_port && !collector_vrf)
      {
        if (i == 0)
          vty_out(vty, "         %s/%d/%s%s",
                 sflow_row->targets[i], SFL_DEFAULT_COLLECTOR_PORT,
                 DEFAULT_VRF_NAME, VTY_NEWLINE);
        else
          vty_out(vty, "                              %s/%d/%s%s",
                 sflow_row->targets[i], SFL_DEFAULT_COLLECTOR_PORT,
                 DEFAULT_VRF_NAME, VTY_NEWLINE);
      }
    }

  if(sflow_row->agent != NULL)
    vty_out(vty, "Agent Interface               %s%s",
            sflow_row->agent, VTY_NEWLINE);
  else
    vty_out(vty, "Agent Interface               NOT SET%s",
            VTY_NEWLINE);

  if(sflow_row->agent_addr_family != NULL)
    vty_out(vty, "Agent Address Family          %s%s",
            sflow_row->agent_addr_family, VTY_NEWLINE);
  else
    vty_out(vty, "Agent Address Family          NOT SET%s",
            VTY_NEWLINE);

  if(sflow_row->sampling != NULL)
    vty_out(vty, "Sampling Rate                 %lld%s",
            *(sflow_row->sampling), VTY_NEWLINE);
  else
    vty_out(vty, "Sampling Rate                 %d%s",
            SFL_DEFAULT_SAMPLING_RATE, VTY_NEWLINE);

  if(sflow_row->polling != NULL)
    vty_out(vty, "Polling Interval              %lld%s",
            sflow_row->polling, VTY_NEWLINE);
  else
    vty_out(vty, "Polling Interval              %d%s",
            SFL_DEFAULT_POLLING_INTERVAL, VTY_NEWLINE);

  if(sflow_row->header != NULL)
    vty_out(vty, "Header Size                   %lld%s",
            sflow_row->header, VTY_NEWLINE);
  else
    vty_out(vty, "Header Size                   %d%s",
            SFL_DEFAULT_HEADER_SIZE, VTY_NEWLINE);

  if(sflow_row->max_datagram != NULL)
    vty_out(vty, "Max Datagram Size             %lld%s",
            sflow_row->max_datagram, VTY_NEWLINE);
  else
    vty_out(vty, "Max Datagram Size             %d%s",
            SFL_DEFAULT_DATAGRAM_SIZE, VTY_NEWLINE);

  if(!smap_is_empty(&sflow_row->statistics))
    vty_out(vty, "Number of Samples             %s%s",
            sflow_row->statistics, VTY_NEWLINE);
  else
    vty_out(vty, "Number of Samples             0%s",
            VTY_NEWLINE);

  return CMD_SUCCESS;

}

/* This function sets/unsets the row name in the sFlow table and a reference
 * to it in the System table. As of now, we provide support for setting  sflow
 * only at the global level*/
static int sflow_set_global_status(bool status)
{
  const struct ovsrec_system *system_row = NULL;
  const struct ovsrec_sflow *sflow_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  const char *sflow_name = OVSDB_SFLOW_GLOBAL_ROW_NAME;

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  system_row = ovsrec_system_first (idl);
  if (!system_row)
    {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
     }

  sflow_row = ovsrec_sflow_first (idl);

  if(status)
    {
      if(system_row->sflow != NULL)
        {
          vty_out (vty, "\nsFlow already enabled.%s\n", VTY_NEWLINE);
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
          ovsrec_system_set_sflow (system_row, sflow_row);
        }
     }
   else
     {
       if(sflow_row && system_row->sflow != NULL)
         ovsrec_system_set_sflow (system_row, NULL);
       else
         {
           vty_out (vty, "\nsFlow already disabled.%s\n", VTY_NEWLINE);
           cli_do_config_abort (status_txn);
           return CMD_SUCCESS;
         }
     }
  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    return CMD_SUCCESS;
  else
    {
       VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
       return CMD_OVSDB_FAILURE;
    }
}

/* This function sets/unsets the sflow sampling rate provided by the user */
static int
sflow_set_sampling_rate( int64_t *rate )
{
  const struct ovsrec_system *system_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  const struct ovsrec_sflow *sflow_row = NULL;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  const char *sflow_name = OVSDB_SFLOW_GLOBAL_ROW_NAME;

  if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

  system_row = ovsrec_system_first (idl);
  if (!system_row)
    {
        VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }
  sflow_row = ovsrec_sflow_first (idl);

  if(!sflow_row && !rate)
    {
      vty_out (vty, "\nNo sFlow configuration present.%s\n", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  if(!sflow_row && rate)
    {
      sflow_row = ovsrec_sflow_insert (status_txn);
      ovsrec_sflow_set_name (sflow_row, sflow_name);
    }

  if (rate)
    ovsrec_sflow_set_sampling(sflow_row, rate, 1);
  else
    ovsrec_sflow_set_sampling(sflow_row, rate, 0);

  txn_status = cli_do_config_finish(status_txn);
  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    return CMD_SUCCESS;
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }

}


/* This function sets/unsets the collector(target) ip, port number and
 * vrf provided by the user */
static int sflow_set_collector(const char *ip, const char *port,
                               const char *vrf, bool set)
{
  const struct ovsrec_system *system_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  const struct ovsrec_sflow *sflow_row = NULL;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  char **target;
  size_t i=0,n=0,m=0;
  bool sflow_collector_match = false;
  uint64_t MAX_CMD_LEN = IPV6_ADDRESS_LENGTH + MAX_PORT_SIZE +
            VRF_NAME_MAX_LENGTH + 3;
  char cmd_str[MAX_CMD_LEN];
  char *collector_ip = NULL;
  char *collector_port = NULL;
  char temp_ip[MAX_CMD_LEN];
  char *sflow_name = OVSDB_SFLOW_GLOBAL_ROW_NAME;
  int DFLT_PORT_LEN = 5;
  char dflt_port[DFLT_PORT_LEN];
  int ret;
  struct prefix p;
  char prefix_str[256];
  char *prefix_copy = NULL;

  if (status_txn == NULL)
    {
       VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
       cli_do_config_abort (status_txn);
       return CMD_OVSDB_FAILURE;
    }

  system_row = ovsrec_system_first (idl);
  if (!system_row)
    {
       VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
       cli_do_config_abort (status_txn);
       return CMD_SUCCESS;
    }

  ret = str2prefix (ip, &p);
  if (ret <= 0)
    {
      vty_out (vty, "\n Malformed address format%s\n", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_WARNING;
    }

  memset (prefix_str, 0, sizeof(prefix_str));
  prefix2str ((const struct prefix*) &p, prefix_str, sizeof(prefix_str));
  prefix_copy = strtok(prefix_str, "/");

  if (strcmp (prefix_copy, ip))
    {
      vty_out (vty, "\nInvalid ip address. Valid address: %s\n", prefix_str);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  if(vrf != NULL)
    {
      if (strcmp(vrf, DEFAULT_VRF_NAME) != 0)
        {
           vty_out (vty, "\nOnly vrf_default is permitted.%s\n", VTY_NEWLINE);
           cli_do_config_abort (status_txn);
           return CMD_SUCCESS;
        }
   }

  sflow_row = ovsrec_sflow_first (idl);
  if(!sflow_row && set)
    {
      sflow_row = ovsrec_sflow_insert (status_txn);
      ovsrec_sflow_set_name (sflow_row, sflow_name);
    }

  if(!sflow_row && !set)
    {
      vty_out (vty, "\nNo sFlow configuration present.%s\n", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_SUCCESS;
    }

  memset(cmd_str, 0, sizeof(cmd_str));
  if (port == NULL && vrf == NULL)
    snprintf(cmd_str, MAX_CMD_LEN,"%s", ip);
  else if(port != NULL && vrf == NULL)
   snprintf(cmd_str, MAX_CMD_LEN,"%s/%s", ip, port);
  else if(port == NULL && vrf != NULL)
    snprintf(cmd_str, MAX_CMD_LEN,"%s/%s", ip, vrf);
  else
    snprintf(cmd_str, MAX_CMD_LEN,"%s/%s/%s", ip, port, vrf);

  if(set)
    {
      for (m = 0; m < sflow_row->n_targets; m++)
        {
          memset(temp_ip, 0, sizeof(temp_ip));
          strcpy(temp_ip, sflow_row->targets[m]);
          collector_ip = strtok(temp_ip, "/");
          collector_port = strtok(NULL, "/");
          if (strcmp (ip, collector_ip) == 0)
            {
              if (collector_port && port)
                {
                  if (strcmp (port, collector_port) == 0)
                    {
                      vty_out (vty, "\nsFlow collector already present%s\n",
                              VTY_NEWLINE);
                      cli_do_config_abort (status_txn);
                      return CMD_SUCCESS;
                    }
                }
            }
        }
      target = xmalloc(MAX_CMD_LEN * (sflow_row->n_targets + 1));

      for (i = 0; i <sflow_row->n_targets; i++) {
        target[i] = sflow_row->targets[i];
      }

      target[sflow_row->n_targets] = cmd_str;
      if(sflow_row)
          ovsrec_sflow_set_targets(sflow_row, target,
                                   sflow_row->n_targets + 1);
      free (target);
    }
  else
    {
      if(sflow_row->n_targets==0)
        {
          vty_out (vty, "\nNo sFlow collector present.%s\n", VTY_NEWLINE);
          cli_do_config_abort (status_txn);
          return CMD_SUCCESS;
        }
      for (i = 0; i < sflow_row->n_targets; i++)
        {
          if (strcmp (cmd_str, sflow_row->targets[i]) == 0)
            {
              sflow_collector_match = true;
              break;
            }
        }
       if (!sflow_collector_match)
         {
            vty_out (vty, "\nsFlow collector not found.%s\n", VTY_NEWLINE);
            cli_do_config_abort (status_txn);
            return CMD_SUCCESS;
         }
       target = xmalloc(MAX_CMD_LEN * sflow_row->n_targets -
                        (sizeof(sflow_row->targets[i])));

       for (i = n = 0; i < sflow_row->n_targets; i++)
         {
           if (strcmp (cmd_str, sflow_row->targets[i])!= 0)
             target[n++] = sflow_row->targets[i];
         }
       ovsrec_sflow_set_targets(sflow_row, target, n);
       free (target);
    }

  txn_status = cli_do_config_finish(status_txn);
  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
      return CMD_SUCCESS;
    }
  else
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
}

/* This function sets/unsets agent interface and its address family to be
 * used to communicate with the collector. Interface needs to be an L3
 * interface. */
static int sflow_set_agent_interface(const char *interface, const char *family,
                                     bool set)
{
  const struct ovsrec_system *system_row = NULL;
  const struct ovsrec_sflow *sflow_row = NULL;
  const struct ovsrec_port *port_row = NULL;
  const struct ovsrec_interface * intf_row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  const char *sflow_name = OVSDB_SFLOW_GLOBAL_ROW_NAME;
  bool intf_found = false;
  bool port_found = false;

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  system_row = ovsrec_system_first (idl);
  if (!system_row)
    {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_SUCCESS;
     }

  if (set)
    {
      /* Validating if the interface is a valid interface */
      OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
        {
          if (strcmp(intf_row->name, interface) == 0)
            {
              intf_found = true;
              break;
            }
        }

      if (!intf_found)
        {
          vty_out(vty, "\nInvalid interface%s\n", VTY_NEWLINE);
          cli_do_config_abort(status_txn);
          return CMD_SUCCESS;
        }

      /* Validating if the interface is an L3 interface */
      OVSREC_PORT_FOR_EACH(port_row, idl)
        {
          if(strcmp(port_row->name, interface) == 0)
            {
              port_found = true;
              break;
            }
        }

      if (!port_found)
        {
          vty_out(vty, "\nInterface %s is not L3.%s\n", interface,
                  VTY_NEWLINE);
          cli_do_config_abort(status_txn);
          return CMD_SUCCESS;
        }

      sflow_row = ovsrec_sflow_first (idl);
      if(!sflow_row)
        {
          sflow_row = ovsrec_sflow_insert (status_txn);
          ovsrec_sflow_set_name (sflow_row, sflow_name);
        }

      ovsrec_sflow_set_agent(sflow_row, interface);
      ovsrec_sflow_set_agent_addr_family(sflow_row, family);
    }
  /* Handling the "no" form of the command and setting fields to none */
  else
    {
      sflow_row = ovsrec_sflow_first (idl);
      if(!sflow_row)
        {
          vty_out (vty, "\nNo sFlow configuration present.%s\n", VTY_NEWLINE);
          cli_do_config_abort(status_txn);
          return CMD_SUCCESS;
        }
      ovsrec_sflow_set_agent(sflow_row, interface);
      ovsrec_sflow_set_agent_addr_family(sflow_row, family);
    }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
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
       "Enable sflow feature\n")
{
    return sflow_set_global_status(true);
}

DEFUN (cli_sflow_no_set_global_status,
       cli_sflow_no_set_global_status_cmd,
       "no sflow enable",
       NO_STR
       SFLOW_STR
       "Enable sflow feature\n")
{
    return sflow_set_global_status(false);
}

DEFUN (cli_flow_set_sampling_rate,
       cli_sflow_set_sampling_rate_cmd,
       "sflow sampling <1-1000000000>",
       SFLOW_STR
       "Set Sampling rate\n"
       "Sampling rate range\n")
{
    int64_t s_rate = (int64_t) atoi(argv[0]);
    return sflow_set_sampling_rate(&s_rate);
}

DEFUN (cli_sflow_no_set_sampling_rate,
       cli_sflow_no_set_sampling_rate_cmd,
       "no sflow sampling",
       NO_STR
       SFLOW_STR
       "Set sampling rate\n")
{
    return sflow_set_sampling_rate(NULL);
}



DEFUN (cli_sflow_set_collector,
       cli_sflow_set_collector_cmd,
       "sflow collector (A.B.C.D|X:X::X:X)"
       " {port <0-65535> | vrf VRF_NAME}",
       SFLOW_STR
       "Set collector ip address configuration\n"
       "IPv4 address\n"
       "IPv6 address\n"
       "Port information\n"
       "Port number (Default: 6343)\n"
       "Assign a vrf\n"
       "VRF name (Default: vrf_default)\n")
{
    return sflow_set_collector(argv[0], argv[1], argv[2], true);
}

DEFUN (cli_sflow_no_set_collector,
       cli_sflow_no_set_collector_cmd,
       "no sflow collector (A.B.C.D|X:X::X:X)"
       " {port <0-65535> | vrf VRF_NAME}",
       NO_STR
       SFLOW_STR
       "Set collector ip address configuration\n"
       "IPv4 address\n"
       "IPv6 address\n"
       "Port information\n"
       "Port number (Default: 6343)\n"
       "Assign a vrf\n"
       "VRF name (Default: vrf_default)\n")
{
    return sflow_set_collector(argv[0], argv[1], argv[2], false);
}
DEFUN (cli_sflow_set_agent_interface,
       cli_sflow_set_agent_interface_cmd,
       "sflow agent-interface INTERFACE"
       " {agent-address-family (ipv4|ipv6)}",
       SFLOW_STR
       "Set agent interface\n"
       "Interface name\n"
       "Agent address family\n"
       "IPv4 address\n"
       "IPv6 address\n")
{
    return sflow_set_agent_interface(argv[0], argv[1], true);
}

DEFUN (cli_sflow_no_set_agent_interface,
       cli_sflow_no_set_agent_interface_cmd,
       "no sflow agent-interface",
       NO_STR
       SFLOW_STR
       "Set agent interface\n")
{
    return sflow_set_agent_interface(NULL, NULL, false);
}

DEFUN (cli_sflow_show,
       cli_sflow_show_cmd,
       "show sflow",
       SHOW_STR
       SFLOW_STR)
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
