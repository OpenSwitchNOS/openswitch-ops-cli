/* Interface CLI commands
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: intf_vty.c
 *
 * Purpose:  To add Interface/Port related configuration and display commands.
 */

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "intf_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/mgmt_intf_vty.h"

VLOG_DEFINE_THIS_MODULE(vtysh_interface_cli);
extern struct ovsdb_idl *idl;

#define INTF_NAME_SIZE 50

/*
 * CLI "shutdown"
 * defatult : enabled
 */
DEFUN (cli_intf_shutdown,
      cli_intf_shutdown_cmd,
      "shutdown",
      "Enable/disable an interface\n")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         if(vty_flags & CMD_FLAG_NO_CMD)
         {
            smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_ADMIN,
                  OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);
         }
         else
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_ADMIN);
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_shutdown,
      cli_intf_shutdown_cmd,
      "shutdown",
      "Enable/disable an interface\n");


/*
 * CLI "speed"
 * defatult : auto
 */
DEFUN (cli_intf_speed,
      cli_intf_speed_cmd,
      "speed (auto|1000|10000|100000|40000)",
      "Configure the interface speed\nAuto negotiate speed\n"
      "1Gb/s\n10Gb/s\n100Gb/s\n40Gb/s")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         if(vty_flags & CMD_FLAG_NO_CMD)
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_SPEEDS);
         }
         else
         {
            if(strcmp(INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT, argv[0]) == 0)
            {
               smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_SPEEDS);
            }
            else
            {
               smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_SPEEDS,
                     argv[0]);
            }
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_speed,
      cli_intf_speed_cmd,
      "speed",
      "Enter the interface speed\n");


/*
 * CLI "mtu"
 * defatult : auto
 */
DEFUN (cli_intf_mtu,
      cli_intf_mtu_cmd,
      "mtu (auto|<576-16360>)",
      "Configure mtu for the interface\nUse Default MTU (1500 bytes)\nEnter MTU (in bytes)\n")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         if(vty_flags & CMD_FLAG_NO_CMD)
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_MTU);
         }
         else
         {
            if(strcmp(INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT, argv[0]) == 0)
            {
               smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_MTU);
            }
            else
            {
               smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_MTU,
                     argv[0]);
            }
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_mtu,
      cli_intf_mtu_cmd,
      "mtu",
      "Configure mtu for the interface\n");


/*
 * CLI "duplex"
 * defatult : full
 */
DEFUN (cli_intf_duplex,
      cli_intf_duplex_cmd,
      "duplex (half|full)",
      "Configure the interface duplex mode\nConfigure half-duplex\nConfigure full-duplex")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         if((vty_flags & CMD_FLAG_NO_CMD)
               || (strcmp(argv[0], "full") == 0))
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_DUPLEX);
         }
         else
         {
            smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_DUPLEX,
                  INTERFACE_USER_CONFIG_MAP_DUPLEX_HALF);
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_duplex,
      cli_intf_duplex_cmd,
      "duplex",
      "Configure the interface duplex mode\n");


/*
 * CLI "flowcontrol"
 * defatult : off
 */
DEFUN (cli_intf_flowcontrol,
      cli_intf_flowcontrol_cmd,
      "flowcontrol (receive|send) (off|on)",
      "Configure interface flow control\n"
      "Receive pause frames\nSend pause frames\n"
      "Turn off flow-control\nTurn on flow-control\n")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn* status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         const char *state_value = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_PAUSE);
         char new_value[INTF_NAME_SIZE] = {0};

         if (strcmp(argv[0], "send") == 0)
         {
            if(strcmp(argv[1], "on") == 0)
            {
               if ((NULL == state_value)
                     || (strcmp(state_value, INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0))
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_TX);
               }
               else
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RXTX);
               }
            }
            else /* both "flowcontrol send off" and "no flowcontrol send "*/
            {
               if((NULL == state_value) ||
                     (strcmp(state_value, INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0))
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE);
               }
               else
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RX);
               }
            }
         }
         else /* flowcontrol receive */
         {
            if(strcmp(argv[1], "on") == 0)
            {

               if ((NULL == state_value)
                     || (strcmp(state_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0))
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RX);
               }
               else
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RXTX);
               }
            }
            else /* both "flowcontrol receive off" and "no flowcontrol receive" */
            {
               if((NULL == state_value) ||
                     (strcmp(state_value, INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0))
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE);
               }
               else
               {
                  strcpy(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_TX);
               }
            }
         }

         if(strcmp(new_value, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) == 0)
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_PAUSE);
         }
         else
         {
            smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_PAUSE,
                  new_value);
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_flowcontrol,
      cli_intf_flowcontrol_cmd,
      "flowcontrol (receive|send)",
      "Configure interface flow control\n"
      "Receive pause frames\nSend pause frames\n");



