/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0.9

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
*/
/****************************************************************************
 * @ingroup quagga
 *
 * @file l3static_vty.c
 * Source to configure l3 static routes into ovsdb tables.
 *
 ***************************************************************************/

#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "prefix.h"

#include "l3static_vty.h"


VLOG_DEFINE_THIS_MODULE (vtysh_l3static_cli);
extern struct ovsdb_idl *idl;
#define DEFAULT_DISTANCE 1

DEFUN (vtysh_ip_route,
       vtysh_ip_route_cmd,
       "ip route A.B.C.D/M (A.B.C.D|INTERFACE) [<1-255>]",
       IP_STR
       "Establish static routes\n"
       "IP destination prefix (e.g. 10.0.0.0/8)\n"
       "Nexthop IP (eg. 10.0.0.1)\n"
       "Outgoing interface\n"
       "Distance for this route. Default is 1 for static routes\n")
{
    const struct ovsrec_route *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;
    const struct ovsrec_port *row_port = NULL;

    struct in_addr nexthop;
    struct in_addr mask;
    struct prefix_ipv4 p;
    int ret;
    int64_t distance;
    enum ovsdb_idl_txn_status status;
    struct ovsdb_idl_txn *status_txn = NULL;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if(status_txn == NULL) {
        VLOG_ERR("Couldn't create the OVSDB transaction.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }

    row_vrf = ovsrec_vrf_first(idl);
    if(!row_vrf) {
        VLOG_ERR("No vrf information yet.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }
    row = ovsrec_route_insert(status_txn);

    ovsrec_route_set_vrf(row, row_vrf);

    ovsrec_route_set_address_family(row, OVSREC_ROUTE_ADDRESS_FAMILY_IPV4);

    ovsrec_route_set_sub_address_family(row, OVSREC_ROUTE_SUB_ADDRESS_FAMILY_UNICAST);

    ret = str2prefix_ipv4 (argv[0], &p);
    if (ret <= 0) {
        vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    ovsrec_route_set_prefix(row, (const char *)argv[0]);

    ovsrec_route_set_from(row, OVSREC_ROUTE_FROM_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        OVSREC_PORT_FOR_EACH(row_port, idl) {
            if (!strcmp(row_port->name, argv[1])) {
                break;
            }
        }

        if(row_port == NULL) {
            vty_out(vty, "\nInterface %s not configured%s", argv[1], VTY_NEWLINE);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_OVSDB_FAILURE;
        }
        ovsrec_nexthop_set_ports(row_nh, &row_port,row_nh->n_ports + 1);
    }

    if(argc < 3) {
        /*
         * Hardcode the n_distance param to 1 for static routes
         */
        distance = DEFAULT_DISTANCE;
        ovsrec_route_set_distance(row, &distance, 1);
    } else {
        distance = atoi(argv[2]);
        ovsrec_route_set_distance(row, &distance, 1);
    }

    ovsrec_route_set_nexthops(row, &row_nh, row->n_nexthops + 1);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
        && (status != TXN_UNCHANGED))){
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }

}


static int show_routes_ip(struct vty *vty)
{
    const struct ovsrec_route *row_route = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    int flag = 0;
    int disp_flag = 1;
    int len = 0;
    char str[20];

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (!strcmp(row_route->address_family, "ipv4") && disp_flag == 1) {
                flag = 1;
                vty_out (vty, "\nDisplaying IP routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (strcmp(row_route->address_family, "ipv4")) {
            continue;
        }

        if (row_route->prefix) {
            memset(str, 0, sizeof(str));
            len = 0;
            len = snprintf(str, sizeof(str), "%s", row_route->prefix);
            vty_out(vty, "%s", str);
        }

        if (row_route->n_nexthops) {
            vty_out(vty, ",  %d %s next-hops %s", row_route->n_nexthops,
                row_route->sub_address_family, VTY_NEWLINE);
        }

        if (row_route->n_nexthops) {
            memset(str, 0, sizeof(str));
            len = 0;

            if (row_route->nexthops[0]->ip_address) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_route->nexthops[0]->ports[0]->name) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ports[0]->name);
                vty_out(vty, "\t*via %s", str);
            }

        } else {
            vty_out(vty, "\t*via Null0");
        }

        vty_out(vty, ",  [%d", *row_route->distance);

        vty_out(vty, "/%d]", row_route->metric);

        vty_out(vty, ",  %s", row_route->from);

        vty_out(vty, VTY_NEWLINE);

    }

    if (flag == 0) {
        vty_out(vty,"\nNo ip routes configured %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    } else {
        return CMD_SUCCESS;
    }
}

DEFUN (vtysh_show_ip_route,
       vtysh_show_ip_route_cmd,
       "show ip route",
       SHOW_STR
       IP_STR
       ROUTE_STR)
{
    int retval;

    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);

    retval = show_routes_ip(vty);
    vty_out(vty, VTY_NEWLINE);

    return retval;
}

