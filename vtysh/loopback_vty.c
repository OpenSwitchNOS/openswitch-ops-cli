/*
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
 * @file loopback_intf_vty.c
 * LOOPBACK CLI Commands
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
#include "loopback_vty.h"
#include "vtysh/vtysh_utils.h"

VLOG_DEFINE_THIS_MODULE (vtysh_loopback_if_cli);
extern struct ovsdb_idl *idl;


DEFUN (vtysh_loopback_interface,
        vtysh_loopback_interface_cmd,
        "interface loopback <1-2147483647>",
        "Select an interface to configure\n"
        "Configure loopback interface\n"
        "Interface number\n")
{
    static char ifname[MAX_IFNAME_LENGTH]={0};
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *intf_row;
    const struct ovsrec_vrf *default_vrf_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_interface **iface_list;
    struct smap smap_user_config;
    bool port_found = false;
    struct ovsdb_idl_txn *txn = NULL;
    enum ovsdb_idl_txn_status status_txn;
    int i = 0;
    int max_loopback_intf = 0;
    struct ovsrec_port **ports = NULL;
    long long int loopback_number;

    snprintf(ifname, MAX_IFNAME_LENGTH, "%s%s", "lo", argv[0]);
    loopback_number = (long long int)atoi(argv[0]);

    if ((loopback_number < MIN_LOOPBACK_INTF_RANGE) ||
            (loopback_number > MAX_LOOPBACK_INTF_RANGE))
    {
        vty_out(vty, "Invalid input. %s", VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->type, OVSREC_INTERFACE_TYPE_LOOPBACK) == 0)
        {
            max_loopback_intf++;
        }
        if (max_loopback_intf >= MAX_LOOPBACK_INTF_COUNT)
        {
            vty_out(vty, "Maximun number of loopback interface exceeded.%s",
                    VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, ifname) == 0)
        {
            port_found = true;
            break;
        }
    }
    if (!port_found)
    {
        txn = cli_do_config_start();
        if (txn == NULL)
        {
            VLOG_DBG("Transaction creation failed by %s. Function=%s, Line=%d",
                    " cli_do_config_start()", __func__, __LINE__);
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        /* create interface table entry */
        intf_row = ovsrec_interface_insert(txn);
        ovsrec_interface_set_name(intf_row, ifname);
        ovsrec_interface_set_type(intf_row, OVSREC_INTERFACE_TYPE_LOOPBACK);

        smap_clone(&smap_user_config, &intf_row->user_config);
        smap_replace(&smap_user_config, INTERFACE_USER_CONFIG_MAP_ADMIN,
                OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);
        ovsrec_interface_set_user_config(intf_row, &smap_user_config);

        /*create port table entry*/
        port_row = ovsrec_port_insert(txn);
        ovsrec_port_set_name(port_row, ifname);

        iface_list = xmalloc(sizeof(struct ovsrec_interface));
        iface_list[0] = (struct ovsrec_interface *)intf_row;
        ovsrec_port_set_interfaces(port_row, iface_list, 1);
        free(iface_list);
        ovsrec_port_set_admin(port_row,
                        OVSREC_INTERFACE_ADMIN_STATE_UP);

        OVSREC_VRF_FOR_EACH (vrf_row, idl)
        {
            if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0)
            {
                default_vrf_row = vrf_row;
                break;
            }
        }

        if (default_vrf_row == NULL)
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
        ports[default_vrf_row->n_ports] =
            CONST_CAST(struct ovsrec_port*,port_row);
        ovsrec_vrf_set_ports(default_vrf_row, ports,
                default_vrf_row->n_ports + 1);
        free(ports);

        status_txn = cli_do_config_finish(txn);
        if ((status_txn != TXN_SUCCESS) && (status_txn != TXN_UNCHANGED))
        {
            VLOG_ERR("Transaction commit failed in function=%s, line=%d",
                    __func__,__LINE__);
            return CMD_OVSDB_FAILURE;
        }
    }

    vty->index = ifname;
    vty->node = LOOPBACK_INTERFACE_NODE;

    return CMD_SUCCESS;
}

/*
 * This function mask subnet from the entered ip address
 * using subnet bits.
 * It returns subnet_mask for the  ip4 address passed.
 */
