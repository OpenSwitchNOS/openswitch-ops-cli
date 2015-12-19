/* OSPF CLI implementation with openswitch vtysh.
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
 * File: ospf_vty.h
 *
 * Purpose: This file contains function declarations of OSPF CLI.
 */
#ifndef _OSPF_VTY_H
#define _OSPF_VTY_H

#include "openswitch-idl.h"
#include "ovsdb-idl.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_if.h"

#define OSPF_CMD_AS_RANGE "<0-4294967295>"

#define OSPF_FEATURE        "ospfv2"
#define NUM_OF_FEATURES     1
#define ENABLE_STR          "Enable the feature\n"
#define OSPF_CONF_STR       "Configure OSPF\n"
#define OSPF_AREA_STR       "Configure area related information\n"
#define OSPF_AREA_RANGE     "Enter an area id\n"
#define OSPF_AREA_IP_STR    "Enter an area id in dotted decimal format\n"
#define BORDER_ROUTER_STR   "Border router information\n"
#define DETAIL_STR          "Display detailed information\n"
#define ALL_STR             "Display all the information\n"
#define OSPF_NETWORK_STR    "Configure network related information\n"
#define OSPF_NETWORK_RANGE_STR    "Configure network range\n"
#define RUNNING_CONFIG_STR        "Current running configuration\n"
#define ROUTER_SHOW_STR     "Display router information\n"


#define OSPF_DEFAULT_STR    "0.0.0.0"
#define OSPF_STRING_NULL    "null"

#define OSPF_SHOW_STR_LEN   25

#define OSPF_NETWORK_RANGE_LEN  25

/*
** depending on the outcome of the db transaction, returns
** the appropriate value for the cli command execution.
*/
inline static int
cli_command_result (enum ovsdb_idl_txn_status status)
{
    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED)) {
        return CMD_SUCCESS;
    }
    return CMD_WARNING;
}
/********************** standard database txn operations ***********************/

#define OSPF_START_DB_TXN(txn)                                  \
    do {                                                        \
        txn = cli_do_config_start();                            \
        if (txn == NULL) {                                      \
            VLOG_ERR(OVSDB_TXN_CREATE_ERROR);                   \
            cli_do_config_abort(txn);                           \
            return CMD_OVSDB_FAILURE;                           \
        }                                                       \
    } while (0)

#define OSPF_END_DB_TXN(txn)                              \
    do {                                                  \
        enum ovsdb_idl_txn_status status;                 \
        status = cli_do_config_finish(txn);               \
        return cli_command_result(status);                \
    } while (0)

#define OSPF_ERRONEOUS_DB_TXN(txn, error_message)                   \
    do {                                                            \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, " %s\n", error_message);                       \
        return CMD_WARNING;                                         \
    } while (0)

/* used when NO error is detected but still need to terminate */
#define OSPF_ABORT_DB_TXN(txn, message)                             \
    do {                                                            \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, " %s\n", message);                             \
        return CMD_SUCCESS;                                         \
    } while (0)

/********************** helper routines which find things ***********************/


#define OSPF_IP_STRING_CONVERT(string, ip) snprintf(string, 15, "%i.%i.%i.%i",\
                                          ((ip) >> 24) & 0xFF,               \
                                          ((ip) >> 16) & 0xFF,               \
                                          ((ip) >> 8) & 0xFF,                \
                                          ((ip) & 0xFF));


/* OSPF options. */
#define OSPF_OPTION_T                    0x01  /* TOS. */
#define OSPF_OPTION_E                    0x02
#define OSPF_OPTION_MC                   0x04
#define OSPF_OPTION_NP                   0x08
#define OSPF_OPTION_EA                   0x10
#define OSPF_OPTION_DC                   0x20
#define OSPF_OPTION_O                    0x40


#define OSPF_TIME_SIZE              25
#define OSPF_OPTION_STR_MAXLEN      24

void ospf_vty_init (void);

#endif /* _OSPF_VTY_H */