DEFUN (vtysh_no_ip_route,
       vtysh_no_ip_route_cmd,
       "no ip route A.B.C.D/M (A.B.C.D|INTERFACE) [<1-255>]",
       NO_STR
       IP_STR
       "Established static route\n"
       "IP destination prefix (e.g. 10.0.0.0)\n"
       "Nexthop IP (eg. 10.0.0.1)\n"
       "Outgoing interface\n")
{

    const struct ovsrec_route *row_route = NULL;
    int flag = 0;
    char *prefix = argv[0];
    int found_flag = 0;
    int len = 0;
    char str[17];
    int distance_match = 0;

    enum ovsdb_idl_txn_status status;
    struct ovsdb_idl_txn *status_txn = NULL;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);


    if(status_txn == NULL) {
        VLOG_ERR("Couldn't create the OVSDB transaction.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (strcmp(row_route->address_family, "ipv4")) {
                continue;
            }
        }

        if (row_route->prefix != NULL && !strcmp(row_route->address_family, "ipv4")) {
            /* Checking for presence of Prefix and Nexthop entries in a row */
            if (0 == strcmp(argv[0],row_route->prefix )) {
                if (row_route->n_nexthops) {
                    memset(str, 0, sizeof(str));
                    len = 0;
                    distance_match = 0;

                    if (atoi(argv[2])) {
                        (atoi(argv[2]) == *row_route->distance) ? (distance_match = 1)
                            : (distance_match = 0);
                    } else {
                        distance_match = 1;
                    }

                    /* Checking for presence of Nexthop IP or Interface*/
                    if ((row_route->nexthops[0]->ip_address) ||
                        (row_route->nexthops[0]->ports[0]->name)) {
                        if (row_route->nexthops[0]->ip_address != NULL) {
                            if (0 == strcmp(argv[1], row_route->nexthops[0]->ip_address)
                                && (distance_match == 1)) {
                                found_flag = 1;
                                ovsrec_nexthop_delete(row_route->nexthops[0]);
                                ovsrec_route_delete(row_route);
                            }
                        } else if (row_route->nexthops[0]->ports[0]->name != NULL) {
                            if (0 == strcmp(argv[1], row_route->nexthops[0]->ports[0]->name)
                                && (distance_match == 1)) {
                                found_flag = 1;
                                ovsrec_nexthop_delete(row_route->nexthops[0]);
                                ovsrec_route_delete(row_route);
                            }
                        }
                    }
                }
            }
        }
        flag = 1;
    }

    if(flag == 0) {
        vty_out(vty,"No ip routes configured %s", VTY_NEWLINE);
    }

    if (found_flag == 0) {
        vty_out(vty, "No such ip route found %s\n", VTY_NEWLINE);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
                    && (status != TXN_UNCHANGED))) {
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }
}

/* IPv6 CLIs*/

