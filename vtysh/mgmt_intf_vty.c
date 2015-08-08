/* Management Interface CLI commands header file
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: mgmt_intf_vty.c
 *
 * Purpose:  To define and handle configuration required by management interface
 */
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "mgmt_intf_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include <arpa/inet.h>
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"

VLOG_DEFINE_THIS_MODULE(vtysh_mgmt_int_cli);
extern struct ovsdb_idl *idl;

static int mgmt_intf_set_dhcp()
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct smap smap_status = SMAP_INITIALIZER(&smap_status);
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if(!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);

    smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_MODE, OPEN_VSWITCH_MGMT_INTF_MAP_MODE_DHCP);

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);

    ovsrec_open_vswitch_set_mgmt_intf_status(row, &smap_status);

    smap_destroy(&smap);
    smap_destroy(&smap_status);

    status = cli_do_config_finish(status_txn);
    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}


DEFUN (cli_mgmt_intf_set_mode_dhcp,
          mgmt_intf_set_mode_dhcp_cmd,
          "ip dhcp",
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_DHCP_STR)
{
    return mgmt_intf_set_dhcp();
}

static int mgmt_intf_set_static(const char *ip, const char *subnet)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct smap smap_status = SMAP_INITIALIZER(&smap_status);
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    if (!is_valid_ip_address(ip))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if(!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);

    smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_MODE, OPEN_VSWITCH_MGMT_INTF_MAP_MODE_STATIC);
    smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP, ip);
    smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK, subnet);


    smap_add_once(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_IP,ip);
    smap_add_once(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK,subnet);

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);
    ovsrec_open_vswitch_set_mgmt_intf_status(row, &smap_status);

    smap_destroy(&smap);
    smap_destroy(&smap_status);

    status = cli_do_config_finish(status_txn);
    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN (cli_mgmt_intf_set_mode_static,
          mgmt_intf_set_mode_static_cmd,
          "ip static A.B.C.D X.X.X.X",
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_STATIC_STR
          MGMT_INTF_IPV4_STR
          MGMT_INTF_SUBNET_STR)
{
    return mgmt_intf_set_static(argv[0], argv[1]);
}


bool is_mode_static(const struct ovsrec_open_vswitch *ovs)
{
    const char *mode_value = NULL;

    mode_value = smap_get(&ovs->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if (!mode_value || (strcmp(mode_value, "static") != 0))
        return false;

    return true;
}

static int mgmt_intf_set_default_gw(bool set, const char *gw)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct smap smap_status = SMAP_INITIALIZER(&smap_status);
    const char *cfg_gw = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    if (!is_valid_ip_address(gw))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if(!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if(!is_mode_static(row))
    {
        VLOG_ERR(OVSDB_MODE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);
    smap_clone(&smap_status, &row->mgmt_intf_status);

    if(set)
    {
        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY, gw);
        smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY, gw);
    }
    else
    {
        cfg_gw = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
        if (!cfg_gw || strcmp(gw,cfg_gw) != 0)
        {
            VLOG_ERR(OVSDB_INVALID_VALUE_ERROR);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }

        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY, MGMT_INTF_DEFAULT_IP);
        smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY, MGMT_INTF_DEFAULT_IP);
    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);
    ovsrec_open_vswitch_set_mgmt_intf_status(row, &smap_status);

    smap_destroy(&smap);
    smap_destroy(&smap_status);

    status = cli_do_config_finish(status_txn);
    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}


DEFUN (cli_mgmt_intf_set_default_gw,
          mgmt_intf_set_default_gw_cmd,
          "default-gateway A.B.C.D",
          MGMT_INTF_DEFAULT_GW_STR
          MGMT_INTF_IPV4_STR)
{
    return mgmt_intf_set_default_gw(true, argv[0]);
}

DEFUN (cli_no_mgmt_intf_set_default_gw,
          mgmt_intf_no_set_default_gw_cmd,
          "no default-gateway A.B.C.D",
          NO_STR
          MGMT_INTF_DEFAULT_GW_STR
          MGMT_INTF_IPV4_STR)
{
    return mgmt_intf_set_default_gw(false, argv[0]);
}

