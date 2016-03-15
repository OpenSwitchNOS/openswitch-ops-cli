/*
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
 */
/****************************************************************************
 *
 * @file vlan_vtysh_utils.c
 ***************************************************************************/

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh.h"
#include "vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "utils/vlan_vtysh_utils.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"

VLOG_DEFINE_THIS_MODULE(vlan_vtysh_utils);

extern struct ovsdb_idl *idl;

/*-----------------------------------------------------------------------------
| Function : port_lookup
| Responsibility : Lookup port table entry for interface name
| Parameters :
|   const char *if_name : Interface name
|   const struct ovsdb_idl *idl : IDL for vtysh
| Return : bool : returns true/false
-----------------------------------------------------------------------------*/
const struct ovsrec_port* port_lookup(const char *if_name,
                                const struct ovsdb_idl *idl)
{
    const struct ovsrec_port *port_row = NULL;
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
      if (strcmp(port_row->name, if_name) == 0) {
        return port_row;
      }
    }
    return NULL;
}

/* Function : check_internal_vlan
 * Description : Checks if interface vlan is being created for
 * an already used internal VLAN.
 * param in : vlanid - to check if it is already in use
 */
int
check_internal_vlan(uint16_t vlanid)
{
    const struct ovsrec_vlan *vlan_row = NULL;

    OVSREC_VLAN_FOR_EACH(vlan_row, idl)
    {
        if (smap_get(&vlan_row->internal_usage,
                VLAN_INTERNAL_USAGE_L3PORT))
        {
            VLOG_DBG("%s Used internally for l3 interface", __func__);
            /* now check if this vlan is used for creating vlan interface */
            if (vlanid == vlan_row->id) {
                VLOG_DBG("%s This is a internal vlan = %d", __func__, vlanid);
                return 0;
            }
        }
    }

    return 1;
}

/* Function : create_vlan_interface
 * Description : Creates a vlan interface. Will create an
 * interface and port with name same as for VLAN interface.
 * and then associate it with the VRF and Bridge row.
 * param in : vlan_if - Vlan interface name
 */
