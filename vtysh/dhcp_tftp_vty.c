/*
 * (C) Copyright 2015 Hewlett Packard Enterprise Development LP
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0.9
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/****************************************************************************
 * @ingroup dhcp_tftp_vty.c
 *
 * @file dhcp_tftp_vty.c
 * Source to configure dhcp-tftp server configuration details
 *
 ***************************************************************************/
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
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "prefix.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "dhcp_tftp_vty.h"

VLOG_DEFINE_THIS_MODULE (vtysh_dhcp_tftp_cli);
extern struct ovsdb_idl *idl;

#define DHCP_LEASE_SCRIPT "/usr/bin/dhcp_leases"

bool is_tag_or_name_valid (char *tag)
{
    if (strlen(tag) > MAX_DHCP_CONFIG_NAME_LENGTH) {
        return false;
    }

    return true;

}

bool is_tags_valid ( char *tags_list)
{
    char *list=(char *)malloc(strlen(tags_list));
    char *token = NULL;

    strcpy(list,tags_list);
    token = strtok(list,",");

    while (token != NULL) {
        if (!is_tag_or_name_valid(token)) {
            vty_out(vty,"%s is invalid %s", token, VTY_NEWLINE);
            free(list);
            return false;
        } else {
            token=strtok(NULL,",");
        }
    }

    free (list);

    return true;
}

bool is_valid_netmask (char *netmask)
{
    unsigned int i,bytes[4],temp=0;

    sscanf(netmask,"%u.%u.%u.%u",
                    &bytes[3], &bytes[2], &bytes[1], &bytes[0]);

    for (i = 0; i < 4; ++i) {
         temp += bytes[i] << (i * 8);
    }

    temp = ~temp + 1;
    if ((temp & (temp - 1)) == 0) {
        return true;
    }

    return false;
}

bool is_valid_mac_address (char *mac_addr)
{
    int i = 0,j = 0;

    while (*mac_addr) {
        if (isxdigit(*mac_addr)) {
            i++;
        } else if (*mac_addr == ':' || *mac_addr == '-') {
            if (i == 0 || i / 2 - 1 != j) {
                break;
            }
            ++j;

        } else {
            j = -1;
        }
        ++mac_addr;
    }

    if (i == 12 && j == 5) {
        return true;
    }

    return false;
}

bool  is_mac_addresses_valid (char *mac_addresses_list)
{
    char *list=(char *)malloc(strlen(mac_addresses_list));
    char *token = NULL;

    strcpy(list, mac_addresses_list);
    token = strtok(list,",");

    while (token != NULL) {
        if (!is_valid_mac_address(token)) {
            vty_out(vty, "%s is invalid%s", token, VTY_NEWLINE);
            free(list);
            return false;

        } else {
            token=strtok(NULL,",");
        }
    }

    free (list);

    return true;
}

bool is_valid_net (char *start_ip_address,char *end_ip_address,char *netmask)
{
    struct sockaddr_in start_address,end_address,subnet_mask;

    start_address.sin_family = AF_INET;
    end_address.sin_family = AF_INET;
    subnet_mask.sin_family = AF_INET;

    inet_aton(start_ip_address, &start_address.sin_addr);
    inet_aton(end_ip_address, &end_address.sin_addr);
    inet_aton(netmask, &subnet_mask.sin_addr);

    if ((start_address.sin_addr.s_addr & subnet_mask.sin_addr.s_addr) == \
        (end_address.sin_addr.s_addr & subnet_mask.sin_addr.s_addr)) {
        return true;
    }

    return false;

}

int ip_type(char *ip_value)
{
    struct in_addr addr;
    struct in6_addr addrv6;

    memset (&addr, 0, sizeof (struct in_addr));
    memset (&addrv6, 0, sizeof (struct in6_addr));

    if(NULL == ip_value) {
       return -2;
    }

    if(inet_pton(AF_INET, ip_value, &addr) == 1) {
        return 0;
    } else if(inet_pton(AF_INET6, ip_value, &addrv6) == 1) {
        return 1;
    }

    return -1;
}

bool is_valid_broadcast_addr(char *start_ip,char *netmask,char *broadcast_ip)
{
    struct in_addr start_address,subnet_mask,broad_ip;
    char *broadcast_address=(char *)malloc(INET_ADDRSTRLEN);

    memset (&start_address, 0, sizeof (struct in_addr));
    memset (&subnet_mask, 0, sizeof (struct in_addr));
    memset (&broad_ip, 0, sizeof (struct in_addr));

    inet_pton (AF_INET, start_ip, &start_address);
    inet_pton (AF_INET, netmask, &subnet_mask);

    broad_ip.s_addr = start_address.s_addr | ~subnet_mask.s_addr;
    inet_ntop (AF_INET, &broad_ip, broadcast_address,INET_ADDRSTRLEN);

    if (strcmp(broadcast_ip, broadcast_address) == 0) {
        return true;
    } else {
        return false;
    }
}