/*
 * CLI "autonegotiation"
 * defatult : default
 */
DEFUN (cli_intf_autoneg,
      cli_intf_autoneg_cmd,
      "autonegotiation (on|off)",
      "Configure auto-negotiation process for the interface\n"
      "Turn on autonegotiation\nTurn off autonegotiation\n")
{
   struct ovsrec_interface * row = NULL;
   struct ovsdb_idl_txn *status_txn = cli_do_config_start();
   enum ovsdb_idl_txn_status status;

   if(status_txn == NULL)
   {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   row = ovsrec_interface_first(idl);
   if(!row)
   {
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(strcmp(row->name, (char*)vty->index) == 0)
      {
         if(vty_flags & CMD_FLAG_NO_CMD)
         {
            smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG);
         }
         else
         {
            if(strcmp(INTERFACE_USER_CONFIG_MAP_AUTONEG_DEFAULT, argv[0]) == 0)
            {
               smap_remove(&row->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG);
            }
            else
            {
               smap_replace(&row->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG,
                     argv[0]);
            }
         }
         ovsrec_interface_set_user_config(row, &row->user_config);
         break;
      }
   }

   status = cli_do_config_finish(status_txn);

   if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
   {
      return CMD_SUCCESS;
   }
   else
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
   }

   return CMD_OVSDB_FAILURE;
}

DEFUN_NO_FORM (cli_intf_autoneg,
      cli_intf_autoneg_cmd,
      "autonegotiation",
      "Configure autonegotiation process\n");

#define PRINT_INT_HEADER_IN_SHOW_RUN if(!bPrinted) \
{ \
   bPrinted = true;\
   vty_out (vty, "Interface %s %s", row->name, VTY_NEWLINE);\
}


int cli_show_run_interface_exec (struct cmd_element *self, struct vty *vty,
      int flags, int argc, const char *argv[])
{
   struct ovsrec_interface *row = NULL;
   const char *cur_state =NULL;
   bool bPrinted = false;

   OVSREC_INTERFACE_FOR_EACH(row, idl)
   {
      if(0 != argc)
      {
         if((NULL != argv[0]) && (0 != strcmp(argv[0], row->name)))
         {
            continue;
         }
      }
      bPrinted = false;
      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_ADMIN);
      if ((NULL != cur_state)
            && (strcmp(cur_state, OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP) == 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         vty_out(vty, "   no shutdown %s", VTY_NEWLINE);
      }

      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_SPEEDS);
      if ((NULL != cur_state)
            && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_SPEEDS_DEFAULT) != 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         vty_out(vty, "   speed %s %s", cur_state, VTY_NEWLINE);
      }

      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_MTU);
      if ((NULL != cur_state)
            && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_MTU_DEFAULT) != 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         vty_out(vty, "   mtu %s %s", cur_state, VTY_NEWLINE);
      }

      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_DUPLEX);
      if ((NULL != cur_state)
            && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_DUPLEX_FULL) != 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         vty_out(vty, "   duplex %s %s", cur_state, VTY_NEWLINE);
      }

      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_PAUSE);
      if ((NULL != cur_state)
            && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) != 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN;
         if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0)
         {
            vty_out(vty, "   flowcontrol receive on %s", VTY_NEWLINE);
         }
         else if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0)
         {
            vty_out(vty, "   flowcontrol send on %s", VTY_NEWLINE);
         }
         else
         {
            vty_out(vty, "   flowcontrol receive on %s", VTY_NEWLINE);
            vty_out(vty, "   flowcontrol send on %s", VTY_NEWLINE);
         }

         if(0 != argc)  /* filter applied, break - as data printed */
         {
            break;
         }
      }

      cur_state = smap_get(&row->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG);
      if ((NULL != cur_state)
            && (strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_AUTONEG_DEFAULT) != 0))
      {
         PRINT_INT_HEADER_IN_SHOW_RUN
         vty_out(vty, "   autonegotiation %s %s", cur_state, VTY_NEWLINE);
      }
      if(bPrinted)
      {
         vty_out(vty, "   exit%s", VTY_NEWLINE);
      }
   }

   return CMD_SUCCESS;
}

