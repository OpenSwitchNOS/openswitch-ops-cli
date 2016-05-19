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
#include <arpa/inet.h>
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
| Parameters : type : To specify the type of the protocol
| Return : CMD_SUCCESS for success , CMD_OVSDB_FAILURE for failure
-----------------------------------------------------------------------------*/
static int
show_source_interface_selection(source_interface_protocol type)
{
    const struct ovsrec_system *row = NULL;
    char *buff = NULL;

    row = ovsrec_system_first(idl);
    if (!row) {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    vty_out(vty, "%sSource-interface Configuration Information %s",
                  VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "---------------------------------------- %s", VTY_NEWLINE);
    vty_out(vty, "Protocol        Source Interface %s", VTY_NEWLINE);
    vty_out(vty, "--------        ---------------- %s", VTY_NEWLINE);

    switch (type) {
    case ALL_PROTOCOL:
        /*
        * To display the source interface details for
        * all the specified protocols
        */
        buff = (char *)smap_get(&row->other_config,
                                SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
        if (buff == NULL) {
            buff = (char *)smap_get(&row->other_config,
                                    SYSTEM_OTHER_CONFIG_MAP_PROTOCOLS_SOURCE);
            if (buff == NULL) {
                vty_out(vty, " %-15s %-6s ", "tftp","");
            } else {
                vty_out(vty, " %-15s %-46s ", "tftp", buff);
            }
            vty_out(vty, "%s", VTY_NEWLINE);
        } else {
            vty_out(vty, " %-15s %-46s ", "tftp", buff);
            vty_out(vty, "%s", VTY_NEWLINE);
        }
        break;

    case TFTP_PROTOCOL:
        /* To display the source interface details for TFTP protocol */
        buff = (char *)smap_get(&row->other_config,
                                SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
        if (buff == NULL) {
            vty_out(vty, " %-15s %-6s ","tftp", "");
            vty_out(vty, "%s", VTY_NEWLINE);
        } else {
            vty_out(vty, " %-15s %-46s", "tftp", buff);
            vty_out(vty, "%s", VTY_NEWLINE);
        }
        break;

    default:
        break;
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Name : source_interface_selection
| Responsibility : To set and reset the source interface to TFTP server and all
|                  the specified protocols in ovsdb system
|                  table other config column
| Parameters : const char* source : Stores the source-interface
|              type : To specify the type of the protocol
|              add : If value is true, set source-interface
|                    else unset source-interface
| Return : CMD_SUCCESS for success , CMD_OVSDB_FAILURE for failure
-----------------------------------------------------------------------------*/
static int
source_interface_selection(const char *source,
                           source_interface_protocol type,
                           bool add)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char *buff = NULL;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs_row = ovsrec_system_first(idl);
    if (!ovs_row) {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &ovs_row->other_config);

    switch (type) {
    case ALL_PROTOCOL:
        /*
        * To set the source interface to all the specified
        * protocols in ovsdb system table other config column.
        */
        if (add) {
            smap_replace(&smap,
                         SYSTEM_OTHER_CONFIG_MAP_PROTOCOLS_SOURCE, source);
            smap_replace(&smap, SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE, source);
        } else {
            buff = (char *)smap_get(&ovs_row->other_config,
                                SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
            if (buff != NULL) {
                smap_remove(&smap, SYSTEM_OTHER_CONFIG_MAP_PROTOCOLS_SOURCE);
                smap_remove(&smap, SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
            } else {
                vty_out(vty, "No source interface was configured for all the "
                        "defined protocols. %s", VTY_NEWLINE);
            }
        }
        break;

    case TFTP_PROTOCOL:
        /*
        * To set the source interface to TFTP server
        * in ovsdb system table other config column.
        */
        if (add) {
            smap_replace(&smap,
                         SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE, source);
        } else {
            buff = (char *)smap_get(&ovs_row->other_config,
                                SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
            if (buff != NULL) {
                smap_remove(&smap, SYSTEM_OTHER_CONFIG_MAP_TFTP_SOURCE);
            } else {
                vty_out(vty, "No source interface was configured for tftp "
                        "protocol. %s", VTY_NEWLINE);
            }
        }
        break;

    default :
        break;
    }

    ovsrec_system_set_other_config(ovs_row, &smap);
    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    }
    else {
        return CMD_OVSDB_FAILURE;
    }
}
/*-----------------------------------------------------------------------------
| Defun for source IP interface
| Responsibility : Configure source IP interface
-----------------------------------------------------------------------------*/
DEFUN(ip_source_interface,
      ip_source_interface_cmd,
      "ip source-interface (tftp | all) interface IFNAME ",
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING
      INTERFACE_STR
      IFNAME_STR)
{
    if (strcmp(TFTP, (char*)argv[0]) == 0) {
        return source_interface_selection(argv[1], TFTP_PROTOCOL, 1);
    } else if (strcmp(ALL, (char*)argv[0]) == 0){
        return source_interface_selection(argv[1], ALL_PROTOCOL, 1);
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for source IP address
| Responsibility : Configure source IP address
-----------------------------------------------------------------------------*/
DEFUN(ip_source_address,
      ip_source_address_cmd,
      "ip source-interface (tftp | all) address A.B.C.D",
      IP_STR
      SOURCE_STRING
      TFTP_STRING
      ALL_STRING
      ADDRESS_STRING
      ADDRESS_STRING)
{
    struct in_addr addr;
    memset (&addr, 0, sizeof (struct in_addr));
    inet_pton (AF_INET, (char*)argv[1], &addr);
    if (!IS_VALID_IPV4(htonl(addr.s_addr)))
    {
        vty_out(vty,"Broadcast, multicast and "
                "loopback addresses are not allowed.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(TFTP, (char*)argv[0]) == 0) {
        return source_interface_selection(argv[1], TFTP_PROTOCOL, 1);
    } else if (strcmp(ALL, (char*)argv[0]) == 0) {
        return source_interface_selection(argv[1], ALL_PROTOCOL, 1);
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for no source interface
| Responsibility : Unset source interface
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
    if (strcmp(TFTP, (char*)argv[0]) == 0) {
        return source_interface_selection(NULL, TFTP_PROTOCOL, 0);
    } else if (strcmp(ALL, (char*)argv[0]) == 0) {
        return source_interface_selection(NULL, ALL_PROTOCOL, 0);
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for show source interface
| Responsibility :Displays the source interface configuration
-----------------------------------------------------------------------------*/
DEFUN(show_source_interface,
      show_source_interface_cmd,
      "show ip source-interface {tftp} ",
      SHOW_STR
      IP_STR
      SOURCE_STRING
      TFTP_STRING)
{
    if(argv[0]) {
        if (strcmp(TFTP, (char*)argv[0]) == 0) {
            return show_source_interface_selection(TFTP_PROTOCOL);
        }
    }
    return show_source_interface_selection(ALL_PROTOCOL);
}

/* Install source interface related vty commands */
void
source_interface_selection_vty_init(void)
{
    install_element (CONFIG_NODE, &ip_source_interface_cmd);
    install_element (CONFIG_NODE, &ip_source_address_cmd);
    install_element (ENABLE_NODE, &show_source_interface_cmd);
    install_element (CONFIG_NODE, &no_ip_source_interface_cmd);
}