static int show_dhcp_leases(voidn)
{
    char cmd_buff[256];
    char time[256], mac_addr[256], ip_addr[256];
    char hostname[256], client_id[1024];
    FILE *leasestream;
    unsigned long lease_expiry;
    time_t lease_time;
    char *expiry_str;
    int length;
    bool print_header = 1;

    strcpy(cmd_buff, DHCP_LEASE_SCRIPT);
    strcat(cmd_buff, " show");

    leasestream = popen(cmd_buff, "r");

    if (leasestream) {
        while (fscanf(leasestream, "%s", time)!= EOF) {
            if (fscanf(leasestream, "%s %s %s %s",
                       mac_addr, ip_addr, hostname, client_id) != 4) {
                /*
                 * We should always have mac_addr, ip_addr, hostname and
                 * client-id for each entry in leases db. If not, log error
                 * and break out of the loop.
                 */
                VLOG_ERR("DHCP leases entry is incomplete in leases DB.");
                vty_out(vty, "DHCP leases entry is incomplete in leases DB.%s",
                              VTY_NEWLINE);
                break;
            }

            if (print_header) {
                vty_out(vty, "Expiry Time                MAC Address         "
                             "IP Address       Hostname and Client-id%s",
                              VTY_NEWLINE);
                vty_out(vty, "-----------------------------------------------"
                             "---------------------------------------%s",
                              VTY_NEWLINE);
                print_header = 0;
            }

            lease_expiry = atol(time);
            lease_time = (time_t)lease_expiry;
            expiry_str = ctime(&lease_time);

            length = strlen(expiry_str) - 1;
            if (length >= 0 && expiry_str[length] == '\n') {
                expiry_str[length] = '\0';
            }

            vty_out(vty, "%-26s %-19s %-16s %s       %s%s",
                          expiry_str, mac_addr, ip_addr,
                          hostname, client_id,
                          VTY_NEWLINE);

        }

        pclose(leasestream);
    }

    if (print_header) {
        vty_out(vty, "No DHCP leases in the database.%s",
                      VTY_NEWLINE);
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}


static int show_dhcp_range(void)
{
    const struct ovsrec_dhcpsrv_range *row = NULL;
    size_t i;
    char prefix_len[8], lease_duration[16];
    bool print_header = 1;

    row = ovsrec_dhcpsrv_range_first(idl);

    vty_out(vty,"%sDHCP dynamic IP allocation configuration%s",
                 VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "----------------------------------------%s",
                 VTY_NEWLINE);

    if (!row) {
        vty_out(vty, "DHCP IP address range is not "
                     "configured.%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    }

    OVSREC_DHCPSRV_RANGE_FOR_EACH(row, idl){

        if (row->prefix_len) {
            snprintf(prefix_len, 8, "%ld", row->prefix_len[0]);
        } else {
            snprintf(prefix_len, 8, "%s", "*");
        }

        if (row->lease_duration) {
            snprintf(lease_duration, 16, "%ld", row->lease_duration[0]);
        } else {
            snprintf(lease_duration, 16, "%s", "*");
        }

        if (print_header) {
            vty_out(vty, "Name              Start IP Address                 "
                         "             End IP Address                        "
                         "        Netmask          Broadcast        Prefix-len"
                         "  Lease time(min)  Static  Set tag          Match "
                         "tags%s",  VTY_NEWLINE);
            vty_out(vty, "----------------------------------------------------"
                         "----------------------------------------------------"
                         "----------------------------------------------------"
                         "----------------------------------------------------"
                         "%s", VTY_NEWLINE);

            print_header = 0;
        }

        vty_out(vty, "%-17s %-45s %-45s %-16s %-16s %-11s %-16s %-7s %-16s ",
                     row->name, row->start_ip_address,
                     row->end_ip_address ? row->end_ip_address : "*",
                     row->netmask ? row->netmask : "*",
                     row->broadcast ? row->broadcast : "*",
                     prefix_len, lease_duration,
                     row->is_static && *row->is_static == 1 ? "True" : "False",
                     row->set_tag ? row->set_tag : "*");

        if (row->n_match_tags == 0) {
            vty_out(vty, "*");
        }
        for (i = 0; i < row->n_match_tags; i++) {
            vty_out(vty,"%s",row->match_tags[i]);
            if (i != (row->n_match_tags - 1)) {
                vty_out(vty, ",");
            }
        }

        vty_out(vty,"%s",VTY_NEWLINE);
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;

}


static int show_dhcp_host(void)
{
    const struct ovsrec_dhcpsrv_static_host *row = NULL;
    size_t i;
    char lease_duration[16];
    bool print_header = 1;

    row = ovsrec_dhcpsrv_static_host_first(idl);

    vty_out(vty, "%sDHCP static IP allocation configuration %s",
                   VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "--------------------------------------- %s",
                  VTY_NEWLINE);

    if (!row) {
        vty_out(vty, "DHCP static host is not configured.%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    }

    OVSREC_DHCPSRV_STATIC_HOST_FOR_EACH(row, idl){

        if (row->lease_duration) {
            snprintf(lease_duration, 16, "%ld", row->lease_duration[0]);
        } else {
            snprintf(lease_duration, 16, "%s", "*");
        }

        if (print_header) {
            vty_out(vty, "IP Address                                     "
                         "Hostname          Client-id         Lease time(min) "
                         " MAC-Address        Set tags%s", VTY_NEWLINE);
            vty_out(vty, "-----------------------------------------------"
                         "----------------------------------------------------"
                         "----------------------------%s", VTY_NEWLINE);

            print_header = 0;
        }

        vty_out(vty, "%-46s %-17s %-17s %-16s ",
                     row->ip_address,
                     row->client_hostname ? row->client_hostname : "*",
                     row->client_id ? row->client_id : "*",
                     lease_duration);

        if (row->n_mac_addresses == 0) {
            vty_out(vty, "*                ");
        }
        for (i = 0; i < row->n_mac_addresses; i++) {
            vty_out(vty,"%s",row->mac_addresses[i]);
            if (i != (row->n_mac_addresses - 1)) {
                vty_out(vty, ",");
            }
        }
        vty_out(vty, "  ");

        if (row->n_set_tags == 0) {
            vty_out(vty, "*");
        }
        for (i = 0; i < row->n_set_tags; i++) {
            vty_out(vty,"%s",row->set_tags[i]);
            if (i != (row->n_set_tags - 1)) {
                vty_out(vty, ",");
            }
        }

        vty_out(vty,"%s",VTY_NEWLINE);

    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;

}


static int show_dhcp_options(void)
{
    const struct ovsrec_dhcpsrv_option *row = NULL;
    size_t i;
    char option_number[8];
    bool print_header = 1;

    row = ovsrec_dhcpsrv_option_first(idl);

    vty_out(vty,"%sDHCP options configuration %s",
                 VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "--------------------------%s",
                 VTY_NEWLINE);

    if (!row) {
        vty_out(vty, "DHCP options are not configured.%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    }

    OVSREC_DHCPSRV_OPTION_FOR_EACH(row, idl){

        if (row->option_number) {
            snprintf(option_number, 8, "%ld", row->option_number[0]);
        } else {
            snprintf(option_number, 8, "%s", "*");
        }

        if (print_header) {
            vty_out(vty, "Option Number  Option Name       Option Value"
                         "          ipv6   Match tags%s", VTY_NEWLINE);
            vty_out(vty, "---------------------------------------------"
                         "---------------------------%s", VTY_NEWLINE);

            print_header = 0;
        }

        vty_out(vty, "%-14s %-17s %-21s %-6s ",
                      option_number,
                      row->option_name ? row->option_name : "*",
                      row->option_value ? row->option_value : "*",
                      row->ipv6 && *row->ipv6 == 1 ? "True" : "False");

        if (row->n_match_tags == 0) {
            vty_out(vty, "*");
        }
        for (i = 0; i < row->n_match_tags; i++) {
            vty_out(vty,"%s",row->match_tags[i]);
            if (i != (row->n_match_tags - 1)) {
                vty_out(vty, ",");
            }
        }

        vty_out(vty,"%s",VTY_NEWLINE);

    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;

}


static int show_dhcp_match(void)
{
    const struct ovsrec_dhcpsrv_match *row = NULL;
    char option_number[8];
    bool print_header = 1;

    row = ovsrec_dhcpsrv_match_first(idl);

    vty_out(vty, "%sDHCP Match configuration%s",
                  VTY_NEWLINE,VTY_NEWLINE);
    vty_out(vty, "------------------------%s",
                  VTY_NEWLINE);

    if (!row) {
        vty_out(vty, "DHCP match is not configured.%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    }

    OVSREC_DHCPSRV_MATCH_FOR_EACH(row, idl){

        if (row->option_number) {
            snprintf(option_number, 8, "%ld", row->option_number[0]);
        } else {
            snprintf(option_number, 8, "%s", "*");
        }

        if (print_header) {
            vty_out(vty, "Option Number  Option Name       Option Value"
                         "          Set tag%s", VTY_NEWLINE);
            vty_out(vty, "---------------------------------------------"
                         "-----------------%s", VTY_NEWLINE);

            print_header = 0;
        }

        vty_out(vty, "%-14s %-17s %-21s %s",
                      option_number,
                      row->option_name ? row->option_name : "*",
                      row->option_value ? row->option_value : "*",
                      row->set_tag);

        vty_out(vty,"%s",VTY_NEWLINE);

    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;

}


static int show_dhcp_bootp(void)
{
    const struct ovsrec_dhcp_server *row = NULL;
    const struct smap_node *node;
    bool print_header = 1;

    row = ovsrec_dhcp_server_first(idl);

    vty_out(vty, "%sDHCP BOOTP configuration%s",
                  VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "------------------------%s",
                  VTY_NEWLINE);

    if (!row) {
        vty_out(vty, "DHCP BOOTP is not configured.%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    }

    SMAP_FOR_EACH(node,&row->bootp) {
        if (print_header) {
            vty_out(vty, "Tag               File%s", VTY_NEWLINE);
            vty_out(vty, "----------------------%s", VTY_NEWLINE);

            print_header = 0;
        }

        if (strcmp(node->key,"no_matching_tag") != 0){
            vty_out(vty, "%-17s %s%s",
                          node->key, node->value, VTY_NEWLINE);
        } else {
            vty_out(vty, "*                 %s%s",
                          node->value, VTY_NEWLINE);
        }
    }

    if (print_header) {
        vty_out(vty, "DHCP BOOTP is not configured.%s", VTY_NEWLINE);
    }

    vty_out(vty, "%s", VTY_NEWLINE);
    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}


static int show_dhcp_server(void)
{
    show_dhcp_range();
    show_dhcp_host();
    show_dhcp_options();
    show_dhcp_match();
    show_dhcp_bootp();
    return CMD_SUCCESS;
}

static int show_tftp_server(void)
{
    const struct ovsrec_system *row = NULL;
    char *buff = NULL;

    row = ovsrec_system_first(idl);
    if (!row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_OVSDB_FAILURE;
    }


    vty_out(vty, "%sTFTP server configuration %s",
                  VTY_NEWLINE,VTY_NEWLINE);
    vty_out(vty, "-------------------------%s",
                  VTY_NEWLINE);

    buff = (char *)smap_get(&row->other_config,"tftp_server_enable");
    if (buff == NULL) {
        vty_out(vty, "TFTP server : Disabled%s", VTY_NEWLINE);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    } else {
        if (strcmp(buff, "true") == 0) {
            vty_out(vty, "TFTP server : Enabled%s", VTY_NEWLINE);
        } else {
            vty_out(vty, "TFTP server : Disabled%s", VTY_NEWLINE);
        }
        buff = NULL;
        buff = (char *)smap_get(&row->other_config,"tftp_server_secure");
        if (buff != NULL && strcmp(buff, "true") == 0) {
            vty_out(vty, "TFTP server secure mode : Enabled%s",
                          VTY_NEWLINE);
        } else {
            vty_out(vty, "TFTP server secure mode : Disabled%s",
                          VTY_NEWLINE);
        }

        buff = (char *)smap_get(&row->other_config,"tftp_server_path");
        if (buff != NULL) {
            vty_out(vty, "TFTP server file path : %s%s",
                          buff, VTY_NEWLINE);
        } else {
            vty_out(vty, "TFTP server file path : Not configured%s",
                          VTY_NEWLINE);
        }

    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}


static const struct ovsrec_vrf* dhcp_server_vrf_lookup(const char *vrf_name)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, vrf_name) == 0) {
            return vrf_row;
        }
    }
    return NULL;
}

static int tftp_server_enable_disable(bool enable)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs_row = ovsrec_system_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }


    smap_clone(&smap, &ovs_row->other_config);
    if (enable) {
        smap_replace(&smap, "tftp_server_enable", "true");
    } else {
        smap_replace(&smap, "tftp_server_enable", "false");
    }

    ovsrec_system_set_other_config(ovs_row, &smap);

    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        if (enable) {
            vty_out(vty, "TFTP server is enabled successfully%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server is enabled successfully\n");
        } else {
            vty_out(vty, "TFTP server is disabled successfully%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server is disabled successfully\n");
        }
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        if (enable) {
            vty_out(vty, "TFTP server is already enabled%s", VTY_NEWLINE);
            VLOG_INFO("TFTP server is already enabled\n");
        } else {
            vty_out(vty, "TFTP server is already disabled%s", VTY_NEWLINE);
            VLOG_INFO("TFTP server is already disabled\n");
        }
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static int tftp_server_secure_enable_disable(bool enable)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs_row = ovsrec_system_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }


    smap_clone(&smap, &ovs_row->other_config);
    if (enable) {
        smap_replace(&smap, "tftp_server_secure", "true");
    } else {
        smap_replace(&smap, "tftp_server_secure", "false");
    }


    ovsrec_system_set_other_config(ovs_row, &smap);

    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        if (enable) {
            vty_out(vty, "TFTP server secure mode is enabled successfully%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server secure mode is enabled successfully\n");
        } else {
            vty_out(vty, "TFTP server secure mode is disabled successfully%s",
                         VTY_NEWLINE);
            VLOG_INFO("TFTP server secure mode is disabled successfully\n");
        }
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        if (enable) {
            vty_out(vty, "TFTP server secure mode is already enabled%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server secure mode is already enabled\n");
        } else {
            vty_out(vty, "TFTP server secure mode is already disabled%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server secure mode is already disabled\n");
        }
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static int tftp_server_path_config(const char *path, bool add)
{
    const struct ovsrec_system *ovs_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;
    char *curr_path;

    enum ovsdb_idl_txn_status status;

    /*
     * OPS_TODO:
     *   Move all cli_do_config_start and system row to a
     *   separate function or macro and call it from each function.
     */
    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    ovs_row = ovsrec_system_first(idl);
    if (!ovs_row) {
        vty_out(vty, "Error: Could not fetch default System data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }


    smap_clone(&smap, &ovs_row->other_config);
    if (!add) {
        curr_path = (char *)smap_get(&smap, "tftp_server_path");
        if (curr_path && strcmp(curr_path, path) != 0) {
            vty_out(vty, "The path %s is not configured for TFTP server%s",
                         path, VTY_NEWLINE);
            cli_do_config_abort(status_txn);
            return CMD_SUCCESS;
        }
        smap_remove(&smap, "tftp_server_path");
    } else {
        smap_replace(&smap, "tftp_server_path", path);
    }

    ovsrec_system_set_other_config(ovs_row, &smap);

    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        if (add) {
            vty_out(vty, "TFTP server path is added successfully%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server path is added successfully\n");
        } else {
            vty_out(vty, "TFTP server path is deleted successfully%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server path is deleted successfully\n");
        }
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        if (add) {
            vty_out(vty, "TFTP server path is already configured%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server path is already configured\n");
        } else {
            vty_out(vty, "TFTP server path is already deleted%s",
                          VTY_NEWLINE);
            VLOG_INFO("TFTP server path is already deleted\n");
        }
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static int dhcp_server_delete_bootp(dhcp_srv_bootp_params_t *bootp_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        vty_out(vty, "DHCP server configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    } else {
        dhcp_server_row = vrf_row->dhcp_server;
    }

    smap_clone(&smap, &dhcp_server_row->bootp);

    if (bootp_params->match_tag != NULL) {
        smap_remove(&smap, bootp_params->match_tag);
    } else {
        smap_remove(&smap, "no_matching_tag");
    }

    ovsrec_dhcp_server_set_bootp(dhcp_server_row, &smap);

    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp bootp \"%s\"=\"%s\" "
                  "was deleted successfully.\n",
                   __func__,
                   bootp_params->match_tag, bootp_params->file);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. Check if "
                 "dhcp bootp \"%s\"=\"%s\" "
                 "is already present", __func__,
                  bootp_params->match_tag, bootp_params->file);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static int dhcp_server_add_bootp(dhcp_srv_bootp_params_t *bootp_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    struct ovsdb_idl_txn *status_txn = NULL;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        dhcp_server_row = ovsrec_dhcp_server_insert(status_txn);
        ovsrec_vrf_set_dhcp_server(vrf_row, dhcp_server_row);
    } else {
        dhcp_server_row = vrf_row->dhcp_server;
    }

    smap_clone(&smap, &dhcp_server_row->bootp);

    if (bootp_params->match_tag != NULL) {
        smap_replace(&smap, bootp_params->match_tag, bootp_params->file);
    } else {
        smap_replace(&smap, "no_matching_tag", bootp_params->file);
    }

    ovsrec_dhcp_server_set_bootp(dhcp_server_row, &smap);

    smap_destroy(&smap);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp bootp "
                  "\"%s\"=\"%s\" was added "
                  "successfully.\n", __func__,
                   bootp_params->match_tag, bootp_params->file);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                 "Check if dhcp bootp \"%s\"=\"%s\" "
                 "is already present", __func__,
                  bootp_params->match_tag, bootp_params->file);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static bool dhcp_server_compare_match_params(dhcp_srv_match_params_t *m_params,
                                      struct ovsrec_dhcpsrv_match *m_row)
{

    /*
     * Compare each column considering some columns may not
     * be set to any value
     */
    if (strcmp(m_row->set_tag, m_params->set_tag) == 0 &&
        ((m_params->option_number == -1 && m_row->n_option_number == 0) ||
        (m_params->option_number != -1 && m_row->n_option_number == 1 &&
        m_params->option_number == *(m_row->option_number))) &&
        ((m_params->option_name == NULL && m_row->option_name == NULL) ||
        (m_params->option_name != NULL && m_row->option_name != NULL &&
        strcmp(m_params->option_name, m_row->option_name) == 0)) &&
        ((m_params->option_value == NULL && m_row->option_value == NULL) ||
        (m_params->option_value != NULL && m_row->option_value != NULL &&
        strcmp(m_params->option_value, m_row->option_value) == 0))) {
        return 1;
    } else {
        return 0;
    }
}

static int dhcp_server_delete_match(dhcp_srv_match_params_t *match_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_match *dhcpsrv_match_row = NULL;
    struct ovsrec_dhcpsrv_match *dhcpsrv_match_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_match **d_match = NULL;
    size_t n, i;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        vty_out(vty, "DHCP server configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    dhcp_server_row = vrf_row->dhcp_server;
    if (dhcp_server_row->n_matches <= 0) {
        vty_out(vty, "DHCP server match configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server match configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    for (i = 0; i < dhcp_server_row->n_matches; i++) {
        dhcpsrv_match_temp = dhcp_server_row->matches[i];
        if (dhcp_server_compare_match_params(match_params,
            dhcpsrv_match_temp) == 1) {
            dhcpsrv_match_row = dhcpsrv_match_temp;
            break;
        }
    }

    if (dhcpsrv_match_row == NULL) {
        vty_out(vty, "DHCP match config doesn't "
                     "exist.%s ",
                      VTY_NEWLINE);
        VLOG_ERR( "DHCP match config doesn't exist.");
        cli_do_config_abort(status_txn);
        return (CMD_SUCCESS);
    }

    d_match=xmalloc(sizeof *dhcp_server_row->matches *
                    (dhcp_server_row->n_matches - 1));
    for (i = n = 0; i < dhcp_server_row->n_matches; i++) {
        if (dhcp_server_row->matches[i] != dhcpsrv_match_row) {
            d_match[n++] = dhcp_server_row->matches[i];
        }
    }

    ovsrec_dhcp_server_set_matches(dhcp_server_row,
                                   d_match, dhcp_server_row->n_matches - 1);
    ovsrec_dhcpsrv_match_delete(dhcpsrv_match_row);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                 struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_match);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp match config was deleted "
                  "successfully.\n", __func__);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                 "Check if dhcp match config "
                 "is already deleted", __func__);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static int dhcp_server_add_match(dhcp_srv_match_params_t *match_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_match *dhcpsrv_match_row = NULL;
    struct ovsrec_dhcpsrv_match *dhcpsrv_match_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_match **d_match = NULL;
    size_t i;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        dhcp_server_row = ovsrec_dhcp_server_insert(status_txn);
        ovsrec_vrf_set_dhcp_server(vrf_row, dhcp_server_row);
    } else {
        dhcp_server_row = vrf_row->dhcp_server;
        for (i = 0; i < dhcp_server_row->n_matches; i++) {
            dhcpsrv_match_temp = dhcp_server_row->matches[i];
            if (dhcp_server_compare_match_params(match_params,
                dhcpsrv_match_temp) == 1) {
                vty_out(vty, "This DHCP match config is "
                             "already configured.%s",
                              VTY_NEWLINE);
                VLOG_ERR( "DHCP match config is already configured.");
                cli_do_config_abort(status_txn);
                return (CMD_SUCCESS);
            }
        }
    }

    dhcpsrv_match_row = ovsrec_dhcpsrv_match_insert(status_txn);
    if (match_params->option_name != NULL) {
        ovsrec_dhcpsrv_match_set_option_name(dhcpsrv_match_row,
                                             match_params->option_name);
    } else {
        ovsrec_dhcpsrv_match_set_option_number(dhcpsrv_match_row,
                                             &match_params->option_number, 1);
    }
    ovsrec_dhcpsrv_match_set_option_value(dhcpsrv_match_row,
                                        match_params->option_value);
    ovsrec_dhcpsrv_match_set_set_tag(dhcpsrv_match_row,
                                     match_params->set_tag);

    d_match = xmalloc(sizeof *dhcp_server_row->matches *
                      (dhcp_server_row->n_matches + 1));
    for (i = 0; i < dhcp_server_row->n_matches; i++) {
        d_match[i] = dhcp_server_row->matches[i];
    }

    struct ovsrec_dhcpsrv_match *temp_dhcp_row = CONST_CAST(
                      struct ovsrec_dhcpsrv_match*, dhcpsrv_match_row);
    d_match[dhcp_server_row->n_matches] = temp_dhcp_row;
    ovsrec_dhcp_server_set_matches(dhcp_server_row,
                                   d_match, dhcp_server_row->n_matches + 1);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                      struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_match);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp match was added "
                  "successfully.\n", __func__);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. Check if dhcp match "
                "is already present", __func__);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static bool dhcp_server_compare_option_params(
                               struct ovsrec_dhcpsrv_option *o_row_1,
                               struct ovsrec_dhcpsrv_option *o_row_2)
{

    /*
     * Compare each column except match tags considering some columns may not
     * be set to any value
     */
    if (o_row_1->n_match_tags == o_row_2->n_match_tags &&
        strcmp(o_row_1->option_value, o_row_2->option_value) == 0 &&
        o_row_1->n_option_number == o_row_2->n_option_number &&
        (o_row_2->n_option_number == 0 || (*(o_row_1->option_number) ==
        *(o_row_2->option_number))) &&
        ((o_row_1->option_name == NULL && o_row_2->option_name == NULL) ||
        (o_row_1->option_name != NULL && o_row_2->option_name != NULL &&
        strcmp(o_row_1->option_name, o_row_2->option_name) == 0))) {
        return 1;
    } else {
        return 0;
    }
}

static int dhcp_server_delete_option(dhcp_srv_option_params_t *option_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_option *dhcpsrv_option_row = NULL;
    struct ovsrec_dhcpsrv_option *dhcpsrv_option_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_option **d_option = NULL;
    size_t n, i, j;
    char *tags, *token;
    char **m_tags;
    int num_tags;
    bool entry_found = 0;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        vty_out(vty, "DHCP server configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    dhcp_server_row = vrf_row->dhcp_server;
    if (dhcp_server_row->n_dhcp_options <= 0) {
        vty_out(vty, "DHCP server option configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server option configurations are not present.",
                 __func__);
             cli_do_config_abort(status_txn);
            return CMD_SUCCESS;
    }

    dhcpsrv_option_temp = ovsrec_dhcpsrv_option_insert(status_txn);
    if (option_params->option_name != NULL) {
        ovsrec_dhcpsrv_option_set_option_name(dhcpsrv_option_temp,
                                              option_params->option_name);
    } else {
        ovsrec_dhcpsrv_option_set_option_number(dhcpsrv_option_temp,
                                  &option_params->option_number, 1);
    }
        ovsrec_dhcpsrv_option_set_option_value(dhcpsrv_option_temp,
                                  option_params->option_value);

    if (option_params->is_ipv6) {
        ovsrec_dhcpsrv_option_set_ipv6(dhcpsrv_option_temp,
                               &option_params->is_ipv6, 1);
    }

    if (option_params->match_tags != NULL) {
        tags=(char *)xmalloc(strlen(option_params->match_tags)+1);
        /*
         * OPS_TODO: TODO:
         * For all dynamic memory allocation in this file, check for
         * error cases (when it fails) and handle it accordingly.
         */
        strcpy(tags, option_params->match_tags);
        num_tags = 0;
        token=strtok(tags,",");

        /*
         * First parse the tags list to get the number of tags
         * to allocate dynamic memory and then re-parse the tags
         * list to assign tags in the allocated memory.
         *
         * OPS_TODO:
         *   Make this as a separate function as the same procedure is
         *   used in other functions too.
         */
        while(token!=NULL) {
            num_tags++;
            token=strtok(NULL,",");
        }

        strcpy(tags, option_params->match_tags);
        token=strtok(tags,",");
        m_tags = xmalloc(MAX_TAG_LENGTH * num_tags);
        num_tags = 0;

        while (token != NULL) {
            m_tags[num_tags] = token;
            num_tags++;
            token=strtok(NULL,",");
        }

        ovsrec_dhcpsrv_option_set_match_tags(dhcpsrv_option_temp,
                                                m_tags, num_tags);
        free(m_tags);
        free(tags);
    }

    for (i = 0; i < dhcp_server_row->n_dhcp_options; i++) {
        if (dhcp_server_compare_option_params(
            dhcp_server_row->dhcp_options[i],
            dhcpsrv_option_temp) == 1) {
            entry_found = 1;
            for (j = 0 ; j < dhcpsrv_option_temp->n_match_tags; j++) {
                if (strcmp(
                    dhcp_server_row->dhcp_options[i]->match_tags[j],
                    dhcpsrv_option_temp->match_tags[j]) != 0) {
                    entry_found = 0;
                    break;
                }
            }
            if (entry_found == 1) {
                dhcpsrv_option_row = dhcp_server_row->dhcp_options[i];
                break;
            }
        }
    }

    if (dhcpsrv_option_row == NULL) {
        ovsrec_dhcpsrv_option_delete(dhcpsrv_option_temp);
        vty_out(vty, "This DHCP option config doesn't exist.%s",
                      VTY_NEWLINE);
        VLOG_ERR("%s The DHCP option config doesn't exist. Check if dhcp option"
                " is configured.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    d_option = xmalloc(sizeof *dhcp_server_row->dhcp_options *
                        (dhcp_server_row->n_dhcp_options - 1));
    for (i = n = 0; i < dhcp_server_row->n_dhcp_options; i++) {
        if (dhcp_server_row->dhcp_options[i] != dhcpsrv_option_row) {
            d_option[n++] = dhcp_server_row->dhcp_options[i];
        }
    }

    ovsrec_dhcp_server_set_dhcp_options(dhcp_server_row,
                              d_option, dhcp_server_row->n_dhcp_options - 1);
    ovsrec_dhcpsrv_option_delete(dhcpsrv_option_temp);
    ovsrec_dhcpsrv_option_delete(dhcpsrv_option_row);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                                struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_option);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp option was deleted "
                  "successfully.\n", __func__);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. Check if dhcp option "
                "is configured.", __func__);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static int dhcp_server_add_option(dhcp_srv_option_params_t *option_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_option *dhcpsrv_option_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_option **d_option = NULL;
    size_t i, j;
    char *tags, *token;
    char **m_tags;
    int num_tags;
    bool duplicate_entry = 0;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        dhcp_server_row = ovsrec_dhcp_server_insert(status_txn);
        ovsrec_vrf_set_dhcp_server(vrf_row, dhcp_server_row);
    } else {
        dhcp_server_row = vrf_row->dhcp_server;

    }

    dhcpsrv_option_row = ovsrec_dhcpsrv_option_insert(status_txn);
    if (option_params->option_name != NULL) {
        ovsrec_dhcpsrv_option_set_option_name(dhcpsrv_option_row,
                                      option_params->option_name);
    } else {
        ovsrec_dhcpsrv_option_set_option_number(dhcpsrv_option_row,
                                  &option_params->option_number, 1);
    }
    ovsrec_dhcpsrv_option_set_option_value(dhcpsrv_option_row,
                                 option_params->option_value);

    if (option_params->is_ipv6) {
        ovsrec_dhcpsrv_option_set_ipv6(dhcpsrv_option_row,
                              &option_params->is_ipv6, 1);
    }

    if (option_params->match_tags != NULL) {
        tags=(char *)xmalloc(strlen(option_params->match_tags)+1);
        /*
         * OPS_TODO: TODO:
         * For all dynamic memory allocation in this file, check for
         * error cases (when it fails) and handle it accordingly.
         */
        strcpy(tags, option_params->match_tags);
        num_tags = 0;
        token=strtok(tags,",");

        /*
         * First parse the tags list to get the number of tags
         * to allocate dynamic memory and then re-parse the tags
         * list to assign tags in the allocated memory.
         *
         * OPS_TODO:
         *   Make this as a separate function as the same procedure is
         *   used in other functions too.
         */
        while(token!=NULL) {
            num_tags++;
            token=strtok(NULL,",");
        }

        strcpy(tags, option_params->match_tags);
        token=strtok(tags,",");
        m_tags = xmalloc(MAX_TAG_LENGTH * num_tags);
        num_tags = 0;

        while (token != NULL) {
            m_tags[num_tags] = token;
            num_tags++;
            token=strtok(NULL,",");
        }

        ovsrec_dhcpsrv_option_set_match_tags(dhcpsrv_option_row,
                                              m_tags, num_tags);
        free(m_tags);
        free(tags);
    }

    for (i = 0; i < dhcp_server_row->n_dhcp_options; i++) {
        if (dhcp_server_compare_option_params(
            dhcp_server_row->dhcp_options[i],
            dhcpsrv_option_row) == 1) {
            duplicate_entry = 1;
            for (j = 0 ; j < dhcpsrv_option_row->n_match_tags; j++) {
                if (strcmp(
                    dhcp_server_row->dhcp_options[i]->match_tags[j],
                    dhcpsrv_option_row->match_tags[j]) != 0) {
                    duplicate_entry = 0;
                    break;
                }
            }
            if (duplicate_entry) {
                break;
            }
        }
    }

    if (duplicate_entry) {
        ovsrec_dhcpsrv_option_delete(dhcpsrv_option_row);
        vty_out(vty, "This DHCP option config is already present.%s",
                      VTY_NEWLINE);
        VLOG_ERR("%s The command resulted in no change. Check if dhcp option "
                "is already present", __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    d_option = xmalloc(sizeof *dhcp_server_row->dhcp_options *
                       (dhcp_server_row->n_dhcp_options + 1));
    for (i = 0; i < dhcp_server_row->n_dhcp_options; i++) {
        d_option[i] = dhcp_server_row->dhcp_options[i];
    }

    struct ovsrec_dhcpsrv_option *temp_dhcp_row = CONST_CAST(
                         struct ovsrec_dhcpsrv_option*, dhcpsrv_option_row);
    d_option[dhcp_server_row->n_dhcp_options] = temp_dhcp_row;
    ovsrec_dhcp_server_set_dhcp_options(dhcp_server_row,
                        d_option, dhcp_server_row->n_dhcp_options + 1);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                        struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_option);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp option was added "
                  "successfully.\n", __func__);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. Check if dhcp option "
                "is already present", __func__);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                        ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static int dhcp_server_add_static_host(
                            dhcp_srv_static_host_params_t *static_host_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_static_host *dhcpsrv_static_host_row = NULL;
    struct ovsrec_dhcpsrv_static_host *dhcpsrv_static_host_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_static_host **d_static_host = NULL;
    size_t i;
    char *tags, *token, *macs;
    char **set_tags, **mac_list;
    int num_tags, num_macs;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        dhcp_server_row = ovsrec_dhcp_server_insert(status_txn);
        ovsrec_vrf_set_dhcp_server(vrf_row, dhcp_server_row);
    } else {
        dhcp_server_row = vrf_row->dhcp_server;
        for (i = 0; i < dhcp_server_row->n_static_hosts; i++) {
            dhcpsrv_static_host_temp = dhcp_server_row->static_hosts[i];
            if (strcmp(dhcpsrv_static_host_temp->ip_address,
                       static_host_params->ip_address) == 0) {
                vty_out(vty, "Static host with IP address \"%s\" is "
                             "already configured. "
                             "Please use different IP address or delete the "
                             "existing config and reconfigure.%s",
                              static_host_params->ip_address, VTY_NEWLINE);
                VLOG_ERR( "Static host with IP address \"%s\" is "
                          "already configured.",
                           static_host_params->ip_address);
                cli_do_config_abort(status_txn);
                return (CMD_SUCCESS);
            }
        }

    }

    dhcpsrv_static_host_row = ovsrec_dhcpsrv_static_host_insert(status_txn);
    ovsrec_dhcpsrv_static_host_set_ip_address(dhcpsrv_static_host_row,
                                      static_host_params->ip_address);
    ovsrec_dhcpsrv_static_host_set_client_hostname(dhcpsrv_static_host_row,
                                      static_host_params->client_hostname);
    ovsrec_dhcpsrv_static_host_set_client_id(dhcpsrv_static_host_row,
                                       static_host_params->client_id);
    ovsrec_dhcpsrv_static_host_set_lease_duration(dhcpsrv_static_host_row,
                                  &static_host_params->lease_duration, 1);

    if (static_host_params->mac_addresses != NULL) {
        macs=(char *)xmalloc(strlen(static_host_params->mac_addresses)+1);
        /*
         * OPS_TODO: TODO:
         * For all dynamic memory allocation in this file, check for
         * error cases (when it fails) and handle it accordingly.
         */
        strcpy(macs, static_host_params->mac_addresses);
        num_macs = 0;
        token=strtok(macs, ",");

        /*
         * First parse the mac addr list to get the number of macs
         * to allocate dynamic memory and then re-parse the macs
         * list to assign macs in the allocated memory.
         *
         * OPS_TODO:
         *   Make this as a separate function as the same procedure is
         *   used in other functions too.
         */
        while(token!=NULL) {
            num_macs++;
            token=strtok(NULL, ",");
        }

        strcpy(macs, static_host_params->mac_addresses);
        token=strtok(macs, ",");
        mac_list = xmalloc(MAX_MAC_LENGTH * num_macs);
        num_macs = 0;

        while (token != NULL) {
            mac_list[num_macs] = token;
            num_macs++;
            token=strtok(NULL, ",");
        }

        ovsrec_dhcpsrv_static_host_set_mac_addresses(dhcpsrv_static_host_row,
                                                       mac_list, num_macs);
        free(mac_list);
        free(macs);
    }

    if (static_host_params->set_tags != NULL) {
        tags=(char *)xmalloc(strlen(static_host_params->set_tags)+1);
        /*
         * OPS_TODO: TODO:
         * For all dynamic memory allocation in this file, check for
         * error cases (when it fails) and handle it accordingly.
         */
        strcpy(tags, static_host_params->set_tags);
        num_tags = 0;
        token=strtok(tags,",");

        /*
         * First parse the tags list to get the number of tags
         * to allocate dynamic memory and then re-parse the tags
         * list to assign tags in the allocated memory.
         *
         * OPS_TODO:
         *   Make this as a separate function as the same procedure is
         *   used in other functions too.
         */
        while(token!=NULL) {
            num_tags++;
            token=strtok(NULL,",");
        }

        strcpy(tags, static_host_params->set_tags);
        token=strtok(tags,",");
        set_tags = xmalloc(MAX_TAG_LENGTH * num_tags);
        num_tags = 0;

        while (token != NULL) {
            set_tags[num_tags] = token;
            num_tags++;
            token=strtok(NULL,",");
        }

        ovsrec_dhcpsrv_static_host_set_set_tags(dhcpsrv_static_host_row,
                                                    set_tags, num_tags);
        free(set_tags);
        free(tags);
    }

    d_static_host=xmalloc(sizeof *dhcp_server_row->static_hosts *
                          (dhcp_server_row->n_static_hosts + 1));
    for (i = 0; i < dhcp_server_row->n_static_hosts; i++) {
        d_static_host[i] = dhcp_server_row->static_hosts[i];
    }

    struct ovsrec_dhcpsrv_static_host *temp_dhcp_row = CONST_CAST(
                  struct ovsrec_dhcpsrv_static_host*, dhcpsrv_static_host_row);
    d_static_host[dhcp_server_row->n_static_hosts] = temp_dhcp_row;
    ovsrec_dhcp_server_set_static_hosts(dhcp_server_row,
                       d_static_host, dhcp_server_row->n_static_hosts + 1);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                                 struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_static_host);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and "
                  "dhcp-static-host \"%s\" was added "
                  "successfully.\n", __func__,
                   static_host_params->ip_address);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                 "Check if dhcp-static-host\"%s\" "
                 "is already present", __func__,
                  static_host_params->ip_address);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}

static int dhcp_server_delete_static_host(
                         dhcp_srv_static_host_params_t *static_host_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_static_host *dhcpsrv_static_host_row = NULL;
    struct ovsrec_dhcpsrv_static_host *dhcpsrv_static_host_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_static_host **d_static_host = NULL;
    size_t n,i;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        vty_out(vty, "DHCP server configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    dhcp_server_row = vrf_row->dhcp_server;
    if (dhcp_server_row->n_static_hosts <= 0) {
        vty_out(vty, "DHCP server static host configurations "
                     "are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server static host configurations "
                 "are not present.",
                 __func__);
             cli_do_config_abort(status_txn);
            return CMD_SUCCESS;
    }

    for (i = 0; i < dhcp_server_row->n_static_hosts; i++) {
        dhcpsrv_static_host_temp = dhcp_server_row->static_hosts[i];
        if (strcmp(dhcpsrv_static_host_temp->ip_address,
                   static_host_params->ip_address) == 0) {
            dhcpsrv_static_host_row = dhcpsrv_static_host_temp;
            break;
        }
    }

    if (dhcpsrv_static_host_row == NULL) {
        vty_out(vty, "Static host config with IP address \"%s\" doesn't "
                     "exist.%s ",
                      static_host_params->ip_address, VTY_NEWLINE);
        VLOG_ERR( "Static host config with IP address \"%s\" doesn't exist.",
                   static_host_params->ip_address);
        cli_do_config_abort(status_txn);
        return (CMD_SUCCESS);
    }

    d_static_host=xmalloc(sizeof *dhcp_server_row->static_hosts *
                                 (dhcp_server_row->n_static_hosts - 1));
    for (i = n = 0; i < dhcp_server_row->n_static_hosts; i++) {
        if (dhcp_server_row->static_hosts[i] != dhcpsrv_static_host_row) {
            d_static_host[n++] = dhcp_server_row->static_hosts[i];
        }
    }

    ovsrec_dhcp_server_set_static_hosts(dhcp_server_row, d_static_host,
                                  dhcp_server_row->n_static_hosts - 1);
    ovsrec_dhcpsrv_static_host_delete(dhcpsrv_static_host_row);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                                 struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_static_host);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and static host config with "
                  "IP address \"%s\" was deleted "
                  "successfully.\n", __func__,
                   static_host_params->ip_address);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                "Check if static host config with IP address \"%s\" "
                "is already deleted", __func__,
                 static_host_params->ip_address);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static int dhcp_server_delete_range(dhcp_srv_range_params_t *range_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_range *dhcpsrv_range_row = NULL;
    struct ovsrec_dhcpsrv_range *dhcpsrv_range_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_range **d_range = NULL;
    size_t n,i;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        vty_out(vty, "DHCP server configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server configurations are not present.",
                 __func__);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }

    dhcp_server_row = vrf_row->dhcp_server;
    if (dhcp_server_row->n_ranges <= 0) {
        vty_out(vty, "DHCP server range configurations are not present%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s DHCP server range configurations are not present.",
                 __func__);
             cli_do_config_abort(status_txn);
            return CMD_SUCCESS;
    }

    for (i = 0; i < dhcp_server_row->n_ranges; i++) {
        dhcpsrv_range_temp = dhcp_server_row->ranges[i];
        if (strcmp(dhcpsrv_range_temp->name, range_params->name) == 0) {
            dhcpsrv_range_row = dhcpsrv_range_temp;
            break;
        }
    }

    if (dhcpsrv_range_row == NULL) {
        vty_out(vty, "IP address range config with name \"%s\" doesn't "
                     "exist.%s ",
                      range_params->name, VTY_NEWLINE);
        VLOG_ERR( "IP address range with name \"%s\" doesn't exist.",
                   range_params->name);
        cli_do_config_abort(status_txn);
        return (CMD_SUCCESS);
    }

    d_range=xmalloc(sizeof *dhcp_server_row->ranges *
                          (dhcp_server_row->n_ranges - 1));
    for (i = n = 0; i < dhcp_server_row->n_ranges; i++) {
        if (dhcp_server_row->ranges[i] != dhcpsrv_range_row) {
            d_range[n++] = dhcp_server_row->ranges[i];
        }
    }

    ovsrec_dhcp_server_set_ranges(dhcp_server_row,
              d_range, dhcp_server_row->n_ranges - 1);
    ovsrec_dhcpsrv_range_delete(dhcpsrv_range_row);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                           struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_range);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp-range \"%s\" was deleted "
                  "successfully.\n", __func__, range_params->name);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                 "Check if dhcp-range\"%s\" "
                "is already deleted", __func__, range_params->name);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