DEFUN (vtysh_ipv6_route,
       vtysh_ipv6_route_cmd,
       "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) [<1-255>]",
       IP_STR
       "Establish static routes\n"
       "IPv6 destination prefix (e.g. 2010:bd9::/32)\n"
       "Nexthop IPv6 (eg. 2010:bda::)\n"
       "Outgoing interface\n"
       "Distance for this route. Default is 1 for static routes\n")
{
    const struct ovsrec_route *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;
    const struct ovsrec_port *row_port = NULL;

    struct in6_addr nexthop;
    struct in6_addr mask;
    struct prefix_ipv6 p;
    int ret;
    int64_t distance;
    enum ovsdb_idl_txn_status status;
    struct ovsdb_idl_txn *status_txn = NULL;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);

    if(status_txn == NULL) {
        VLOG_ERR("Couldn't create the OVSDB transaction.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }

    row_vrf = ovsrec_vrf_first(idl);
    if(!row_vrf) {
        VLOG_ERR("No vrf information yet.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_route_insert(status_txn);

    ovsrec_route_set_vrf(row, row_vrf);

    ovsrec_route_set_address_family(row, OVSREC_ROUTE_ADDRESS_FAMILY_IPV6);

    ovsrec_route_set_sub_address_family(row, OVSREC_ROUTE_SUB_ADDRESS_FAMILY_UNICAST);

    ret = str2prefix_ipv6 (argv[0], &p);
    if (ret <= 0) {
        vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    ovsrec_route_set_prefix(row, (const char *)argv[0]);

    ovsrec_route_set_from(row, OVSREC_ROUTE_FROM_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET6, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        OVSREC_PORT_FOR_EACH(row_port, idl) {
            if (!strcmp(row_port->name, argv[1])) {
                break;
            }
        }

        if(row_port == NULL) {
            vty_out(vty, "\nInterface %s not configured%s", argv[1], VTY_NEWLINE);
            ovsdb_idl_txn_destroy(status_txn);
            status_txn = NULL;
            return CMD_OVSDB_FAILURE;
        }
        ovsrec_nexthop_set_ports(row_nh, &row_port,row_nh->n_ports + 1);
    }

    if(argc < 3) {
        /*
         * Hardcode the n_distance param to 1 for static routes
         */
        distance = DEFAULT_DISTANCE;
        ovsrec_route_set_distance(row, &distance, 1);
    } else {
        distance = atoi(argv[2]);
        ovsrec_route_set_distance(row, &distance, 1);
    }

    ovsrec_route_set_nexthops(row, &row_nh, row->n_nexthops + 1);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
        && (status != TXN_UNCHANGED))) {
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }

}


static int show_routes_ipv6(struct vty *vty)
{
    const struct ovsrec_route *row_route = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    int flag = 0;
    int disp_flag = 1;
    int len = 0;
    char str[50];

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (!strcmp(row_route->address_family, "ipv6") && disp_flag == 1 ) {
                flag = 1;
                vty_out (vty, "\nDisplaying IPv6 routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (strcmp(row_route->address_family, "ipv6")) {
            continue;
        }

        if (row_route->prefix) {
            memset(str, 0, sizeof(str));
            len = 0;
            len = snprintf(str, sizeof(str), "%s", row_route->prefix);
            vty_out(vty, "%s", str);
        }

        if (row_route->n_nexthops) {
            vty_out(vty, ",  %d %s next-hops %s", row_route->n_nexthops,
                row_route->sub_address_family, VTY_NEWLINE);
        }

        if (row_route->n_nexthops) {
            memset(str, 0, sizeof(str));
            len = 0;

            if (row_route->nexthops[0]->ip_address) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_route->nexthops[0]->ports[0]->name) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ports[0]->name);
                vty_out(vty, "\t*via %s", str);
            }

        } else {
            vty_out(vty, "\t*via Null0");
        }

        vty_out(vty, ",  [%d", *row_route->distance);

        vty_out(vty, "/%d]", row_route->metric);

        vty_out(vty, ",  %s", row_route->from);

        vty_out(vty, VTY_NEWLINE);
    }

    if (flag == 0) {
        vty_out(vty,"\nNo ipv6 routes configured %s", VTY_NEWLINE);
        return CMD_SUCCESS;
    } else {
        return CMD_SUCCESS;
    }

}