int
create_vlan_interface(const char *vlan_if)
{
    const struct ovsrec_interface *if_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;

    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    struct ovsrec_port **vrf_port_list;
    struct ovsrec_port **bridge_port_list;
    struct ovsrec_interface **iface_list;

    bool intf_exist = false, port_exist = false;

    int i;
    int64_t tag = atoi(vlan_if + 4);

    ovsdb_idl_run(idl);

    status_txn = cli_do_config_start();

    if (!status_txn) {
        VLOG_ERR(
                "%s Got an error when trying to create a transaction"
                " using ovsdb_idl_txn_create()", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /*verify if interface exists */
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if (strcmp(if_row->name, vlan_if) == 0) {
            intf_exist = true;
        }
    }

    /*verify if port exists */
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, vlan_if) == 0) {
            port_exist = true;
        }
    }

    /* If both port and interface exists return success
       nothing to change here */
    if (intf_exist == true && port_exist == true) {
        VLOG_DBG("%s Both interface and port exists"
                " for this Vlan interface name", __func__);
        cli_do_config_finish(status_txn);
        return CMD_SUCCESS;
    } else if (!(intf_exist == false && port_exist == false)) {
        /* Only if both do not exist then go ahead else return ERROR */
        VLOG_ERR(
                "%s Interface OR Port row already exists for this Vlan"
                " interface. Ideally we should either have BOTH already"
                " existing or BOTH non existing.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Get vrf row so that we can add the port to it */
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
            break;
        }
    }

    if (!vrf_row) {
        VLOG_ERR("%s Error: Could not fetch VRF data.", __func__);
        VLOG_DBG(
                "%s VRF table did not have any rows. Ideally it"
                " should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Get Bridge row so that we can add the port to it */
    OVSREC_BRIDGE_FOR_EACH (bridge_row, idl)
    {
        if (strcmp(bridge_row->name, DEFAULT_BRIDGE_NAME) == 0) {
            break;
        }
    }

    if (!bridge_row) {
        VLOG_ERR("%s Error: Could not fetch Bridge data.", __func__);
        VLOG_DBG(
                "%s Bridge table did not have any rows.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* adding an interface */
    if_row = ovsrec_interface_insert(status_txn);
    ovsrec_interface_set_name(if_row, vlan_if);
    ovsrec_interface_set_type(if_row, OVSREC_INTERFACE_TYPE_INTERNAL);

    struct smap smap_user_config;
    smap_clone(&smap_user_config,&if_row->user_config);

    /* Set the admin state */
    smap_replace(&smap_user_config, INTERFACE_USER_CONFIG_MAP_ADMIN,
            OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);

    ovsrec_interface_set_user_config(if_row, &smap_user_config);
    smap_destroy(&smap_user_config);

    iface_list = xmalloc(sizeof(struct ovsrec_interface));
    iface_list[0] = (struct ovsrec_interface *)if_row;

    /* Adding a port to the corresponding interface*/
    port_row = ovsrec_port_insert(status_txn);
    ovsrec_port_set_name(port_row, vlan_if);
    ovsrec_port_set_interfaces(port_row, iface_list, 1);
    ovsrec_port_set_tag(port_row, &tag, 1);
    ovsrec_port_set_vlan_mode(port_row, 0 /*PORT_VLAN_ACCESS*/);

    /* Add the port to vrf port list */
    vrf_port_list =
        xmalloc(sizeof(struct ovsrec_port) * (vrf_row->n_ports + 1));
    for (i = 0; i < vrf_row->n_ports; i++) {
        vrf_port_list[i] = vrf_row->ports[i];
    }
    vrf_port_list[vrf_row->n_ports] = (struct ovsrec_port *)port_row;
    ovsrec_vrf_set_ports(vrf_row, vrf_port_list, vrf_row->n_ports + 1);
    free(vrf_port_list);

    /* Add the port to bridge */
    bridge_port_list =
        xmalloc(sizeof(struct ovsrec_port) * (bridge_row->n_ports + 1));
    for (i = 0; i < bridge_row->n_ports; i++) {
        bridge_port_list[i] = bridge_row->ports[i];
    }
    bridge_port_list[bridge_row->n_ports] = (struct ovsrec_port *)port_row;
    ovsrec_bridge_set_ports(bridge_row,
            bridge_port_list, bridge_row->n_ports + 1);
    free(bridge_port_list);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" was create",
                __func__, vlan_if);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_DBG(
                "%s The command resulted in no change "
                "Check if interface \"%s\" was create",
                __func__, vlan_if);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}


/* Function : delete_vlan_interface
 * Description : Deletes a vlan interface. Will delete the
 * interface and port with name same as for VLAN interface.
 * and then remove port associated in VRF and Bridge row.
 * param in : vlan_if - Vlan interface name
 */
int
delete_vlan_interface(const char *vlan_if)
{
    const struct ovsrec_interface *if_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;

    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    struct ovsrec_port **vrf_port_list;
    struct ovsrec_port **bridge_port_list;

    bool intf_exist = false, port_exist = false;

    int i, j;

    status_txn = cli_do_config_start();

    if (!status_txn) {
        VLOG_ERR(
                "%s Got an error when trying to create a transaction"
                " using ovsdb_idl_txn_create()", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /*verify if interface exists */
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if (strcmp(if_row->name, vlan_if) == 0) {
            intf_exist = true;
        }
    }

    /*verify if port exists */
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, vlan_if) == 0) {
            port_exist = true;
        }
    }

    /* If port OR interface does not exist return failure */
    if (intf_exist == false || port_exist == false) {
        vty_out(vty,
                "Vlan interface does not exist. Cannot delete %s",
                VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Remove the port row from vrf */
    /* Iterate through each VRF */
    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        vrf_port_list =
            xmalloc(sizeof(struct ovsrec_port) * (vrf_row->n_ports - 1));
        for (i = 0, j = 0; i < vrf_row->n_ports; i++) {
            if (strcmp(vrf_row->ports[i]->name, vlan_if) != 0) {
                vrf_port_list[j] = vrf_row->ports[i];
                j++;
            }
        }
        /* If we find the interface then update the vrf port list */
        if (i > j) {
            ovsrec_vrf_set_ports(vrf_row, vrf_port_list, vrf_row->n_ports - 1);
        }
        free(vrf_port_list);
    }

    /* Remove the port row from bridge */
    OVSREC_BRIDGE_FOR_EACH(bridge_row, idl)
    {
        bridge_port_list =
            xmalloc(sizeof(struct ovsrec_port) * (bridge_row->n_ports - 1));
        for (i = 0, j = 0; i < bridge_row->n_ports; i++) {
            if (strcmp(bridge_row->ports[i]->name, vlan_if) != 0) {
                bridge_port_list[j] = bridge_row->ports[i];
                j++;
            }
        }
        if (i > j) {
            ovsrec_bridge_set_ports(bridge_row, bridge_port_list,
                    bridge_row->n_ports - 1);
        }
        free(bridge_port_list);
    }

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_DBG(
                "%s The command succeeded and interface \"%s\" was create",
                __func__, vlan_if);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_DBG(
                "%s The command resulted in no change "
                "Check if interface \"%s\" was create",
                __func__, vlan_if);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(
                "%s While trying to commit transaction to DB, got a status"
                " response : %s", __func__,
                ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }
}


/* Function : verify_ifname. The function handles case sensitive commands
 *            Like VLAN10 or Vlan10. Also checks for invalid inputs like
 *            vlan 10aa or vlan 5000.
 * Description : verifies if the user input is valid.
 * param in : str - User passed Vlan interface name
 */
bool
verify_ifname(char *str)
{
    uint16_t vlanid = 0;
    char *endptr = NULL;

    if (VERIFY_VLAN_IFNAME(str) != 0) {
        return 0;
    }

    while(*str) {
        if (isdigit(*str)) {
            vlanid = strtol(str, &endptr, 10);
            VLOG_DBG("%s vlanid = %d, str = %s\n", __func__, vlanid, str);
            break;
        } else {
            str++;
            if (*str == '\0')
                return 0;
        }
    }

    /* For handling characters after/before <vlan id> */
    if (*endptr != '\0') {
        vty_out(vty, "Error : Invalid vlan input\n");
        return 0;
    }

    /* The VLANID is outside valid vlan range */
    if (vlanid <= 0 || vlanid >= 4095) {
        vty_out(vty, "Error : Vlanid outside valid vlan range <1-4094>\n");
        return 0;
    }

    /* The VLANID is internal vlan */
    if (check_internal_vlan(vlanid) == 0) {
        vty_out(vty, "VLAN%d is used as an internal VLAN. No further configuration allowed.\n", vlanid);
        return 0;
    }

    return 1;
}