static int mgmt_intf_set_dns(bool set, const char *dns1, const char *dns2)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct smap smap_status = SMAP_INITIALIZER(&smap_status);
    const char *cfg_dns1 = NULL;
    const char *cfg_dns2 = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();
    if(status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if(!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if(!is_mode_static(row))
    {
        VLOG_ERR(OVSDB_MODE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);
    smap_clone(&smap_status, &row->mgmt_intf_status);


    /* Handle primary DNS server configuration */
    if(set)
    {
        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1, dns1);
        smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1, dns1);

        if(dns2)
        {
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, dns2);
            smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, dns2);
        }
        else
        {
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, MGMT_INTF_DEFAULT_IP);
            smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, MGMT_INTF_DEFAULT_IP);
        }
    }
    else
    {
        cfg_dns1 = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
        if (!cfg_dns1 || strcmp(dns1,cfg_dns1) != 0)
        {
            VLOG_ERR(OVSDB_INVALID_VALUE_ERROR);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }

        cfg_dns2 = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
        if(dns2)
        {
            if (!cfg_dns2 || strcmp(dns2,cfg_dns2) != 0)
            {
                VLOG_ERR(OVSDB_INVALID_VALUE_ERROR);
                cli_do_config_abort(status_txn);
                return CMD_OVSDB_FAILURE;
            }

            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, MGMT_INTF_DEFAULT_IP);
            smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, MGMT_INTF_DEFAULT_IP);
        }
        else if (cfg_dns2)
        {
            VLOG_ERR(OVSDB_DNS_DEPENDENCY_ERROR);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }

        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1, MGMT_INTF_DEFAULT_IP);
        smap_replace(&smap_status, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1, MGMT_INTF_DEFAULT_IP);
    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);
    ovsrec_open_vswitch_set_mgmt_intf_status(row, &smap_status);

    smap_destroy(&smap);
    smap_destroy(&smap_status);

    status = cli_do_config_finish(status_txn);
    if(status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN (cli_mgmt_intf_set_dns_1,
       mgmt_intf_set_dns_1_cmd,
       "nameserver A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_STR)
{
    if (!is_valid_ip_address(argv[0]))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    return mgmt_intf_set_dns(true, argv[0], NULL);
}

DEFUN (cli_mgmt_intf_set_dns_2,
       mgmt_intf_set_dns_2_cmd,
       "nameserver A.B.C.D A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    if (strcmp(argv[0], argv[1]) == 0)
    {
        VLOG_ERR(OVSDB_DUPLICATE_VALUE_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    return mgmt_intf_set_dns(true, argv[0], argv[1]);
}

DEFUN (cli_no_mgmt_intf_set_dns_1,
       mgmt_intf_no_set_dns_1_cmd,
       "no nameserver A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR)
{
    if (!is_valid_ip_address(argv[0]))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    return mgmt_intf_set_dns(false, argv[0], NULL);
}

DEFUN (cli_no_mgmt_intf_set_dns_2,
       mgmt_intf_no_set_dns_2_cmd,
       "no nameserver A.B.C.D A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        VLOG_ERR(OVSDB_INVALID_IPV4_ERROR);
        return CMD_ERR_NO_MATCH;
    }

    return mgmt_intf_set_dns(false, argv[0], argv[1]);
}

void mgmt_intf_show(const struct ovsrec_open_vswitch *row)
{
    const char *val;
    const char *subnet;

    val = smap_get(&row->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if(val)
        vty_out(vty, "  Address Mode\t\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Address Mode\t\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    subnet = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);

    if(val && subnet)
        vty_out(vty, "  IP address/subnet-mask\t: %s/%s%s",val, subnet, VTY_NEWLINE);
    else
        vty_out(vty, "  IP address/subnet-mask\t: %s", VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if(val)
        vty_out(vty, "  Default gateway\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Default gateway\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if(val)
        vty_out(vty, "  Primary Nameserver\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Primary Nameserver\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if(val)
        vty_out(vty, "  Secondary Nameserver\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Secondary Nameserver\t\t: %s",VTY_NEWLINE);
}

DEFUN (cli_mgmt_intf_show,
       mgmt_intf_show_cmd,
       "show interface mgmt",
       SHOW_STR
       INTERFACE_STR
       MGMT_INTF_MGMT_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    row = ovsrec_open_vswitch_first(idl);

    if(!row)
    {
       return CMD_OVSDB_FAILURE;
    }

    mgmt_intf_show(row);

    return CMD_SUCCESS;
}

void mgmt_intf_vty_init (void)
{
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_mode_dhcp_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_mode_static_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_default_gw_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_default_gw_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_1_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_2_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_1_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_2_cmd);

    install_element (ENABLE_NODE, &mgmt_intf_show_cmd);
}
