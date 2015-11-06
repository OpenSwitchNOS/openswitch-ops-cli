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

#define OSPF_CMD_AS_RANGE "<1-4294967295>"


#define OSPF_FEATURE        "ospfv2"
#define NUM_OF_FEATURES     1
#define ENABLE_STR          "Enable the feature"
#define OSPF_AREA_STR       "Configure area related information\n"
#define OSPF_AREA_RANGE     "Enter an area id\n"
#define OSPF_AREA_IP_STR    "Enter an area id in dottet decimal format\n"
#define BORDER_ROUTER_STR   "Border router information\n"

#define OSPF_DEFAULT_STR    "0.0.0.0"
#define OSPF_STRING_NULL    "null"
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

#define START_DB_TXN(txn)                                       \
    do {                                                        \
        txn = cli_do_config_start();                            \
        if (txn == NULL) {                                      \
            VLOG_ERR(OVSDB_TXN_CREATE_ERROR);                   \
            cli_do_config_abort(txn);                           \
            return CMD_OVSDB_FAILURE;                           \
        }                                                       \
    } while (0)

#define END_DB_TXN(txn)                                   \
    do {                                                  \
        enum ovsdb_idl_txn_status status;                 \
        status = cli_do_config_finish(txn);               \
        return cli_command_result(status);                \
    } while (0)

