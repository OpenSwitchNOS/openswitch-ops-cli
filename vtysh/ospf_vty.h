/* OSPF CLI implementation with openswitch vtysh.
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
 * File: ospf_vty.h
 *
 * Purpose: This file contains function declarations of OSPF CLI.
 */
#ifndef _OSPF_VTY_H
#define _OSPF_VTY_H

#define OSPF_CMD_AS_RANGE "<0-4294967295>"

#define SAFE_FREE(x) if (x) {free(x);x=NULL;}

#define OSPF_FEATURE        "ospfv2"
#define NUM_OF_FEATURES     1
#define ENABLE_STR          "Enable the feature\n"
#define OSPF_CONF_STR       "Configure OSPF\n"
#define OSPF_AREA_STR       "Configure area parameters\n"
#define OSPF_AREA_RANGE     "Configure an area id as a decimal value\n"
#define OSPF_AREA_IP_STR    \
                "Specify an area id by IPv4 address notation\n"
#define OSPF_AUTH_ENABLE    "Enable authentication\n"
#define OSPF_AUTH_MD5       "Configure message-digest authentication\n"
#define OSPF_AUTH_NULL_STR  "Use null authentication\n"
#define BORDER_ROUTER_STR   "Border router information\n"
#define DETAIL_STR          "Display detailed information\n"
#define ALL_STR             "Display all the information\n"
#define OSPF_NETWORK_STR    "Configure network related information\n"
#define OSPF_NETWORK_RANGE_STR    "Configure network range\n"
#define RUNNING_CONFIG_STR        "Current running configuration\n"
#define ROUTER_SHOW_STR     "Display router information\n"
#define OSPF_ROUTER_ID_STR  "Configure router identifier for the OSPF instance\n"
#define OSPF_ROUTER_ID_VAL_STR  \
            "Configure OSPF router-id in IPv4 address notation\n"
#define OSPF_NEIGHBOR_ID_STR        "Enter the neighbor router id\n"
#define OSPF_NEIGHBOR_SHOW_STR      "OSPF neighbor information\n"
#define OSPF_HELLO_INTERVAL_STR     "Time between HELLO packets\n"
#define OSPF_HELLO_INTERVAL_VAL_STR  \
                    "Time between HELLO packets in seconds (Default: 10)\n"
#define OSPF_DEAD_INTERVAL_STR      \
                    "Interval after which a neighbor is declared dead\n"
#define OSPF_DEAD_INTERVAL_VAL_STR      \
                    "Interval value in seconds (Default: 40)\n"

#define OSPF_MAX_METRIC_STR     "OSPF maximum / infinite-distance metric\n"
#define OSPF_ROUTER_LSA_STR     \
    "Advertise own Router-LSA with infinite distance (stub router)\n"
#define OSPF_ON_STARTUP_STR     \
    "Automatically advertise stub Router-LSA on startup of OSPF\n"
#define OSPF_STARTUP_TIME_STR   \
"Time (seconds) to advertise self as stub-router\n"
#define OSPF_AUTH_KEY       "Authentication password (key)\n"
#define OSPF_AUTH_KEY_VAL   "The OSPF password (key)\n"
#define OSPF_MD5_KEY        "Message digest authentication password (key)\n"
#define OSPF_MD5_KEY_ID     "Key ID\n"
#define OSPF_MD5            "Use MD5 algorithm\n"
#define OSPF_MD5_PASSWORD   "The OSPF password (key)"
#define OSPF_NO_SUMMARY_STR "Do not inject inter-area routes into nssa\n"
#define OSPF_INTERFACE_OSPF "OSPF interface commands\n"
#define OSPF_AREA_ID_FORMAT_ADDRESS         1
#define OSPF_AREA_ID_FORMAT_DECIMAL         2

