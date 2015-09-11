/*
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
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file vlan_vty.c
 * vlan internal range <min_vlan> <max_vlan> (ascending|descending)
 * no vlan internal range
 * show vlan internal
 ***************************************************************************/

#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vlan_vty.h"
#include "vrf_vty.h"

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
        /* HALON_TODO: Generic comment. "Macro'ize" it in lib and use*/
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

DEFUN(cli_vlan_description,
    cli_vlan_description_cmd,
    "description WORD",
    VLAN_DESCRIPTION_STR
    "VLAN description\n")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    char *description = argv[0];
    int vlan_id = atoi((char *) vty->index);

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_VLAN_SET_DESCRIPTION_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            break;
        }
    }

    if (strlen(description) > VLAN_DESCRIPTION_LENGTH)
    {
        vty_out(vty, VLAN_DESCRIPTION_LENGTH_ERROR, VTY_NEWLINE);
    }

    ovsrec_vlan_set_description(vlan_row, description);
    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set description. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_VLAN_SET_DESCRIPTION_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_no_vlan_description,
    cli_no_vlan_description_cmd,
    "no description",
    NO_STR
    VLAN_DESCRIPTION_STR)
{
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) vty->index);

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_VLAN_REMOVE_DESCRIPTION_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            break;
        }
    }

    ovsrec_vlan_set_description(vlan_row, NULL);
    status = cli_do_config_finish(status_txn);
    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove description. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_VLAN_SET_DESCRIPTION_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_vlan_admin,
    cli_vlan_admin_cmd,
    "shutdown",
    "Disable the VLAN\n")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) vty->index);

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_VLAN_SHUTDOWN_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            break;
        }
    }

    ovsrec_vlan_set_admin(vlan_row, OVSREC_VLAN_ADMIN_DOWN);
    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to shutdown vlan. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_VLAN_SHUTDOWN_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_no_vlan_admin,
    cli_no_vlan_admin_cmd,
    "no shutdown",
    NO_STR
    "Disable the VLAN")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) vty->index);

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_VLAN_NO_SHUTDOWN_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            break;
        }
    }

    ovsrec_vlan_set_admin(vlan_row, OVSREC_VLAN_ADMIN_UP);
    status = cli_do_config_finish(status_txn);
    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to enable vlan. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_VLAN_SET_DESCRIPTION_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_vlan_access,
    cli_intf_vlan_access_cmd,
    "vlan access <1-4094>",
    VLAN_STR
    "Access Configuration\n"
    "The VLAN identifier")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_ACCESS_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to set access VLAN. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_ACCESS);
    int64_t* trunks = NULL;
    int trunk_count = 0;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
    int64_t* tag = NULL;
    tag = xmalloc(sizeof *vlan_port_row->tag);
    tag[0] = vlan_id;
    int tag_count = 1;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);
    free(tag);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set access vlan %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_ACCESS_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_no_vlan_access,
    cli_intf_no_vlan_access_cmd,
    "no vlan access",
    NO_STR
    VLAN_STR
    "Access Configuration\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_ACCESS_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to remove access VLAN. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL,
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) != 0)
    {
        vty_out(vty, "The interface is not in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, NULL);
    int64_t* trunks = NULL;
    int trunk_count = 0;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
    int64_t* tag = NULL;
    int tag_count = 0;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove access vlan. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_ACCESS_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_vlan_trunk_allowed,
    cli_intf_vlan_trunk_allowed_cmd,
    "vlan trunk allowed <1-4094>",
    VLAN_STR
    TRUNK_STR
    "Allowed VLANs on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to set allowed trunk VLAN. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode == NULL)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The interface is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }

    int64_t* trunks = NULL;
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        if (vlan_id == vlan_port_row->trunks[i])
        {
            vty_out(vty, "The VLAN is already allowed on the interface%s", VTY_NEWLINE);
            status = cli_do_config_finish(status_txn);
            if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
            {
                return CMD_SUCCESS;
            }
            else
            {
                VLOG_DBG("Transaction failed to set allowed trunk VLAN %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
                vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
                return CMD_SUCCESS;
            }
        }
    }

    trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks + 1));
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        trunks[i] = vlan_port_row->trunks[i];
    }
    trunks[vlan_port_row->n_trunks] = vlan_id;
    int trunk_count = vlan_port_row->n_trunks + 1;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);

    status = cli_do_config_finish(status_txn);
    free(trunks);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set allowed trunk VLAN %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_no_vlan_trunk_allowed,
    cli_intf_no_vlan_trunk_allowed_cmd,
    "no vlan trunk allowed <1-4094>",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Allowed vlans on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, n = 0, found_trunk = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, "Failed to remove trunk VLAN%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to remove trunk VLAN. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        vty_out(vty, "The interface is not in trunk mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    int64_t* trunks = NULL;
    int trunk_count = vlan_port_row->n_trunks;
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        if (vlan_id == vlan_port_row->trunks[i])
        {
            trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks - 1));
            for (i = n = 0; i < vlan_port_row->n_trunks; i++)
            {
                if (vlan_id != vlan_port_row->trunks[i])
                {
                    trunks[n++] = vlan_port_row->trunks[i];
                }
            }
            trunk_count = vlan_port_row->n_trunks - 1;
            ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
            break;
        }
    }

    if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) == 0)
    {
        if (trunk_count == 0)
        {
            ovsrec_port_set_vlan_mode(vlan_port_row, NULL);
        }
    }

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove trunk VLAN. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_vlan_trunk_native,
    cli_intf_vlan_trunk_native_cmd,
    "vlan trunk native <1-4094>",
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_ERROR,vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to add native vlan. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode == NULL)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The interface is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    }

    int64_t* tag = NULL;
    tag = xmalloc(sizeof *vlan_port_row->tag);
    tag[0] = vlan_id;
    int tag_count = 1;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);
    free(tag);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set native vlan %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_no_vlan_trunk_native,
    cli_intf_no_vlan_trunk_native_cmd,
    "no vlan trunk native",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port* vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0, n = 0, found_trunk = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to remove native VLAN. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        vty_out(vty, "The interface is not in native mode. Please verify the mode.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    int64_t* tag = NULL;
    int n_tag = 0;
    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    ovsrec_port_set_tag(vlan_port_row, tag, n_tag);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove native VLAN. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_vlan_trunk_native_tag,
    cli_intf_vlan_trunk_native_tag_cmd,
    "vlan trunk native tag",
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "Tag configuration on the trunk port\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to set native VLAN tagging. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The interface is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set native VLAN tagging. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_intf_no_vlan_trunk_native_tag,
    cli_intf_no_vlan_trunk_native_tag_cmd,
    "no vlan trunk native tag",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0, n = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    if (!check_iface_in_bridge(ifname))
    {
        vty_out(vty, "Failed to remove native VLAN tagging. Disable routing on the interface.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN. Interface is part of LAG %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (vlan_port_row == NULL)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0)
    {
        vty_out(vty, "The interface is not in native-tagged mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove native VLAN tagging. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_vlan_access,
    cli_lag_vlan_access_cmd,
    "vlan access <1-4094>",
    VLAN_STR
    "Access Configuration\n"
    "The VLAN identifier")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_ACCESS_ERROR,vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to set access VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_ACCESS);
    int64_t* trunks = NULL;
    int trunk_count = 0;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
    int64_t* tag = NULL;
    tag = xmalloc(sizeof *vlan_port_row->tag);
    tag[0] = vlan_id;
    int tag_count = 1;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);
    free(tag);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set access VLAN %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_ACCESS_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_no_vlan_access,
    cli_lag_no_vlan_access_cmd,
    "no vlan access",
    NO_STR
    VLAN_STR
    "Access Configuration\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_ACCESS_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to remove access VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) != 0)
    {
        vty_out(vty, "The LAG is not in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, NULL);
    int64_t* trunks = NULL;
    int trunk_count = 0;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
    int64_t* tag = NULL;
    int tag_count = 0;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove access VLAN. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_ACCESS_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_vlan_trunk_allowed,
    cli_lag_vlan_trunk_allowed_cmd,
    "vlan trunk allowed <1-4094>",
    VLAN_STR
    TRUNK_STR
    "Allowed vlans on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to remove access VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode == NULL)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The LAG is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }

    int64_t* trunks = NULL;
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        if (vlan_id == vlan_port_row->trunks[i])
        {
            vty_out(vty, "The VLAN is already allowed on the LAG%s", VTY_NEWLINE);
            status = cli_do_config_finish(status_txn);
            if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
            {
                return CMD_SUCCESS;
            }
            else
            {
                VLOG_DBG("Transaction failed to set allowed trunk VLAN %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
                vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
                return CMD_SUCCESS;
            }
        }
    }

    trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks + 1));
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        trunks[i] = vlan_port_row->trunks[i];
    }
    trunks[vlan_port_row->n_trunks] = vlan_id;
    int trunk_count = vlan_port_row->n_trunks + 1;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);

    status = cli_do_config_finish(status_txn);
    free(trunks);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set allowed trunk VLAN %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_no_vlan_trunk_allowed,
    cli_lag_no_vlan_trunk_allowed_cmd,
    "no vlan trunk allowed <1-4094>",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Allowed vlans on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, n = 0, found_trunk = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_ALLOWED_ERROR,vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to remove trunk VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        vty_out(vty, "The LAG is not in trunk mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    int64_t* trunks = NULL;
    int trunk_count = vlan_port_row->n_trunks;
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        if (vlan_id == vlan_port_row->trunks[i])
        {
            trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks - 1));
            for (i = n = 0; i < vlan_port_row->n_trunks; i++)
            {
                if (vlan_id != vlan_port_row->trunks[i])
                {
                    trunks[n++] = vlan_port_row->trunks[i];
                }
            }
            trunk_count = vlan_port_row->n_trunks - 1;
            ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);
            break;
        }
    }

    if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_TRUNK) == 0)
    {
        if (trunk_count == 0)
        {
            ovsrec_port_set_vlan_mode(vlan_port_row, NULL);
        }
    }

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove trunk vlan. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_vlan_trunk_native,
    cli_lag_vlan_trunk_native_cmd,
    "vlan trunk native <1-4094>",
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "The VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to add native VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            found_vlan = 1;
            break;
        }
    }

    if (found_vlan == 0)
    {
        vty_out(vty, "VLAN %d not found%s", vlan_id, VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode == NULL)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The LAG is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    }

    int64_t* tag = NULL;
    tag = xmalloc(sizeof *vlan_port_row->tag);
    tag[0] = vlan_id;
    int tag_count = 1;
    ovsrec_port_set_tag(vlan_port_row, tag, tag_count);

    status = cli_do_config_finish(status_txn);
    free(tag);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set native vlan %d. Function:%s, Line:%s", vlan_id, __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_no_vlan_trunk_native,
    cli_lag_no_vlan_trunk_native_cmd,
    "no vlan trunk native",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port* vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0, n = 0, found_trunk = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to remove native VLAN. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        vty_out(vty, "The LAG is not in native mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    int64_t* tag = NULL;
    int n_tag = 0;
    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    ovsrec_port_set_tag(vlan_port_row, tag, n_tag);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove native VLAN. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_vlan_trunk_native_tag,
    cli_lag_vlan_trunk_native_tag_cmd,
    "vlan trunk native tag",
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "Tag configuration on the trunk port\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to set native VLAN tagging. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The LAG is in access mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to set native VLAN tagging. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_lag_no_vlan_trunk_native_tag,
    cli_lag_no_vlan_trunk_native_tag_cmd,
    "no vlan trunk native tag",
    NO_STR
    VLAN_STR
    TRUNK_STR
    "Native VLAN on the trunk port\n"
    "Tag configuration on the trunk port\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int i = 0, n = 0;

    if (status_txn == NULL)
    {
        VLOG_DBG("Failed to create transaction. Function:%s, Line:%s", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *lagname = (char *) vty->index;
    if (!check_port_in_bridge(lagname))
    {
        vty_out(vty, "Failed to remove native VLAN tagging. Disable routing on the LAG.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, lagname) == 0)
        {
            vlan_port_row = port_row;
            break;
        }
    }

    if (vlan_port_row->vlan_mode != NULL &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0)
    {
        vty_out(vty, "The LAG is not in native-tagged mode%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED);
    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_DBG("Transaction failed to remove native VLAN tagging. Function:%s, Line:%s", __func__, __LINE__);
        vty_out(vty, OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_TAG_ERROR, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN(cli_show_vlan_summary,
    cli_show_vlan_summary_cmd,
    "show vlan summary",
    SHOW_STR
    SHOW_VLAN_STR
    "The summary of VLANs\n")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    int i = 0;

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "Number of existing VLANs: 0%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        i++;
    }

    vty_out(vty, "Number of existing VLANs: %d%s", i, VTY_NEWLINE);
    return CMD_SUCCESS;
}

DEFUN(cli_show_vlan,
    cli_show_vlan_cmd,
    "show vlan",
    SHOW_STR
    SHOW_VLAN_STR)
{
    const struct ovsrec_vlan *vlan_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    int i = 0;

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "No vlan is configured%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    vty_out(vty, "%s", VTY_NEWLINE);
    vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
    vty_out(vty, "VLAN    Name      Status   Reason         Reserved       Ports%s", VTY_NEWLINE);
    vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);


    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        char vlan_id[5] = { 0 };
        snprintf(vlan_id, 5, "%d", vlan_row->id);
        vty_out(vty, "%-8s", vlan_id);
        vty_out(vty, "%-10s", vlan_row->name);
        vty_out(vty, "%-9s", vlan_row->oper_state);
        vty_out(vty, "%-15s", vlan_row->oper_state_reason);
        vty_out(vty, "%-15s", vlan_row->internal_usage);
        int count = 0, print_tag = 0;
        port_row = ovsrec_port_first(idl);
        if (port_row != NULL)
        {
            OVSREC_PORT_FOR_EACH(port_row, idl)
            {
                for (i = 0; i < port_row->n_trunks; i++)
                {
                    if (vlan_row->id == port_row->trunks[i])
                    {
                        if (port_row->n_tag == 1 && *port_row->tag == vlan_row->id)
                        {
                            print_tag = 1;
                        }
                        if (count == 0)
                        {
                            vty_out(vty, "%s", port_row->name);
                            count++;
                        }
                        else
                        {
                            vty_out(vty, ", %s", port_row->name);
                        }
                    }
                }
                if (print_tag == 0 && port_row->n_tag == 1 && *port_row->tag == vlan_row->id)
                {
                    if (count == 0)
                    {
                        vty_out(vty, "%s", port_row->name);
                        count++;
                    }
                    else
                    {
                        vty_out(vty, ", %s", port_row->name);
                    }
                }
            }
        }

        vty_out(vty, "%s", VTY_NEWLINE);
    }

    return CMD_SUCCESS;
}

DEFUN(cli_show_vlan_id,
    cli_show_vlan_id_cmd,
    "show vlan <1-4094>",
    SHOW_STR
    SHOW_VLAN_STR
    "The VLAN ID")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    const struct ovsrec_vlan *temp_vlan_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    int vlan_id = atoi((char*) argv[0]);
    int i = 0;

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row == NULL)
    {
        vty_out(vty, "No vlan is configured%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (vlan_row->id == vlan_id)
        {
            temp_vlan_row = vlan_row;
            break;
        }
    }

    if (temp_vlan_row == NULL)
    {
        vty_out(vty, "VLAN %d has not been configured%s", vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    vty_out(vty, "%s", VTY_NEWLINE);
    vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
    vty_out(vty, "VLAN    Name      Status   Reason         Reserved       Ports%s", VTY_NEWLINE);
    vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);


    vty_out(vty, "%-8s", argv[0]);
    vty_out(vty, "%-10s", temp_vlan_row->name);
    vty_out(vty, "%-9s", temp_vlan_row->oper_state);
    vty_out(vty, "%-15s", temp_vlan_row->oper_state_reason);
    vty_out(vty, "%-15s", temp_vlan_row->internal_usage);
    int count = 0, print_tag = 0;
    port_row = ovsrec_port_first(idl);
    if (port_row != NULL)
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_trunks; i++)
            {
                if (vlan_row->id == port_row->trunks[i])
                {
                    if (port_row->n_tag == 1 && *port_row->tag == vlan_row->id)
                    {
                        print_tag = 1;
                    }
                    if (count == 0)
                    {
                        vty_out(vty, "%s", port_row->name);
                        count++;
                    }
                    else
                    {
                        vty_out(vty, ", %s", port_row->name);
                    }
                }
            }
            if (print_tag == 0 && port_row->n_tag == 1 && *port_row->tag == vlan_row->id)
            {
                if (count == 0)
                {
                    vty_out(vty, "%s", port_row->name);
                    count++;
                }
                else
                {
                    vty_out(vty, ", %s", port_row->name);
                }
            }
        }
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
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
    install_element(ENABLE_NODE, &cli_show_vlan_summary_cmd);
    install_element(ENABLE_NODE, &cli_show_vlan_cmd);
    install_element(ENABLE_NODE, &cli_show_vlan_id_cmd);

    install_element(VLAN_NODE, &config_exit_cmd);
    install_element(VLAN_NODE, &config_end_cmd);
    install_element(VLAN_NODE, &cli_vlan_description_cmd);
    install_element(VLAN_NODE, &cli_no_vlan_description_cmd);
    install_element(VLAN_NODE, &cli_vlan_admin_cmd);
    install_element(VLAN_NODE, &cli_no_vlan_admin_cmd);

    install_element(INTERFACE_NODE, &cli_intf_vlan_access_cmd);
    install_element(INTERFACE_NODE, &cli_intf_no_vlan_access_cmd);
    install_element(INTERFACE_NODE, &cli_intf_vlan_trunk_allowed_cmd);
    install_element(INTERFACE_NODE, &cli_intf_no_vlan_trunk_allowed_cmd);
    install_element(INTERFACE_NODE, &cli_intf_vlan_trunk_native_cmd);
    install_element(INTERFACE_NODE, &cli_intf_no_vlan_trunk_native_cmd);
    install_element(INTERFACE_NODE, &cli_intf_vlan_trunk_native_tag_cmd);
    install_element(INTERFACE_NODE, &cli_intf_no_vlan_trunk_native_tag_cmd);

    install_element(LINK_AGGREGATION_NODE, &cli_lag_vlan_access_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_no_vlan_access_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_vlan_trunk_allowed_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_no_vlan_trunk_allowed_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_vlan_trunk_native_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_no_vlan_trunk_native_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_vlan_trunk_native_tag_cmd);
    install_element(LINK_AGGREGATION_NODE, &cli_lag_no_vlan_trunk_native_tag_cmd);
}
