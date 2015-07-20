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
 * @ingroup quagga/vtysh
 *
 * @file vrf_vty.c
 * VRF CLI Commands
 *
 ***************************************************************************/

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "vrf_vty.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(vtysh_vrf_cli);
extern struct ovsdb_idl *idl;

struct ovsrec_vrf* vrf_lookup(const char *vrf_name) {
    const struct ovsrec_vrf *vrf_row = NULL;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, vrf_name) == 0) {
            return vrf_row;
        }
    }
    return NULL;
}

struct ovsrec_port* port_lookup(const char *port_name) {
    const struct ovsrec_port *port_row = NULL;
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, port_name) == 0) {
            return port_row;
        }
    }
    return NULL;
}

/*
 * Checks if interface is already part of bridge.
 */
bool check_iface_in_bridge(const struct ovsrec_open_vswitch *ovs_row,
                           const char *if_name) {
    size_t i, j, k;
    for (i = 0; i < ovs_row->n_bridges; i++) {
        struct ovsrec_bridge *br_cfg = ovs_row->bridges[i];
        for (j = 0; j < br_cfg->n_ports; j++) {
            struct ovsrec_port *port_cfg = br_cfg->ports[j];
            for (k = 0; k < port_cfg->n_interfaces; k++) {
                struct ovsrec_interface *iface_cfg = port_cfg->interfaces[k];
                if (strcmp(if_name, iface_cfg->name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/*
 * Checks if interface is already part of a VRF.
 */
bool check_iface_in_vrf(const struct ovsrec_open_vswitch *ovs_row,
                        const char *if_name) {
    size_t i, j, k;
    for (i = 0; i < ovs_row->n_vrfs; i++) {
        struct ovsrec_vrf *vrf_cfg = ovs_row->vrfs[i];
        for (j = 0; j < vrf_cfg->n_ports; j++) {
            struct ovsrec_port *port_cfg = vrf_cfg->ports[j];
            for (k = 0; k < port_cfg->n_interfaces; k++) {
                struct ovsrec_interface *iface_cfg = port_cfg->interfaces[k];
                if (strcmp(if_name, iface_cfg->name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/*
 * Adds a new VRF to the VRF table.
 * Takes VRF name as an argument.
 */
static int vrf_add(const char *vrf_name) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_open_vswitch *ovs_row = NULL;
    enum ovsdb_idl_txn_status status;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    if (strlen(vrf_name) > VRF_NAME_MAX_LENGTH) {
        vty_out(vty, "Error: VRF name cannot be more than 32 characters.%s",
        VTY_NEWLINE);
        VLOG_DBG(
                "%s VRF Name can only be 32 characters. Check the VRF name"
                " \"%s\" and try again!", __func__, vrf_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    /* HALON_TODO: In case multiple vrfs. */
#if 0
    vrf_row = vrf_lookup(vrf_name);
    if(vrf_row)
    {
        vty_out (vty, "Error: VRF already exists.%s", VTY_NEWLINE);
        VLOG_DBG("%s Trying to add a VRF which is already present. Check if"
                " VRF name \"%s\" is already present.", __func__, vrf_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }
#else
    vrf_row = ovsrec_vrf_first(idl);
    if (vrf_row) {
        vty_out(vty,
                "Error: Command not supported. Default VRF already exists.%s",
                VTY_NEWLINE);
        VLOG_DBG(
                "%s Only default VRF is allowed right now. This default VRF is"
                " created as part of system bootup.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }
#endif

    ovs_row = ovsrec_open_vswitch_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch Open VSwitch data.%s",
        VTY_NEWLINE);
        VLOG_DBG(
                "%s Open VSwitch table did not have any rows. Ideally it"
                " should have just one entry.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    vrf_row = ovsrec_vrf_insert(status_txn);
    ovsrec_vrf_set_name(vrf_row, vrf_name);

    /* HALON_TODO: In case multiple vrfs. */
#if 0
    struct ovsrec_vrf **vrfs;
    size_t i;
    vrfs = xmalloc(sizeof *ovs_row->vrfs * (ovs_row->n_vrfs + 1));
    for (i = 0; i < ovs_row->n_vrfs; i++) {
        vrfs[i] = ovs_row->vrfs[i];
    }
    vrfs[ovs_row->n_vrfs] = vrf_row;
    ovsrec_open_vswitch_set_vrfs(ovs_row, vrfs, ovs_row->n_vrfs + 1);
    free(vrfs);
#else
    ovsrec_open_vswitch_set_vrfs(ovs_row, &vrf_row, ovs_row->n_vrfs + 1);
#endif

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and VRF \"%s\" was added "
                "successfully", __func__, vrf_name);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got"
                " a status response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * Adds an interface/port to a VRF.
 * Takes interface name and VRF name as arguments.
 */
static int vrf_add_port(const char *if_name, const char *vrf_name) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_interface *if_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_open_vswitch *ovs_row = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_port **ports;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction"
                " using ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = vrf_lookup(vrf_name);
    if (!vrf_row) {
        vty_out(vty, "Error: VRF %s not found.%s", vrf_name, VTY_NEWLINE);
        VLOG_DBG("%s VRF \"%s\" is not present in VRF table.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if_row = ovsrec_interface_first(idl);
    if (!if_row) {
        vty_out(vty, "Error: Could not fetch interface data.%s",
        VTY_NEWLINE);
        VLOG_DBG("%s No rows found in the Interface table", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    ovs_row = ovsrec_open_vswitch_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch Open VSwitch data.%s",
                VTY_NEWLINE);
        VLOG_DBG(
                "%s Open VSwitch table did not have any rows. Ideally it"
                " should have just one entry.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (check_iface_in_bridge(ovs_row, if_name)) {
        vty_out(vty, "Error: Interface %s has already been added to bridge.%s",
                if_name, VTY_NEWLINE);
        VLOG_DBG(
                "%s Interface \"%s\" has already been added to bridge. Check"
                " Interface and Port tables", __func__, if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (check_iface_in_vrf(ovs_row, if_name)) {
        vty_out(vty, "Error: Interface %s has already been added to vrf.%s",
                if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" has already been added to vrf.",
                __func__, if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    /* HALON_TODO: Check to see if we can directly fetch the uuid from if_name
     instead of looping through the entire interface table.
     Tried with uuid_from_string() but did not work. */
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if (strcmp(if_row->name, if_name) == 0) {
            port_row = ovsrec_port_insert(status_txn);
            ovsrec_port_set_name(port_row, if_name);
            ovsrec_port_set_interfaces(port_row, &if_row, 1);
            break;
        }
    }

    ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports + 1));
    for (i = 0; i < vrf_row->n_ports; i++) {
        ports[i] = vrf_row->ports[i];
    }
    ports[vrf_row->n_ports] = port_row;
    ovsrec_vrf_set_ports(vrf_row, ports, vrf_row->n_ports + 1);
    free(ports);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" was attached"
                " to VRF \"%s\"", __func__, if_name, vrf_name);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete a VRF and all ports linked to it.
 */
static int vrf_delete(const char *vrf_name) {
    /*
     *  HALON_TODO: Deleting VRF means to delete all references from all
     *  tables such as bgp routes, ports, interfaces, and so on. For now
     *  we will only delete ports.
     */

    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_open_vswitch *ovs_row = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i;
    struct ovsrec_port *port_row = NULL;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    if (strcmp(vrf_name, "vrf_default") == 0) {
        vty_out(vty, "Error: Cannot delete default VRF.%s", VTY_NEWLINE);
        VLOG_DBG("%s Cannot delete default VRF.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    vrf_row = vrf_lookup(vrf_name);
    if (!vrf_row) {
        vty_out(vty, "Error: VRF %s not found.%s", vrf_name, VTY_NEWLINE);
        VLOG_DBG("%s VRF \"%s\" is not found.", __func__, vrf_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    for (i = 0; i < vrf_row->n_ports; i++)
        ovsrec_port_delete(vrf_row->ports[i]);

    ovs_row = ovsrec_open_vswitch_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch Open VSwitch data.%s",
                VTY_NEWLINE);
        VLOG_DBG(
                "%s Open VSwitch table did not have any rows. Ideally it"
                " should have just one entry.", __func__);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    /* HALON_TODO: In case multiple vrfs. */
#if 0
    struct ovsrec_vrf **vrfs;
    vrfs = xmalloc(sizeof *ovs_row->vrfs * (ovs_row->n_vrfs - 1));
    for (i = 0; i < ovs_row->n_vrfs; i++) {
        if (strcmp(ovs_row->vrfs[i]->name,vrf_name) != 0)
        vrfs[i] = ovs_row->vrfs[i];
    }
    ovsrec_open_vswitch_set_vrfs(ovs_row, vrfs, ovs_row->n_vrfs - 1);
    free(vrfs);
#else
    ovsrec_open_vswitch_set_vrfs(ovs_row, NULL, 0);
#endif

    ovsrec_vrf_delete(vrf_row);
    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG("%s The command succeeded and VRF \"%s\" was deleted",
                __func__, vrf_name);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete a port linked to a VRF.
 */
static int vrf_del_port(const char *if_name, const char *vrf_name) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_open_vswitch *ovs_row = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_port **ports;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = vrf_lookup(vrf_name);
    if (!vrf_row) {
        vty_out(vty, "Error: VRF %s not found.%s", vrf_name, VTY_NEWLINE);
        VLOG_DBG("%s VRF \"%s\" is not found.", __func__, vrf_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    for (i = 0; i < vrf_row->n_ports; i++) {
        if (strcmp(vrf_row->ports[i]->name, if_name) == 0) {
            port_row = vrf_row->ports[i];
            break;
        }
    }

    if (!port_row) {
        vty_out(vty, "Error: Interface %s not attached to given VRF.%s",
                if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not attached to VRF \"%s\".",
                __func__, if_name, vrf_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    ovsrec_port_delete(port_row);

    ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports - 1));
    for (i = 0; i < vrf_row->n_ports; i++) {
        if (strcmp(vrf_row->ports[i]->name, if_name) != 0)
            ports[i] = vrf_row->ports[i];
    }
    ovsrec_vrf_set_ports(vrf_row, ports, vrf_row->n_ports - 1);
    free(ports);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface %s was detached from"
                " VRF %s", __func__, if_name, vrf_name);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to configure an IP address for a port
 * which is attached to a VRF.
 */
static int vrf_config_ip(const char *if_name, const char *ip4, bool secondary) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char **secondary_ip4_addresses;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        for (i = 0; i < vrf_row->n_ports; i++) {
            if (strcmp(vrf_row->ports[i]->name, if_name) == 0) {
                port_row = vrf_row->ports[i];
                break;
            }
        }
    }

    if (!port_row) {
        vty_out(vty, "Error: Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not attached to any VRF.", __func__,
                if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (!secondary) {
        ovsrec_port_set_ip4_address(port_row, ip4);
    } else {
        secondary_ip4_addresses = xmalloc(
        IP_ADDRESS_LENGTH * (port_row->n_ip4_address_secondary + 1));
        for (i = 0; i < port_row->n_ip4_address_secondary; i++)
            secondary_ip4_addresses[i] = port_row->ip4_address_secondary[i];
        secondary_ip4_addresses[port_row->n_ip4_address_secondary] = ip4;
        ovsrec_port_set_ip4_address_secondary(port_row, secondary_ip4_addresses,
                port_row->n_ip4_address_secondary + 1);
        free(secondary_ip4_addresses);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" was configured"
                " with IP address \"%s\"", __func__, if_name, ip4);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete an IP address assigned for a port
 * which is attached to a VRF.
 */
static int vrf_del_ip(const char *if_name, const char *ip4, bool secondary) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char **secondary_ip4_addresses;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        for (i = 0; i < vrf_row->n_ports; i++) {
            if (strcmp(vrf_row->ports[i]->name, if_name) == 0) {
                port_row = vrf_row->ports[i];
                break;
            }
        }
    }

    if (!port_row) {
        vty_out(vty, "Error: Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not part of any VRF.", __func__,
                if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (!secondary) {
        if (!port_row->ip4_address) {
            vty_out(vty, "Error: No IP Address configured on interface %s.%s",
                    if_name, VTY_NEWLINE);
            VLOG_DBG("%s No IP address configured on interface \"%s\".",
                    __func__, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        if (strcmp(port_row->ip4_address, ip4) != 0) {
            vty_out(vty, "Error: IP Address %s not found.%s", ip4, VTY_NEWLINE);
            VLOG_DBG("%s IP address \"%s\" not configured on interface "
                    "\"%s\".", __func__, ip4, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        ovsrec_port_set_ip4_address(port_row, NULL);
    } else {
        if (!port_row->n_ip4_address_secondary) {
            vty_out(vty,
                    "Error: No secondary IP Address configured on"
                    " interface %s.%s", if_name, VTY_NEWLINE);
            VLOG_DBG(
                    "%s No secondary IP address configured on interface"
                    " \"%s\".", __func__, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        bool ip4_address_match = false;
        for (i = 0; i < port_row->n_ip4_address_secondary; i++) {
            if (strcmp(ip4, port_row->ip4_address_secondary[i]) == 0) {
                ip4_address_match = true;
                break;
            }
        }

        if (!ip4_address_match) {
            vty_out(vty, "Error: IP Address %s not found.%s", ip4, VTY_NEWLINE);
            VLOG_DBG("%s IP address \"%s\" not configured on interface"
                    " \"%s\".", __func__, ip4, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        secondary_ip4_addresses = xmalloc(
        IP_ADDRESS_LENGTH * (port_row->n_ip4_address_secondary - 1));
        for (i = 0; i < port_row->n_ip4_address_secondary; i++) {
            if (strcmp(ip4, port_row->ip4_address_secondary[i]) != 0)
                secondary_ip4_addresses[i] = port_row->ip4_address_secondary[i];
        }
        ovsrec_port_set_ip4_address_secondary(port_row, secondary_ip4_addresses,
                port_row->n_ip4_address_secondary - 1);
        free(secondary_ip4_addresses);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" no longer has"
                " the IP address \"%s\"", __func__, if_name, ip4);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status "
                "response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to configure an IPv6 address for a port
 * which is attached to a VRF.
 */
static int vrf_config_ipv6(const char *if_name, const char *ipv6,
                           bool secondary) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char **secondary_ipv6_addresses;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        for (i = 0; i < vrf_row->n_ports; i++) {
            if (strcmp(vrf_row->ports[i]->name, if_name) == 0) {
                port_row = vrf_row->ports[i];
                break;
            }
        }
    }

    if (!port_row) {
        vty_out(vty, "Error: Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not part of any VRF.", __func__,
                if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (!secondary) {
        ovsrec_port_set_ip6_address(port_row, ipv6);
    } else {
        secondary_ipv6_addresses = xmalloc(
        IPV6_ADDRESS_LENGTH * (port_row->n_ip6_address_secondary + 1));
        for (i = 0; i < port_row->n_ip6_address_secondary; i++)
            secondary_ipv6_addresses[i] = port_row->ip6_address_secondary[i];
        secondary_ipv6_addresses[port_row->n_ip6_address_secondary] = ipv6;
        ovsrec_port_set_ip6_address_secondary(port_row,
                secondary_ipv6_addresses,
                port_row->n_ip6_address_secondary + 1);
        free(secondary_ipv6_addresses);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" was configured"
                " with IPv6 address \"%s\"", __func__, if_name, ipv6);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function is used to delete an IPv6 address assigned for a port
 * which is attached to a VRF.
 */
static int vrf_del_ipv6(const char *if_name, const char *ipv6,
                        bool secondary) {
    struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char **secondary_ipv6_addresses;
    size_t i;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if (!status_txn) {
        VLOG_DBG(
                "%s Got an error when trying to create a transaction using"
                " ovsdb_idl_txn_create()", __func__);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        for (i = 0; i < vrf_row->n_ports; i++) {
            if (strcmp(vrf_row->ports[i]->name, if_name) == 0) {
                port_row = vrf_row->ports[i];
                break;
            }
        }
    }

    if (!port_row) {
        vty_out(vty, "Error: Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG("%s Interface \"%s\" is not part of any VRF.", __func__,
                if_name);
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_SUCCESS;
    }

    if (!secondary) {
        if (!port_row->ip6_address) {
            vty_out(vty, "Error: No IPv6 Address configured on interface"
                    " %s.%s", if_name, VTY_NEWLINE);
            VLOG_DBG("%s No IPv6 address configured on interface"
                    " \"%s\".", __func__, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        if (strcmp(port_row->ip6_address, ipv6) != 0) {
            vty_out(vty, "Error: IPv6 Address %s not found.%s", ipv6,
                    VTY_NEWLINE);
            VLOG_DBG(
                    "%s IPv6 address \"%s\" not configured on interface"
                    " \"%s\".", __func__, ipv6, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        ovsrec_port_set_ip6_address(port_row, NULL);
    } else {
        if (!port_row->n_ip6_address_secondary) {
            vty_out(vty,
                    "Error: No secondary IPv6 Address configured on interface"
                    " %s.%s", if_name, VTY_NEWLINE);
            VLOG_DBG(
                    "%s No secondary IPv6 address configured on interface"
                    " \"%s\".", __func__, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        bool ipv6_address_match = false;
        for (i = 0; i < port_row->n_ip6_address_secondary; i++) {
            if (strcmp(ipv6, port_row->ip6_address_secondary[i]) == 0) {
                ipv6_address_match = true;
                break;
            }
        }

        if (!ipv6_address_match) {
            vty_out(vty, "Error: IPv6 Address %s not found.%s", ipv6,
                    VTY_NEWLINE);
            VLOG_DBG(
                    "%s IPv6 address \"%s\" not configured on interface"
                    " \"%s\".", __func__, ipv6, if_name);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_SUCCESS;
        }
        secondary_ipv6_addresses = xmalloc(
        IPV6_ADDRESS_LENGTH * (port_row->n_ip6_address_secondary - 1));
        for (i = 0; i < port_row->n_ip6_address_secondary; i++) {
            if (strcmp(ipv6, port_row->ip6_address_secondary[i]) != 0)
                secondary_ipv6_addresses[i] =
                        port_row->ip6_address_secondary[i];
        }
        ovsrec_port_set_ip6_address_secondary(port_row,
                secondary_ipv6_addresses,
                port_row->n_ip6_address_secondary - 1);
        free(secondary_ipv6_addresses);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" no longer"
                " has IPv6 address \"%s\"", __func__, if_name, ipv6);
        return CMD_SUCCESS;
    } else {
        VLOG_DBG("%s While trying to commit transaction to DB, "
                "got a status response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}

static int show_vrf_info() {
    struct ovsrec_vrf *vrf_row = NULL;
    size_t i;

    ovsdb_idl_run(idl);

    vrf_row = ovsrec_vrf_first(idl);
    if (!vrf_row) {
        vty_out(vty, "No VRF found.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    vty_out(vty, "VRF Configuration:%s", VTY_NEWLINE);
    vty_out(vty, "------------------%s", VTY_NEWLINE);
    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        vty_out(vty, "VRF Name : %s%s\n", vrf_row->name, VTY_NEWLINE);
        vty_out(vty, "\tInterfaces : %s", VTY_NEWLINE);
        vty_out(vty, "\t------------%s", VTY_NEWLINE);
        for (i = 0; i < vrf_row->n_ports; i++)
            vty_out(vty, "\t%s%s", vrf_row->ports[i]->name, VTY_NEWLINE);
    }

    return CMD_SUCCESS;
}

DEFUN (cli_vrf_add,
        cli_vrf_add_cmd,
        "vrf VRF_NAME",
        VRF_STR
        "Name of the vrf.\n") {
    return vrf_add(argv[0]);
}

DEFUN (cli_vrf_delete,
        cli_vrf_delete_cmd,
        "no vrf VRF_NAME",
        NO_STR
        VRF_STR
        "Name of the vrf.\n") {
    return vrf_delete(argv[0]);
}

DEFUN (cli_vrf_add_port,
        cli_vrf_add_port_cmd,
        "vrf attach VRF_NAME",
        VRF_STR
        "Attach the port to a VRF.\n"
        "Name of the vrf.\n") {
    return vrf_add_port((char*) vty->index, argv[0]);
}

DEFUN (cli_vrf_del_port,
        cli_vrf_del_port_cmd,
        "no vrf attach VRF_NAME",
        NO_STR
        VRF_STR
        "Detach the port from a VRF.\n"
        "Name of the vrf.\n") {
    return vrf_del_port((char*) vty->index, argv[0]);
}

DEFUN (cli_vrf_config_ip,
        cli_vrf_config_ip_cmd,
        "ip address A.B.C.D/M [secondary]",
        IP_STR
        "Set the IP Address\n"
        "IP address of port.\n"
        "IP address is secondary.\n") {
    if (argc > 1) {
        return vrf_config_ip((char*) vty->index, argv[0], true);
    } else {
        return vrf_config_ip((char*) vty->index, argv[0], false);
    }
}

DEFUN (cli_vrf_del_ip,
        cli_vrf_del_ip_cmd,
        "no ip address A.B.C.D/M [secondary]",
        NO_STR
        IP_STR
        "Set the IP Address\n"
        "IP address of port.\n"
        "IP address is secondary.\n") {
    if (argc > 1) {
        return vrf_del_ip((char*) vty->index, argv[0], true);
    } else {
        return vrf_del_ip((char*) vty->index, argv[0], false);
    }
}

DEFUN (cli_vrf_config_ipv6,
        cli_vrf_config_ipv6_cmd,
        "ipv6 address X:X::X:X/M [secondary]",
        IPV6_STR
        "Set the IP Address\n"
        "IPv6 address of port.\n"
        "IP address is secondary.\n") {
    if (argc > 1) {
        return vrf_config_ipv6((char*) vty->index, argv[0], true);
    } else {
        return vrf_config_ipv6((char*) vty->index, argv[0], false);
    }
}

DEFUN (cli_vrf_del_ipv6,
        cli_vrf_del_ipv6_cmd,
        "no ipv6 address X:X::X:X/M [secondary]",
        NO_STR
        IPV6_STR
        "Set the IP Address\n"
        "IPv6 address of port.\n"
        "IP address is secondary.\n") {
    if (argc > 1) {
        return vrf_del_ipv6((char*) vty->index, argv[0], true);
    } else {
        return vrf_del_ipv6((char*) vty->index, argv[0], false);
    }
}

DEFUN (cli_vrf_show,
        cli_vrf_show_cmd,
        "show vrf",
        SHOW_STR
        VRF_STR) {
    return show_vrf_info();
}

/* Install VRF related vty commands. */
void vrf_vty_init(void) {
    install_element(CONFIG_NODE, &cli_vrf_add_cmd);
    install_element(CONFIG_NODE, &cli_vrf_delete_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_add_port_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_del_port_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_config_ip_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_config_ipv6_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_del_ip_cmd);
    install_element(INTERFACE_NODE, &cli_vrf_del_ipv6_cmd);
    install_element(ENABLE_NODE, &cli_vrf_show_cmd);
}