DEFUN (cli_intf_show_run_intf,
      cli_intf_show_run_intf_cmd,
      "show running-config interface",
      SHOW_STR
      "Current running configuration\n"
      INTERFACE_STR)
{
   return cli_show_run_interface_exec (self, vty, vty_flags, 0, 0);
}

DEFUN (cli_intf_show_run_intf_if,
      cli_intf_show_run_intf_if_cmd,
      "show running-config interface IFNAME",
      SHOW_STR
      "Current running configuration\n"
      INTERFACE_STR
      IFNAME_STR)
{
   return cli_show_run_interface_exec (self, vty, vty_flags, 1, argv);
}

int cli_show_run_interface_mgmt_exec (struct cmd_element *self, struct vty *vty)
{
    const struct ovsrec_open_vswitch *vswrow;
    const char *data = NULL;
    const char *ip = NULL;
    const char *subnet = NULL;
    const char *dns_1 = NULL;
    const char *dns_2 = NULL;

   vswrow = ovsrec_open_vswitch_first(idl);
   if(!vswrow)
   {
       VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
       return CMD_OVSDB_FAILURE;
   }

   data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
   if (!data)
   {
       /* If not present then mode is dhcp. So nothing to display since dhcp is the default. */
       return e_vtysh_ok;
   }

   if (VTYSH_STR_EQ(data, OPEN_VSWITCH_MGMT_INTF_MAP_MODE_STATIC))
   {
       vty_out(vty, "%s%s", "interface mgmt", VTY_NEWLINE);
       ip = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_IP);
       subnet = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
       if (ip && subnet && (strcmp(ip,MGMT_INTF_DEFAULT_IP) != 0) )
           vty_out(vty, "%4sip static %s/%s%s","",ip,subnet, VTY_NEWLINE);

       ip = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
       if (ip && (strcmp(ip,MGMT_INTF_DEFAULT_IPV6) != 0))
       {
           vty_out(vty, "%4sip static %s %s%s","",ip,subnet, VTY_NEWLINE);
       }
   }
   else
       return CMD_SUCCESS;

   data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
   if (data && (strcmp(data,MGMT_INTF_DEFAULT_IP) != 0))
   {
       vty_out(vty, "%4sdefault-gateway %s%s","",data, VTY_NEWLINE);
   }

   /* Ipv6 show running commands */

   data = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
   if (data && (strcmp(data,MGMT_INTF_DEFAULT_IPV6) != 0))
   {
       vty_out(vty, "%4sdefault-gateway %s%s","",data, VTY_NEWLINE);
   }

   dns_1 = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
   dns_2 = smap_get(&vswrow->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
   if (dns_1 && dns_2 && (strcmp(dns_1,MGMT_INTF_DEFAULT_IP) != 0) &&
                                              (strcmp(dns_2,MGMT_INTF_DEFAULT_IP) != 0))
   {
          vty_out(vty, "%4snameserver %s %s%s","", dns_1,dns_2, VTY_NEWLINE);
   }else if(dns_1 && (strcmp(dns_1,MGMT_INTF_DEFAULT_IP) != 0)) {
          vty_out(vty, "%4snameserver %s %s","", dns_1, VTY_NEWLINE);
   }

   return CMD_SUCCESS;

}

DEFUN (cli_intf_show_run_intf_mgmt,
      cli_intf_show_run_intf_mgmt_cmd,
      "show running-config interface mgmt",
      SHOW_STR
      "Current running configuration\n"
      INTERFACE_STR
      "management interface\n")
{
   return cli_show_run_interface_mgmt_exec (self, vty);
}

