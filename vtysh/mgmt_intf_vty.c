/* Management Interface CLI commands source file.
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: mgmt_intf_vty.c
 *
 * Purpose: To define and handle configuration required by
 *           management interface.
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
#include "vtysh_utils.h"

VLOG_DEFINE_THIS_MODULE(vtysh_mgmt_int_cli);
extern struct ovsdb_idl *idl;

/* Check management interface is static.
 * On success, returns true. On failure, returns false.
 */
bool
is_mode_static(const struct ovsrec_open_vswitch *ovs)
{
    const char *mode_value = NULL;

    mode_value = smap_get(&ovs->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if (!mode_value || (strcmp(mode_value, "static") != 0))
        return false;

    return true;
}


/* Removes Ipv4, default gw from DB. */
void
mgmt_intf_clear_ipv4_config_db(const struct ovsrec_open_vswitch *row)
{
    const char* ip_addr;

    if (NULL == row)
    {
        VLOG_ERR("Invalid Open_vSwitch row pointer");
        return;
    }

    ip_addr = smap_get(&row->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                                   OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    }

    ip_addr = smap_get(&row->mgmt_intf,
                       OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                                 OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    }
    return;
}

/* Removes Ipv6 default gw and dns server configs from DB. */
void
mgmt_intf_clear_ipv6_config_db(const struct ovsrec_open_vswitch *row)
{
    const char* ip_addr;

    if (NULL == row)
    {
        VLOG_ERR("Invalid Open_vSwitch row pointer");
        return;
    }
    ip_addr = smap_get(&row->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                                  OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    }

    ip_addr = smap_get(&row->mgmt_intf,
                       OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                           OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
    }

    ip_addr = smap_get(&row->mgmt_intf,
                       OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                     OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    }

    ip_addr = smap_get(&row->mgmt_intf,
                        OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (ip_addr != NULL)
    {
        smap_remove((struct smap *)&row->mgmt_intf,
                               OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    }
    return;
}

/* Print management interface configuration.
 */
void
mgmt_intf_show(const struct ovsrec_open_vswitch *row)
{
    const char *val;
    const char *subnet;

    val = smap_get(&row->mgmt_intf,OPEN_VSWITCH_MGMT_INTF_MAP_MODE);
    if (val)
        vty_out(vty, "  Address Mode\t\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Address Mode\t\t\t: dhcp%s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status, OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    subnet = smap_get(&row->mgmt_intf_status,
                      OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);

    if (val && subnet && (strcmp(val,MGMT_INTF_DEFAULT_IP) != 0))
        vty_out(vty, "  IPv4 address/subnet-mask\t: %s/%s%s",val, subnet,
                                                               VTY_NEWLINE);
    else
        vty_out(vty, "  IPv4 address/subnet-mask\t: %s", VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,
                               OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if (val && (strcmp(val,MGMT_INTF_DEFAULT_IP) != 0))
        vty_out(vty, "  Default gateway IPv4\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Default gateway IPv4\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    if (val)
        vty_out(vty, "  IPv6 address/prefix\t\t: %s%s",val, VTY_NEWLINE);
    else
        vty_out(vty, "  IPv6 address/prefix\t\t: %s", VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,
                              OPEN_VSWITCH_MGMT_INTF_MAP_IPV6_LINKLOCAL);
    if (val)
        vty_out(vty, "  IPv6 link local address/prefix: %s%s",
                                                        val, VTY_NEWLINE);
    else
        vty_out(vty, "  IPv6 link local address/prefix: %s", VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,
                          OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
    if (val)
        vty_out(vty, "  Default gateway IPv6\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Default gateway IPv6\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,
                              OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if (val && (strcmp(val,MGMT_INTF_DEFAULT_IP) != 0))
        vty_out(vty, "  Primary Nameserver\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Primary Nameserver\t\t: %s",VTY_NEWLINE);

    val = smap_get(&row->mgmt_intf_status,
                              OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (val && (strcmp(val,MGMT_INTF_DEFAULT_IP) != 0))
        vty_out(vty, "  Secondary Nameserver\t\t: %s%s",val,VTY_NEWLINE);
    else
        vty_out(vty, "  Secondary Nameserver\t\t: %s",VTY_NEWLINE);
}

/* Configure management interface as dhcp mode.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_OVSDB_FAILURE.
 */
static int
mgmt_intf_set_dhcp()
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }


    /* If current mode is static remove static configs from DB. */
    if (is_mode_static(row))
    {
        mgmt_intf_clear_ipv4_config_db(row);
        mgmt_intf_clear_ipv6_config_db(row);
    }

    smap_replace((struct smap *)&row->mgmt_intf,
                  OPEN_VSWITCH_MGMT_INTF_MAP_MODE,
                  OPEN_VSWITCH_MGMT_INTF_MAP_MODE_DHCP);

    ovsrec_open_vswitch_set_mgmt_intf(row, &row->mgmt_intf);

    status = cli_do_config_finish(status_txn);
    if (TXN_SUCCESS == status || TXN_UNCHANGED == status)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/* Check for any static def-gateway or
 * nameserver config before removing static IPv4.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_ERR_NOTHING_TODO.
 */
static int
mgmt_intf_remove_static_ipv4_address(struct smap *smap,
                                     const char *ip,
                                     const char *subnet)
{
    const char *cfg_ip, *cfg_subnet, *cfg_gw, *cfg_dns, *cfg_ipv6;
    struct in_addr addr;

    memset (&addr, 0, sizeof (struct in_addr));

    if (NULL == smap || NULL == ip || NULL == subnet)
    {
        VLOG_ERR("Invalid parameters");
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_ip =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    if (!cfg_ip || strcmp(ip,cfg_ip) != 0)
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_subnet =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
    if (!cfg_subnet || strcmp(subnet,cfg_subnet) != 0)
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_gw =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
    if (cfg_gw && strcmp(MGMT_INTF_DEFAULT_IP,cfg_gw) != 0)
    {
        vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV4_STATIC_CONF,VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_dns =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if (cfg_dns && (inet_pton(AF_INET, cfg_dns,&addr) == 1))
    {
        if (strcmp(MGMT_INTF_DEFAULT_IP,cfg_dns) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV4_STATIC_CONF,VTY_NEWLINE);
            return CMD_ERR_NOTHING_TODO;
        }
    }

    cfg_dns =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (cfg_dns && (inet_pton(AF_INET, cfg_dns,&addr) == 1))
    {
        if (strcmp(MGMT_INTF_DEFAULT_IP,cfg_dns) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV4_STATIC_CONF,VTY_NEWLINE);
            return CMD_ERR_NOTHING_TODO;
        }
    }

    /* If no static IPv6 address then change the mode to DHCP. */
    cfg_ipv6 =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    if (!cfg_ipv6 || strcmp(MGMT_INTF_DEFAULT_IPV6,cfg_ipv6) == 0)
    {
        smap_replace(smap, OPEN_VSWITCH_MGMT_INTF_MAP_MODE,
                           OPEN_VSWITCH_MGMT_INTF_MAP_MODE_DHCP);
        smap_remove(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP);
        smap_remove(smap, OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
    }
    else
    {
        /* Replace defualt IPv4 value in to DB to flush previous
         * config from stack.
         */
        smap_replace(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP,
                                          MGMT_INTF_DEFAULT_IP);
        smap_remove(smap, OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK);
    }
    return CMD_SUCCESS;
}

/* Check for any static def-gateway or nameserver config
 * before removing static IPv6.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_ERR_NOTHING_TODO.
 */
static int
mgmt_intf_remove_static_ipv6_address(struct smap *smap,const char *ipv6)
{
    const char *cfg_ip, *cfg_subnet, *cfg_gw, *cfg_dns, *cfg_ipv6;
    struct in6_addr addrv6;

    memset (&addrv6, 0, sizeof (struct in6_addr));

    if (NULL == smap || NULL == ipv6)
    {
        VLOG_ERR("Invalid parameters");
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_ipv6 =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    if (!cfg_ipv6 || strcmp(ipv6,cfg_ipv6) != 0)
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_gw =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
    if (cfg_gw && strcmp(MGMT_INTF_DEFAULT_IPV6,cfg_gw) != 0)
    {
        vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV6_STATIC_CONF,VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    cfg_dns =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
    if (cfg_dns && (inet_pton(AF_INET6, cfg_dns,&addrv6) == 1))
    {
        if (strcmp(MGMT_INTF_DEFAULT_IPV6,cfg_dns) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV6_STATIC_CONF,VTY_NEWLINE);
            return CMD_ERR_NOTHING_TODO;
        }
    }

    cfg_dns =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
    if (cfg_dns && (inet_pton(AF_INET6, cfg_dns,&addrv6) == 1))
    {
        if (strcmp(MGMT_INTF_DEFAULT_IPV6,cfg_dns) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_REMOVE_IPV6_STATIC_CONF,VTY_NEWLINE);
            return CMD_ERR_NOTHING_TODO;
        }
    }

    /* If no static IPv4 address then change the mode to DHCP. */
    cfg_ip =  smap_get(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP);
    if (!cfg_ip || strcmp(MGMT_INTF_DEFAULT_IP,cfg_ip) == 0)
    {
        smap_replace(smap, OPEN_VSWITCH_MGMT_INTF_MAP_MODE,
                           OPEN_VSWITCH_MGMT_INTF_MAP_MODE_DHCP);
        smap_remove(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6);
    }
    else
    {
    /* Replace defualt IPv6 value in to DB to flush previous
     * config from stack.
     */
        smap_replace(smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6,
                                           MGMT_INTF_DEFAULT_IPV6);
    }
    return CMD_SUCCESS;
}

/* Configuring static ip on management interface.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_OVSDB_FAILURE.
 */
static int
mgmt_intf_set_static(bool set,const char *ip, enum ip_type type)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    const char *subnet = NULL;
    const char *ip_addr = NULL;
    unsigned short subnet_in = 0;
    char buf[MAX_IPV4_OR_IPV6_SUBNET_CIDR_STR_LEN];

    if (!is_valid_ip_address(ip))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    memset(buf, 0, MAX_IPV4_OR_IPV6_SUBNET_CIDR_STR_LEN);
    strncpy(buf, ip, MAX_IPV4_OR_IPV6_SUBNET_CIDR_STR_LEN);
    ip_addr   = strtok(buf,"/");
    subnet    = strtok(NULL,"\0");
    subnet_in = atoi(subnet);

    if (IPV4 == type)
    {
        if (IS_INVALID_IPV4_SUBNET(subnet_in))
        {
            vty_out(vty, "  %s %s",OVSDB_INVALID_SUBNET_ERROR,VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    } else if (IPV6 == type) {
        if (IS_INVALID_IPV6_SUBNET(subnet_in))
        {
            vty_out(vty, "  %s %s",OVSDB_INVALID_SUBNET_ERROR,VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }


    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);

    if (set)
        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_MODE,
                            OPEN_VSWITCH_MGMT_INTF_MAP_MODE_STATIC);

    if (IPV4 == type) {
        if (set)
        {
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP, ip_addr);
            smap_replace(&smap,
                         OPEN_VSWITCH_MGMT_INTF_MAP_SUBNET_MASK, subnet);
        }
        else
        {
            if (mgmt_intf_remove_static_ipv4_address(&smap,ip_addr,subnet)
                                                              != CMD_SUCCESS)
            {
                cli_do_config_abort(status_txn);
                smap_destroy(&smap);
                return CMD_SUCCESS;
            }
        }
    } else if (IPV6 == type) {
        if (set)
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6, ip);
        else
        {
            if (mgmt_intf_remove_static_ipv6_address(&smap,ip) != CMD_SUCCESS)
            {
                cli_do_config_abort(status_txn);
                smap_destroy(&smap);
                return CMD_SUCCESS;
            }
        }
    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);

    smap_destroy(&smap);

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

/* Configuring IPv4 default gateway on management interface.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_OVSDB_FAILURE.
 */
static int
mgmt_intf_set_default_gw(bool set, const char *gw)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    const char *cfg_gw = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    if (!is_valid_ip_address(gw))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }


    smap_clone(&smap, &row->mgmt_intf);

    if (!smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_IP))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        smap_destroy(&smap);
        return CMD_SUCCESS;
    }

    if (set)
    {
        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY, gw);
    }
    else
    {
        cfg_gw = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY);
        if (!cfg_gw || strcmp(gw,cfg_gw) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
            cli_do_config_abort(status_txn);
            smap_destroy(&smap);
            return CMD_SUCCESS;
        }

        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY,
                                                      MGMT_INTF_DEFAULT_IP);
    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);

    smap_destroy(&smap);

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

/* Configuring IPv6 default gateway on management interface.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_OVSDB_FAILURE.
 */
static int
mgmt_intf_set_default_gw_ipv6(bool set, const char *gw_v6)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    const char *cfg_gw = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    if (!is_valid_ip_address(gw_v6))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    smap_clone(&smap, &row->mgmt_intf);

    if (!smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        smap_destroy(&smap);
        return CMD_SUCCESS;
    }

    if (set)
    {
        smap_replace(&smap,
                     OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6, gw_v6);
    }
    else
    {
        cfg_gw = smap_get(&smap,
                          OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6);
        if (!cfg_gw || strcmp(gw_v6,cfg_gw) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
            cli_do_config_abort(status_txn);
            smap_destroy(&smap);
            return CMD_SUCCESS;
        }

        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DEFAULT_GATEWAY_V6,
                                                     MGMT_INTF_DEFAULT_IPV6);

    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);

    smap_destroy(&smap);

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

/* Configuring management interface DNS server.
 * On success, returns CMD_SUCCESS. On failure, returns CMD_OVSDB_FAILURE.
 */
static int
mgmt_intf_set_dns(bool set, const char *dns1, const char *dns2)
{
    const struct ovsrec_open_vswitch *row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    const char *cfg_dns1 = NULL;
    const char *cfg_dns2 = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();
    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_open_vswitch_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->mgmt_intf);

    /* Handle primary DNS server configuration. */
    if (set)
    {
        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1, dns1);

        if (dns2)
        {
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2, dns2);
        }
        else
        {
            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2,
                                                       MGMT_INTF_DEFAULT_IP);
        }
    }
    else
    {
        cfg_dns1 = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1);
        if (!cfg_dns1 || strcmp(dns1,cfg_dns1) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
            cli_do_config_abort(status_txn);
            smap_destroy(&smap);
            return CMD_SUCCESS;
        }

        cfg_dns2 = smap_get(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2);
        if (dns2)
        {
            if (!cfg_dns2 || strcmp(dns2,cfg_dns2) != 0)
            {
                vty_out(vty, "  %s %s",OVSDB_INVALID_VALUE_ERROR,VTY_NEWLINE);
                cli_do_config_abort(status_txn);
                smap_destroy(&smap);
                return CMD_SUCCESS;
            }

            smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_2,
                                                        MGMT_INTF_DEFAULT_IP);
        }
        else if (cfg_dns2 && strcmp(cfg_dns2, MGMT_INTF_DEFAULT_IP) != 0)
        {
            vty_out(vty, "  %s %s",OVSDB_DNS_DEPENDENCY_ERROR,VTY_NEWLINE);
            cli_do_config_abort(status_txn);
            smap_destroy(&smap);
            return CMD_SUCCESS;
        }

        smap_replace(&smap, OPEN_VSWITCH_MGMT_INTF_MAP_DNS_SERVER_1,
                                                        MGMT_INTF_DEFAULT_IP);
    }

    ovsrec_open_vswitch_set_mgmt_intf(row, &smap);

    smap_destroy(&smap);

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