#define OSPF_DEFAULT_STR            "        "
#define OSPF_STRING_NULL            "null"
#define OSPF_SHOW_STR_LEN           25
#define OSPF_NETWORK_RANGE_LEN      25
#define OSPF_TIMER_KEY_MAX_LENGTH   80
#define OSPF_STAT_NAME_LEN          64
/* OPS_TODO : Need to check if router LSAs with 0 link are installed or not */
#define OSPF_DEFAULT_HEXA_VALUE          "0x00000000"
#define OSPF_DEFAULT_LSA_ROUTER_LINKS          0

/* Neighbor FSM states */
#define OSPF_NFSM_STATE_ATTEMPT           "Attempt"
#define OSPF_NFSM_STATE_DELETED           "Deleted"
#define OSPF_NFSM_STATE_DEPEND_UPON       "DependUpon"
#define OSPF_NFSM_STATE_DOWN              "Down"
#define OSPF_NFSM_STATE_EXSTART           "ExStart"
#define OSPF_NFSM_STATE_EXCHANGE          "Exchange"
#define OSPF_NFSM_STATE_FULL              "Full"
#define OSPF_NFSM_STATE_INIT              "Init"
#define OSPF_NFSM_STATE_LOADING           "Loading"
#define OSPF_NFSM_STATE_2_WAY             "2-Way"

#define OSPF_LSA_TYPES_CMD_STR                                                \
    "asbr-summary|external|network|router|summary|nssa-external|opaque-link|opaque-area|opaque-as"

#define OSPF_LSA_TYPES_DESC                                                   \
   "ASBR summary link states\n"                                               \
   "External link states\n"                                                   \
   "Network link states\n"                                                    \
   "Router link states\n"                                                     \
   "Network summary link states\n"                                            \
   "NSSA external link state\n"                                               \
   "Link local Opaque-LSA\n"                                                  \
   "Link area Opaque-LSA\n"                                                   \
   "Link AS Opaque-LSA\n"

#define OSPF_DEFAULT_INFO_ORIGINATE                "default_info_originate"
#define OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS         "always"
#define OSPF_DEFAULT_INFO_ORIGINATE_SET            "true"
#define OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS_SET     "true"

#define REDIST_STR "Redistribute information from another routing protocol\n"
#define CLI_REDIST_HELP_STR_OSPFD                                             \
  "Connected routes (directly attached subnet or host)\n"                     \
  "Statically configured routes\n"                                            \
  "Border Gateway Protocol (BGP)\n"

#define DEFAULT_REDIST_STR "Control distribution of default information\n"
#define DEFAULT_REDIST_ORIGINATE_STR "Distribute a default route\n"
#define DEFAULT_REDIST_ALWAYS_STR "Always advertise default route\n"

#define OSPF_DEFAULT_INSTANCE_ID 1

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

/* Macros. */
#define OSPF_GET_AREA_ID(V, STR)                                              \
{                                                                             \
    int retv;                                                                 \
    retv = ospf_str_to_area_id ((STR), &(V));                                 \
    if (retv < 0)                                                             \
    {                                                                         \
        vty_out (vty, "Invalid OSPF area ID.%s", VTY_NEWLINE);                \
        return CMD_WARNING;                                                   \
    }                                                                         \
}

#define OSPF_GET_AREA_ID_NO_BB(NAME,V,STR)                                    \
{                                                                             \
    OSPF_GET_AREA_ID(V, STR)                                                  \
    if ((V).s_addr == 0)                                                      \
    {                                                                         \
        vty_out (vty, "%% Cannot configure %s to backbone%s",                 \
                 NAME, VTY_NEWLINE);                                          \
        return CMD_WARNING;                                                   \
    }                                                                         \
}

#define OSPF_MAX_LSA_AGE    3600
#define OSPF_LSA_AGE(LSA_BIRTH, LSA_AGE)                                      \
{                                                                             \
    int64_t time_1 = time(NULL);                                              \
    (LSA_AGE) = (((time_1 -(LSA_BIRTH)) > OSPF_MAX_LSA_AGE) ?                \
                OSPF_MAX_LSA_AGE : (time_1 - (LSA_BIRTH)));                  \
}