#define ERRONEOUS_DB_TXN(txn, error_message)                        \
    do {                                                            \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction failed: %s: %d -- %s\n", \
                __FILE__, __LINE__, error_message);                 \
        return CMD_WARNING;                                         \
    } while (0)

/* used when NO error is detected but still need to terminate */
#define ABORT_DB_TXN(txn, message)                             \
    do {                                                       \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction aborted: %s: %d, %s\n",  \
                __FILE__, __LINE__, message);                       \
        return CMD_SUCCESS;                                         \
    } while (0)

/********************** helper routines which find things ***********************/


#define OSPF_IP_STRING_CONVERT(string, ip) snprintf(string, 5, "%i.%i.%i.%i",\
                                          ((ip) >> 24) & 0xFF,               \
                                          ((ip) >> 16) & 0xFF,               \
                                          ((ip) >> 8) & 0xFF,                \
                                          ((ip) & 0xFF));

void ospf_vty_init (void);

enum ospf_spf_key_types
{
    OSPF_SPF_DELAY,
    OSPF_SPF_HOLD_TIME,
    OSPF_SPF_MAX_WAIT,
    OSPF_SPF_MAX

};

enum ospf_lsa_timer_config_types
{
    OSPF_LSA_ARRIVAL_INTERVAL,
    OSPF_LSA_GROUP_PACING,
    OSPF_LSA_START_TIME,
    OSPF_LSA_HOLD_INTERVAL,
    OSPF_LSA_MAX_DELAY,
    OSPF_LSA_MAX
};

#if 0
enum ospf_router_stub_config
{
    OSPF_ROUTER_STUB_ROUTER_CONFIG_ADMIN,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_EXT_LSA,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_INCLUDE_STUB,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP_WAITBGP,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_SUMMARY_LSA,
    OSPF_ROUTER_STUB_ROUTER_CONFIG_MAX
};
#endif

#define OSPF_ROUTER_STUB_ROUTER_CONFIG_MAX  \
                OSPF_ROUTER_STUB_ROUTER_CONFIG_SUMMARY_LSA + 1

enum ospf_area_statistics
{
    OSPF_AREA_STATISTICS_SPF_CALC,
    OSPF_AREA_STATISTICS_ABR_COUNT,
    OSPF_AREA_STATISTICS_ASBR_COUNT,
    OSPF_AREA_STATISTICS_MAX
};

#define OSPF_NUM_SPF_KEYS                   OSPF_SPF_MAX
#define OSPF_NUM_LSA_TIMER_KEYS             OSPF_LSA_MAX
#define OSPF_NUM_AREA_KEYS                  1

#define OSPF_KEY_ROUTER_ID_VAL              "router_id_val"

/* OSPF interface config */
#define OSPF_KEY_TRANSMIT_DELAY             "transmit_delay"
#define OSPF_KEY_RETRANSMIT_INTERVAL        "retransmit_interval"
#define OSPF_KEY_HELLO_INTERVAL             "hello_interval"
#define OSPF_KEY_DEAD_INTERVAL              "dead_interval"
#define OSPF_KEY_PRIORITY                   "priority"
#define OSPF_KEY_MTU_IGNORE                 "mtu_ignore"
#define OSPF_KEY_AUTH_CONF_TYPE              "auth_type"
#define OSPF_KEY_AUTH_CONF_KEY               "auth_key"
#define OSPF_KEY_INTERFACE_TYPE              "intf_type"
#define OSPF_KEY_INTERFACE_OUT_COST          "if_out_cost"
#define OSPF_KEY_HELLO_DUE                   "hello_due_at"

#define OSPF_KEY_DISTANCE_ALL                   "all"
#define OSPF_KEY_DISTANCE_EXTERNAL              "external"
#define OSPF_KEY_DISTANCE_INTRA_AREA            "intra_area"
#define OSPF_KEY_DISTANCE_INTER_AREA            "inter_area"

#define OSPF_KEY_ROUTER_ID_STATIC           "router_id_static"
#define OSPF_KEY_ROUTER_ID_VAL              "router_id_val"
#define OSPF_KEY_DEFAULT_INFO_ORIG          "default_info_originate"
#define OSPF_KEY_ALWAYS                     "always"

//SPF config
#define OSPF_KEY_SPF_DELAY                   "spf_delay"
#define OSPF_KEY_SPF_HOLD_TIME               "spf_holdtime"
#define OSPF_KEY_SPF_MAX_WAIT                "spf_max_wait"

#define OSPF_KEY_SPF_HOLD_MULTIPLIER            "spf_hold_multiplier"
#define OSPF_KEY_CAPABILITY_OPAQUE              "capability_opaque"

#define OSPF_KEY_RESTART_ENABLE_GRACEFUL        "enable_graceful_restart"
#define OSPF_KEY_RESTART_HELPER_DISABLE         "restart_helper_disable"
#define OSPF_KEY_RESTART_PLANNED_ONLY           "restart_planned_only"
#define OSPF_KEY_RESTART_INTERVAL               "restart_interval"
#define OSPF_KEY_RESTART_STRICT_LSA_CHECKING    "restart_strict_lsa_checking"


#define OSPF_KEY_ROUTER_DEFAULT_METRIC          "default_metric"
#define OSPF_KEY_AUTO_COST_REF_BW               "auto_cost_ref_bw"
#define OSPF_KEY_MAX_PATHS                      "maxpaths"
#define OSPF_KEY_LOG_ADJACENCY_CHGS             "log_adjacency_changes"
#define OSPF_KEY_LOG_ADJACENCY_DETAIL           "log_adjacency_details"
#define OSPF_KEY_RFC1583_COMPATIBLE             "ospf_rfc1583_compatible"
#define OSPF_KEY_ENABLE_OPAQUE_LSA              "enable_ospf_opaque_lsa"
#define OSPF_KEY_ENABLE_STUB_ROUTER_SUPPORT     "stub_router_support"
#define OSPF_KEY_ROUTER_STATUS_ABR              "abr"
#define OSPF_KEY_ROUTER_STATUS_ASBR             "asbr"

#define OSPF_NEIGHBOR_FSM_FULL                  "full"

// Stub router config keys
#define OSPF_KEY_ROUTER_STUB_ADMIN              "admin"
#define OSPF_KEY_ROUTER_STUB_STARTUP            "startup"
#define OSPF_KEY_ROUTER_STUB_STARTUP_WAIT       "startup_waitbgp"
#define OSPF_KEY_ROUTER_STUB_INCLUDE            "include_stub"
#define OSPF_KEY_ROUTER_STUB_EXT_LSA            "ext_lsa"
#define OSPF_KEY_ROUTER_STUB_SUMMARY_LSA        "summary_lsa"

#define OSPF_KEY_ARRIVAL_INTERVAL           "lsa_arrival_interval"
#define OSPF_KEY_LSA_GROUP_PACING           "lsa_group_pacing"
#define OSPF_KEY_LSA_START_TIME             "throttle_lsa_start_time"
#define OSPF_KEY_LSA_HOLD_INTERVAL          "throttle_lsa_hold_interval"
#define OSPF_KEY_LSA_MAX_DELAY              "throttle_lsa_max_delay"

#define OSPF_KEY_AREA_TYPE                  "area_type"
#define OSPF_KEY_AREA_NO_SUMMARY            "no_summary"
#define OSPF_KEY_AREA_STUB_DEFAULT_COST          "stub_default_cost"
#define OSPF_KEY_AREA_NSSA_TRANSLATOR_ROLE          "NSSA_translator_role"

/* Area status */
#define OSPF_KEY_AREA_ACTIVE_INTERFACE          "active_interfaces"
#define OSPF_KEY_AREA_FULL_NEIGHBORS            "full_nbrs"
#define OSPF_KEY_AREA_SPF_LAST_RUN              "spf_last_run_timestamp"
#define OSPF_KEY_AREA_ROUTER_CHKSUM             "router_lsas_sum_cksum"
#define OSPF_KEY_AREA_NETWORK_CHKSUM            "network_lsas_sum_cksum"
#define OSPF_KEY_AREA_ABR_SUMMARY_CHKSUM        "abr_summary_lsas_sum_cksum"
#define OSPF_KEY_AREA_ASBR_SUMMARY_CHKSUM       "asbr_summary_lsas_sum_cksum"
#define OSPF_KEY_AREA_NSSA_CHKSUM               "as_nssa_lsas_sum_cksum"
#define OSPF_KEY_AREA_OPAQUE_AREA_CHKSUM        "opaque_area_lsas_sum_cksum"
#define OSPF_KEY_AREA_OPAQUE_LINK_CHKSUM        "opaque_link_lsas_sum_cksum"


/* Area statistics */
#define OSPF_KEY_AREA_STATS_SPF_EXEC            "spf_calc"

#define OSPF_KEY_INTERFACE_ACTIVE               "active"



#define OSPF_PASSIVE_INTERFACE_DEFAULT      "default"
#define OSPF_ROUTER_ID_STATIC_DEFAULT       "false"
#define OSPF_DEFAULT_INFO_ORIG_DEFAULT      "false"
#define OSPF_ALWAYS_DEFAULT                 "false"

#define OSPF_RESTART_ENABLE_GRACEFUL_DEFAULT            "false"
#define OSPF_RESTART_HELPER_DISABLE_DEFAULT             "true"
#define OSPF_RESTART_PLANNED_ONLY_DEFAULT               "false"
#define OSPF_RESTART_INTERVAL_DEFAULT                   "60"
#define OSPF_RESTART_STRICT_LSA_CHECKING_DEFAULT        "true"

#define OSPF_RESTART_ROUTER_DEFAULT_METRIC_DEFAULT           "20"
#define OSPF_AUTO_COST_REF_BW_DEFAULT                        "40000"
#define OSPF_MAX_PATHS_DEFAULT                               "8"
#define OSPF_LOG_ADJACENCY_CHGS_DEFAULT                      "false"
#define OSPF_LOG_ADJACENCY_DETAIL_DEFAULT                    "false"
#define OSPF_RFC1583_COMPATIBLE_DEFAULT                      "false"
#define OSPF_ENABLE_OPAQUE_LSA_DEFAULT                       "false"

#define OSPF_LSA_ARRIVAL_INTERVAL_DEFAULT               1000
#define OSPF_LSA_GROUP_PACING_DEFAULT                   10
#define OSPF_LSA_START_TIME_DEFAULT                     1000
#define OSPF_LSA_HOLD_INTERVAL_DEFAULT                  5000
#define OSPF_LSA_MAX_DELAY_DEFAULT                      5000

#define OSPF_AREA_TYPE_DEFAULT                      "default"
#define OSPF_AREA_NO_SUMMARYDEFAULT                 "false"
#define OSPF_AREA_STUB_DEFAULT_COST                 "1"
#define OSPF_AREA_NSSA_TRANSLATOR_ROLE                 "candidate"

#define OSPF_PRIORITY_DEFAULT                       "1"


// Stub router defaults
#define OSPF_ROUTER_STUB_ADMIN_DEFAULT              0
#define OSPF_ROUTER_STUB_STARTUP_DEFAULT            0
#define OSPF_ROUTER_STUB_STARTUP_WAIT_DEFAULT       0
#define OSPF_ROUTER_STUB_INCLUDE_DEFAULT            0
#define OSPF_ROUTER_STUB_EXT_LSA_DEFAULT            0
#define OSPF_ROUTER_STUB_SUMMARY_LSA_DEFAULT        0


#endif /* _OSPF_VTY_H */