DEFUN (cli_mgmt_intf_set_dns_1,
       mgmt_intf_set_dns_1_cmd,
       "nameserver A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IP))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
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
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IP))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(argv[0], argv[1]) == 0)
    {
        vty_out(vty, "  %s %s",OVSDB_DUPLICATE_VALUE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(true, argv[0], argv[1]);
}

DEFUN (cli_mgmt_intf_set_dns_3,
       mgmt_intf_set_dns_1_ipv6_cmd,
       "nameserver X:X::X:X",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(true, argv[0], NULL);
}

DEFUN (cli_mgmt_intf_set_dns_4,
       mgmt_intf_set_dns_2_ipv6_cmd,
       "nameserver X:X::X:X X:X::X:X",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR
       MGMT_INTF_DNS_2_IPV6_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(argv[0], argv[1]) == 0)
    {
        vty_out(vty, "  %s %s",OVSDB_DUPLICATE_VALUE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(true, argv[0], argv[1]);
}

DEFUN (cli_mgmt_intf_set_dns_5,
       mgmt_intf_set_dns_ipv4_ipv6_cmd,
       "nameserver A.B.C.D X:X::X:X",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_IPV6_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }


    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IP) || \
        !smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(argv[0], argv[1]) == 0)
    {
        vty_out(vty, "  %s %s",OVSDB_DUPLICATE_VALUE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(true, argv[0], argv[1]);
}

DEFUN (cli_mgmt_intf_set_dns_6,
       mgmt_intf_set_dns_ipv6_ipv4_cmd,
       "nameserver X:X::X:X A.B.C.D",
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR
       MGMT_INTF_DNS_2_STR)
{
    const struct ovsrec_open_vswitch *row = NULL;

    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    row = ovsrec_open_vswitch_first(idl);
    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    if (!is_mode_static(row))
    {
        vty_out(vty, "  %s %s",OVSDB_MODE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }


    if (!smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IP) || \
        !smap_get(&row->mgmt_intf, OPEN_VSWITCH_MGMT_INTF_MAP_IPV6))
    {
        vty_out(vty, "  %s %s",OVSDB_NO_IP_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (strcmp(argv[0], argv[1]) == 0)
    {
        vty_out(vty, "  %s %s",OVSDB_DUPLICATE_VALUE_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(true, argv[0], argv[1]);
}

DEFUN (cli_no_mgmt_intf_set_dns_1,
       mgmt_intf_no_set_dns_1_cmd,
       "no nameserver A.B.C.D",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR)
{
    if (!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], NULL);
}

DEFUN (cli_no_mgmt_intf_set_dns_2,
       mgmt_intf_no_set_dns_2_cmd,
       "no nameserver A.B.C.D A.B.C.D",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], argv[1]);
}

DEFUN (cli_no_mgmt_intf_set_dns_3,
       mgmt_intf_no_set_dns_1_ipv6_cmd,
       "no nameserver X:X::X:X",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR)
{
    if (!is_valid_ip_address(argv[0]))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], NULL);
}