#define OSPF_LSA_AGE_GET(LSA_BIRTH, LSA_AGE)                                  \
{                                                                             \
    int64_t time_1 = time(NULL);                                              \
    (LSA_AGE) = (time_1 -(LSA_BIRTH));                                        \
}


/* OSPF options. */
#define OSPF_OPTION_T                    0x01  /* TOS. */
#define OSPF_OPTION_E                    0x02
#define OSPF_OPTION_MC                   0x04
#define OSPF_OPTION_NP                   0x08
#define OSPF_OPTION_EA                   0x10
#define OSPF_OPTION_DC                   0x20
#define OSPF_OPTION_O                    0x40

/*OSPF flags. */
#define OSPF_FLAGS_ABR                   0x01
#define OSPF_FLAGS_ASBR                  0x02
#define OSPF_FLAGS_VL                    0x04

#define OSPF_TIME_SIZE              25
#define OSPF_OPTION_STR_MAXLEN      24
#define OSPF_FLAG_STR_MAXLEN        24


void ospf_vty_init (void);
/*Funtion to get the intervals from port table. */
int64_t
ospf_get_port_intervals(const struct ovsrec_port* port_row,
                            const char *key);
int
vtysh_init_intf_ospf_context_clients();


/* OSPF KEYS */

#define OSPF_KEY_STUB_ROUTER_STATE_ACTIVE       "stub_router_state_active"
#define OSPF_KEY_ROUTE_AREA_ID                  "area_id"
#define OSPF_KEY_ROUTE_TYPE_ABR                 "area_type_abr"
#define OSPF_KEY_ROUTE_TYPE_ASBR                "area_type_asbr"
#define OSPF_KEY_ROUTE_EXT_TYPE                 "ext_type"
#define OSPF_KEY_ROUTE_EXT_TAG                  "ext_tag"
#define OSPF_KEY_ROUTE_TYPE2_COST               "type2_cost"

#define OSPF_KEY_ROUTE_COST                     "cost"

#define OSPF_KEY_AREA_STATS_ABR_COUNT           "abr_count"
#define OSPF_KEY_AREA_STATS_ASBR_COUNT          "asbr_count"


#define OSPF_OPTION_STRING_T               "type_of_service"
#define OSPF_OPTION_STRING_E               "external_routing"
#define OSPF_OPTION_STRING_MC              "multicast"
#define OSPF_OPTION_STRING_NP              "type_7_lsa"
#define OSPF_OPTION_STRING_EA              "external_attributes_lsa"
#define OSPF_OPTION_STRING_DC              "demand_circuits"
#define OSPF_OPTION_STRING_O               "opaque_lsa"

#define OSPF_FLAGS_STRING_ABR              "area_border_router"
#define OSPF_FLAGS_STRING_ASBR             "autonomus_system_boundary_router"
#define OSPF_FLAGS_STRING_VL               "virtual_link_endpoint"

#define OSPF_PATH_TYPE_STRING_INTER_AREA       "inter_area"
#define OSPF_PATH_TYPE_STRING_INTRA_AREA       "intra_area"
#define OSPF_PATH_TYPE_STRING_EXTERNAL         "external"

#define OSPF_EXT_TYPE_STRING_TYPE1             "ext_type_1"
#define OSPF_EXT_TYPE_STRING_TYPE2             "ext_type_2"

/* OSPF Default values */
#define OSPF_HELLO_INTERVAL_DEFAULT         10
#define OSPF_DEAD_INTERVAL_DEFAULT         (4 * OSPF_HELLO_INTERVAL_DEFAULT)
#define OSPF_TRANSMIT_DELAY_DEFAULT         1
#define OSPF_RETRANSMIT_INTERVAL_DEFAULT    5
#define OSPF_ROUTE_TYPE2_COST_DEFAULT       16777215
#define OSPF_ROUTER_PRIORITY_DEFAULT        1
/* TBD - To be modified when auto reference bandwidth is supported. */
#define OSPF_DEFAULT_COST                   10

#define OSPF_AREA_STUB_DEFAULT_COST_DEFAULT 1
#endif /* _OSPF_VTY_H */
