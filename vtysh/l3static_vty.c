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
       "Weight for this route. Default is 1 hop\n")
{
    const struct ovsrec_rib *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;

    struct in_addr nexthop;
    struct in_addr mask;
    struct prefix p;
    char *token;
    int ret;
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

    row = ovsrec_rib_insert(status_txn);

    ovsrec_rib_set_vrf(row, row_vrf);

    ovsrec_rib_set_address_family(row, OVSREC_RIB_ADDRESS_FAMILY_IPV4);

    ovsrec_rib_set_sub_address_family(row, OVSREC_RIB_SUB_ADDRESS_FAMILY_UNICAST);

    ret = str2prefix_ipv4 (argv[0], &p);
    if (ret <= 0) {
        vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    token = strtok(argv[0], "/");
    ovsrec_rib_set_prefix(row, (const char *)token);

    token = strtok(NULL, "/");
    ovsrec_rib_set_prefix_len(row, atoi(token));

    ovsrec_rib_set_from_protocol(row, OVSREC_RIB_FROM_PROTOCOL_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        ovsrec_nexthop_set_port(row_nh, argv[1]);
    }

    if(argc < 3) {
        ovsrec_rib_set_distance(row, 1);
        ovsrec_nexthop_set_weight(row_nh, 1);
    } else {
        ovsrec_rib_set_distance(row, atoi(argv[2]));
        ovsrec_nexthop_set_weight(row_nh, atoi(argv[2]));
    }

    ovsrec_rib_set_nexthop_list(row, &row_nh, row->n_interface_list+1);

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


static void show_routes_ip(struct vty *vty)
{
    const struct ovsrec_rib *row_rib = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    int flag = 0;
    int disp_flag = 1;
    int ipv4_flag = 0;

    OVSREC_RIB_FOR_EACH(row_rib, idl) {
        if (row_rib->address_family != NULL) {
            if (!strcmp(row_rib->address_family, "ipv4") && disp_flag == 1) {
                flag = 1;
                vty_out (vty, "\nDisplaying IP routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (!strcmp(row_rib->address_family, "ipv4")) {
            ipv4_flag = 1;
        }

        if (row_rib->prefix && ipv4_flag == 1) {
            char str[20];
            int len = 0;
            len = snprintf(str, sizeof(str), "%s/%d", row_rib->prefix, row_rib->prefix_len);
            vty_out(vty, "%s", str);
        }

        if (row_rib->n_nexthop_list && ipv4_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_rib->n_nexthop_list,
                row_rib->sub_address_family, VTY_NEWLINE);
        }

        if (row_rib->n_interface_list && ipv4_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_rib->n_interface_list,
                row_rib->sub_address_family, VTY_NEWLINE);
        }

        if (row_rib->n_nexthop_list) {
            char str[17];
            int len = 0;

            if (row_rib->nexthop_list[0]->ip_address && ipv4_flag == 1) {
                len = snprintf(str, sizeof(str), " %s",
                    row_rib->nexthop_list[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_rib->nexthop_list[0]->port && ipv4_flag == 1){
                len = snprintf(str, sizeof(str), " %s",
                    row_rib->nexthop_list[0]->port);
                vty_out(vty, "\t*via %s", str);
            }

        } else if (ipv4_flag == 1) {
            vty_out(vty, "\t*via Null0");
        }

        if (row_rib->distance && ipv4_flag == 1) {
            vty_out(vty, ",  [%d", row_rib->distance);
        } else if(ipv4_flag == 1){
            vty_out(vty, ",  [0");
        }

        if (row_rib->metric && ipv4_flag == 1) {
            vty_out(vty, "/%d]", row_rib->metric);
        } else if (ipv4_flag == 1) {
            vty_out(vty, "/0]");
        }

        if (row_rib->from_protocol && ipv4_flag == 1) {
            vty_out(vty, ",  %s", row_rib->from_protocol);
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
    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);

    show_routes_ip(vty);
    vty_out(vty, VTY_NEWLINE);
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

    const struct ovsrec_rib *row_rib = NULL;
    int flag = 0;
    int disp_flag = 0;
    char prefix;
    int prefix_len = 0;
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

    prefix = strtok(argv[0], "/");
    prefix_len = atoi(strtok(NULL, "/"));

    OVSREC_RIB_FOR_EACH(row_rib, idl) {
        if (row_rib->address_family != NULL) {
            if (!strcmp(row_rib->address_family, "ipv4")) {
                flag = 1;
                disp_flag = 1;
            }
        }

        if (flag == 1 && disp_flag == 1) {
            if(row_rib->prefix != NULL && !strcmp(row_rib->address_family, "ipv4")) {
                if(0 == strcmp(argv[0],row_rib->prefix )) {
                    if (row_rib->n_nexthop_list) {
                        char str[17];
                        int len = 0;
                        int weight_match = 0;

                        if(atoi(argv[2])) {
                            (atoi(argv[2]) == row_rib->nexthop_list[0]->weight) ? (weight_match = 1)
                                : (weight_match = 0);
                        } else {
                            weight_match = 1;
                        }

                        if ((row_rib->nexthop_list[0]->ip_address) ||
                            (row_rib->nexthop_list[0]->port)) {
                            if(row_rib->nexthop_list[0]->ip_address != NULL) {
                                if(0 == strcmp(argv[1], row_rib->nexthop_list[0]->ip_address)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_rib->nexthop_list[0]);
                                    ovsrec_rib_delete(row_rib);
                                }
                            } else if(row_rib->nexthop_list[0]->port != NULL) {
                                if(0 == strcmp(argv[1], row_rib->nexthop_list[0]->port)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_rib->nexthop_list[0]);
                                    ovsrec_rib_delete(row_rib);
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
       "Weight for this route. Default is 1 hop\n")
{
    const struct ovsrec_rib *row = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    const struct ovsrec_vrf *row_vrf = NULL;

    struct in6_addr nexthop;
    struct in6_addr mask;
    struct prefix_ipv6 p;
    char *token;
    int ret;
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

    row = ovsrec_rib_insert(status_txn);

    ovsrec_rib_set_vrf(row, row_vrf);

    ovsrec_rib_set_address_family(row, OVSREC_RIB_ADDRESS_FAMILY_IPV6);

    ovsrec_rib_set_sub_address_family(row, OVSREC_RIB_SUB_ADDRESS_FAMILY_UNICAST);

    ret = str2prefix_ipv6 (argv[0], &p);
    if (ret <= 0) {
        vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    token = strtok(argv[0], "/");
    ovsrec_rib_set_prefix(row, (const char *)token);

    token = strtok(NULL, "/");
    ovsrec_rib_set_prefix_len(row, atoi(token));

    ovsrec_rib_set_from_protocol(row, OVSREC_RIB_FROM_PROTOCOL_STATIC);

    row_nh = ovsrec_nexthop_first(idl);

    row_nh = ovsrec_nexthop_insert(status_txn);

    ret = inet_pton (AF_INET6, argv[1], &nexthop);
    if(ret) {
        ovsrec_nexthop_set_ip_address(row_nh, argv[1]);
    } else {
        ovsrec_nexthop_set_port(row_nh, argv[1]);
    }

    if(argc < 3) {
        ovsrec_rib_set_distance(row, 1);
        ovsrec_nexthop_set_weight(row_nh, 1);
    } else {
        ovsrec_nexthop_set_weight(row_nh, atoi(argv[2]));
        ovsrec_rib_set_distance(row, atoi(argv[2]));
    }

    ovsrec_rib_set_nexthop_list(row, &row_nh, row->n_interface_list+1);

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


static void show_routes_ipv6(struct vty *vty)
{
    const struct ovsrec_rib *row_rib = NULL;
    const struct ovsrec_nexthop *row_nh = NULL;
    int flag = 0;
    int disp_flag = 1;
    int ipv6_flag = 0;

    OVSREC_RIB_FOR_EACH(row_rib, idl) {
        if (row_rib->address_family != NULL) {
            if (!strcmp(row_rib->address_family, "ipv6") && disp_flag == 1 ) {
                flag = 1;
                vty_out (vty, "\nDisplaying IPv6 routes %s\n", VTY_NEWLINE);
                disp_flag = 0;
            }
        }

        if (!strcmp(row_rib->address_family, "ipv6")) {
            ipv6_flag = 1;
        }

        if (row_rib->prefix && ipv6_flag == 1) {
            char str[50];
            int len = 0;
            len = snprintf(str, sizeof(str), "%s/%d", row_rib->prefix, row_rib->prefix_len);
            vty_out(vty, "%s", str);
        }

        if (row_rib->n_nexthop_list && ipv6_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_rib->n_nexthop_list,
                row_rib->sub_address_family, VTY_NEWLINE);
        }

        if (row_rib->n_interface_list && ipv6_flag == 1) {
            vty_out(vty, ",  %d %s next-hops %s", row_rib->n_interface_list,
                row_rib->sub_address_family, VTY_NEWLINE);
        }

        if (row_rib->n_nexthop_list) {
            char str[50];
            int len = 0;

            if (row_rib->nexthop_list[0]->ip_address && ipv6_flag == 1) {
                len = snprintf(str, sizeof(str), " %s",
                    row_rib->nexthop_list[0]->ip_address);
                vty_out(vty, "\t*via %s", str);
            } else if (row_rib->nexthop_list[0]->port && ipv6_flag == 1){
                len = snprintf(str, sizeof(str), " %s",
                    row_rib->nexthop_list[0]->port);
                vty_out(vty, "\t*via %s", str);
            }

        } else if (ipv6_flag == 1){
            vty_out(vty, "\t*via Null0");
        }

        if (row_rib->distance && ipv6_flag == 1) {
            vty_out(vty, ",  [%d", row_rib->distance);
        } else if (!strcmp(row_rib->address_family, "ipv6")) {
            vty_out(vty, ",  [0");
        }

        if (row_rib->metric && ipv6_flag == 1) {
            vty_out(vty, "/%d]", row_rib->metric);
        } else if (!strcmp(row_rib->address_family, "ipv6")) {
            vty_out(vty, "/0]");
        }

        if (row_rib->from_protocol && ipv6_flag == 1) {
            vty_out(vty, ",  %s", row_rib->from_protocol);
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
    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);

    show_routes_ipv6(vty);
    vty_out(vty, VTY_NEWLINE);
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

    const struct ovsrec_rib *row_rib = NULL;
    int flag = 0;
    int disp_flag = 0;
    char *prefix;
    int prefix_len = 0;
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

    prefix = strtok(argv[0], "/");
    prefix_len = atoi(strtok(NULL, "/"));

    OVSREC_RIB_FOR_EACH(row_rib, idl) {
        if (row_rib->address_family != NULL) {
            if (!strcmp(row_rib->address_family, "ipv6")) {
                flag = 1;
                disp_flag = 1;
            }
        }
        if (flag == 1 && disp_flag == 1) {
            if(row_rib->prefix != NULL && !strcmp(row_rib->address_family, "ipv6")) {
                if(0 == strcmp(prefix,row_rib->prefix ) && (prefix_len == row_rib->prefix_len)) {
                    if (row_rib->n_nexthop_list) {
                        char str[17];
                        int len = 0;
                        int weight_match = 0;

                        if(atoi(argv[2])) {
                            (atoi(argv[2]) == row_rib->nexthop_list[0]->weight) ? (weight_match = 1)
                                : (weight_match = 0);
                        } else {
                            weight_match = 1;
                        }

                        if ((row_rib->nexthop_list[0]->ip_address) ||
                            (row_rib->nexthop_list[0]->port)) {
                            if(row_rib->nexthop_list[0]->ip_address != NULL) {
                                if(0 == strcmp(argv[1], row_rib->nexthop_list[0]->ip_address)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_rib->nexthop_list[0]);
                                    ovsrec_rib_delete(row_rib);
                                }
                            } else if(row_rib->nexthop_list[0]->port != NULL) {
                                if(0 == strcmp(argv[1], row_rib->nexthop_list[0]->port)
                                    && (weight_match == 1)) {
                                    found_flag = 1;
                                    ovsrec_nexthop_delete(row_rib->nexthop_list[0]);
                                    ovsrec_rib_delete(row_rib);
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
