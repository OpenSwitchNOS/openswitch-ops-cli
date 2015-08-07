/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file vlan_vty.c
 * vlan internal range <min_vlan> <max_vlan> (ascending|descending)
 * no vlan internal range
 * show vlan internal
 ***************************************************************************/

#include "vlan_vty.h"

VLOG_DEFINE_THIS_MODULE(vtysh_vlan_cli);
extern struct ovsdb_idl *idl;

/*-----------------------------------------------------------------------------
 | Function: vlan_int_range_add
 | Responsibility: Add a vlan range to Open vSwitch table. This range is used to
 |                 assign VLAN ID's internally to L3 ports to enable L3 support
 |                 on the ASIC.
 | Parameters:
 |      min_vlan: start value of the range.
 |      max_vlan: end value of the range
 |      policy: Assignment policy for internal vlans: ascending (default),
 |              descending
 | Return:
 |      CMD_SUCCESS - Config executed successfully.
 |      CMD_OVSDB_FAILURE - DB failure.
 ------------------------------------------------------------------------------
 */
static int vlan_int_range_add(const char *min_vlan,
                              const char *max_vlan,
                              const char *policy)
{
    const struct ovsrec_open_vswitch *const_row = NULL;
    struct smap other_config;
    struct ovsdb_idl_txn *status_txn = NULL;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        /* HALON_TODO: Generic comment. "Macro'ize" it in lib and use */
        VLOG_ERR("[%s:%d]: Failed to create OVSDB transaction\n", __FUNCTION__, __LINE__);
        cli_do_config_abort(NULL);
        return CMD_OVSDB_FAILURE;
    }

    /* There will be only one row in Open_vSwitch table */
    const_row = ovsrec_open_vswitch_first(idl);

    if (!const_row) {
        /* HALON_TODO: Generic comment. "Macro'ize" it in lib and use */
        VLOG_ERR("[%s:%d]: Failed to retrieve a row from Open_vSwitch table\n",
                    __FUNCTION__, __LINE__);

        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Copy a const type to writeable other_config type and eliminate GCC
     * warnings */
    smap_clone(&other_config, &const_row->other_config);

    smap_replace(&other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN, min_vlan);
    smap_replace(&other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN, max_vlan);
    smap_replace(&other_config, OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY, policy);

    ovsrec_open_vswitch_set_other_config(const_row, &other_config);

    smap_destroy(&other_config);

    if (cli_do_config_finish(status_txn)) {
        return CMD_SUCCESS;
    } else {
        return CMD_OVSDB_FAILURE;
    }
}

/* vlan internal configuration command */
DEFUN  (cli_vlan_int_range_add,
        cli_vlan_int_range_add_cmd,
        "vlan internal range <1-4094> <1-4094> (ascending|descending)",
        VLAN_STR
        VLAN_INT_STR
        VLAN_INT_RANGE_STR
        "Start VLAN, between 1 and 4094\n"
        "End VLAN, between Start VLAN and 4094\n"
        "Assign VLANs in ascending order\n"
        "Assign VLANs in descending order\n")
{
    uint16_t    min_vlan, max_vlan;
    char vlan_policy_str[VLAN_POLICY_STR_LEN];

    /* argv[0] = min/start VLAN ID
     * argv[1] = max/end VLAN ID
     * argv[2] = ascending or descending
     */
    min_vlan = atoi(argv[0]);
    max_vlan = atoi(argv[1]);

    /* argv[2] contains user input as is (so, partial word is possible). Convert
     * it to full policy name */
    if (*argv[2] == 'a') {
        strncpy(vlan_policy_str,
                OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY_ASCENDING_DEFAULT,
                VLAN_POLICY_STR_LEN);
    } else {
        strncpy(vlan_policy_str,
                OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY_DESCENDING,
                VLAN_POLICY_STR_LEN);
    }

    /* invalid range, log an error and notify user (in CLI) */
    if (max_vlan < min_vlan) {
        vty_out(vty, "Invalid VLAN range. End VLAN must be greater or equal to start VLAN\n");
        return CMD_SUCCESS;
    }

    return vlan_int_range_add(argv[0], argv[1], vlan_policy_str);
}

/*-----------------------------------------------------------------------------
 | Function: vlan_int_range_del
 | Responsibility: Remove an internal vlan range to Open vSwitch table. Instead, insert a
 |                 default range. VLAN in this range are assigned to interfaces
 |                 in ascending order, by default.
 | Parameters:
 | Return:
 |      CMD_SUCCESS - Config executed successfully.
 |      CMD_OVSDB_FAILURE - DB failure.
 ------------------------------------------------------------------------------
 */
