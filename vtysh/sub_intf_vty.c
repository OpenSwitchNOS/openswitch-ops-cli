/*
 * Copyright (C) 2000 Kunihiro Ishiguro
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
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file sub_intf_vty.c
 * SUB_IF CLI Commands
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
#include "intf_vty.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "sub_intf_vty.h"
#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vlan_vty.h"
#include "vrf_vty.h"


VLOG_DEFINE_THIS_MODULE (vtysh_sub_intf_cli);
extern struct ovsdb_idl *idl;


/*
 * This function is used to configure an IP address for a port
 * which is attached to a sub interface.
 */
static int
sub_intf_config_ip (const char *if_name, const char *ip4)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char **secondary_ip4_addresses;
    size_t i;
    bool port_found;

    if(!is_valid_ip_address(ip4))
    {
        vty_out(vty, "Invalid IP address %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, if_name) == 0)
        {
            port_found = true;
            break;
        }
    }

    if(!port_found){
        vty_out(vty, "Port %s not found\n", if_name);
        return CMD_SUCCESS;
    }

    if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" is not attached to any SUB_IF. "
                "It is attached to default bridge",
                __func__, if_name);
        return CMD_SUCCESS;
    }

    if ((NULL != ip4) && (NULL != port_row->ip4_address)
            && (strcmp (port_row->ip4_address, ip4) != 0))
    {
        vty_out (vty, "IP address %s not configured.%s", ip4, VTY_NEWLINE);
        VLOG_DBG ("%s IP address \"%s\" not configured on interface "
                "\"%s\".",
                __func__, ip4, if_name);
        return CMD_SUCCESS;
    }

    bool is_secondary;
    if (check_ip_addr_duplicate (ip4, port_row, true, &is_secondary))
    {
        vty_out (vty, "IP address is already assigned to interface %s"
                " as %s.%s",
                if_name, is_secondary ? "secondary" : "primary", VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" already has the IP address \"%s\""
                " assigned to it as \"%s\".",
                __func__, if_name, ip4,
                is_secondary ? "secondary" : "primary");
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start ();
    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_port_set_ip4_address(port_row, ip4);

    status = cli_do_config_finish (status_txn);
    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED))
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN (cli_sub_intf_config_ip4,
        cli_sub_intf_config_ip4_cmd,
        "ip address A.B.C.D/M",
        IP_STR
        "Set IP address\n"
        "Interface IP address\n")
{
    return sub_intf_config_ip((char*) vty->index, argv[0]);
}


DEFUN (cli_sub_intf_del_ip4,
        cli_sub_intf_del_ip4_cmd,
        "no ip address A.B.C.D/M",
        NO_STR
        IP_STR
        "Set IP address\n"
        "Interface IP address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i, n;
    bool port_found;
    const char *if_name = (char*)vty->index;
    char *ip4[IP_ADDRESS_LENGTH];

    if(NULL != argv[0])
    {
        sprintf(ip4,"%s",argv[0]);
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, if_name) == 0)
        {
            port_found = true;
            break;
        }
    }

    if(!port_found){
        vty_out(vty,"Port %s not found\n",if_name);
        return CMD_OVSDB_FAILURE;
    }


    if (!port_row->ip4_address)
    {
        VLOG_DBG ("%s No IP address configured on interface \"%s\".",
                __func__, if_name);
        return CMD_SUCCESS;
    }

    if ((NULL != ip4) && (NULL != port_row->ip4_address)
            && (strcmp (port_row->ip4_address, ip4) != 0))
    {
        vty_out (vty, "IP address %s not configured.%s", ip4, VTY_NEWLINE);
        VLOG_DBG ("%s IP address \"%s\" not configured on interface "
                "\"%s\".",
                __func__, ip4, if_name);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start ();
    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_port_set_ip4_address (port_row, NULL);

    status = cli_do_config_finish (status_txn);

    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED))
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
    return CMD_SUCCESS;
}


