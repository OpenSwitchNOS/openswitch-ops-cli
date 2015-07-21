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

DEFUN (vtysh_ip_route_weight,
       vtysh_ip_route_weight_cmd,
       "ip route A.B.C.D/M (A.B.C.D|INTERFACE) [<1-255>]",
       IP_STR
       "Establish static routes\n"
       "IP destination prefix (e.g. 10.0.0.0/8)\n"
       "Nexthop IP (eg. 10.0.0.1)\n"
       "Nexthop interface\n"
       "Distance for this route. Default is 1 for static routes\n")
{
    const struct ovsrec_route *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;

    struct in_addr nexthop;
    struct in_addr mask;
    struct prefix p;
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

    ovsrec_route_set_prefix(row, argv[0]);

    ovsrec_route_set_from(row, OVSREC_ROUTE_FROM_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        /* The number of NH ports that can be configured from CLI for a static
         * route at a time is 1. So hardcode the last parameter.
         */
        ovsrec_nexthop_set_ports(row_nh, argv[1], 1);
    }

    if(argc < 3) {
        /*
         * Hardcode the n_distance param to 1 for static routes
         */
        distance = 1;
        ovsrec_route_set_distance(row, &distance, 1);
    } else {
        distance = atoi(argv[2]);
        ovsrec_route_set_distance(row, &distance, 1);
    }

    ovsrec_route_set_nexthops(row, &row_nh, row->n_nexthops + 1);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if(((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
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
    int ipv4_flag = 0;

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (!strcmp(row_route->address_family, "ipv4") && disp_flag == 1) {
                flag = 1;
                vty_out (vty, "\nDisplaying IP routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (!strcmp(row_route->address_family, "ipv4")) {
            ipv4_flag = 1;
        }

        if (row_route->prefix && ipv4_flag == 1) {
            char str[20];
            int len = 0;
            len = snprintf(str, sizeof(str), "%s", row_route->prefix);
            vty_out(vty, "%s", str);
        }

        if (row_route->n_nexthops && ipv4_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_route->n_nexthops,
                row_route->sub_address_family, VTY_NEWLINE);
        }

        if (row_route->n_nexthops) {
            char str[17];
            int len = 0;

            if (row_route->nexthops[0]->ip_address && ipv4_flag == 1) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_route->nexthops[0]->ports && ipv4_flag == 1){
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ports);
                vty_out(vty, "\t*via %s", str);
            }

        } else if (ipv4_flag == 1) {
            vty_out(vty, "\t*via Null0");
        }

        if (row_route->distance && ipv4_flag == 1) {
            vty_out(vty, ",  [%d", row_route->distance);
        } else if(ipv4_flag == 1){
            vty_out(vty, ",  [0");
        }

        if (row_route->metric && ipv4_flag == 1) {
            vty_out(vty, "/%d]", row_route->metric);
        } else if (ipv4_flag == 1) {
            vty_out(vty, "/0]");
        }

        if (row_route->from && ipv4_flag == 1) {
            vty_out(vty, ",  %s", row_route->from);
        }

        if (ipv4_flag == 1) {
            vty_out(vty, VTY_NEWLINE);
        }

        ipv4_flag = 0;
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

DEFUN (vtysh_no_ip_route_weight,
       vtysh_no_ip_route_weight_cmd,
       "no ip route A.B.C.D/M (A.B.C.D|INTERFACE) [<1-255>]",
       NO_STR
       IP_STR
       "Established static route\n"
       "IP destination prefix (e.g. 10.0.0.0)\n"
       "Nexthop IP (eg. 10.0.0.1)\n"
       "Nexthop interface\n")
{

    const struct ovsrec_route *row_route = NULL;
    int flag = 0;
    int disp_flag = 0;
    char *prefix = argv[0];
    int found_flag = 0;
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
            if (!strcmp(row_route->address_family, "ipv4")) {
                flag = 1;
                disp_flag = 1;
            }
        }

        if (flag == 1 && disp_flag == 1) {
            if(row_route->prefix != NULL && !strcmp(row_route->address_family, "ipv4")) {
                if(0 == strcmp(argv[0],row_route->prefix )) {
                    if (row_route->n_nexthops) {
                        char str[17];
                        int len = 0;
                        int weight_match = 0;

                        if(atoi(argv[2])) {
                            (atoi(argv[2]) == row_route->nexthops[0]->weight) ? (weight_match = 1)
                                : (weight_match = 0);
                        } else {
                            weight_match = 1;
                        }

                        if ((row_route->nexthops[0]->ip_address) ||
                            (row_route->nexthops[0]->ports)) {
                            if(row_route->nexthops[0]->ip_address != NULL) {
                                if(0 == strcmp(argv[1], row_route->nexthops[0]->ip_address)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_route->nexthops[0]);
                                    ovsrec_route_delete(row_route);
                                }
                            } else if(row_route->nexthops[0]->ports != NULL) {
                                if(0 == strcmp(argv[1], row_route->nexthops[0]->ports)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_route->nexthops[0]);
                                    ovsrec_route_delete(row_route);
                                }
                            }
                        }
                    }
                }
            }
        }
        disp_flag = 0;
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

    if(((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
                    && (status != TXN_UNCHANGED))) {
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }
}

/* IPv6 CLIs*/

