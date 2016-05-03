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
#include "vrf_vty.h"
#include "vtysh/vtysh_utils.h"
#include "vtysh/utils/vlan_vtysh_utils.h"
#include "vtysh/utils/intf_vtysh_utils.h"

VLOG_DEFINE_THIS_MODULE (vtysh_sub_intf_cli);
extern struct ovsdb_idl *idl;


/*
 * This function mask subnet from the entered IP address
 * using subnet bits.
 * Parameter 1 : IPv4 address
 * Return      : subnet_mask for the ip4 address
 */
static int
mask_ip4_subnet(const char* ip4)
{
   char ipAddressString[24]="";
   int mask_bits = 0, addr = 0;
   unsigned int i = 0;
   unsigned int subnet_bits = 0;

   mask_bits = atoi(strchr(ip4,'/') + 1);
   strcpy(ipAddressString, ip4);
   strcpy(strchr(ipAddressString, '/'), "\0");

   inet_pton(AF_INET, ipAddressString, &addr);

   while(i < mask_bits)
       subnet_bits |= (1 << i++);

   return (addr & subnet_bits);
}

/*
 * CLI "shutdown"
 * default : enabled
 */
DEFUN (cli_sub_intf_shutdown,
        cli_sub_intf_shutdown_cmd,
        "shutdown",
        "Enable/disable an interface. Default disabled.\n")
{
    const struct ovsrec_interface * row = NULL;
    const struct ovsrec_port * port_row = NULL;
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

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, (char*)vty->index) == 0)
        {
            if (vty_flags & CMD_FLAG_NO_CMD)
            {
                ovsrec_port_set_admin(port_row,
                        OVSREC_INTERFACE_ADMIN_STATE_UP);
            }
            else
            {
                ovsrec_port_set_admin(port_row,
                        OVSREC_INTERFACE_ADMIN_STATE_DOWN);
            }
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

/*VLAN Encapsulation configuration.*/
DEFUN  (cli_encapsulation_dot1Q_vlan,
        cli_encapsulation_dot1Q_vlan_cmd,
        "encapsulation dot1Q <1-4094>",
        "Set encapsulation type for an interface\n"
        "IEEE 802.1Q  VLAN\n"
        "VLAN identifier\n")
{
    const struct ovsrec_interface * row = NULL, *parent_intf_row = NULL;
    const struct ovsrec_interface * tmp_row = NULL;
    struct ovsdb_idl_txn* status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int64_t *key_subintf_parent;
    struct ovsrec_interface  **val_subintf_parent;
    int new_size = 1;
    int64_t enc_vlan = 0;

    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_INTERFACE_FOR_EACH(tmp_row, idl)
    {
        if (check_internal_vlan(atoi(argv[0])) == 0)
        {
           vty_out(vty, "Error : Vlan ID is an internal vlan.%s",
                   VTY_NEWLINE);
           cli_do_config_abort(status_txn);
           return CMD_SUCCESS;
        }
    }

    OVSREC_INTERFACE_FOR_EACH(tmp_row, idl)
    {
        if (strcmp(tmp_row->name, (char*)vty->index) == 0)
        {
            row = tmp_row;
        }
    }

    if (row->n_subintf_parent > 0)
    {
        parent_intf_row = row->value_subintf_parent[0];
    }

    OVSREC_INTERFACE_FOR_EACH(tmp_row, idl)
    {
        if (strcmp(tmp_row->name, (char*)vty->index) != 0)
        {
            if ((tmp_row->n_subintf_parent > 0)
                    && (tmp_row->value_subintf_parent[0] == parent_intf_row)
                    && (tmp_row->key_subintf_parent[0] == atoi(argv[0])))
            {
                vty_out(vty, "Encapsulation VLAN is already"
                        " configured on interface %s. %s",
                        tmp_row->name, VTY_NEWLINE);
                cli_do_config_abort(status_txn);
                return CMD_SUCCESS;
            }
        }
    }


    if (vty_flags & CMD_FLAG_NO_CMD)
    {
        enc_vlan = 0;
    }
    else
    {
        enc_vlan = atoi(argv[0]);
    }
    if (row->n_subintf_parent > 0)
    {
        parent_intf_row = row->value_subintf_parent[0];
    }

    key_subintf_parent = xmalloc(sizeof(int64_t) * new_size);
    if (key_subintf_parent != NULL)
    {
        val_subintf_parent = xmalloc(sizeof(struct ovsrec_interface *)
                * new_size);
        if (val_subintf_parent != NULL)
        {
            key_subintf_parent[0] = enc_vlan;
            val_subintf_parent[0] = (struct ovsrec_interface *)parent_intf_row;

            ovsrec_interface_set_subintf_parent(row, key_subintf_parent,
                    val_subintf_parent, new_size);

            free(val_subintf_parent);
        }
        free(key_subintf_parent);
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
        "encapsulation dot1Q <1-4094>",
        "Set encapsulation type for an interface\n"
        "IEEE 802.1Q  VLAN\n"
        "VLAN identifier\n");

/*
 * This function is used to configure an IP address for a port
 * which is attached to a sub interface.
 * This function accepts 2 arguments both as const char type.
 * Parameter 1 : sub-intf name
 * Parameter 2 : ipv4 address
 * Return      : On Success it returns CMD_SUCCESS
 *               On Failure it returns CMD_OVSDB_FAILURE
 */
static int
sub_intf_config_ip (const char *if_name, const char *ip4)
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    bool port_found;
    int input_ip_subnet, port_ip_subnet;

    if (!is_valid_ip_address(ip4))
    {
        vty_out(vty, "Invalid IP address. %s", VTY_NEWLINE);
        return CMD_SUCCESS;
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
        else if (port_row->n_ip4_address_secondary)
        {
            int i = 0;
            for (i = 0; i < port_row->n_ip4_address_secondary; i++){
                port_ip_subnet = mask_ip4_subnet(port_row->ip4_address_secondary);

                if (input_ip_subnet == port_ip_subnet)
                {
                    vty_out(vty, "Duplicate IP Address.%s",VTY_NEWLINE);
                    return CMD_SUCCESS;
                }
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
        vty_out(vty, "Port %s not found.%s", if_name, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (check_iface_in_bridge (if_name) && (VERIFY_VLAN_IFNAME (if_name) != 0))
    {
        vty_out (vty, "Interface %s is not L3.%s", if_name, VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" is not attached to any SUB_IF.%s"
                "It is attached to default bridge",
                __func__, if_name, VTY_NEWLINE);
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

DEFUN (cli_sub_intf_vrf_add_port,
    cli_sub_intf_vrf_add_port_cmd,
    "vrf attach VRF_NAME",
    VRF_STR
    "Attach Sub Interface to VRF\n"
    "VRF name\n")
{
  return vrf_add_port((char*) vty->index, argv[0]);
}

DEFUN (cli_sub_intf_vrf_del_port,
    cli_sub_intf_vrf_del_port_cmd,
    "no vrf attach VRF_NAME",
    NO_STR
    VRF_STR
    "Detach Sub Interface from VRF\n"
    "VRF name\n")
{
  return vrf_del_port((char*) vty->index, argv[0]);
}

DEFUN (cli_sub_intf_config_ip4,
        cli_sub_intf_config_ip4_cmd,
        "ip address A.B.C.D/M",
        IP_STR
        "Set IP address\n"
        "Sub Interface IP address\n")
{
    return sub_intf_config_ip((char*) vty->index, argv[0]);
}

DEFUN (cli_sub_intf_del_ip4,
        cli_sub_intf_del_ip4_cmd,
        "no ip address A.B.C.D/M",
        NO_STR
        IP_STR
        "Delete IP address\n"
        "Delete Sub Interface IP address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    bool port_found = false;
    const char *if_name = (char*)vty->index;
    char ip4[IP_ADDRESS_LENGTH];

    if (NULL != argv[0])
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

    if (!port_found)
    {
        vty_out(vty,"Port %s not found.%s", if_name, VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    if (!port_row->ip4_address)
    {
        VLOG_DBG ("%s No IPv4 address configured on interface \"%s\".%s",
                  __func__, if_name, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if ((NULL != ip4) && (NULL != port_row->ip4_address)
            && (strcmp (port_row->ip4_address, ip4) != 0))
    {
        vty_out (vty, "IPv4 address %s not configured.%s", ip4, VTY_NEWLINE);
        VLOG_DBG ("%s IPv4 address \"%s\" not configured on interface "
                "\"%s\".%s", __func__, ip4, if_name, VTY_NEWLINE);
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
        "Set IPv6 address\n"
        "Interface IPv6 address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    bool is_secondary = false;

    const char *if_name = (char*) vty->index;
    const char *ipv6 = argv[0];

    if (!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "Invalid IP address.%s", VTY_NEWLINE);
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

    if (check_ip_addr_duplicate (ipv6, port_row, true, &is_secondary))
    {
        vty_out (vty, "IPv6 address is already assigned to interface %s"
                " as %s.%s",
                if_name, is_secondary ? "secondary" : "primary", VTY_NEWLINE);
        VLOG_DBG ("%s Interface \"%s\" already has the IP address \"%s\""
                " assigned to it as \"%s\".%s",
                __func__, if_name, ipv6,
                is_secondary ? "secondary" : "primary", VTY_NEWLINE);
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
        IPV6_STR
        "Set IP address\n"
        "Interface IP address\n")
{
    const struct ovsrec_port *port_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    const char *if_name = (char*) vty->index;
    const char *ipv6 = NULL;

    if (NULL != argv[0])
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
        VLOG_DBG ("%s Interface \"%s\" does not have any port configuration.%s",
                __func__, if_name, VTY_NEWLINE);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if (!port_row->ip6_address)
    {
        vty_out (vty, "No IPv6 address configured on interface"
                " %s.%s", if_name, VTY_NEWLINE);
        VLOG_DBG ("%s No IPv6 address configured on interface"
                " \"%s\".%s", __func__, if_name, VTY_NEWLINE);
        cli_do_config_abort (status_txn);
        return CMD_SUCCESS;
    }

    if ((NULL != ipv6) && (strcmp (port_row->ip6_address, ipv6) != 0))
    {
        vty_out (vty, "IPv6 address %s not found.%s", ipv6, VTY_NEWLINE);
        VLOG_DBG ("%s IPv6 address \"%s\" not configured on interface"
                " \"%s\".%s", __func__, ipv6, if_name, VTY_NEWLINE);
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

/*
 * This function is used to display subinterface configurations.
 * This function accepts 2 arguments.
 * Parameter 1 : interface row
 * Parameter 2 : bool for breif description
 * Return      : It returns CMD_SUCCESS
 */
int
cli_show_subinterface_row(const struct ovsrec_interface *ifrow, bool brief)
{
    const char *cur_state =NULL;
    const struct ovsrec_interface *if_parent_row = NULL;
    const struct ovsdb_datum *datum;

    unsigned int index;
    int64_t intVal = 0;
    union ovsdb_atom atom;
    int64_t key_subintf_parent = 0;

    if ((NULL == ifrow) ||
            (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0))
    {
        return CMD_SUCCESS;
    }
    if (ifrow->n_subintf_parent > 0)
    {
        if_parent_row = ifrow->value_subintf_parent[0];
    }

    if (brief)
    {
        int64_t key_subintf_parent = 0;
        if (ifrow->n_subintf_parent > 0)
        {
            key_subintf_parent = ifrow->key_subintf_parent[0];
        }

        show_subinterface_status(ifrow, brief, if_parent_row, key_subintf_parent);

        intVal = 0;
        datum = ovsrec_interface_get_link_speed(if_parent_row,
                OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }

        if (intVal == 0)
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

        if (ifrow->n_subintf_parent > 0)
        {
            key_subintf_parent = ifrow->key_subintf_parent[0];
        }

        show_subinterface_status(ifrow, brief, if_parent_row, key_subintf_parent);

        if (0 != key_subintf_parent)
        {
            vty_out (vty, " Encapsulation dot1Q %lu %s",
                    key_subintf_parent, VTY_NEWLINE);
        }
        else
        {
            vty_out (vty, " Encapsulation dot1Q not configured %s",
                    VTY_NEWLINE);
        }


        vty_out (vty, " Hardware: Ethernet, MAC Address: %s %s",
                if_parent_row->mac_in_use, VTY_NEWLINE);

        /* Displaying IPV4 and IPV6 primary and secondary addresses. */
        show_ip_addresses(ifrow->name, vty);

        datum = ovsrec_interface_get_mtu(if_parent_row, OVSDB_TYPE_INTEGER);
        if ((NULL!=datum) && (datum->n >0))
        {
            intVal = datum->keys[0].integer;
        }

        cur_state = if_parent_row->pause;
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
        show_l3_stats(vty, ifrow);
    }

    return CMD_SUCCESS;
}

/*
 * This function is used to display status information of Subinterfaces
 * This function accepts 4 arguments.
 * Parameter 1 : interface row
 * Parameter 2 : bool for brief description
 * Parameter 3 : parent interface row for this subinterface
 * Parameter 4 : subinterface vlan id
 * Return      : void
 */
void
show_subinterface_status(const struct ovsrec_interface *ifrow, bool brief,
        const struct ovsrec_interface *if_parent_row, int64_t key_subintf_parent)
{
    bool parent_intf_down = false;

    if ((NULL != if_parent_row->admin_state) &&
            (strcmp(if_parent_row->admin_state,
                    OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN) == 0))
        parent_intf_down = true;

    if (brief)
    {
        /* Display brief information. */
        vty_out (vty, " %-12s ", ifrow->name);
        vty_out(vty, "%4lu    ", key_subintf_parent); /*VLAN */
        vty_out(vty, "eth  "); /*type */
        vty_out(vty, "routed "); /* mode - routed or not */

        vty_out (vty, "%-6s ",
                ((strcmp(if_parent_row->link_state,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP) == 0) &&
                ((ifrow->admin_state != NULL) &&
                 (strcmp(ifrow->admin_state,
                    OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN)))) ?
                ifrow->link_state :
                if_parent_row->link_state);

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
    }

    else
    {
        vty_out (vty, "Interface %s is %s.%s", ifrow->name,
                ((strcmp(if_parent_row->link_state,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP) == 0) &&
                (smap_get(&ifrow->user_config, "admin") != NULL)) ?
                OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP :
                OVSREC_INTERFACE_USER_CONFIG_ADMIN_DOWN, VTY_NEWLINE);

        if ((NULL != ifrow->admin_state)
                && strcmp(ifrow->admin_state,
                        OVSREC_INTERFACE_USER_CONFIG_ADMIN_UP) == 0)
        {
            vty_out (vty, "%s", VTY_NEWLINE);
            vty_out (vty, " Admin state is up%s", VTY_NEWLINE);
        }
        else
        {
            vty_out (vty, "(Administratively down) %s", VTY_NEWLINE);
            vty_out (vty, " Admin state is down%s", VTY_NEWLINE);
        }

        vty_out (vty, " Parent interface is %s %s",
                if_parent_row->name, VTY_NEWLINE);
        if (parent_intf_down)
        {
            vty_out (vty, " Parent interface is administratively down%s",
                    VTY_NEWLINE);
        }

        if (if_parent_row->error != NULL)
        {
            vty_out (vty, " State information: %s%s",
                    if_parent_row->error, VTY_NEWLINE);
        }
    }
}

/* Comparator for sorting sub interfaces */
static int
compare_nodes_by_subintf_id (const void *a_, const void *b_)
{
    const struct shash_node *const *a = a_;
    const struct shash_node *const *b = b_;
    unsigned int id_intf1 = 0, id_intf2 = 0;
    unsigned long tag_sub_intf1 = 0, tag_sub_intf2 = 0;

    /* Extract the interface and sub-interface id */
    sscanf((*a)->name, "%u.%lu", &id_intf1, &tag_sub_intf1);
    sscanf((*b)->name, "%u.%lu", &id_intf2, &tag_sub_intf2);

    if(id_intf1 == id_intf2) {
        if (tag_sub_intf1 == tag_sub_intf2) {
            return 0;
        }
        else if (tag_sub_intf1 < tag_sub_intf2) {
            return -1;
        }
         else {
            return 1;
        }
    }
    else if (id_intf1 < id_intf2) {
        return -1;
    }
    else {
        return 1;
    }
}

const struct shash_node **
sort_sub_interfaces (const struct shash *sh)
{
    if (shash_is_empty(sh)) {
        return NULL;
    } else {
        const struct shash_node **nodes;
        struct shash_node *node;

        size_t i, n;

        n = shash_count(sh);
        nodes = xmalloc(n * sizeof *nodes);
        i = 0;
        SHASH_FOR_EACH (node, sh) {
            nodes[i++] = node;
        }
        ovs_assert(i == n);

        qsort(nodes, n, sizeof *nodes, compare_nodes_by_subintf_id);
        return nodes;
    }
}

DEFUN (cli_intf_show_subintferface_ifname,
        cli_intf_show_subintferface_ifname_cmd,
        "show interface A.B {brief}",
        SHOW_STR
        INTERFACE_STR
        "Show sub-interface details\n"
        "Show brief information of a sub-interface\n")
{
    const struct ovsrec_interface *ifrow = NULL;
    bool brief = false;

    if ((NULL != argv[1]) && (strcmp(argv[1], "brief") == 0))
    {
        brief = true;
        /* Display the brief information. */
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "---------------------------------------------------"
                     "-----------------------------%s", VTY_NEWLINE);
        vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason   "
                     "                Speed    Port%s", VTY_NEWLINE);
        vty_out(vty, "Interface                                          "
                     "                (Mb/s)   Ch# %s", VTY_NEWLINE);
        vty_out(vty, "----------------------------------------------------"
                     "----------------------------%s", VTY_NEWLINE);
    }

    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        if (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
        {
            continue;
        }
        if (strcmp(ifrow->name, argv[0]) != 0)
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
    bool brief = false;
    const struct shash_node **nodes;
    int idx, count;

    if ((argv[0] != NULL) && (strchr(argv[0], '.'))){
         return CMD_ERR_NO_MATCH;
    }
    if ((NULL != argv[1]) && (strcmp(argv[1], "brief") == 0))
    {
        brief = true;
        /* Display the brief information. */
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "---------------------------------------------------"
                     "-----------------------------%s", VTY_NEWLINE);
        vty_out(vty, "Ethernet      VLAN    Type Mode   Status  Reason   "
                     "                Speed    Port%s", VTY_NEWLINE);
        vty_out(vty, "Interface                                          "
                     "                (Mb/s)   Ch# %s", VTY_NEWLINE);
        vty_out(vty, "----------------------------------------------------"
                     "----------------------------%s", VTY_NEWLINE);
    }

    if (NULL != argv[0])
    {
        strcpy(subif_prefix, argv[0]);
        subif_ptr = strtok(subif_prefix, ".");
    }

    shash_init(&sorted_interfaces);

    OVSREC_INTERFACE_FOR_EACH(ifrow, idl)
    {
        shash_add(&sorted_interfaces, ifrow->name, (void *)ifrow);
    }

    nodes = sort_sub_interfaces(&sorted_interfaces);
    count = shash_count(&sorted_interfaces);

    for (idx = 0; idx < count; idx++)
    {
        ifrow = (const struct ovsrec_interface *)nodes[idx]->data;

        if (strcmp(ifrow->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
        {
            continue;
        }
        if (strncmp(ifrow->name,subif_ptr, strlen(subif_ptr)) != 0)
        {
            continue;
        }
        cli_show_subinterface_row(ifrow, brief);
    }

    shash_destroy(&sorted_interfaces);
    free(nodes);
}

DEFUN (vtysh_sub_interface,
        vtysh_sub_interface_cmd,
        "interface A.B",
        "Select an interface to configure\n"
        "Subinterface name as physical_interface.subinterface name\n")
{
    return create_sub_interface(argv[0]);
}

/*
 * This function will create sub interface in port table and interface
 * table. It accepts only one argument of char pointer type.
 * Parameter 1 : sub-intf name to be created
 * Return      :  On Success it returns CMD_SUCCESS
 *                On Failure it returns CMD_OVSDB_FAILURE
 */
int
create_sub_interface(char* subifname)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *intf_row, *parent_intf_row = NULL;
    struct ovsrec_interface **iface_list;
    struct ovsdb_idl_txn *txn = NULL;
    enum ovsdb_idl_txn_status status_txn;
    static char ifnumber[MAX_IFNAME_LENGTH]={0};
    const struct ovsrec_vrf *default_vrf_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_interface  **val_subintf_parent;
    struct ovsrec_port **ports = NULL;
    int64_t *key_subintf_parent;
    bool port_found = false, parent_intf_found = false;
    char phy_intf[MAX_IFNAME_LENGTH];
    char sub_intf[MAX_IFNAME_LENGTH];
    long long int sub_intf_number;
    int max_sub_intf = 0;
    int new_size = 1;
    int i = 0;

    if(NULL == strchr(subifname, '.'))
    {
        /* Wrong format. */
        return CMD_ERR_NO_MATCH;
    }
    memcpy(ifnumber, subifname, MAX_IFNAME_LENGTH);

    sub_intf_number = (long long int)atoi(strchr(subifname, '.') + 1);

    if ((sub_intf_number < MIN_SUB_INTF_RANGE) ||
            (sub_intf_number > MAX_SUB_INTF_RANGE)){
        vty_out (vty, "Invalid input %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    memcpy(phy_intf, subifname, MAX_IFNAME_LENGTH);
    for (i = 0; i < strlen(phy_intf); i++)
    {
        if(phy_intf[i] == '.')
        {
            phy_intf[i] = '\0';
        }
    }

    sprintf(sub_intf, "%s.%lu", phy_intf, sub_intf_number);
    if (strcmp(sub_intf, subifname) != 0)
    {
        vty_out (vty, "Invalid input.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    sprintf(sub_intf, "%lu", sub_intf_number);

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, ifnumber) == 0)
        {
            vty->index = ifnumber;
            vty->node = SUB_INTERFACE_NODE;
            return CMD_SUCCESS;
        }
    }

    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, phy_intf) == 0)
        {
            parent_intf_row = intf_row;
            parent_intf_found = true;
        }
        if (strcmp(intf_row->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) == 0)
        {
            if (strncmp (intf_row->name, phy_intf, strlen(phy_intf)) == 0)
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

    if (!parent_intf_found)
    {
        vty_out (vty, "Parent interface does not exist.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    /* Check if port row is part of bridge. */
    if (check_iface_in_bridge(phy_intf))
    {
        vty_out(vty, "Parent interface is not L3. "
                "Enable routing on the parent interface.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        if (strcmp(port_row->name, ifnumber) == 0)
        {
            port_found = true;
            if(port_row->n_interfaces > 1)
            {
                vty_out(vty, "Subinterface is not supported on LAG.%s",
                        VTY_NEWLINE);
                return CMD_SUCCESS;
            }
            break;
        }
    }

    if (!port_found)
    {
        txn = cli_do_config_start();
        if (txn == NULL)
        {
            VLOG_DBG("Transaction creation failed by %s.%s",
                    " cli_do_config_start()", VTY_NEWLINE);
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        /* Adding an interface  table entry. */
        intf_row = ovsrec_interface_insert(txn);
        ovsrec_interface_set_name(intf_row, ifnumber);
        ovsrec_interface_set_type(intf_row, OVSREC_INTERFACE_TYPE_VLANSUBINT);

        /* Set the parent interface & encapsulation vlan id. */
        key_subintf_parent = xmalloc(sizeof(int64_t) * new_size);
        if (key_subintf_parent != NULL)
        {
           val_subintf_parent = xmalloc(sizeof(struct ovsrec_interface *)
                   * new_size);
           if (val_subintf_parent != NULL)
           {
              key_subintf_parent[0] = 0;
              val_subintf_parent[0] = (struct ovsrec_interface *)parent_intf_row;

              ovsrec_interface_set_subintf_parent(intf_row, key_subintf_parent,
                      val_subintf_parent, new_size);

              free(val_subintf_parent);
           }
           free(key_subintf_parent);
        }

        /* Create parent port row, if not present. */
        port_row = port_check_and_add (phy_intf, true, true, txn);

        /* Create port table entry. */
        port_row = ovsrec_port_insert(txn);
        ovsrec_port_set_name(port_row, ifnumber);

        /* Adding a port to the corresponding interface. */
        iface_list = xmalloc(sizeof(struct ovsrec_interface));
        if (iface_list != NULL)
        {
           iface_list[0] = (struct ovsrec_interface *)intf_row;
           ovsrec_port_set_interfaces(port_row, iface_list, 1);
           free(iface_list);
        }

        OVSREC_VRF_FOR_EACH (vrf_row, idl)
        {
            if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
                default_vrf_row = vrf_row;
                break;
            }
        }

        if (default_vrf_row == NULL)
        {
            assert(0);
            VLOG_DBG("Couldn't fetch default VRF row. Function=%s, Line=%d.%s",
                    __func__, __LINE__, VTY_NEWLINE);
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        ports = xmalloc(sizeof *default_vrf_row->ports *
                (default_vrf_row->n_ports + 1));
        if (ports != NULL)
        {
           for (i = 0; i < default_vrf_row->n_ports; i++)
           {
               ports[i] = default_vrf_row->ports[i];
           }
           ports[default_vrf_row->n_ports] =
               CONST_CAST(struct ovsrec_port*, port_row);
           ovsrec_vrf_set_ports(default_vrf_row, ports,
                   default_vrf_row->n_ports + 1);
           free(ports);
        }

        status_txn = cli_do_config_finish(txn);

        if (status_txn == TXN_SUCCESS || status_txn == TXN_UNCHANGED)
        {
            vty->index = ifnumber;
            vty->node = SUB_INTERFACE_NODE;
            return CMD_SUCCESS;
        }
        else
        {
            VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
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

DEFUN (cli_del_sub_intf,
        cli_del_sub_intf_cmd,
        "no interface A.B",
        NO_STR
        "Select a subinterface to remove\n"
        "Subinterface name as physical_interface.subinterface name\n")
{
    return delete_sub_intf(argv[0]);
}

/*
 * This function is used to delete subinterface
 * This function accepts 1 argument as const char type.
 * Parameter 1 : sub-intf name
 * Return      :  On Success it returns CMD_SUCCESS
 *                On Failure it returns CMD_OVSDB_FAILURE
 */
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

    /* Find the interface table entry and delete it. */
    OVSREC_INTERFACE_FOR_EACH(if_row, idl)
    {
        if (strcmp(if_row->name, sub_intf_name) == 0)
        {
            interface_row = if_row;
            break;
        }
    }
    if (NULL == interface_row)
    {
        vty_out(vty, "Interface does not exist.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(interface_row->type, OVSREC_INTERFACE_TYPE_VLANSUBINT) != 0)
    {
        vty_out(vty, "Interface does not exist.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovsrec_interface_delete(interface_row);

    /* Find the port table entry and delete it. */
    OVSREC_PORT_FOR_EACH(sub_intf_port_row, idl)
    {
        if (strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
        {
            port_found = true;
        }
    }

    if (!port_found)
    {
        /* Commit the interface row deletion and return. */
        status = cli_do_config_finish(status_txn);

        if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
        {
            VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
            return CMD_SUCCESS;
        }
        else
        {
            VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
            return CMD_OVSDB_FAILURE;
        }
    }

    /* Check if the given sub interface port is part of VRF. */
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        for (k = 0; k < vrf_row->n_ports; k++)
        {
            sub_intf_port_row = vrf_row->ports[k];
            if (strcmp(sub_intf_port_row->name, sub_intf_name) == 0)
            {
                ports = xmalloc(sizeof *vrf_row->ports * (vrf_row->n_ports-1));
                if (ports != NULL)
                {
                   for (i = n = 0; i < vrf_row->n_ports; i++)
                   {
                       if (vrf_row->ports[i] != sub_intf_port_row)
                       {
                           ports[n++] = vrf_row->ports[i];
                       }
                   }
                   ovsrec_vrf_set_ports(vrf_row, ports, n);
                   free(ports);
               }
            }
        }
    }

    ovsrec_port_delete(sub_intf_port_row);

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

/* Install sub interface related vty commands. */
void
sub_intf_vty_init (void)
{
    install_element (CONFIG_NODE, &vtysh_sub_interface_cmd);
    install_element (CONFIG_NODE, &cli_del_sub_intf_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_vrf_add_port_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_vrf_del_port_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ip4_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ip4_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_config_ipv6_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_del_ipv6_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_sub_intf_shutdown_cmd);
    install_element (SUB_INTERFACE_NODE, &no_cli_sub_intf_shutdown_cmd);
    install_element (SUB_INTERFACE_NODE, &cli_encapsulation_dot1Q_vlan_cmd);
    install_element (SUB_INTERFACE_NODE, &no_cli_encapsulation_dot1Q_vlan_cmd);
    install_element (ENABLE_NODE, &cli_intf_show_subintferface_ifname_cmd);
    install_element (ENABLE_NODE, &cli_intf_show_subintferface_if_all_cmd);
}
