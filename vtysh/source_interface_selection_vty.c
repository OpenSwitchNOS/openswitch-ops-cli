/* Source Interface Selection CLI commands
 *
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
 * File: source_interface_selection_vty.c
 *
 * Purpose: To add source interface CLI commands.
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
#include <stdbool.h>
#include <stdlib.h>
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "prefix.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "source_interface_selection_vty.h"

VLOG_DEFINE_THIS_MODULE (source_interface_selection_vty);

extern struct ovsdb_idl *idl;

/*----------------------------------------------------------------------------
| Name : show_source_interface_selection
| Responsibility : To display the source interface details
-----------------------------------------------------------------------------*/
static int show_source_interface_selection(source_interface_arguments type)
{
    const struct ovsrec_system *row = NULL;
    char *buff = NULL;

    row = ovsrec_system_first(idl);
    if (!row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_OVSDB_FAILURE;
    }

    vty_out(vty, "%sSource-interface Configuration Information %s %s",
                  VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "Protocol        Source Interface%s", VTY_NEWLINE);
    vty_out(vty, "--------        ----------------%s", VTY_NEWLINE);

    switch(type)
    {
        case ALL_PROTOCOL:
        {
            /*
             * To display the source interface details for
             *all the defined server protocols
            */
            buff = (char *)smap_get(&row->other_config,
                                    "source_interface_tftp");
            if (buff == NULL) {
                buff = (char *)smap_get(&row->other_config,
                                        "source_interface_all");
                if(buff==NULL) {
                    vty_out(vty, "%-15s %-6s ", "tftp", "N/A");
                }
                else {
                    vty_out(vty, "%-15s %-46s ", "tftp", buff);
                }
            vty_out(vty, "%s", VTY_NEWLINE);
            }
            else{
                vty_out(vty, "%-15s %-46s ", "tftp", buff);
                vty_out(vty, "%s", VTY_NEWLINE);
            }
            break;
        }

        case TFTP_PROTOCOL:
        {
            /* To display the source interface details for tftp protocol */
            buff = (char *)smap_get(&row->other_config,
                                    "source_interface_tftp");
            if(buff == NULL) {
                vty_out(vty, "%-15s %-6s ","tftp", "N/A");
            }
            else{
                vty_out(vty, "%-15s %-46s", "tftp", buff);
                vty_out(vty, "%s", VTY_NEWLINE);
            }
            break;
        }

        default : break;
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Name : source_interface_selection
| Responsibility : To set the source interface to tftp server
|                  in ovsdb system table other config column
| Parameters : const char* soource : pointer to char
|              add : bool type
| Return : CMD_SUCCESS for success , CMD_OVSDB_FAILURE for failure
-----------------------------------------------------------------------------*/
static int source_interface_selection(const char *source,
                                      source_interface_arguments type,
                                      bool add)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    /*
     * OPS_TODO:
     *   Move all cli_do_config_start and system row to a
     *   separate function or macro and call it from each function.
     */
    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs_row = ovsrec_system_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &ovs_row->other_config);
    switch(type)
    {
        case ALL_PROTOCOL:
        {
            /*
            * To set the source interface to all the specified
            * server protocols in ovsdb system table other config column.
            */
            if (add) {
                smap_replace(&smap, "source_interface_all", source);
                smap_replace(&smap, "source_interface_tftp", source);
            }
            else {
                smap_replace(&smap, "source_interface_all", "N/A");
                smap_replace(&smap, "source_interface_tftp", "N/A");
            }
            break;
        }

        case TFTP_PROTOCOL:
        {
             /*
            * To set the source interface to tftp server
            * in ovsdb system table other config column.
            */
            if (add) {
                smap_replace(&smap, "source_interface_tftp", source);
            }
            else {
                smap_replace(&smap, "source_interface_tftp", "N/A");
            }
            break;
        }

        default : break;
    }

    ovsrec_system_set_other_config(ovs_row, &smap);
    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        return CMD_SUCCESS;
        }
    else if (status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
        }
    else {
        return CMD_OVSDB_FAILURE;
        }
}
/*-----------------------------------------------------------------------------
| Defun for source IP interface
| Responsibility : Configure sorce IP interface
-----------------------------------------------------------------------------*/
DEFUN(ip_source_interface,
      ip_source_interface_cmd,
      "ip source-interface (tftp | all) interface IFNAME ",
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING
      INTERFACE_STRING
      INTERFACE_INPUT)
{
    if (strcmp(TFTP, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], TFTP_PROTOCOL, 1);
    }

	else if (strcmp(ALL, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], ALL_PROTOCOL, 1);
    }
}

/*-----------------------------------------------------------------------------
| Defun for source IP address
| Responsibility : Configure sorce IP address
-----------------------------------------------------------------------------*/
DEFUN(ip_source_address,
      ip_source_address_cmd,
      "ip source-interface (tftp|all) address A.B.C.D",
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING
      ADDRESS_STRING
      ADDRESS_INPUT)
{
    struct in_addr addr;
    if (inet_pton(AF_INET, (char*)argv[1], &addr) <= 0)
    {
        vty_out (vty, "Invalid IPv4 address. %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(TFTP, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], TFTP_PROTOCOL, 1);
    }

    else if (strcmp(ALL, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], ALL_PROTOCOL, 1);
    }
}

/*-----------------------------------------------------------------------------
| Defun for no source interface
| Responsibility : Unset sorce interface
-----------------------------------------------------------------------------*/
DEFUN(no_ip_source_interface,
      no_ip_source_interface_cmd,
      "no ip source-interface (tftp | all) ",
      NO_STR
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING)
{
    if (strcmp(TFTP, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], TFTP_PROTOCOL, 0);
    }

    else if (strcmp(ALL, (char*)argv[0]) == 0)
    {
        return source_interface_selection(argv[1], ALL_PROTOCOL, 0);
    }
}

/*-----------------------------------------------------------------------------
| Defun for show source IP
| Responsibility :Displays the sorce IP configuration
-----------------------------------------------------------------------------*/
DEFUN(show_source_ip,
      show_source_ip_cmd,
      "show ip source-interface (tftp|all) ",
      SHOW_STR
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING)
{
    if (strcmp(TFTP, (char*)argv[0]) == 0)
    {
        return show_source_interface_selection(TFTP_PROTOCOL);
    }

    else if (strcmp(ALL, (char*)argv[0]) == 0)
    {
        return show_source_interface_selection(ALL_PROTOCOL);
    }
}

/* Install source interface related vty commands */
void
source_interface_selection_vty_init(void)
{
    install_element (CONFIG_NODE, &ip_source_interface_cmd);
    install_element (CONFIG_NODE, &ip_source_address_cmd);
    install_element (ENABLE_NODE, &show_source_ip_cmd);
    install_element (CONFIG_NODE, &no_ip_source_interface_cmd);
}