int cli_show_interface_exec (struct cmd_element *self, struct vty *vty,
      int flags, int argc, const char *argv[], bool brief)
{
   struct ovsrec_interface *ifrow = NULL;

   const struct ovsdb_datum *datum;
   static char *interface_statistics_keys [] = {
      "rx_packets",
      "rx_bytes",
      "tx_packets",
      "tx_bytes",
      "rx_dropped",
      "rx_frame_err",
      "rx_over_err",
      "rx_crc_err",
      "rx_errors",
      "tx_dropped",
      "collisions",
      "tx_errors"
   };

   unsigned int index;
   int64_t intVal = 0;

   if(brief)
   {
      /* Display the brief information */
      vty_out(vty, "%s", VTY_NEWLINE);
      vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
      vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason                   Speed     Port%s", VTY_NEWLINE);
      vty_out(vty, "Interface                                                          (Mb/s)    Ch#%s", VTY_NEWLINE);
      vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
   }
   else
   {
      vty_out (vty, "%s", VTY_NEWLINE);
   }

   OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
   {
      union ovsdb_atom atom;
      if((NULL != argv[0]) && (0 != strcmp(argv[0],ifrow->name)))
      {
         continue;
      }

      if(brief)
      {
         /* Display the brief information */
         vty_out (vty, " %-12s ", ifrow->name);
         vty_out(vty, "--      "); /*vVLAN */
         vty_out(vty, "eth  "); /*type */
         vty_out(vty, "--     "); /* mode - routed or not */

         vty_out (vty, "%-6s ", ifrow->link_state);

         if(strcmp(ifrow->admin_state, OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0)
         {
            vty_out (vty, "Administratively down    ");
         }
         else
         {
            vty_out (vty, "                         ");
         }
         intVal = 0;
         datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
         if(NULL!=datum) intVal = datum->keys[0].integer;
         if(intVal == 0)
         {
            vty_out(vty, " %-6s", "auto");
         }
         else
         {
            vty_out(vty, " %-6d", intVal/1000000);
         }
         vty_out(vty, "   -- ");  /* Port channel */
         vty_out (vty, "%s", VTY_NEWLINE);
      }
      else
      {
         const char *cur_state =NULL;
         intVal = 0;

         vty_out (vty, "Interface %s is %s ", ifrow->name, ifrow->link_state);
         if((NULL != ifrow->admin_state)
               && strcmp(ifrow->admin_state, OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0)
         {
            vty_out (vty, "(Administratively down) %s", VTY_NEWLINE);
            vty_out (vty, " Admin state is down%s",
                  VTY_NEWLINE);
         }
         else
         {
            vty_out (vty, "%s", VTY_NEWLINE);
            vty_out (vty, " Admin state is up%s", VTY_NEWLINE);
         }

         vty_out (vty, " Hardware: Ethernet, MAC Address: %s %s", ifrow->mac_in_use, VTY_NEWLINE);

         datum = ovsrec_interface_get_mtu(ifrow, OVSDB_TYPE_INTEGER);
         if(NULL!=datum) intVal = datum->keys[0].integer;

         vty_out(vty, " MTU %d %s", intVal, VTY_NEWLINE);

         if(strcmp(ifrow->duplex, "half") == 0)
         {
            vty_out(vty, " Half-duplex %s", VTY_NEWLINE);
         }
         else
         {
            vty_out(vty, " Full-duplex %s", VTY_NEWLINE);
         }

         intVal = 0;
         datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
         if(NULL!=datum) intVal = datum->keys[0].integer;
         vty_out(vty, " Speed %lld Mb/s %s",intVal/1000000 , VTY_NEWLINE);

         cur_state = smap_get(&ifrow->user_config, INTERFACE_USER_CONFIG_MAP_AUTONEG);
         if((NULL == cur_state) ||
               strcmp(cur_state, "off") !=0)
         {
            vty_out(vty, " Auto-Negotiation is turned on %s", VTY_NEWLINE);
         }
         else
         {
            vty_out(vty, " Auto-Negotiation is turned off %s", VTY_NEWLINE);
         }

         cur_state = ifrow->pause;
         if(NULL != cur_state)
         {
            if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) ==0)

            {
               vty_out(vty, " Input flow-control is off, output flow-control is off%s",VTY_NEWLINE);
            }
            else if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_RX) ==0)
            {
               vty_out(vty, " Input flow-control is on, output flow-control is off%s",VTY_NEWLINE);
            }
            else if(strcmp(cur_state, INTERFACE_USER_CONFIG_MAP_PAUSE_TX) ==0)
            {
               vty_out(vty, " Input flow-control is off, output flow-control is on%s",VTY_NEWLINE);
            }
            else
            {
               vty_out(vty, " Input flow-control is on, output flow-control is on%s",VTY_NEWLINE);
            }
         }
         else
         {
            vty_out(vty, " Input flow-control is off, output flow-control is off%s",VTY_NEWLINE);
         }

         datum = ovsrec_interface_get_statistics(ifrow, OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

         if(NULL==datum) continue;

         vty_out(vty, " RX%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[0];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d input packets  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         atom.string = interface_statistics_keys[1];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d bytes  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[8];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d input error    ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         atom.string = interface_statistics_keys[4];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d dropped  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[5];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d short frame    ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         atom.string = interface_statistics_keys[6];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d overrun  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[7];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d CRC/FCS  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         vty_out(vty, " TX%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[2];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d output packets ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         atom.string = interface_statistics_keys[3];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d bytes  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[11];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d input error    ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         atom.string = interface_statistics_keys[9];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d dropped  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         atom.string = interface_statistics_keys[10];
         index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
         vty_out(vty, "   %10d collision  ", (index == UINT_MAX)? 0 : datum->values[index].integer);
         vty_out(vty, "%s", VTY_NEWLINE);

         vty_out(vty, "%s", VTY_NEWLINE);

         if(NULL != argv[0])
         {
            break;
         }
      }
   }

   return CMD_SUCCESS;
}