static int dhcp_server_add_range(dhcp_srv_range_params_t *range_params)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsrec_dhcp_server *dhcp_server_row = NULL;
    struct ovsrec_dhcpsrv_range *dhcpsrv_range_row = NULL;
    struct ovsrec_dhcpsrv_range *dhcpsrv_range_temp = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcpsrv_range **d_range = NULL;
    size_t i;
    char *tags, *token;
    char **m_tags;
    int num_tags;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR("%s Got an error when trying to create a transaction using "
                  "cli_do_config_start()", __func__);
                   cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    vrf_row = dhcp_server_vrf_lookup(DEFAULT_VRF_NAME);
    if (!vrf_row) {
        vty_out(vty, "Error: Could not fetch default VRF data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s VRF table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
             cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }

    if (!vrf_row->dhcp_server) {
        dhcp_server_row = ovsrec_dhcp_server_insert(status_txn);
        ovsrec_vrf_set_dhcp_server(vrf_row, dhcp_server_row);
    } else {
        dhcp_server_row = vrf_row->dhcp_server;
        for (i = 0; i < dhcp_server_row->n_ranges; i++) {
            dhcpsrv_range_temp = dhcp_server_row->ranges[i];
            if (strcmp(dhcpsrv_range_temp->name, range_params->name) == 0) {
                vty_out(vty, "IP address range with name \"%s\" is "
                             "already configured. "
                             "Please use different name or delete the "
                             "existing config and reconfigure.%s",
                              range_params->name, VTY_NEWLINE);
                VLOG_ERR( "IP address range with name \"%s\" "
                          "is already configured.",
                           range_params->name);
                cli_do_config_abort(status_txn);
                return (CMD_SUCCESS);
            }
        }

    }

    dhcpsrv_range_row = ovsrec_dhcpsrv_range_insert(status_txn);
    ovsrec_dhcpsrv_range_set_name(dhcpsrv_range_row, range_params->name);
    ovsrec_dhcpsrv_range_set_start_ip_address(dhcpsrv_range_row,
                                 range_params->start_ip_address);
    ovsrec_dhcpsrv_range_set_lease_duration(dhcpsrv_range_row,
                             &range_params->lease_duration, 1);
    ovsrec_dhcpsrv_range_set_end_ip_address(dhcpsrv_range_row,
                                 range_params->end_ip_address);
    ovsrec_dhcpsrv_range_set_netmask(dhcpsrv_range_row,
                                range_params->netmask);
    if (range_params->prefix_len != 0) {
        ovsrec_dhcpsrv_range_set_prefix_len(dhcpsrv_range_row,
                                 &range_params->prefix_len, 1);
    }
    ovsrec_dhcpsrv_range_set_broadcast(dhcpsrv_range_row,
                                  range_params->broadcast);
    ovsrec_dhcpsrv_range_set_set_tag(dhcpsrv_range_row,
                                range_params->set_tag);

    if (range_params->is_static) {
        ovsrec_dhcpsrv_range_set_is_static(dhcpsrv_range_row,
                                 &range_params->is_static, 1);
    }

    if (range_params->match_tags != NULL) {
        tags=(char *)xmalloc(strlen(range_params->match_tags)+1);
        /*
         * OPS_TODO: TODO:
         * For all dynamic memory allocation in this file, check for
         * error cases (when it fails) and handle it accordingly.
         */
        strcpy(tags, range_params->match_tags);
        num_tags = 0;
        token=strtok(tags,",");

        /*
         * First parse the tags list to get the number of tags
         * to allocate dynamic memory and then re-parse the tags
         * list to assign tags in the allocated memory.
         *
         * OPS_TODO:
         *   Make this as a separate function as the same procedure is
         *   used in other functions too.
         */
        while(token!=NULL) {
            num_tags++;
            token=strtok(NULL,",");
        }

        strcpy(tags, range_params->match_tags);
        token=strtok(tags,",");
        m_tags = xmalloc(MAX_TAG_LENGTH * num_tags);
        num_tags = 0;

        while (token != NULL) {
            m_tags[num_tags] = token;
            num_tags++;
            token=strtok(NULL,",");
        }

        ovsrec_dhcpsrv_range_set_match_tags(dhcpsrv_range_row,
                                            m_tags, num_tags);
        free(m_tags);
        free(tags);
    }

    d_range=xmalloc(sizeof *dhcp_server_row->ranges *
                    (dhcp_server_row->n_ranges + 1));
    for (i = 0; i < dhcp_server_row->n_ranges; i++) {
        d_range[i] = dhcp_server_row->ranges[i];
    }

    struct ovsrec_dhcpsrv_range *temp_dhcp_row = CONST_CAST(
                             struct ovsrec_dhcpsrv_range*, dhcpsrv_range_row);
    d_range[dhcp_server_row->n_ranges] = temp_dhcp_row;
    ovsrec_dhcp_server_set_ranges(dhcp_server_row,
              d_range, dhcp_server_row->n_ranges + 1);
    struct ovsrec_dhcp_server *temp_dhcp_server = CONST_CAST(
                                 struct ovsrec_dhcp_server*, dhcp_server_row);
    ovsrec_vrf_set_dhcp_server(vrf_row, temp_dhcp_server);
    free(d_range);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {
        VLOG_INFO("%s The command succeeded and dhcp-range \"%s\" was added "
                  "successfully.\n", __func__, range_params->name);
        return CMD_SUCCESS;
    } else if (status == TXN_UNCHANGED) {
        VLOG_ERR("%s The command resulted in no change. "
                 "Check if dhcp-range\"%s\" "
                 "is already present", __func__, range_params->name);
        return CMD_SUCCESS;
    } else {
        VLOG_ERR("%s While trying to commit transaction to DB, got "
                 "status response : %s", __func__,
                  ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


DEFUN(cli_tftp_server_disable,
      cli_tftp_server_disable_cmd,
      "no enable",
      "Remove TFTP Server configuration\n"
      "Disable TFTP Server\n"
      )
{
    return tftp_server_enable_disable(0);
}

DEFUN(cli_tftp_server_enable,
      cli_tftp_server_enable_cmd,
      "enable",
      "Enable TFTP Server\n"
      )
{
    return tftp_server_enable_disable(1);
}

DEFUN(cli_tftp_server_secure_diable,
      cli_tftp_server_secure_disable_cmd,
      "no secure-mode",
      "Remove TFTP Server configuration\n"
      "Disable TFTP server secure mode\n"
      )
{
    return tftp_server_secure_enable_disable(0);
}

DEFUN(cli_tftp_server_secure_enable,
      cli_tftp_server_secure_enable_cmd,
      "secure-mode",
      "Enable TFTP server secure mode\n"
      )
{
    return tftp_server_secure_enable_disable(1);
}

DEFUN(cli_tftp_server_path_delete,
      cli_tftp_server_path_delete_cmd,
      "no path PATH",
      "Remove TFTP Server configuration\n"
      "File path configuration\n"
      "Enter file path for TFTP server\n"
      )
{
    return tftp_server_path_config(argv[0], 0);
}

DEFUN(cli_tftp_server_path_add,
      cli_tftp_server_path_add_cmd,
      "path PATH",
      "File path configuration\n"
      "Enter file path for TFTP server\n"
      )
{
    return tftp_server_path_config(argv[0], 1);
}

DEFUN(cli_dhcp_leases_show,
      cli_dhcp_leases_show_cmd,
      "show dhcp-server leases",
      SHOW_STR
      "Display DHCP Server Configuration\n"
      "Show DHCP leases maintained by DHCP server.\n"
      )
{
    return show_dhcp_leases();
}

DEFUN(cli_show_tftp_server,
      cli_show_tftp_server_cmd,
      "show tftp-server",
      SHOW_STR
      "Display TFTP Server Configuration\n"
      )
{
      return show_tftp_server();
}

DEFUN(cli_show_dhcp_server,
      cli_show_dhcp_server_cmd,
      "show dhcp-server",
      SHOW_STR
      "Display DHCP Server Configuration\n"
      )
{
    return show_dhcp_server();
}

DEFUN (cli_dhcp_server_bootp_delete,
       cli_dhcp_server_bootp_delete_cmd,
       "no boot set file FILE {match tag TAG}",
       "Remove DHCP server configuration\n"
       "BOOTP configuration\n"
       "Set file\n"
       "Set file configuration\n"
       "Enter file name\n"
       "Match tag\n"
       "Match tag\n"
       "Enter match tag\n")
{
    int ret_code;
    dhcp_srv_bootp_params_t bootp_params;

    bootp_params.file = (char *)argv[0];
    if( argv[1] == NULL) {
        bootp_params.match_tag = (char *)argv[1];
    } else {
        if (!is_tag_or_name_valid((char *)argv[1])) {
            vty_out(vty,"%s is invalid%s", argv[1], VTY_NEWLINE);
            return CMD_SUCCESS; /* check CMD_SUCCESS */
        } else {
            bootp_params.match_tag = (char *)argv[1];
        }
    }

    ret_code = dhcp_server_delete_bootp(&bootp_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_bootp_add,
       cli_dhcp_server_bootp_add_cmd,
       "boot set file FILE {match tag TAG}",
       "BOOTP configuration\n"
       "Set file\n"
       "Set file configuration\n"
       "Enter file name\n"
       "Match tag\n"
       "Match tag\n"
       "Enter match tag\n")
{
    int ret_code;
    dhcp_srv_bootp_params_t bootp_params;

    bootp_params.file = (char *)argv[0];
    if( argv[1] == NULL) {
        bootp_params.match_tag = (char *)argv[1];
    } else {
        if (!is_tag_or_name_valid((char *)argv[1])) {
            vty_out(vty,"%s is invalid%s", argv[1], VTY_NEWLINE);
            return CMD_SUCCESS; /* check CMD_SUCCESS */
        } else {
            bootp_params.match_tag = (char *)argv[1];
        }
    }

    ret_code = dhcp_server_add_bootp(&bootp_params);

    return (ret_code);

}

DEFUN (cli_dhcp_server_match_name_delete,
       cli_dhcp_server_match_name_delete_cmd,
       "no match set tag TAG match-option-name WORD {match-option-value WORD}",
       "Remove DHCP server configuration\n"
       "DHCP match configuration\n"
       "Set tag\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Match option name\n"
       "Enter option name\n"
       "Match option value\n"
       "Enter option value\n")
{
    int ret_code;
    dhcp_srv_match_params_t match_params;

    if (argv[0] == NULL) {
        match_params.set_tag = (char *)argv[0];

    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            return CMD_SUCCESS;
        } else {
            match_params.set_tag = (char *)argv[0];
        }
    }

    if (argv[1] == NULL) {
       match_params.option_name = (char *)argv[1];

    } else {
        if (!is_tag_or_name_valid((char *)argv[1])) {
            vty_out(vty,"%s is invalid%s", argv[1], VTY_NEWLINE);
            return CMD_SUCCESS;
        } else {
             match_params.option_name = (char *)argv[1];
        }
    }

    match_params.option_number = -1;
    match_params.option_value = (char *)argv[2];

    ret_code = dhcp_server_delete_match(&match_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_match_number_delete,
       cli_dhcp_server_match_number_delete_cmd,
       "no match set tag TAG match-option-number WORD "
       "{match-option-value WORD}",
       "Remove DHCP server configuration\n"
       "DHCP match configuration\n"
       "Set tag\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Match option number\n"
       "Enter option number\n"
       "Match option value\n"
       "Enter option value\n")
{
    int ret_code;
    dhcp_srv_match_params_t match_params;

    if (argv[0]==NULL) {
        match_params.set_tag = (char *)argv[0];
    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            /* check message */
            return CMD_SUCCESS;
        } else {
            match_params.set_tag = (char *)argv[0];
        }
    }

   if (atoi(argv[1]) < 255) {
        match_params.option_number = atoi(argv[1]);
    } else {
        vty_out(vty,"%s is invalid%s",argv[1],VTY_NEWLINE);
    }

    match_params.option_name = NULL;
    match_params.option_value = (char *)argv[2];

    ret_code = dhcp_server_delete_match(&match_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_match_name_add,
       cli_dhcp_server_match_name_add_cmd,
       "match set tag TAG match-option-name WORD {match-option-value WORD}",
       "DHCP match configuration\n"
       "Set tag\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Match option name\n"
       "Enter option name\n"
       "Match option value\n"
       "Enter option value\n")
{
    int ret_code;
    dhcp_srv_match_params_t match_params;

    if (argv[0] == NULL) {
        match_params.set_tag = (char *)argv[0];

    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            return CMD_SUCCESS;
        } else {
            match_params.set_tag = (char *)argv[0];
        }
    }

    if (argv[1] == NULL) {
       match_params.option_name = (char *)argv[1];

    } else {
        if (!is_tag_or_name_valid((char *)argv[1])) {
            vty_out(vty,"%s is invalid%s", argv[1], VTY_NEWLINE);
            return CMD_SUCCESS;
        } else {
             match_params.option_name = (char *)argv[1];
        }
    }

    match_params.option_number = -1;
    match_params.option_value = (char *)argv[2];

    ret_code = dhcp_server_add_match(&match_params);

    return (ret_code);

}

DEFUN (cli_dhcp_server_match_number_add,
       cli_dhcp_server_match_number_add_cmd,
       "match set tag TAG match-option-number <0-255> "
       "{match-option-value WORD}",
       "DHCP match configuration\n"
       "Set tag\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Match option number\n"
       "Enter option number\n"
       "Match option value\n"
       "Enter option value\n")
{
    int ret_code;
    dhcp_srv_match_params_t match_params;

    if (argv[0]==NULL) {
        match_params.set_tag = (char *)argv[0];
    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            /* check message */
            return CMD_SUCCESS;
        } else {
            match_params.set_tag = (char *)argv[0];
        }
    }

    if (atoi(argv[1]) < 255) {
        match_params.option_number = atoi(argv[1]);
    } else {
        vty_out(vty,"%s is invalid%s",argv[1],VTY_NEWLINE);
    }


    match_params.option_name = NULL;
    match_params.option_value = (char *)argv[2];

    ret_code = dhcp_server_add_match(&match_params);

    return (ret_code);

}

DEFUN (cli_dhcp_server_option_name_delete,
       cli_dhcp_server_option_name_delete_cmd,
       "no option set option-name WORD option-value WORD "
       "{match tags TAGS | ipv6}",
       "Remove DHCP server configuration\n"
       "DHCP Options configuration\n"
       "Set option-number/option-name\n"
       "Set option name\n"
       "Enter option name\n"
       "Set option value\n"
       "Enter option value\n"
       "Match tags\n"
       "Match tags configuration\n"
       "Enter match tags\n"
       "Specify if ipv6 option\n")
{
    int ret_code;
    dhcp_srv_option_params_t option_params;

    if (argv[0] == NULL) {
        option_params.option_name = (char *)argv[0];

    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            return CMD_SUCCESS; /* check */

        } else {
            option_params.option_name = (char *)argv[0];
        }
    }
    option_params.option_number = -1;
    option_params.option_value = (char *)argv[1];

    if (argv[2] == NULL) {
        option_params.match_tags = (char *)argv[2];
    } else {
        if(!is_tags_valid((char *)argv[2])) {
            return CMD_SUCCESS;  /* check */

        } else {
            option_params.match_tags = (char *)argv[2];
        }
    }
    if (argv[3] == NULL) {
        option_params.is_ipv6 = 0;
    } else {
        option_params.is_ipv6 = 1;
    }

    ret_code = dhcp_server_delete_option(&option_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_option_number_delete,
       cli_dhcp_server_option_number_delete_cmd,
       "no option set option-number <0-255> option-value WORD "
       "{match tags TAGS | ipv6}",
       "Remove DHCP server configuration\n"
       "DHCP Options configuration\n"
       "Set option-number/option-name\n"
       "Set option number\n"
       "Enter option number\n"
       "Set option value\n"
       "Enter option value\n"
       "Match tags\n"
       "Match tags configuration\n"
       "Enter match tags\n"
       "Specify if ipv6 option\n")
{
    int ret_code;
    dhcp_srv_option_params_t option_params;
    if (atoi(argv[0]) < 255) {
        option_params.option_number = atoi(argv[0]);
    } else {
        vty_out(vty,"%s is invalid%s",argv[0],VTY_NEWLINE);
    }

    option_params.option_name = NULL;
    option_params.option_value = (char *)argv[1];

    /* match tag validation */
    if(argv[2] == NULL) {
        option_params.match_tags = (char *)argv[2];
    } else {
        if (!is_tags_valid((char *)argv[2])) {
            return CMD_SUCCESS; /* check */
        } else {
            option_params.match_tags = (char *)argv[2];
        }
    }

    if (argv[3] == NULL) {
        option_params.is_ipv6 = 0;

    } else {
        option_params.is_ipv6 = 1;
    }

    ret_code = dhcp_server_delete_option(&option_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_option_name_add,
       cli_dhcp_server_option_name_add_cmd,
       "option set option-name WORD option-value WORD "
       "{match tags TAGS | ipv6}",
       "DHCP Options configuration\n"
       "Set option-number/option-name\n"
       "Set option name\n"
       "Enter option name\n"
       "Set option value\n"
       "Enter option value\n"
       "Match tags\n"
       "Match tags configuration\n"
       "Enter match tags\n"
       "Specify if ipv6 option\n")
{
    int ret_code;
    dhcp_srv_option_params_t option_params;

    if (argv[0] == NULL) {
        option_params.option_name = (char *)argv[0];

    } else {
        if (!is_tag_or_name_valid((char *)argv[0])) {
            vty_out(vty,"%s is invalid%s", argv[0], VTY_NEWLINE);
            return CMD_SUCCESS; /* check */

        } else {
            option_params.option_name = (char *)argv[0];
        }
    }

    option_params.option_number = -1;
    option_params.option_value = (char *)argv[1];

    if (argv[2] == NULL) {
        option_params.match_tags = (char *)argv[2];

    } else {
        if(!is_tags_valid((char *)argv[2])) {
            return CMD_SUCCESS;  /* check */
        } else {
            option_params.match_tags = (char *)argv[2];
        }
    }
    if (argv[3] == NULL) {
        option_params.is_ipv6 = 0;
    } else {
        option_params.is_ipv6 = 1;
    }

    ret_code = dhcp_server_add_option(&option_params);

    return (ret_code);

}

DEFUN (cli_dhcp_server_option_number_add,
       cli_dhcp_server_option_number_add_cmd,
       "option set option-number <0-255> option-value WORD "
       "{match tags TAGS | ipv6}",
       "DHCP Options configuration\n"
       "Set option-number/option-name\n"
       "Set option number\n"
       "Enter option number\n"
       "Set option value\n"
       "Enter option value\n"
       "Match tags\n"
       "Match tags configuration\n"
       "Enter match tags\n"
       "Specify if ipv6 option\n")
{
    int ret_code;
    dhcp_srv_option_params_t option_params;
    if (atoi(argv[0]) < 255) {
        option_params.option_number = atoi(argv[0]);
    } else {
        vty_out(vty,"%s is invalid%s",argv[0],VTY_NEWLINE);
    }

    option_params.option_name = NULL;
    option_params.option_value = (char *)argv[1];

    /* match tag validation */
    if(argv[2] == NULL) {
        option_params.match_tags = (char *)argv[2];

    } else {
        if (!is_tags_valid((char *)argv[2]))
        {
            return CMD_SUCCESS; /* check */

        } else {
            option_params.match_tags = (char *)argv[2];
        }
    }

    if (argv[3] == NULL) {
        option_params.is_ipv6 = 0;

    } else {
        option_params.is_ipv6 = 1;
    }

    ret_code = dhcp_server_add_option(&option_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_static_host_delete,
       cli_dhcp_server_static_host_delete_cmd,
       "no static (A.B.C.D|X:X::X:X) "
       "{match-mac-addresses MAC-ADDRs | set tags TAGs | "
       "match-client-hostname WORD | match-client-id WORD | "
       "lease-duration LEASE-TIME}",
       "Remove DHCP server configuration\n"
       "Static leases configuration\n"
       "Enter host static IPv4 address\n"
       "Enter host static IPv6 address\n"
       "Match DHCP host MAC addresses\n"
       "Enter DHCP host MAC address(es) separated by comma\n"
       "Set tags for the DHCP host\n"
       "Set tags configuration\n"
       "Enter tag names\n"
       "Match DHCP hostname\n"
       "Enter DHCP hostname\n"
       "Match DHCP client-id\n"
       "Enter DHCP client-id\n"
       "DHCP lease duration\n"
       "Enter DHCP lease duration time\n")
{
    int ret_code = CMD_SUCCESS;
    dhcp_srv_static_host_params_t static_host_params;

    /* validating ip address */
    if (!is_valid_ip_address((char *)argv[0])) {
        vty_out(vty, "%s is invalid%s",argv[0],VTY_NEWLINE);
        return CMD_SUCCESS;  /* check */

    } else {
        static_host_params.ip_address = (char *)argv[0];
    }

    /* validating mac addresses */
    if (argv[1] == NULL) {
        static_host_params.mac_addresses = (char *)argv[1];

    } else {
        if (!is_mac_addresses_valid((char *)argv[1])) {
            return CMD_SUCCESS; /* check */
        } else {
            static_host_params.mac_addresses = (char *)argv[1];
        }
    }

    /* validating set tags */
    if ( argv[2] == NULL) {
         static_host_params.set_tags = (char *)argv[2];
    } else {
        if (!is_tags_valid((char *)argv[2])) {
            return CMD_SUCCESS; /* check */
        } else {
            static_host_params.set_tags = (char *)argv[2];
        }
    }

    /* validating client hostname */
    if (argv[3] == NULL) {
        static_host_params.client_hostname = (char *)argv[3];
    } else {
        if (!is_tag_or_name_valid((char *)argv[3])) {
                vty_out(vty,"%s is invalid%s", argv[3], VTY_NEWLINE);
                return CMD_SUCCESS; /* check */
        } else {
                static_host_params.client_hostname = (char *)argv[3];

        }
    }

    /* validating client id */
    if (argv[4] == NULL) {
        static_host_params.client_id = (char *)argv[4];

    } else {
        if (!is_tag_or_name_valid((char *)argv[4])) {
                vty_out(vty,"%s is invalid%s", argv[4], VTY_NEWLINE);
                return CMD_SUCCESS;

        } else {
               static_host_params.client_id = (char *)argv[4];

        }
    }

    /* validating lease duration */
    if (argv[5] == NULL) {
        static_host_params.lease_duration = 60;
    } else {
        static_host_params.lease_duration = atoi(argv[5]);
    }

    ret_code = dhcp_server_delete_static_host(&static_host_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_static_host_add,
       cli_dhcp_server_static_host_add_cmd,
       "static (A.B.C.D|X:X::X:X) "
       "{match-mac-addresses MAC-ADDRs | set tags TAGs | "
       "match-client-hostname WORD | "
       "match-client-id WORD | lease-duration LEASE-TIME}",
       "Static leases configuration\n"
       "Enter host static IPv4 address\n"
       "Enter host static IPv6 address\n"
       "Match DHCP host MAC addresses\n"
       "Enter DHCP host MAC address(es) separated by comma\n"
       "Set tags for the DHCP host\n"
       "Set tags configuration\n"
       "Enter tag names\n"
       "Match DHCP hostname\n"
       "Enter DHCP hostname\n"
       "Match DHCP client-id\n"
       "Enter DHCP client-id\n"
       "DHCP lease duration\n"
       "Enter DHCP lease duration time\n")
{
    int ret_code = CMD_SUCCESS;
    dhcp_srv_static_host_params_t static_host_params;

    /* validating ip address */
    if (!is_valid_ip_address((char *)argv[0])) {
        vty_out(vty, "%s is invalid%s",argv[0],VTY_NEWLINE);
        return CMD_SUCCESS;  /* check */
    } else {
        static_host_params.ip_address = (char *)argv[0];
    }

    /* validating mac addresses */
    if (argv[1] == NULL) {
        static_host_params.mac_addresses = (char *)argv[1];
    } else {
        if (!is_mac_addresses_valid((char *)argv[1])) {
            return CMD_SUCCESS; /* check */
        } else {
            static_host_params.mac_addresses = (char *)argv[1];
        }
    }

    /* validating set tags */
    if ( argv[2] == NULL) {
         static_host_params.set_tags = (char *)argv[2];
    } else {
        if (!is_tags_valid((char *)argv[2])) {
            return CMD_SUCCESS; /* check */
        } else {
            static_host_params.set_tags = (char *)argv[2];
        }
    }

    /* validating client hostname */
    if (argv[3] == NULL) {
        static_host_params.client_hostname = (char *)argv[3];

    } else {
        if (!is_tag_or_name_valid((char *)argv[3])) {
            vty_out(vty,"%s is invalid%s", argv[3], VTY_NEWLINE);
            return CMD_SUCCESS; /* check */
        } else {
            static_host_params.client_hostname = (char *)argv[3];

        }
    }

    /* validating client id */
    if (argv[4] == NULL) {
        static_host_params.client_id = (char *)argv[4];
    } else {
        if (!is_tag_or_name_valid((char *)argv[4])) {
            vty_out(vty,"%s is invalid%s", argv[4], VTY_NEWLINE);
            return CMD_SUCCESS;
        } else {
            static_host_params.client_id = (char *)argv[4];
        }
    }

    /* validating lease duration */
    if (argv[5] == NULL) {
        static_host_params.lease_duration = 60;

    } else {
        static_host_params.lease_duration = atoi(argv[5]);
    }

    ret_code = dhcp_server_add_static_host(&static_host_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_range_delete,
       cli_dhcp_server_range_delete_cmd,
       "no range WORD start-ip-address (A.B.C.D|X:X::X:X)"
       " end-ip-address (A.B.C.D|X:X::X:X) {static "
       "| set tag TAG-NAME | match tags TAGS "
       "| netmask A.B.C.D | broadcast A.B.C.D | prefix-len <64-128> | "
       "lease-duration <0-65535>}",
       "Remove DHCP server configuration\n"
       "DHCP server IP address range configuration\n"
       "Enter DHCP server IP address range name\n"
       "Start IP address\n"
       "Enter start IPv4 address\n"
       "Enter start IPv6 address\n"
       "End IP address\n"
       "Enter end IPv4 address\n"
       "Enter end IPv6 address\n"
       "Static IP address configuration for the IP address range\n"
       "Set tag for the IP address range\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Matching tags for the IP address range\n"
       "Matching tags configuration\n"
       "Enter matching tags name separated by comma\n"
       "Netmask for the IP address range\n"
       "Enter netmask for the IP address range\n"
       "Broadcast IP address for the IP address range\n"
       "Enter broadcast IP address for the IP address range\n"
       "Prefix length for IPv6 address\n"
       "Enter prefix length for IPv6 address\n"
       "DHCP lease duration\n"
       "Enter DHCP lease duration in minutes, 0 for \"infinite\"\n")

{

    bool start_ip_ipv4 = false;
    bool start_ip_ipv6 = false;
    bool end_ip_ipv4 = false;
    bool end_ip_ipv6 = false;
    dhcp_srv_range_params_t range_params;
    int ret_code;
    if (argv[0] == NULL) {
        vty_out(vty, "Error: Name is not configured%s",VTY_NEWLINE);
        return CMD_ERR_INCOMPLETE;
    }

    /* validating range name */
    if (strlen(argv[0]) > MAX_DHCP_CONFIG_NAME_LENGTH) {
        vty_out(vty,"%s is invalid%s",argv[0],VTY_NEWLINE);
        return CMD_SUCCESS;

    } else {
        range_params.name = (char *) argv[0];
    }

    /* validating start ip address */
    if (!is_valid_ip_address((char *)argv[1])) {
        vty_out (vty, "%s is invalid%s",argv[1],VTY_NEWLINE);
        return CMD_SUCCESS; /* check this */

    } else {
        range_params.start_ip_address = (char *)argv[1];
        if (ip_type((char *)argv[1]) == 0) {
            start_ip_ipv4 = true;

        } else if (ip_type((char *)argv[1]) == 1) {
            start_ip_ipv6 = true;
        }
    }

    /* end ip address */
    if(argv[2] != NULL) {
        if (!is_valid_ip_address((char *)argv[2])) {
            vty_out (vty, "%s is invalid%s", argv[2],VTY_NEWLINE);
            return CMD_SUCCESS;

        } else {
            if (ip_type((char *)argv[2]) == 0) {
                end_ip_ipv4=true;

            } else if (ip_type((char *)argv[2]) == 1) {
                end_ip_ipv6=true;
            }

            if (start_ip_ipv4 && end_ip_ipv4) {
                range_params.end_ip_address = (char *)argv[2];
            } else if (start_ip_ipv6 && end_ip_ipv6) {
                range_params.end_ip_address = (char *)argv[2];
            }
            else {
                vty_out(vty, "Invalid IP address range%s",VTY_NEWLINE);
                return CMD_SUCCESS;
            }
       }
    }
    else {
        range_params.end_ip_address = (char *)argv[2];
    }

    /* validating static */
    if (argv[3] == NULL) {
        range_params.is_static = 0;
    } else {
        range_params.is_static = 1;
    }

    /* validating set tag */
    if (argv[4] == NULL) {
        range_params.set_tag = (char *)argv[4];

    } else {
        if (!is_tag_or_name_valid((char *)argv[4])) {
            vty_out(vty,"%s is invalid%s",argv[4],VTY_NEWLINE);
            return CMD_SUCCESS;       /* check */

        } else {
            range_params.set_tag = (char *)argv[4];
        }
    }

    /* validating match_tags */
    if (argv[5] == NULL)
    {
        range_params.match_tags = (char *)argv[5];

    } else {
        if (!is_tags_valid((char *)argv[5])) {
            return CMD_SUCCESS; /* check */

        } else {
            range_params.match_tags = (char *)argv[5];
        }
    }

    /* validating netmask */
    if (argv[6] == NULL) {
        range_params.netmask = (char *)argv[6];

    } else {
        if (!is_valid_netmask((char *)argv[6])) {
            vty_out(vty, "%s is invalid%s",argv[6],VTY_NEWLINE);
            return CMD_SUCCESS; /* check */

        } else {
            if (start_ip_ipv4) {
                range_params.netmask = (char *)argv[6];
                if (range_params.start_ip_address != NULL \
                     && range_params.end_ip_address != NULL ) {
                    if (!is_valid_net((char *)argv[1],(char *)argv[2],\
                       (char *)argv[6])) {
                        vty_out(vty,"Invalid IP address range%s",VTY_NEWLINE);
                        return CMD_SUCCESS;
                    }
                }

            } else if(start_ip_ipv6) {
                vty_out(vty, "Error : netmask configuration not allowed for IPv6\
                         %s", VTY_NEWLINE);
                return CMD_SUCCESS; /* check */
            }
        }
    }

    /* validating broadcast_address */
    if (argv[7] == NULL) {
        range_params.broadcast = (char *)argv[7];

    } else {
        if(start_ip_ipv4 && range_params.netmask != NULL) {
            if(is_valid_broadcast_addr(range_params.start_ip_address, \
               range_params.netmask,(char *)argv[7])) {
                range_params.broadcast = (char *)argv[7];
            } else {
                vty_out(vty, "%s is invalid%s",argv[7], \
                         VTY_NEWLINE);
                return CMD_SUCCESS; /* check */
            }

        } else if (range_params.netmask == NULL) {
            vty_out(vty,"Error : netmask must be specified before broadcast\
            address%s",VTY_NEWLINE);
            return CMD_SUCCESS; /* check */
        } else {
            vty_out(vty, "Error : broadcast address not allowed for IPv6%s", \
                              VTY_NEWLINE);
            return CMD_SUCCESS; /* check */
        }
    }

    /* validating prefix length */
    if (argv[8] == NULL) {
        range_params.prefix_len = 0;

    } else {
        if (start_ip_ipv6 && end_ip_ipv6) {
            range_params.prefix_len = atoi((char *)argv[8]);
        } else {
                vty_out(vty, "Error : prefix length configuration not allowed \
                for IPv4 %s",VTY_NEWLINE);
                return CMD_SUCCESS;
        }
    }

    /* lease duration */
    if (argv[9] == NULL) {
        range_params.lease_duration = 60;
    } else {
        range_params.lease_duration = atoi((char *)argv[9]);
    }


    ret_code = dhcp_server_delete_range(&range_params);

    return (ret_code);
}

DEFUN (cli_dhcp_server_range_add,
       cli_dhcp_server_range_add_cmd,
       "range WORD start-ip-address (A.B.C.D|X:X::X:X)"
       " end-ip-address (A.B.C.D|X:X::X:X) {static "
       "| set tag TAG-NAME | match tags TAGS "
       "| netmask A.B.C.D | broadcast A.B.C.D | prefix-len <64-128> | "
       "lease-duration <0-65535>}",
       "DHCP server IP address range configuration\n"
       "Enter DHCP server IP address range name\n"
       "Start IP address\n"
       "Enter start IPv4 address\n"
       "Enter start IPv6 address\n"
       "End IP address\n"
       "Enter end IPv4 address\n"
       "Enter end IPv6 address\n"
       "Static IP address configuration for the IP address range\n"
       "Set tag for the IP address range\n"
       "Set tag configuration\n"
       "Enter tag name\n"
       "Matching tags for the IP address range\n"
       "Matching tags configuration\n"
       "Enter matching tags name separated by comma\n"
       "Netmask for the IP address range\n"
       "Enter netmask for the IP address range\n"
       "Broadcast IP address for the IP address range\n"
       "Enter broadcast IP address for the IP address range\n"
       "Prefix length for IPv6 address\n"
       "Enter prefix length for IPv6 address\n"
       "DHCP lease duration\n"
       "Enter DHCP lease duration in minutes, 0 for \"infinite\"\n")
{
    bool start_ip_ipv4 = false;
    bool start_ip_ipv6 = false;
    bool end_ip_ipv4 = false;
    bool end_ip_ipv6 = false;
    dhcp_srv_range_params_t range_params;
    int ret_code;
    if (argv[0] == NULL) {
        vty_out(vty, "Error: Name is not configured%s ",
                      VTY_NEWLINE);
        return CMD_ERR_INCOMPLETE;
    }

    /* validating range name */
    if (strlen(argv[0]) > MAX_DHCP_CONFIG_NAME_LENGTH) {
        vty_out(vty,"%s is invalid%s ",argv[0],VTY_NEWLINE);
        return CMD_SUCCESS;
    } else {
        range_params.name = (char *) argv[0];
    }

    /* validating start ip address */
    if (!is_valid_ip_address((char *)argv[1])) {
        vty_out (vty, "%s is invalid%s",argv[1],VTY_NEWLINE);
        return CMD_SUCCESS; /* check this */

    } else {
        range_params.start_ip_address = (char *)argv[1];
        if (ip_type((char *)argv[1]) == 0) {
            start_ip_ipv4 = true;

        } else if (ip_type((char *)argv[1]) == 1) {
            start_ip_ipv6 = true;
        }
    }

    /* end ip address */
    if(argv[2] != NULL) {
        if (!is_valid_ip_address((char *)argv[2])) {
            vty_out (vty, "%s is invalid%s", argv[2],VTY_NEWLINE);
            return CMD_SUCCESS;

        } else {
            if (ip_type((char *)argv[2]) == 0) {
              end_ip_ipv4=true;

            } else if (ip_type((char *)argv[2]) == 1) {
              end_ip_ipv6=true;
            }
            if (start_ip_ipv4 && end_ip_ipv4) {
              range_params.end_ip_address = (char *)argv[2];

            } else if (start_ip_ipv6 && end_ip_ipv6) {
              range_params.end_ip_address = (char *)argv[2];
            }
            else {
              vty_out(vty, "Invalid IP address range%s", VTY_NEWLINE);
              return CMD_SUCCESS;
            }
       }
    }
    else {
        range_params.end_ip_address = (char *)argv[2];
    }

    /* validating static */
    if (argv[3] == NULL) {
        range_params.is_static = 0;
    } else {
        range_params.is_static = 1;
    }

    /* validating set tag */
    if (argv[4] == NULL) {
        range_params.set_tag = (char *)argv[4];

    } else {
        if (!is_tag_or_name_valid((char *)argv[4])) {
            vty_out(vty,"%s is invalid%s", argv[4], VTY_NEWLINE);
            return CMD_SUCCESS; /* check */

        } else {
            range_params.set_tag = (char *)argv[4];
        }
    }

    /* validating match_tags */
    if (argv[5] == NULL)
    {
        range_params.match_tags = (char *)argv[5];

    } else {
        if(!is_tags_valid((char *)argv[5])) {
            return CMD_SUCCESS; /* check */

        } else {
            range_params.match_tags = (char *)argv[5];
        }
    }

    /* validating netmask */
    if (argv[6] == NULL) {
        range_params.netmask = (char *)argv[6];

    } else {
        if (!is_valid_netmask((char *)argv[6])) {
            vty_out(vty, "%s is invalid%s", argv[6], VTY_NEWLINE);
            return CMD_SUCCESS; /* check */

        } else {
            if (start_ip_ipv4) {
                range_params.netmask = (char *)argv[6];
                if (range_params.start_ip_address != NULL \
                     && range_params.end_ip_address != NULL ) {
                    if (!is_valid_net((char *)argv[1],(char *)argv[2],\
                       (char *)argv[6])) {
                        vty_out(vty,"Invalid IP address range%s",\
                                     VTY_NEWLINE);
                        return CMD_SUCCESS;
                    }
                }

            } else if(start_ip_ipv6) {
                vty_out(vty, "Error : netmask configuration not allowed for IPv6\
                         %s", VTY_NEWLINE);
                return CMD_SUCCESS; /* check */
            }
        }
    }

    /* validating broadcast address */
    if (argv[7] == NULL) {
        range_params.broadcast = (char *)argv[7];

    } else {
        if(start_ip_ipv4 && range_params.netmask != NULL) {
            if(is_valid_broadcast_addr(range_params.start_ip_address, \
               range_params.netmask,(char *)argv[7])) {
                range_params.broadcast = (char *)argv[7];
            } else {
                vty_out(vty, "%s is invalid%s", argv[7], \
                         VTY_NEWLINE);
                return CMD_SUCCESS; /* check */
            }

        } else if (range_params.netmask == NULL) {
            vty_out(vty,"Error : netmask must be specified before broadcast\
address%s", VTY_NEWLINE);
            return CMD_SUCCESS; /* check */
        } else {
            vty_out(vty, "Error : broadcast address not allowed for IPv6%s", \
                              VTY_NEWLINE);
            return CMD_SUCCESS; /* check */
        }
    }
    /* validating prefix length */
    if (argv[8] == NULL) {
        range_params.prefix_len = 0;

    } else {
        if (start_ip_ipv6 && end_ip_ipv6) {
            range_params.prefix_len = atoi((char *)argv[8]);
        } else {
                vty_out(vty, "Error : prefix length configuration not allowed for \
IPv4 %s", VTY_NEWLINE);
                return CMD_SUCCESS;
        }
    }

    /* lease duration */
    if (argv[9] == NULL) {
        range_params.lease_duration = 60;
    } else {
        range_params.lease_duration = atoi((char *)argv[9]);
    }

    ret_code = dhcp_server_add_range(&range_params);

    return (ret_code);

}


/* Install  DHCP-TFTP related vty commands. */
void
dhcp_tftp_vty_init (void)
{
    install_element (ENABLE_NODE, &cli_dhcp_leases_show_cmd);

    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_range_add_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_range_delete_cmd);

    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_static_host_add_cmd);
    install_element(DHCP_SERVER_NODE,
                    &cli_dhcp_server_static_host_delete_cmd);

    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_option_name_add_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_option_name_delete_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_option_number_add_cmd);
    install_element(DHCP_SERVER_NODE,
                    &cli_dhcp_server_option_number_delete_cmd);

    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_match_number_add_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_match_name_add_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_match_name_delete_cmd);
    install_element(DHCP_SERVER_NODE,
                    &cli_dhcp_server_match_number_delete_cmd);

    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_bootp_add_cmd);
    install_element(DHCP_SERVER_NODE, &cli_dhcp_server_bootp_delete_cmd);

    install_element(TFTP_SERVER_NODE, &cli_tftp_server_enable_cmd);
    install_element(TFTP_SERVER_NODE, &cli_tftp_server_disable_cmd);

    install_element(TFTP_SERVER_NODE, &cli_tftp_server_secure_enable_cmd);
    install_element(TFTP_SERVER_NODE, &cli_tftp_server_secure_disable_cmd);

    install_element(TFTP_SERVER_NODE, &cli_tftp_server_path_add_cmd);
    install_element(TFTP_SERVER_NODE, &cli_tftp_server_path_delete_cmd);

    install_element(ENABLE_NODE, &cli_show_dhcp_server_cmd);
    install_element(ENABLE_NODE, &cli_show_tftp_server_cmd);

}