DEFUN (vtysh_show_ipv6_route,
       vtysh_show_ipv6_route_cmd,
       "show ipv6 route",
       SHOW_STR
       IP_STR
       ROUTE_STR)
{
    int retval;

    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);

    retval = show_routes_ipv6(vty);
    vty_out(vty, VTY_NEWLINE);

    return retval;
}

DEFUN (vtysh_no_ipv6_routet,
       vtysh_no_ipv6_route_cmd,
       "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) [<1-255>]",
       NO_STR
       IP_STR
       "Established static route\n"
       "IP destination prefix (e.g. 2010:bd9::)\n"
       "Nexthop IP (eg. 2010:bda::)\n"
       "Outgoing interface\n")
{

    const struct ovsrec_route *row_route = NULL;
    int flag = 0;
    char *prefix = argv[0];
    int found_flag = 0;
    int len = 0;
    char str[17];
    int distance_match = 0;
    enum ovsdb_idl_txn_status status;
    struct ovsdb_idl_txn *status_txn = NULL;

    ovsdb_idl_run(idl);
    status_txn = ovsdb_idl_txn_create(idl);


    if(status_txn == NULL) {
        VLOG_ERR("Couldn't create the OVSDB transaction.");
        ovsdb_idl_txn_destroy(status_txn);
        status_txn = NULL;
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (strcmp(row_route->address_family, "ipv6")) {
                continue;
            }
        }

        if(row_route->prefix != NULL && !strcmp(row_route->address_family, "ipv6")) {
            /* Checking for presence of Prefix and Nexthop entries in a row */
            if(0 == strcmp(prefix,row_route->prefix )) {
                if (row_route->n_nexthops) {
                    memset(str, 0, sizeof(str));;
                    len = 0;
                    distance_match = 0;

                    if (atoi(argv[2])) {
                        (atoi(argv[2]) == *row_route->distance) ? (distance_match = 1)
                            : (distance_match = 0);
                    } else {
                        distance_match = 1;
                    }

                    /* Checking for presence of Nexthop IP or Interface*/
                    if ((row_route->nexthops[0]->ip_address) ||
                        (row_route->nexthops[0]->ports[0]->name)) {
                        if (row_route->nexthops[0]->ip_address != NULL) {
                            if (0 == strcmp(argv[1], row_route->nexthops[0]->ip_address)
                                && (distance_match == 1)) {
                                found_flag = 1;
                                ovsrec_nexthop_delete(row_route->nexthops[0]);
                                ovsrec_route_delete(row_route);
                            }
                        } else if (row_route->nexthops[0]->ports[0]->name != NULL) {
                            if (0 == strcmp(argv[1], row_route->nexthops[0]->ports[0]->name)
                                && (distance_match == 1)) {
                                found_flag = 1;
                                ovsrec_nexthop_delete(row_route->nexthops[0]);
                                ovsrec_route_delete(row_route);
                            }
                        }
                    }
                }
            }
        }
        flag = 1;
    }

    if(flag == 0) {
        vty_out(vty,"No ipv6 routes configured %s", VTY_NEWLINE);
    }

    if (found_flag == 0) {
        vty_out(vty, "No such ipv6 route found %s\n", VTY_NEWLINE);
    }

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
                    && (status != TXN_UNCHANGED))) {
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }
}


void l3static_vty_init(void) {
    install_element(CONFIG_NODE, &vtysh_ip_route_cmd);
    install_element(ENABLE_NODE, &vtysh_show_ip_route_cmd);
    install_element(CONFIG_NODE, &vtysh_no_ip_route_cmd);

    install_element(CONFIG_NODE, &vtysh_ipv6_route_cmd);
    install_element(ENABLE_NODE, &vtysh_show_ipv6_route_cmd);
    install_element(CONFIG_NODE, &vtysh_no_ipv6_route_cmd);
}