DEFUN (cli_intf_show_intferface_ifname,
      cli_intf_show_intferface_ifname_cmd,
      "show interface IFNAME {brief}",
      SHOW_STR
      INTERFACE_STR
      IFNAME_STR
      "Show brief info of interface\n")
{
   bool brief = false;
   if((NULL != argv[1]) && strcmp(argv[1], "brief") == 0)
   {
      brief = true;
   }
   return cli_show_interface_exec (self, vty, vty_flags, argc, argv, brief);
}

DEFUN (cli_intf_show_intferface_ifname_br,
      cli_intf_show_intferface_ifname_br_cmd,
      "show interface {brief}",
      SHOW_STR
      INTERFACE_STR
      "Show brief info of interface\n")
{
   bool brief = false;
   if((NULL != argv[0]) && strcmp(argv[0], "brief") == 0)
   {
      brief = true;
   }
   argv[0]= NULL;

   return cli_show_interface_exec (self, vty, vty_flags, argc, argv, brief);
}





/* Install Interface related vty commands. */
void
intf_vty_init (void)
{
   /* Config commands */
   install_element (INTERFACE_NODE, &cli_intf_shutdown_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_shutdown_cmd);
   install_element (INTERFACE_NODE, &cli_intf_speed_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_speed_cmd);
   install_element (INTERFACE_NODE, &cli_intf_mtu_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_mtu_cmd);
   install_element (INTERFACE_NODE, &cli_intf_duplex_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_duplex_cmd);
   install_element (INTERFACE_NODE, &cli_intf_flowcontrol_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_flowcontrol_cmd);
   install_element (INTERFACE_NODE, &cli_intf_autoneg_cmd);
   install_element (INTERFACE_NODE, &no_cli_intf_autoneg_cmd);

   /* Show commands */
   install_element (ENABLE_NODE, &cli_intf_show_intferface_ifname_cmd);
   install_element (ENABLE_NODE, &cli_intf_show_intferface_ifname_br_cmd);
   install_element (ENABLE_NODE, &cli_intf_show_run_intf_cmd);
   install_element (ENABLE_NODE, &cli_intf_show_run_intf_if_cmd);
   install_element (ENABLE_NODE, &cli_intf_show_run_intf_mgmt_cmd);

   return;
}