DEFUN (cli_no_mgmt_intf_set_dns_4,
       mgmt_intf_no_set_dns_2_ipv6_cmd,
       "no nameserver X:X::X:X X:X::X:X",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR
       MGMT_INTF_DNS_2_IPV6_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], argv[1]);
}

DEFUN (cli_no_mgmt_intf_set_dns_5,
       mgmt_intf_no_set_dns_ipv4_ipv6_cmd,
       "no nameserver A.B.C.D X:X::X:X",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_STR
       MGMT_INTF_DNS_2_IPV6_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], argv[1]);
}

DEFUN (cli_no_mgmt_intf_set_dns_6,
       mgmt_intf_no_set_dns_ipv6_ipv4_cmd,
       "no nameserver X:X::X:X A.B.C.D",
       NO_STR
       MGMT_INTF_DNS_STR
       MGMT_INTF_DNS_1_IPV6_STR
       MGMT_INTF_DNS_2_STR)
{
    if (!is_valid_ip_address(argv[0]) || (!is_valid_ip_address(argv[1])))
    {
        vty_out(vty, "  %s %s",OVSDB_INVALID_IPV4_IPV6_ERROR,VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    return mgmt_intf_set_dns(false, argv[0], argv[1]);
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

    if (!row)
    {
       return CMD_OVSDB_FAILURE;
    }

    mgmt_intf_show(row);

    return CMD_SUCCESS;
}

DEFUN (cli_mgmt_intf_set_mode_dhcp,
          mgmt_intf_set_mode_dhcp_cmd,
          "ip dhcp",
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_DHCP_STR)
{
    return mgmt_intf_set_dhcp();
}

DEFUN (cli_mgmt_intf_set_mode_static,
          mgmt_intf_set_mode_static_cmd,
          "ip static A.B.C.D/M",
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_STATIC_STR
          MGMT_INTF_IPV4_STR)
{
    return mgmt_intf_set_static(true,argv[0],IPV4);
}

DEFUN (cli_mgmt_intf_no_set_mode_static,
          mgmt_intf_no_set_mode_static_cmd,
          "no ip static A.B.C.D/M",
          NO_STR
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_STATIC_STR
          MGMT_INTF_IPV4_STR)
{
    return mgmt_intf_set_static(false,argv[0],IPV4);
}

DEFUN (cli_mgmt_intf_set_mode_static_ipv6,
          mgmt_intf_set_mode_static_ipv6_cmd,
          "ip static X:X::X:X/M",
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_STATIC_STR
          MGMT_INTF_IPV6_STR)
{
    return mgmt_intf_set_static(true,argv[0],IPV6);
}

DEFUN (cli_mgmt_intf_no_set_mode_static_ipv6,
          mgmt_intf_no_set_mode_static_ipv6_cmd,
          "no ip static X:X::X:X/M",
          NO_STR
          MGMT_INTF_MODE_IP_STR
          MGMT_INTF_STATIC_STR
          MGMT_INTF_IPV6_STR)
{
    return mgmt_intf_set_static(false,argv[0],IPV6);
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

DEFUN (cli_mgmt_intf_set_default_gw_ipv6,
          mgmt_intf_set_default_gw_ipv6_cmd,
          "default-gateway X:X::X:X",
          MGMT_INTF_DEFAULT_GW_STR
          MGMT_INTF_IPV6_STR)
{
    return mgmt_intf_set_default_gw_ipv6(true, argv[0]);
}

DEFUN (cli_no_mgmt_intf_set_default_gw_ipv6,
          mgmt_intf_no_set_default_gw_ipv6_cmd,
          "no default-gateway X:X::X:X",
          NO_STR
          MGMT_INTF_DEFAULT_GW_STR
          MGMT_INTF_IPV6_STR)
{
    return mgmt_intf_set_default_gw_ipv6(false, argv[0]);
}

/* Initialize management interface vty.
 */
void
mgmt_intf_vty_init (void)
{
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_mode_dhcp_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_mode_static_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_mode_static_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_mode_static_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE,
                                      &mgmt_intf_no_set_mode_static_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_default_gw_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_default_gw_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_default_gw_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE,
                                       &mgmt_intf_no_set_default_gw_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_1_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_2_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_1_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_2_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_ipv4_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_set_dns_ipv6_ipv4_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_1_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_2_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_1_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_2_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_ipv4_ipv6_cmd);
    install_element (MGMT_INTERFACE_NODE, &mgmt_intf_no_set_dns_ipv6_ipv4_cmd);
    install_element (ENABLE_NODE, &mgmt_intf_show_cmd);
}