DEFUN (cli_sub_intf_config_ipv6,
        cli_sub_intf_config_ipv6_cmd,
        "ipv6 address X:X::X:X/M",
        IPV6_STR
        "Set IP address\n"
        "Interface IPv6 address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i;

    const char *if_name = (char*) vty->index;
    const char *ipv6 = argv[0];

    if(!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "Invalid IP address %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start ();
    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }


    port_row = port_check_and_add (if_name, true, true, status_txn);
    if (check_iface_in_bridge (if_name))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    bool is_secondary = false;
    if (check_ip_addr_duplicate (ipv6, port_row, true, &is_secondary))
    {
        vty_out (vty, "IP address is already assigned to interface %s"
                " as %s.%s",
                if_name, is_secondary ? "secondary" : "primary", VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" already has the IP address \"%s\""
                " assigned to it as \"%s\".",
                __func__, if_name, ipv6,
                is_secondary ? "secondary" : "primary");
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_ip6_address (port_row, ipv6);

    status = cli_do_config_finish (status_txn);
    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED))
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}


DEFUN (cli_sub_intf_del_ipv6,
        cli_sub_intf_del_ipv6_cmd,
        "no ipv6 address X:X::X:X/M",
        NO_STR
        IP_STR
        "Set IP address\n"
        "Interface IP address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    const char *if_name = (char*) vty->index;
    const char *ipv6 = NULL;

    if(NULL != argv[0])
    {
        ipv6 = argv[0];
    }

    status_txn = cli_do_config_start ();

    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    port_row = port_check_and_add (if_name, false, false, status_txn);

    if (!port_row)
    {
        VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if (!port_row->ip6_address)
    {
        vty_out (vty, "No IPv6 address configured on interface"
                " %s.%s",
                if_name, VTY_NEWLINE);
        VLOG_DBG ("%s No IPv6 address configured on interface"
                " \"%s\".",
                __func__, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if ((NULL != ipv6) && (strcmp (port_row->ip6_address, ipv6) != 0))
    {
        vty_out (vty, "IPv6 address %s not found.%s", ipv6, VTY_NEWLINE);
        VLOG_DBG ("%s IPv6 address \"%s\" not configured on interface"
                " \"%s\".",
                __func__, ipv6, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }
    ovsrec_port_set_ip6_address (port_row, NULL);

    status = cli_do_config_finish (status_txn);

    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED))
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

int
cli_show_subinterface_row(const struct ovsrec_interface *ifrow, bool brief)
{
    const char *cur_state =NULL;
    const struct ovsrec_interface *if_parent_row = NULL;

    const struct ovsdb_datum *datum;
    static char *interface_statistics_keys [] = {
        "rx_packets", "rx_bytes",  "tx_packets",
        "tx_bytes",   "rx_dropped","rx_frame_err",
        "rx_over_err","rx_crc_err","rx_errors",
        "tx_dropped", "collisions","tx_errors"
    };

    unsigned int index;
    int64_t intVal = 0;
    int64_t parent_intf = 0, sub_intf = 0;
    bool parent_intf_down = false;
    char parent_intf_name[MAX_IFNAME_LENGTH];

    if ((NULL == ifrow) ||
            (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0))
    {
        return CMD_SUCCESS;
    }

    strncpy(parent_intf_name, ifrow->name,
                   strchr(ifrow->name,'.') - ifrow->name);

    OVSREC_INTERFACE_FOR_EACH(if_parent_row, idl)
    {
        if (0 == strcmp(parent_intf_name, if_parent_row->name))
        {
            if ((NULL != if_parent_row->admin_state) &&
                    (strcmp(if_parent_row->admin_state,
                            OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0))
                parent_intf_down = true;
        }
    }

    union ovsdb_atom atom;

    if (brief)
    {
        int64_t key_subintf_parent = 0;
        if(ifrow->n_subintf_parent > 0)
        {
            key_subintf_parent = ifrow->key_subintf_parent[0];
        }

         /* Display the brief information */
        vty_out (vty, " %-12s ", ifrow->name);
        vty_out(vty, "%4d    ", key_subintf_parent ); /*vVLAN */
        vty_out(vty, "eth  "); /*type */
        vty_out(vty, "routed "); /* mode - routed or not */

        vty_out (vty, "%-6s ", ifrow->link_state);

        if ((NULL != ifrow->admin_state) &&
                (strcmp(ifrow->admin_state,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0))
        {
            vty_out (vty, "Administratively down    ");
        }
        else
        {
            vty_out (vty, "                         ");
        }
        intVal = 0;
        datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }

        if(intVal == 0)
        {
            vty_out(vty, " %-6s", "auto");
        }
        else
        {
            vty_out(vty, " %-6ld", intVal/1000000);
        }
        vty_out(vty, "   -- ");  /* Port channel */
        vty_out (vty, "%s", VTY_NEWLINE);
    }
    else
    {
        intVal = 0;

        vty_out (vty, "Interface %s is %s ", ifrow->name, ifrow->link_state);
        if ((NULL != ifrow->admin_state)
                && strcmp(ifrow->admin_state,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0)
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

        vty_out (vty, " Parent interface is: %s %s",
                parent_intf_name, VTY_NEWLINE);
        if (parent_intf_down)
        {
            vty_out (vty, " Parent interface is administratively down%s",
                    VTY_NEWLINE);
        }

        if (ifrow->error != NULL)
        {
            vty_out (vty, " State information: %s%s",
                    ifrow->error, VTY_NEWLINE);
        }

        int64_t key_subintf_parent = 0;
        if(ifrow->n_subintf_parent > 0)
        {
            key_subintf_parent = ifrow->key_subintf_parent[0];
        }

        if (0 != key_subintf_parent)
        {
            vty_out (vty, " Encapsulation dot1Q %d %s",
                    key_subintf_parent, VTY_NEWLINE);
        }
        else
        {
            vty_out (vty, " Encapsulation dot1Q not configured %s",
                    VTY_NEWLINE);
        }


        vty_out (vty, " Hardware: Ethernet, MAC Address: %s %s",
                ifrow->mac_in_use, VTY_NEWLINE);

        /* Displaying ipv4 and ipv6 primary and secondary addresses*/
        show_ip_addresses(ifrow->name, vty);

        datum = ovsrec_interface_get_mtu(ifrow, OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }

        vty_out(vty, " MTU %ld %s", intVal, VTY_NEWLINE);

        if ((NULL != ifrow->duplex) &&
                (strcmp(ifrow->duplex, "half") == 0))
        {
            vty_out(vty, " Half-duplex %s", VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, " Full-duplex %s", VTY_NEWLINE);
        }

        intVal = 0;
        datum = ovsrec_interface_get_link_speed(ifrow, OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }
        vty_out(vty, " Speed %ld Mb/s %s",intVal/1000000 , VTY_NEWLINE);

        cur_state = smap_get(&ifrow->user_config,
                INTERFACE_USER_CONFIG_MAP_AUTONEG);
        if ((NULL == cur_state) ||
                strcmp(cur_state, "off") !=0)
        {
            vty_out(vty, " Auto-Negotiation is turned on %s", VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, " Auto-Negotiation is turned off %s",
                    VTY_NEWLINE);
        }

        cur_state = ifrow->pause;
        if (NULL != cur_state)
        {
            if (strcmp(cur_state,
                    INTERFACE_USER_CONFIG_MAP_PAUSE_NONE) == 0)

            {
                vty_out(vty, " Input flow-control is off, "
                        "output flow-control is off%s",VTY_NEWLINE);
            }
            else if (strcmp(cur_state,
                    INTERFACE_USER_CONFIG_MAP_PAUSE_RX) == 0)
            {
                vty_out(vty, " Input flow-control is on, "
                        "output flow-control is off%s",VTY_NEWLINE);
            }
            else if (strcmp(cur_state,
                    INTERFACE_USER_CONFIG_MAP_PAUSE_TX) == 0)
            {
                vty_out(vty, " Input flow-control is off, "
                        "output flow-control is on%s",VTY_NEWLINE);
            }
            else
            {
                vty_out(vty, " Input flow-control is on, "
                        "output flow-control is on%s",VTY_NEWLINE);
            }
        }
        else
        {
            vty_out(vty, " Input flow-control is off, "
                    "output flow-control is off%s",VTY_NEWLINE);
        }

        datum = ovsrec_interface_get_statistics(ifrow,
                OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);

        if (NULL==datum) return CMD_SUCCESS;

        vty_out(vty, " RX%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[0];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld input packets  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        atom.string = interface_statistics_keys[1];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld bytes  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[8];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld input error    ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        atom.string = interface_statistics_keys[4];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld dropped  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[7];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld CRC/FCS  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        vty_out(vty, " TX%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[2];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld output packets ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        atom.string = interface_statistics_keys[3];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld bytes  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[11];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld input error    ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        atom.string = interface_statistics_keys[9];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld dropped  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        atom.string = interface_statistics_keys[10];
        index = ovsdb_datum_find_key(datum, &atom, OVSDB_TYPE_STRING);
        vty_out(vty, "   %10ld collision  ",
                (index == UINT_MAX)? 0 : datum->values[index].integer);
        vty_out(vty, "%s", VTY_NEWLINE);

        vty_out(vty, "%s", VTY_NEWLINE);
    }

    return CMD_SUCCESS;
}



DEFUN (cli_intf_show_subintferface_ifname,
        cli_intf_show_subintferface_ifname_cmd,
        "show interface A.B {brief}",
        SHOW_STR
        INTERFACE_STR
        "Show subinterface details\n"
        "Show brief info of interface\n")
{
    const struct ovsrec_interface *ifrow = NULL;
    bool brief = false;

    if ((NULL != argv[1]) && (strcmp(argv[1], "brief") == 0))
    {
        brief = true;
        /* Display the brief information */
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
        vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason                   Speed    Port%s", VTY_NEWLINE);
        vty_out(vty, "Interface                                                          (Mb/s)   Ch# %s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
    }

    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        if (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
        {
            continue;
        }
        if(strcmp(ifrow->name, argv[0]) != 0)
        {
            continue;
        }
        cli_show_subinterface_row(ifrow, brief);
    }
}


DEFUN (cli_intf_show_subintferface_if_all,
        cli_intf_show_subintferface_if_all_cmd,
        "show interface IFNAME subinterface {brief}",
        SHOW_STR
        INTERFACE_STR
        IFNAME_STR
        "Show subinterfaces configured on this interface\n"
        "Show brief info of interface\n")
{
    const struct ovsrec_interface *ifrow = NULL;
    char subif_prefix[MAX_IFNAME_LENGTH] = {0}, *subif_ptr = NULL;
    struct shash sorted_interfaces;
    const struct shash_node **nodes;
    int idx, count;
    bool brief = false;

    if ((NULL != argv[1]) && (strcmp(argv[1], "brief") == 0))
    {
        brief = true;
        /* Display the brief information */
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
        vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason                   Speed    Port%s", VTY_NEWLINE);
        vty_out(vty, "Interface                                                          (Mb/s)   Ch# %s", VTY_NEWLINE);
        vty_out(vty, "--------------------------------------------------------------------------------%s", VTY_NEWLINE);
    }

    if(NULL != argv[0])
    {
     //sprintf(subif_prefix, "%d.", atoi(argv[0]));
        strcpy(subif_prefix, argv[0]);
        subif_ptr = strtok(subif_prefix, ".");
    }
    shash_init(&sorted_interfaces);
    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        if (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
        {
            continue;
        }
        if(strncmp(ifrow->name,subif_ptr, strlen(subif_ptr)) != 0)
        {
            continue;
        }
        cli_show_subinterface_row(ifrow, brief);

        //shash_add(&sorted_interfaces, ifrow->name, (void *)ifrow);
    }
/*
    nodes = sort_interface(&sorted_interfaces);
    count = shash_count(&sorted_interfaces);

    for (idx = 0; idx < count; idx++)
    {
        ifrow = (const struct ovsrec_interface *)nodes[idx]->data;
        vty_out(vty, "ifrow : %s \n", ifrow->name);
        cli_show_subinterface_row(ifrow, brief);
    }
    shash_destroy(&sorted_interfaces);
    free(nodes); */
}



DEFUN (cli_del_sub_intf,
        cli_del_sub_intf_cmd,
        "no interface A.B",
        NO_STR
        "Select a subinterface to remove\n"
        "Subinterface name as physical_interface.subinterface name\n")
{
    return delete_sub_intf(argv[0]);
}

int
delete_sub_intf(const char *sub_intf_name)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_port *sub_intf_port_row = NULL;
    const struct ovsrec_interface *if_row = NULL, *interface_row = NULL;

    struct ovsrec_port **ports;
    int k=0, n=0, i=0;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    bool port_found = false;

    /* Find the interface table entry and delete it */
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if(strcmp(if_row->name, sub_intf_name) == 0)
        {
            interface_row = if_row;
            break;
        }
    }
    if(NULL == interface_row)
    {
        vty_out(vty, "Interface does not exist%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(interface_row->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
    {
        vty_out(vty, "Interface does not exist%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
        VLOG_ERR(SUB_IF_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_interface_delete(interface_row);

    /* find the port table entry and delete it */
    OVSREC_PORT_FOR_EACH(sub_intf_port_row, idl)
    {
        if (strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
        {
            port_found = true;
        }
    }

    if(!port_found)
    {
        /* commit the interface row deletion and return */
        status = cli_do_config_finish(status_txn);

        if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
        {
            return CMD_SUCCESS;
        }
        else
        {
            VLOG_ERR(SUB_IF_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
            return CMD_OVSDB_FAILURE;
        }
    }

    /* Check if the given sub interface port is part of VRF */
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        for (k = 0; k < vrf_row->n_ports; k++)
        {
            sub_intf_port_row = vrf_row->ports[k];
            if(strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
            {
                ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
                for(i = n = 0; i < vrf_row->n_ports; i++)
                {
                    if(vrf_row->ports[i] != sub_intf_port_row)
                    {
                        ports[n++] = vrf_row->ports[i];
                    }
                }
                ovsrec_vrf_set_ports(vrf_row, ports, n);
                free(ports);
            }
        }
    }

    ovsrec_port_delete(sub_intf_port_row);

    status = cli_do_config_finish(status_txn);

    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(SUB_IF_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
        return CMD_OVSDB_FAILURE;
    }

}

/*vlan envapsulation configuration*/
DEFUN  (cli_encapsulation_dot1Q_vlan,
        cli_encapsulation_dot1Q_vlan_cmd,
        "encapsulation dot1Q <1-4094>",
        "Set encapsulation type for an interface\n"
        "IEEE 802.1Q virtual LAN\n"
        "VLAN identifier\n")
{
    const struct ovsrec_interface * row = NULL, *parent_intf_row = NULL;
    struct ovsdb_idl_txn* status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;

    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_INTERFACE_FOR_EACH(row, idl)
    {
        if (strcmp(row->name, (char*)vty->index) == 0)
        {
            int64_t enc_vlan = 0, prev_key = 0;

            if (vty_flags & CMD_FLAG_NO_CMD)
            {
                enc_vlan = 0;
            }
            else
            {
                enc_vlan = atoi(argv[0]);
            }
            if(row->n_subintf_parent > 0)
            {
                parent_intf_row = row->value_subintf_parent[0];
            }

            if (NULL == parent_intf_row)
            {
                char parent_intf[MAX_IFNAME_LENGTH] = {0};
                sprintf(parent_intf, "%d", atoi((char*)vty->index));
                OVSREC_INTERFACE_FOR_EACH(parent_intf_row, idl)
                {
                    if (strcmp(parent_intf_row->name, parent_intf) == 0)
                    {
                        break;
                    }
                }
                /* parent interface row is guaranteed to exist */
            }

            int64_t *key_subintf_parent;
            struct ovsrec_interface  **val_subintf_parent;
            int new_size = 1;

            key_subintf_parent = xmalloc(sizeof(int64_t) * new_size);
            val_subintf_parent = xmalloc(sizeof(struct ovsrec_interface *)
                    * new_size);

            key_subintf_parent[0] = enc_vlan;
            val_subintf_parent[0] = parent_intf_row;

            ovsrec_interface_set_subintf_parent(row, key_subintf_parent,
                    val_subintf_parent, new_size);

            free(key_subintf_parent);
            free(val_subintf_parent);
        }
    }
    status = cli_do_config_finish(status_txn);
    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
    }

    return CMD_OVSDB_FAILURE;
}


DEFUN_NO_FORM  (cli_encapsulation_dot1Q_vlan,
        cli_encapsulation_dot1Q_vlan_cmd,
        "encapsulation dot1Q {<1-4094>}",
        "Set encapsulation type for an interface\n"
        "IEEE 802.1Q virtual LAN\n"
        "VLAN identifier\n");

/*
 * CLI "shutdown"
 * default : enabled
 */
DEFUN (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n")
{
    const struct ovsrec_interface * row = NULL;
    struct ovsdb_idl_txn* status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    struct smap smap_user_config;

    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_interface_first(idl);
    if (!row)
    {
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_INTERFACE_FOR_EACH(row, idl)
    {
        if (strcmp(row->name, (char*)vty->index) == 0)
        {
            smap_clone(&smap_user_config, &row->user_config);

            if (vty_flags & CMD_FLAG_NO_CMD)
            {
                smap_replace(&smap_user_config,
                        INTERFACE_USER_CONFIG_MAP_ADMIN,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);
            }
            else
            {
                smap_remove(&smap_user_config,
                        INTERFACE_USER_CONFIG_MAP_ADMIN);
            }
            ovsrec_interface_set_user_config(row, &smap_user_config);
            smap_destroy(&smap_user_config);
            break;
        }
    }

    status = cli_do_config_finish(status_txn);
    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN_NO_FORM (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface\n");



DEFUN (vtysh_sub_interface,
        vtysh_sub_interface_cmd,
        "interface A.B",
        "Select an interface to configure\n"
        "Subinterface name as physical_interface.subinterface name\n")
{
    return create_sub_interface(argv[0]);
}
int
create_sub_interface(char* subifname)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *intf_row, *parent_intf_row = NULL;
    struct ovsrec_interface **iface_list;
    bool port_found = false, parent_intf_found = false;
    struct ovsdb_idl_txn *txn = NULL;
    enum ovsdb_idl_txn_status status_txn;
    static char ifnumber[MAX_IFNAME_LENGTH]={0};
    const struct ovsrec_vrf *default_vrf_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    int i=0;
    struct ovsrec_port **ports = NULL;
    char *phy_intf, phy_intf_name[MAX_IFNAME_LENGTH];
    char sub_intf[MAX_IFNAME_LENGTH];
    long long int sub_intf_number;

    memcpy(ifnumber, subifname, MAX_IFNAME_LENGTH);

    sub_intf_number = (long long int)atoi(strchr(subifname,'.') + 1);

    if((sub_intf_number < MIN_SUB_INTF_RANGE) ||
            (sub_intf_number > MAX_SUB_INTF_RANGE)){
        vty_out (vty, "Invalid input %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    memcpy(phy_intf_name, subifname, MAX_IFNAME_LENGTH);
    phy_intf = strtok(phy_intf_name, ".");

    sprintf(sub_intf, "%s.%d", phy_intf, sub_intf_number);

    if(strcmp(sub_intf, subifname) != 0)
    {
        vty_out (vty, "Invalid input %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    sprintf(sub_intf, "%d", sub_intf_number);

    int max_sub_intf = 0;
	OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, phy_intf) == 0)
        {
            parent_intf_row = intf_row;
            parent_intf_found = true;
        }
	if(strcmp(intf_row->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) == 0)
		{
		    if(strncmp (intf_row->name, phy_intf, strlen(phy_intf)) == 0)
		    {
		        max_sub_intf++;
					if (max_sub_intf >= MAX_SUB_INTF_COUNT)
					{
                      vty_out(vty, "Cannot create subinterface. "
                       "Max limit reached for parent interface %s.%s",
                     phy_intf,VTY_NEWLINE);
               return CMD_SUCCESS;
				}
		    }
		}
    }

    if(!parent_intf_found){
        vty_out (vty, "Parent interface does not exists%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }


    /* Check if port row is part of bridge */
    if (check_iface_in_bridge(phy_intf))
    {
        vty_out(vty, "Patent interface is not L3. "
                "Enable routing on the parent interface.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, ifnumber) == 0)
        {
            port_found = true;
            break;
        }
    }
    if(!port_found)
    {
        txn = cli_do_config_start();
        if (txn == NULL)
        {
            VLOG_DBG("Transaction creation failed by %s. Function=%s, Line=%d",
                    " cli_do_config_start()", __func__, __LINE__);
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        /* adding an interface  table entry */
        intf_row = ovsrec_interface_insert(txn);
        ovsrec_interface_set_name(intf_row, ifnumber);
        ovsrec_interface_set_type(intf_row, OVSREC_INTERFACE_TYPE_VLANSUBINT);

        /* set the parent interface & encapsulation vlan id */
        {
            int64_t *key_subintf_parent;
            struct ovsrec_interface  **val_subintf_parent;
            int new_size = 1;

            key_subintf_parent = xmalloc(sizeof(int64_t) * new_size);
            val_subintf_parent = xmalloc(sizeof(struct ovsrec_interface *)
                    * new_size);

            key_subintf_parent[0] = 0;
            val_subintf_parent[0] = parent_intf_row;

            ovsrec_interface_set_subintf_parent(intf_row, key_subintf_parent,
                    val_subintf_parent, new_size);

            free(key_subintf_parent);
            free(val_subintf_parent);
        }

        /* Create port table entry */
        port_row = ovsrec_port_insert(txn);
        ovsrec_port_set_name(port_row, ifnumber);

        /* Adding a port to the corresponding interface*/
        iface_list = xmalloc(sizeof(struct ovsrec_interface));
        iface_list[0] = (struct ovsrec_interface *)intf_row;
        ovsrec_port_set_interfaces(port_row, iface_list, 1);
        free(iface_list);

        OVSREC_VRF_FOR_EACH (vrf_row, idl)
        {
            if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
                default_vrf_row = vrf_row;
                break;
            }
        }

        if(default_vrf_row == NULL)
        {
            assert(0);
            VLOG_DBG("Couldn't fetch default VRF row. Function=%s, Line=%d",
                    __func__, __LINE__);
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        ports = xmalloc(sizeof *default_vrf_row->ports *
                (default_vrf_row->n_ports + 1));
        for (i = 0; i < default_vrf_row->n_ports; i++)
        {
            ports[i] = default_vrf_row->ports[i];
        }
        ports[default_vrf_row->n_ports] = CONST_CAST(struct ovsrec_port*,port_row);
        ovsrec_vrf_set_ports(default_vrf_row, ports,
                default_vrf_row->n_ports + 1);
        free(ports);

        status_txn = cli_do_config_finish(txn);
        if(status_txn == TXN_SUCCESS || status_txn == TXN_UNCHANGED)
        {
            vty->index = ifnumber;
            vty->node = SUB_INTERFACE_NODE;
            return CMD_SUCCESS;
        }
        else
        {
            VLOG_ERR("Transaction commit failed in function=%s, line=%d",
                    __func__,__LINE__);
            return CMD_OVSDB_FAILURE;
        }
    }
    else
    {
        vty->index = ifnumber;
        vty->node = SUB_INTERFACE_NODE;
    }

    return CMD_SUCCESS;
}


/* Install sub interface related vty commands. */
void
sub_intf_vty_init (void)
{
    install_element (CONFIG_NODE,        &vtysh_sub_interface_cmd);
    install_element (CONFIG_NODE,        &cli_del_sub_intf_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ip4_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ip4_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ipv6_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ipv6_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_shutdown_cmd);
    install_element (SUB_INTERFACE_NODE, &no_cli_sub_intf_shutdown_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_encapsulation_dot1Q_vlan_cmd);
    install_element (SUB_INTERFACE_NODE, &no_cli_encapsulation_dot1Q_vlan_cmd);
    install_element (ENABLE_NODE,        &cli_intf_show_subintferface_ifname_cmd);
    install_element (ENABLE_NODE,        &cli_intf_show_subintferface_if_all_cmd);
}