DEFUN (vtysh_ipv6_route_weight,
       vtysh_ipv6_route_weight_cmd,
       "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) [<1-255>]",
       IP_STR
       "Establish static routes\n"
       "IPv6 destination prefix (e.g. 2010:bd9::/32)\n"
       "Nexthop IPv6 (eg. 2010:bda::)\n"
       "Nexthop interface\n"
       "Distance for this route. Default is 1 for static routes\n")
{
    const struct ovsrec_route *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;

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

    ovsrec_route_set_prefix(row, argv[0]);

    ovsrec_route_set_from(row, OVSREC_ROUTE_FROM_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET6, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        /* The number of NH ports that can be configured from CLI for a static
         * route at a time is 1. So hardcode the last parameter.
         */
        ovsrec_nexthop_set_ports(row_nh, argv[1], 1);
    }

    if(argc < 3) {
        /*
         * Hardcode the n_distance param to 1 for static routes
         */
        distance = 1;
        ovsrec_route_set_distance(row, &distance, 1);
    } else {
        distance = atoi(argv[2]);
        ovsrec_route_set_distance(row, &distance, 1);
    }

    ovsrec_route_set_nexthops(row, &row_nh, row->n_nexthops + 1);

    status = ovsdb_idl_txn_commit_block(status_txn);
    ovsdb_idl_txn_destroy(status_txn);
    status_txn = NULL;

    if(((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
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
    int ipv6_flag = 0;

    OVSREC_ROUTE_FOR_EACH(row_route, idl) {
        if (row_route->address_family != NULL) {
            if (!strcmp(row_route->address_family, "ipv6") && disp_flag == 1 ) {
                flag = 1;
                vty_out (vty, "\nDisplaying IPv6 routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (!strcmp(row_route->address_family, "ipv6")) {
            ipv6_flag = 1;
        }

        if (row_route->prefix && ipv6_flag == 1) {
            char str[50];
            int len = 0;
            len = snprintf(str, sizeof(str), "%s", row_route->prefix);
            vty_out(vty, "%s", str);
        }

        if (row_route->n_nexthops && ipv6_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_route->n_nexthops,
                row_route->sub_address_family, VTY_NEWLINE);
        }

        if (row_route->n_nexthops) {
            char str[50];
            int len = 0;

            if (row_route->nexthops[0]->ip_address && ipv6_flag == 1) {
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_route->nexthops[0]->ports && ipv6_flag == 1){
                len = snprintf(str, sizeof(str), " %s",
                    row_route->nexthops[0]->ports);
                vty_out(vty, "\t*via %s", str);
            }

        } else if (ipv6_flag == 1){
            vty_out(vty, "\t*via Null0");
        }

        if (row_route->distance && ipv6_flag == 1) {
            vty_out(vty, ",  [%d", row_route->distance);
        } else if (!strcmp(row_route->address_family, "ipv6")) {
            vty_out(vty, ",  [0");
        }

        if (row_route->metric && ipv6_flag == 1) {
            vty_out(vty, "/%d]", row_route->metric);
        } else if (!strcmp(row_route->address_family, "ipv6")) {
            vty_out(vty, "/0]");
        }

        if (row_route->from && ipv6_flag == 1) {
            vty_out(vty, ",  %s", row_route->from);
        }

        if (ipv6_flag == 1) {
            vty_out(vty, VTY_NEWLINE);
        }

        ipv6_flag = 0;
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

DEFUN (vtysh_no_ipv6_route_weight,
       vtysh_no_ipv6_route_weight_cmd,
       "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) [<1-255>]",
       NO_STR
       IP_STR
       "Established static route\n"
       "IP destination prefix (e.g. 2010:bd9::)\n"
       "Nexthop IP (eg. 2010:bda::)\n"
       "Nexthop interface\n")
{

    const struct ovsrec_route *row_route = NULL;
    int flag = 0;
    int disp_flag = 0;
    char *prefix = argv[0];
    int found_flag = 0;
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
            if (!strcmp(row_route->address_family, "ipv6")) {
                flag = 1;
                disp_flag = 1;
            }
        }
        if (flag == 1 && disp_flag == 1) {
            if(row_route->prefix != NULL && !strcmp(row_route->address_family, "ipv6")) {
                if(0 == strcmp(prefix,row_route->prefix )) {
                    if (row_route->n_nexthops) {
                        char str[17];
                        int len = 0;
                        int weight_match = 0;

                        if(atoi(argv[2])) {
                            (atoi(argv[2]) == row_route->nexthops[0]->weight) ? (weight_match = 1)
                                : (weight_match = 0);
                        } else {
                            weight_match = 1;
                        }

                        if ((row_route->nexthops[0]->ip_address) ||
                            (row_route->nexthops[0]->ports)) {
                            if(row_route->nexthops[0]->ip_address != NULL) {
                                if(0 == strcmp(argv[1], row_route->nexthops[0]->ip_address)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_route->nexthops[0]);
                                    ovsrec_route_delete(row_route);
                                }
                            } else if(row_route->nexthops[0]->ports != NULL) {
                                if(0 == strcmp(argv[1], row_route->nexthops[0]->ports)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_route->nexthops[0]);
                                    ovsrec_route_delete(row_route);
                                }
                            }
                        }
                    }
                }
            }
        }
        disp_flag = 0;
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

    if(((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
                    && (status != TXN_UNCHANGED))) {
        VLOG_ERR("Commiting transaction to DB failed!");
        return CMD_OVSDB_FAILURE;
    } else {
        return CMD_SUCCESS;
    }
}


void l3static_vty_init(void) {
    install_element(CONFIG_NODE, &vtysh_ip_route_weight_cmd);
    install_element(ENABLE_NODE, &vtysh_show_ip_route_cmd);
    install_element(CONFIG_NODE, &vtysh_no_ip_route_weight_cmd);

    install_element(CONFIG_NODE, &vtysh_ipv6_route_weight_cmd);
    install_element(ENABLE_NODE, &vtysh_show_ipv6_route_cmd);
    install_element(CONFIG_NODE, &vtysh_no_ipv6_route_weight_cmd);
}