static int vlan_int_range_del()
{
    const struct ovsrec_open_vswitch *const_row = NULL;
    struct smap other_config;
    char min_vlan[VLAN_ID_LEN], max_vlan[VLAN_ID_LEN];
    struct ovsdb_idl_txn *status_txn = NULL;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("[%s:%d]: Failed to create OVSDB transaction\n", __FUNCTION__, __LINE__);
        cli_do_config_abort(NULL);
        return CMD_OVSDB_FAILURE;
    }

    const_row = ovsrec_open_vswitch_first(idl);

    if (!const_row) {
        VLOG_ERR("[%s:%d]: Failed to retrieve a row from Open_vSwitch table\n",
                    __FUNCTION__, __LINE__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Copy a const data type to writeable type, to 'honor' smap_clone
     * symantics and avoid GCC warnigns */
    smap_clone(&other_config, &const_row->other_config);

    snprintf(min_vlan, VLAN_ID_LEN, "%d",
                DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN_ID);

    smap_replace(&other_config,
                    OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN,
                    min_vlan);

    snprintf(max_vlan, VLAN_ID_LEN, "%d",
                DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN_ID);

    smap_replace(&other_config,
                    OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN,
                    max_vlan);

    smap_replace(&other_config,
                    OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY,
                    OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY_ASCENDING_DEFAULT);

    ovsrec_open_vswitch_set_other_config(const_row, &other_config);

    smap_destroy(&other_config);

    if (cli_do_config_finish(status_txn)) {
        return CMD_SUCCESS;
    } else {
        return CMD_OVSDB_FAILURE;
    }
}

/* Deleting vlan internal configuration. Default config takes effect */
DEFUN  (cli_vlan_int_range_del,
        cli_vlan_int_range_del_cmd,
        "no vlan internal range",
        NO_STR
        VLAN_STR
        VLAN_INT_STR
        VLAN_INT_RANGE_STR)
{
    return vlan_int_range_del();
}

/*-----------------------------------------------------------------------------
 | Function: show_vlan_int_range
 | Responsibility: Handle 'show vlan internal' command
 | Parameters:
 | Return:
 |      CMD_SUCCESS - Config executed successfully.
 |      CMD_OVSDB_FAILURE - DB failure.
 ------------------------------------------------------------------------------
 */
static int show_vlan_int_range()
{
    const struct ovsrec_open_vswitch *const_row = NULL;
    const char *policy;
    uint16_t   min_vlan, max_vlan;
    struct ovsdb_idl_txn *status_txn = NULL;

    /* VLAN info on port */
    const struct ovsrec_port *port_row = NULL;
    const char *port_vlan_str;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("[%s:%d]: Failed to create OVSDB transaction\n", __FUNCTION__, __LINE__);
        cli_do_config_abort(NULL);
        return CMD_OVSDB_FAILURE;
    }

    const_row = ovsrec_open_vswitch_first(idl);

    if (!const_row) {
        VLOG_ERR("[%s:%d]: Failed to retrieve a row from Open_vSwitch table\n",
                    __FUNCTION__, __LINE__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Get values associated with internal vlan. */
    min_vlan  = smap_get_int(&const_row->other_config,
                            OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN,
                            INTERNAL_VLAN_ID_INVALID);

    max_vlan  = smap_get_int(&const_row->other_config,
                            OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN,
                            INTERNAL_VLAN_ID_INVALID);

    policy    = smap_get(&const_row->other_config,
                         OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY);

    if ((min_vlan == INTERNAL_VLAN_ID_INVALID) ||
        (max_vlan == INTERNAL_VLAN_ID_INVALID) ||
        (policy   == NULL)) {
        /* Internal VLAN range is not explicitly configured. Use default
         * values */
        min_vlan = DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN_ID;
        max_vlan = DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN_ID;
        policy   = OPEN_VSWITCH_OTHER_CONFIG_MAP_INTERNAL_VLAN_POLICY_ASCENDING_DEFAULT;
    }

    vty_out(vty, "\nInternal VLAN range  : %d-%d\n", min_vlan, max_vlan);
    vty_out(vty, "Internal VLAN policy : %s\n", policy);
    vty_out(vty, "------------------------\n");

    vty_out(vty, "Assigned Interfaces:\n");
    vty_out(vty, "\t%-4s\t\t%-16s\n", "VLAN","Interface");
    vty_out(vty, "\t%-4s\t\t%-16s\n", "----","---------");

    OVSREC_PORT_FOR_EACH(port_row, idl) {
        port_vlan_str = smap_get(&port_row->hw_config, PORT_HW_CONFIG_MAP_INTERNAL_VLAN_ID);

        if (port_vlan_str == NULL) {
            continue;
        }

        vty_out(vty, "\t%-4s\t\t%-16s\n", port_vlan_str, port_row->name);
    }

done:
    if (cli_do_config_finish(status_txn)) {
        return CMD_SUCCESS;
    } else {
        return CMD_OVSDB_FAILURE;
    }

    return CMD_SUCCESS;
}

DEFUN  (cli_show_vlan_int_range,
        cli_show_vlan_int_range_cmd,
        "show vlan internal",
        SHOW_STR
        SHOW_VLAN_STR
        SHOW_VLAN_INT_STR)
{
    return show_vlan_int_range();
}

/*-----------------------------------------------------------------------------
 | Function: show_vlan_int_range
 | Responsibility: Handles following commands
 |      vlan internal range <start> <end> {descending}
 |      show vlan internal
 | Parameters:
 | Return:
 ------------------------------------------------------------------------------
 */
void vlan_vty_init(void)
{
    install_element(CONFIG_NODE, &cli_vlan_int_range_add_cmd);
    install_element(CONFIG_NODE, &cli_vlan_int_range_del_cmd);
    install_element(ENABLE_NODE, &cli_show_vlan_int_range_cmd);
}
