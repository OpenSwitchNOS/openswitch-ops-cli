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

/*
* This function is to displaty the contents of dhcp_leases database
*/
static int show_dhcp_leases()
{
    char cmd_buff[256];
    char time[256], mac_addr[256], ip_addr[256];
    char hostname[256], client_id[1024];
    FILE *leasestream;
    unsigned long lease_expiry;
    time_t lease_time;
    char *expiry_str;
    int length;

    strcpy(cmd_buff, DHCP_LEASE_SCRIPT);
    strcat(cmd_buff, " show");

    leasestream = popen(cmd_buff, "r");

    if (leasestream) {
        bool print_header = 1;
        while (fscanf(leasestream, "%s", time)!= EOF) {
            if (fscanf(leasestream, "%s %s %s %s",
                       mac_addr, ip_addr, hostname, client_id) != 4) {
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

    return CMD_SUCCESS;
}

/*
* This function is to add a dhcp-range for dynamic IP address allocation to the dhcp-range table
*/
static int add_range(const char *range)
{
    const struct ovsrec_open_vswitch *ovs_row = NULL;
    const struct ovsrec_dhcp_range *dhcp_range_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcp_range **d_range;
    size_t n,i;
    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR(
            "%s Got an error when trying to create a transaction using"
            " cli_do_config_start()", __func__);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }


    ovs_row=ovsrec_open_vswitch_first(idl);
    if (!ovs_row) {

        VLOG_ERR(
            "%s Open VSwitch table did not have any rows. Ideally it"
            " should have just one entry.", __func__);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }
        dhcp_range_row = ovsrec_dhcp_range_insert(status_txn);

        ovsrec_dhcp_range_set_dhcp_range(dhcp_range_row,range);

        d_range=xmalloc(sizeof *ovs_row->dhcp_range * (ovs_row->n_dhcp_range + 1));
        for (i = 0; i < ovs_row->n_dhcp_range; i++) {
            d_range[i] = ovs_row->dhcp_range[i];
        }
    struct ovsrec_dhcp_range *temp_dhcp_row = CONST_CAST(struct ovsrec_dhcp_range*, dhcp_range_row);
    d_range[ovs_row->n_dhcp_range] = temp_dhcp_row;
    ovsrec_open_vswitch_set_dhcp_range(ovs_row, d_range, ovs_row->n_dhcp_range + 1);
    free(d_range);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {

        VLOG_DBG(
            "%s The command succeeded and dhcp-range \"%s\" was added "
            "successfully", __func__, range);
            return CMD_SUCCESS;
    }
    else if (status == TXN_UNCHANGED) {

        VLOG_DBG(
            "%s The command resulted in no change. Check if dhcp-range\"%s\" "
            "is already present", __func__, range);
            return CMD_SUCCESS;
    }
    else{

        VLOG_ERR(
            "%s While trying to commit transaction to DB, got"
            " a status response : %s", __func__,
            ovsdb_idl_txn_status_to_string(status));
            return CMD_OVSDB_FAILURE;
    }

}

/*
* This function is to displaty the contents of dhcp_range table
*/
static int show_dhcp_range()
{
    const struct ovsrec_dhcp_range *row = NULL;
    size_t i;
    row=ovsrec_dhcp_range_first(idl);
    vty_out(vty,"DHCP Dynamic IP Allocation Configuration%s", VTY_NEWLINE);
    vty_out(vty, "---------------------------------------%s", VTY_NEWLINE);
    if (!row) {
        vty_out(vty, "No DHCP Range defined.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    OVSREC_DHCP_RANGE_FOR_EACH(row, idl){

        vty_out(vty,"%s%s",row->dhcp_range,VTY_NEWLINE);
    }return CMD_SUCCESS;

}

/*
* This function is to add a dhcp-host for static IP address allocation to the dhcp-host table
*/
static int add_host(const char *host)
{
    const struct ovsrec_open_vswitch *ovs_row = NULL;
    const struct ovsrec_dhcp_host *dhcp_host_row = NULL;
    struct ovsdb_idl_txn *status_txn = NULL;
    struct ovsrec_dhcp_host **d_host;
    size_t n,i;

    enum ovsdb_idl_txn_status status;

    status_txn = cli_do_config_start();

    if (status_txn == NULL) {
        VLOG_ERR(
            "%s Got an error when trying to create a transaction using"
            " cli_do_config_start()", __func__);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }


    ovs_row=ovsrec_open_vswitch_first(idl);
    if (!ovs_row) {
        VLOG_ERR(
            "%s Open VSwitch table did not have any rows. Ideally it"
            " should have just one entry.", __func__);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
    }



    dhcp_host_row = ovsrec_dhcp_host_insert(status_txn);

    ovsrec_dhcp_host_set_dhcp_host(dhcp_host_row,host);

    d_host=xmalloc(sizeof *ovs_row->dhcp_host * (ovs_row->n_dhcp_host + 1));
        for (i = 0; i < ovs_row->n_dhcp_host; i++) {
        d_host[i] = ovs_row->dhcp_host[i];
        }
    struct ovsrec_dhcp_host *temp_dhcp_row = CONST_CAST(struct ovsrec_dhcp_host*, dhcp_host_row);
    d_host[ovs_row->n_dhcp_host] = temp_dhcp_row;
    ovsrec_open_vswitch_set_dhcp_host(ovs_row, d_host, ovs_row->n_dhcp_host + 1);
    free(d_host);

    status = cli_do_config_finish(status_txn);

    if (status == TXN_SUCCESS) {

        VLOG_DBG(
            "%s The command succeeded and DHCP Host \"%s\" was added "
            "successfully", __func__, host);
            return CMD_SUCCESS;
    }
    else if (status == TXN_UNCHANGED) {

        VLOG_DBG(
            "%s The command resulted in no change. Check if DHCP-Host\"%s\""
            "is already present", __func__, host);
            return CMD_SUCCESS;
    }
    else {

        VLOG_ERR(
            "%s While trying to commit transaction to DB, got"
            " a status response : %s", __func__,
            ovsdb_idl_txn_status_to_string(status));
        return CMD_OVSDB_FAILURE;
    }

}


/*
* This function is to displaty the contents of dhcp_host table
*/
static int show_dhcp_host()
{
    const struct ovsrec_dhcp_host *row = NULL;
    size_t i;
    row=ovsrec_dhcp_host_first(idl);
    vty_out(vty,"DHCP Static IP Allocation Configuration%s", VTY_NEWLINE);
    vty_out(vty, "--------------------------------------%s", VTY_NEWLINE);
    if (!row) {
        vty_out(vty, "No DHCP Hosts defined.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    OVSREC_DHCP_HOST_FOR_EACH(row, idl){

    vty_out(vty,"%s%s",row->dhcp_host,VTY_NEWLINE);
}
    return CMD_SUCCESS;

}

DEFUN(cli_dhcp_leases_show,
    cli_dhcp_leases_show_cmd,
    "show dhcp leases",
    SHOW_STR
    "DHCP Leases\n"
    "\tShow DHCP leases maintained by DHCP server.\n"
    )
{
    return show_dhcp_leases();
}

DEFUN(cli_show_dhcp_host,
    cli_show_dhcp_host_cmd,
    "show dhcp host",
     SHOW_STR
    "DHCP Configuration\n"
    "\tShow static IP address allocation configuration.\n"
    )
{
    return show_dhcp_host() ;
}
DEFUN(cli_add_dhcp_host,
    cli_add_dhcp_host_cmd,
    "add dhcp-host DHCP_HOST",
    SHOW_STR
    "\tStatic IP address : < mac-address >,< ip-address >,< lease-duration >\n"
    )
{
    return add_host(argv[0]);
}


DEFUN(cli_dhcp_range_show,
    cli_dhcp_range_show_cmd,
    "show dhcp range",
     SHOW_STR
    "DHCP Configuration\n"
    "\tShow dynamic IP address allocation configuraion.\n"
    )
{
        return show_dhcp_range();
}

DEFUN(cli_dhcp_range_add,
    cli_dhcp_range_add_cmd,
    "add dhcp-range DHCP_RANGE",
    SHOW_STR
    "\tDynamic IP address range : < start_ip_address >,< end_ip_address >,< lease-duration >\n")
{
    return add_range(argv[0]);
}



/*INSTALL DHCP_TFTP related vty commands. */
void
dhcp_tftp_vty_init (void)
{
    install_element (ENABLE_NODE, &cli_dhcp_range_show_cmd);
    install_element(CONFIG_NODE, &cli_dhcp_range_add_cmd);
    install_element (ENABLE_NODE, &cli_show_dhcp_host_cmd);
    install_element(CONFIG_NODE, &cli_add_dhcp_host_cmd);
    install_element (ENABLE_NODE, &cli_dhcp_leases_show_cmd);

}