static int
mask_ip4_subnet(const char* ip4)
{
    unsigned int i,j;
    unsigned int address[5];
    unsigned int subnet_mask[4];
    unsigned int ip_subnet_mask = 0;
    unsigned char ip_add[4];
    unsigned char subnet_bits[4];

    snprintf(ip_add, strchr(ip4, '/') - ip4 + 1, "%s", ip4);
    snprintf(subnet_bits, strlen(strchr(ip4, '/')), "%s", strchr(ip4, '/') +1);
    for (i=0; i<4; i++)
    {
        address[i] = atoi(ip_add);
        if (strchr(ip_add,'.') != 0)
            strcpy(ip_add,strchr(ip_add, '.') + 1);
    }

    for (i=0; i< atoi(subnet_bits)/8; i++)
    {
        subnet_mask[i] = address[i];
        ip_subnet_mask += subnet_mask[i];
    }

    if ((atoi(subnet_bits) % 8) != 0)
    {
        subnet_mask[i] = address[i];
        subnet_mask[i] >>= (8 - (atoi(subnet_bits) %8));
        ip_subnet_mask += subnet_mask[i];
    }
    return ip_subnet_mask;
}

/*
 * This function is used to configure an IP address for a port
 * which is attached to a SUB_IF.
 */
static int
loopback_if_config_ip (const char *if_name, const char *ip4)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i;
    bool port_found;
    int input_ip_subnet, port_ip_subnet;

    if (!is_valid_ip_address(ip4))
    {
        if (!IS_LOOPBACK_IPV4(ip4))
        {
            vty_out(vty, "Invalid IP address. %s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }

    input_ip_subnet = mask_ip4_subnet(ip4);
    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (port_row->ip4_address != NULL )
        {
            port_ip_subnet = mask_ip4_subnet(port_row->ip4_address);

            if (input_ip_subnet == port_ip_subnet)
            {
                if (strcmp(port_row->name, if_name) == 0)
                {
                    break;
                }
                else
                {
                    vty_out(vty, "Duplicate IP Address.%s",VTY_NEWLINE);
                    return CMD_SUCCESS;
                }
            }
        }
        else if (port_row->ip4_address_secondary != NULL )
        {
            port_ip_subnet = mask_ip4_subnet(port_row->ip4_address_secondary);

            if (input_ip_subnet == port_ip_subnet)
            {
                vty_out(vty, "Duplicate IP Address.%s",VTY_NEWLINE);
                return CMD_SUCCESS;
            }
        }
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, if_name) == 0)
        {
            port_found = true;
            break;
        }
    }

    if (!port_found)
    {
        vty_out(vty,"Port %s not found.\n",if_name);
        return CMD_SUCCESS;
    }

    if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if ((NULL != ip4) && (NULL != port_row->ip4_address)
            && (strcmp (port_row->ip4_address, ip4) == 0))
    {
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

/*
 * This function is used to delete an IP address assigned for a port
 * which is attached to a SUB_IF.
 */
static int
loopback_if_del_ip4 (const char *if_name, const char *ip4)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    int i, n;
    bool port_found = false;

    status_txn = cli_do_config_start ();

    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, if_name) == 0)
        {
            port_found = true;
            break;
        }
    }

    if (!port_found)
    {
        vty_out(vty,"Port %s not found.\n",if_name);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if ((NULL != ip4) && (NULL != port_row->ip4_address)
            && (strcmp (port_row->ip4_address, ip4) != 0))
    {
        vty_out (vty, "IP address %s not configured.%s",
                ip4, VTY_NEWLINE);
        VLOG_DBG ("%s IP address \"%s\" not configured on interface "
                "\"%s\".", __func__, ip4, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
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

}

/*
 * This function is used to configure an IPv6 address for a port
 * which is attached to a SUB_IF.
 */
static int
loopback_if_config_ipv6 (const char *if_name, const char *ipv6)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i;
    bool is_secondary = false;


    if (!is_valid_ip_address(ipv6))
    {
        vty_out(vty, "Invalid IP address. %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start ();

    if (status_txn == NULL)
    {
        VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort (status_txn);
        return CMD_OVSDB_FAILURE;
    }

    /* Check for spit interface conditions */
    if (!check_split_iface_conditions (if_name))
    {
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    port_row = port_check_and_add (if_name, true, true, status_txn);

    if (check_iface_in_bridge (if_name) &&
            (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" is not attached to any SUB_IF. "
                "It is attached to default bridge",
                __func__, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

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

/*
 * This function is used to delete an IPv6 address assigned for a port
 * which is attached to a VRF.
 * Parameter 1 : loopback interface name
 * Parameter 2 : IPv6 Address
 * Return      : CMD_SUCCESS on pass
 *               CMD_OVSDB_FAILURE on failure
 */
static int
loopback_if_del_ipv6 (const char *if_name, const char *ipv6)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    size_t i, n;

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
        vty_out (vty,"%s Interface does not have any port configuration.%s",
                if_name, VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" does not have any port configuration",
                __func__, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" is not attached to any VRF. "
                "It is attached to default bridge",
                __func__, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if ((NULL != ipv6) && (NULL != port_row->ip6_address) &&
            (strcmp (port_row->ip6_address, ipv6) != 0))
    {
        vty_out (vty, "IPv6 address %s not configured.%s", ipv6, VTY_NEWLINE);
        VLOG_DBG ("%s IPv6 address \"%s\" not configured on interface"
                " \"%s\".",
                __func__, ipv6, if_name);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    ovsrec_port_set_ip6_address (port_row, NULL);

    status = cli_do_config_finish (status_txn);

    if ((status == TXN_SUCCESS) ||(status == TXN_UNCHANGED))
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/*
 * This function will delete the created loopback interface
 * Parameter 1 : interface name
 * Return      : CMD_OVSDB_FAILURE on failure
 *               CMD_SUCCESS on pass
 */
int
delete_loopback_intf(const char *if_name)
{
    const struct ovsrec_interface *row = NULL;
    const struct ovsrec_interface *interface_row = NULL;
    const struct ovsrec_interface *if_row = NULL;

    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_port *loopback_row = NULL;
    struct ovsrec_port **ports;

    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    char loopback_if_name[8]={0};
    bool port_found = false;
    struct ovsrec_interface **interfaces;
    const struct ovsrec_port *loopback_if_port = NULL;
    int i=0, n=0, k=0;
    bool interface_found = false;

    /* Check if the LOOPBACK_INTERFACE port is present in DB. */
    OVSREC_PORT_FOR_EACH(loopback_if_port, idl)
    {
        if (strcmp(loopback_if_port->name, if_name) == 0)
        {
            port_found = true;
            break;
        }
    }
    OVSREC_INTERFACE_FOR_EACH(interface_row, idl)
    {
        if (strcmp(interface_row->name, if_name) == 0)
        {
            interface_found = true;
            break;
        }
    }

    if (!port_found || !interface_found)
    {
        vty_out(vty, "Loopback interface does not exist.\n");
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(SUB_IF_OVSDB_TXN_CREATE_ERROR,__func__,__LINE__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_interface_delete(interface_row);

    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        for (k = 0; k < vrf_row->n_ports; k++)
        {
            loopback_row = vrf_row->ports[k];
            if (strcmp(loopback_row->name, if_name) == 0)
            {
                ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
                for (i = n = 0; i < vrf_row->n_ports; i++)
                {
                    if (vrf_row->ports[i] != loopback_row)
                    {
                        ports[n++] = vrf_row->ports[i];
                    }
                }
                ovsrec_vrf_set_ports(vrf_row, ports, n);
                free(ports);
            }
        }
    }
    ovsrec_port_delete(loopback_if_port);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(SUB_IF_OVSDB_TXN_COMMIT_ERROR,__func__,__LINE__);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN (cli_intf_show_intferface_loopback_if,
        cli_intf_show_intferface_loopback_if_cmd,
        "show interface loopback <0-2147483647>",
        SHOW_STR
        INTERFACE_STR
        "Show details of a loopback interface\n"
        "Select a loopback interface\n")
{
    const struct ovsrec_interface *ifrow = NULL;
    const char *cur_state =NULL;
    struct shash sorted_interfaces;
    const struct shash_node **nodes;
    int idx, count;
    char loopbackintf[MAX_IFNAME_LENGTH] = {0};
    bool filter_intf = false;

    const struct ovsdb_datum *datum;

    unsigned int index;
    int64_t intVal = 0;

    if ((argc > 0) && (NULL != argv[0]))
    {
        sprintf(loopbackintf, "lo%s", argv[0]);
        filter_intf = true;
    }

    vty_out (vty, "%s", VTY_NEWLINE);

    shash_init(&sorted_interfaces);

    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        if (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_LOOPBACK) != 0)
        {
            continue;
        }
        if ((filter_intf) && (0 != strcmp(loopbackintf, ifrow->name)))
        {
            continue;
        }
        shash_add(&sorted_interfaces, ifrow->name, (void *)ifrow);
    }

    nodes = shash_sort(&sorted_interfaces);
    count = shash_count(&sorted_interfaces);

    for (idx = 0; idx < count; idx++)
    {
        union ovsdb_atom atom;

        ifrow = (const struct ovsrec_interface *)nodes[idx]->data;

        vty_out (vty, "Interface %s is %s ", ifrow->name,
                 OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP);

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

        vty_out (vty, " Hardware: Loopback %s", VTY_NEWLINE);

        /* Displaying ipv4 and ipv6 primary and secondary addresses*/
        show_ip_addresses(ifrow->name, vty);

        datum = ovsrec_interface_get_mtu(ifrow, OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }

        datum = ovsrec_interface_get_statistics(ifrow,
                OVSDB_TYPE_STRING, OVSDB_TYPE_INTEGER);
#if 0
        if (NULL==datum) continue;

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
#endif
    }

    shash_destroy(&sorted_interfaces);
    free(nodes);

    return CMD_SUCCESS;
}


DEFUN (cli_intf_show_intferface_loopback,
        cli_intf_show_intferface_loopback_cmd,
        "show interface loopback",
        SHOW_STR
        INTERFACE_STR
        "Show details of a loopback interface\n"
        "Select a loopback interface\n")
{
    return cli_intf_show_intferface_loopback_if (self, vty,
            vty_flags, 0, argv);
}

DEFUN (cli_loopback_if_config_ip4,
        cli_loopback_if_config_ip4_cmd,
        "ip address A.B.C.D/M",
        IP_STR
        "Set IP address\n"
        "Interface IP address\n"
        "Set as secondary IP address\n")
{
    return loopback_if_config_ip((char*) vty->index, argv[0]);
}

DEFUN (cli_loopback_if_del_ip4,
        cli_loopback_if_del_ip4_cmd,
        "no ip address A.B.C.D/M",
        NO_STR
        IP_STR
        "Set IP address\n"
        "Interface IP address\n"
        "Set as secondary IP address\n")
{
    return loopback_if_del_ip4((char*) vty->index, argv[0]);
}

DEFUN (cli_loopback_if_config_ipv6,
        cli_loopback_if_config_ipv6_cmd,
        "ipv6 address X:X::X:X/M",
        IPV6_STR
        "Set IP address\n"
        "Interface IPv6 address\n"
        "Set as secondary IPv6 address\n")
{
    return loopback_if_config_ipv6((char*) vty->index, argv[0]);
}

DEFUN (cli_loopback_if_del_ipv6,
        cli_loopback_if_del_ipv6_cmd,
        "no ipv6 address X:X::X:X/M",
        NO_STR
        IP_STR
        "Set IP address\n"
        "Interface IP address\n"
        "Set as secondary IP address\n")
{
    return loopback_if_del_ipv6((char*) vty->index, argv[0]);
}

DEFUN (vtysh_del_loopback_interface,
        vtysh_del_loopback_interface_cmd,
        "no interface loopback <0-2147483647>",
        NO_STR
        INTF_HELP_STR
        "Select a loopback interface\n"
        "Virtual interface number\n")
{
    static char ifname[MAX_IFNAME_LENGTH]={0};
    snprintf(ifname, MAX_IFNAME_LENGTH, "%s%s", "lo", argv[0]);
    return delete_loopback_intf(ifname);
}


/* Install SUB_IF related vty commands. */
void
loopback_intf_vty_init (void)
{
    /*Configurations at CONFIG_NODE*/
    install_element (CONFIG_NODE,  &vtysh_loopback_interface_cmd);
    install_element (CONFIG_NODE,  &vtysh_del_loopback_interface_cmd);

    /*Configurations at LOOPBACK_INTERFACE_NODE*/
    install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_config_ip4_cmd);
    install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_del_ip4_cmd);
    install_element (LOOPBACK_INTERFACE_NODE,
            &cli_loopback_if_config_ipv6_cmd);
    install_element (LOOPBACK_INTERFACE_NODE, &cli_loopback_if_del_ipv6_cmd);

    /*show cammands at enable node */
    install_element (ENABLE_NODE, &cli_intf_show_intferface_loopback_cmd);
    install_element (ENABLE_NODE, &cli_intf_show_intferface_loopback_if_cmd);
}
