/* Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: logrotate_vty.c
 *
 * File to implement logrotate cli.
 */

#include "logrotate_vty.h"
#include "command.h"
#include "openvswitch/vlog.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "openhalon-idl.h"
#include "vtysh.h"

#include <arpa/inet.h>
#include <string.h>

VLOG_DEFINE_THIS_MODULE(vtysh_logrotate);
extern struct ovsdb_idl *idl;

#define VLOG_ERR_LOGROTATE_TRANSACTION_COMMIT_FAILED VLOG_ERR("Logrotate DB : transaction commit failed \n")
#define VLOG_ERR_LOGROTATE_OPENVSWITCH_READ_FAILED VLOG_ERR("Logrotate DB: Openvswitch read failed \n")
#define VLOG_ERR_LOGROTATE_TRANSACTION_CREATE_FAILED  VLOG_ERR(OVSDB_TXN_CREATE_ERROR)

/* Sets logrotation period value in DB*/
static int set_logrotate_period(const char *period_value)
{
    const struct ovsrec_open_vswitch *ovs = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *txn = cli_do_config_start();


    if (NULL == txn)
        {
        VLOG_ERR_LOGROTATE_TRANSACTION_CREATE_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    ovs = ovsrec_open_vswitch_first(idl);
    if (NULL == ovs)
        {
        VLOG_ERR_LOGROTATE_OPENVSWITCH_READ_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    if(NULL != period_value)
        {
        smap_replace((struct smap *)&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD, period_value);
        }

    ovsrec_open_vswitch_set_logrotate_config(ovs, &ovs->logrotate_config);

    txn_status = cli_do_config_finish(txn);

    if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
        {
        return CMD_SUCCESS;
        }
    else
        {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
        }
}

/* Sets logrotation maxsize value in DB*/
static int set_logrotate_maxsize(const char *size_value)
{
    const struct ovsrec_open_vswitch *ovs = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *txn = cli_do_config_start();

    if (NULL == txn)
        {
        VLOG_ERR_LOGROTATE_TRANSACTION_CREATE_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    ovs = ovsrec_open_vswitch_first(idl);
    if (NULL == ovs)
        {
        VLOG_ERR_LOGROTATE_OPENVSWITCH_READ_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    if(NULL != size_value)
        {
        smap_replace((struct smap *)&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE, size_value);
        }

    ovsrec_open_vswitch_set_logrotate_config(ovs, &ovs->logrotate_config);

    txn_status = cli_do_config_finish(txn);

    if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
        {
        return CMD_SUCCESS;
        }
    else
        {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
        }
}

/* Sets logrotation target uri in DB*/
static int set_logrotate_target(const char *uri)
{
    const struct ovsrec_open_vswitch *ovs = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *txn;
    const char *ip_value;
    struct in_addr addr;

    if(strncmp(uri,OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT,5))
        {
        if(strlen(uri) <= 7)
            {
            VLOG_ERR("URI not valid\n");
            return CMD_ERR_NOTHING_TODO;
            }
        if(strncmp(uri,"tftp://",7))
            {
            VLOG_ERR("Only tftp protocol is supported\n");
            return CMD_ERR_NOTHING_TODO;
            }

        ip_value = uri+7;

        if(inet_pton(AF_INET, ip_value,&addr) <= 0)
            {
            VLOG_ERR("Invalid IPv4 address\n");
            return CMD_ERR_NOTHING_TODO;
            }

        if(!IS_VALID_IPV4(htonl(addr.s_addr)))
            {
            VLOG_ERR("Broadcast, multicast and loopback addresses are not allowed\n");
            return CMD_ERR_NOTHING_TODO;
            }
        }

    txn = cli_do_config_start();
    if (NULL == txn)
        {
        VLOG_ERR_LOGROTATE_TRANSACTION_CREATE_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    ovs = ovsrec_open_vswitch_first(idl);
    if (NULL == ovs)
        {
        VLOG_ERR_LOGROTATE_OPENVSWITCH_READ_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
        }

    if((NULL != uri) && (strncmp(uri,OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT,5)))
        {
        smap_replace((struct smap *)&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET, uri);
        }
    else
        {
        smap_remove(&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET);
        }

    ovsrec_open_vswitch_set_logrotate_config(ovs, &ovs->logrotate_config);

    txn_status = cli_do_config_finish(txn);

    if(txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
        {
        return CMD_SUCCESS;
        }
    else
        {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
        }
}

DEFUN (configure_logrotate_period,
       configure_logrotate_period_cmd,
       LOGROTATE_CMD_STR_PERIOD,
       LOGROTATE_HELP_STR_PERIOD)
{
    set_logrotate_period(argv[0]);
}

DEFUN (configure_no_logrotate_period,
       configure_no_logrotate_period_cmd,
       LOGROTATE_NO_CMD_STR_PERIOD,
       LOGROTATE_NO_HELP_STR_PERIOD)
{
    set_logrotate_period(OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT);
}

DEFUN (configure_logrotate_maxsize,
       configure_logrotate_maxsize_cmd,
       LOGROTATE_CMD_STR_MAXSIZE,
       LOGROTATE_HELP_STR_MAXSIZE)
{
    set_logrotate_maxsize(argv[0]);
}

DEFUN (configure_no_logrotate_maxsize,
       configure_no_logrotate_maxsize_cmd,
       LOGROTATE_NO_CMD_STR_MAXSIZE,
       LOGROTATE_NO_HELP_STR_MAXSIZE)
{
    set_logrotate_maxsize(OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT);
}


DEFUN (configure_logrotate_targetRemote,
       configure_logrotate_targetRemote_cmd,
       LOGROTATE_CMD_STR_TARGET,
       LOGROTATE_HELP_STR_TARGET)
{
    set_logrotate_target(argv[0]);
}

DEFUN (configure_no_logrotate_targetRemote,
       configure_no_logrotate_targetRemote_cmd,
       LOGROTATE_NO_CMD_STR_TARGET,
       LOGROTATE_NO_HELP_STR_TARGET)
{
    set_logrotate_target(OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT);
}

DEFUN (show_logrotate_config,
       show_logrotate_cmd,
       "show logrotate",
       SHOW_STR
       "Show logrotate config parameters\n"
       )
{
    const struct ovsrec_open_vswitch *ovs = NULL;
    const char *data = NULL;

    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);

    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs) {
        vty_out (vty, "Logrotate configurations : %s", VTY_NEWLINE);
        data = smap_get(&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD);
        vty_out (vty, "Period            : %s%s", (NULL == data) ? OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT : data, VTY_NEWLINE);
        data = smap_get(&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE);
        vty_out (vty, "Maxsize           : %sMB%s", (NULL == data) ? OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT : data, VTY_NEWLINE);
        data = smap_get(&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET);
        if (data != NULL)
            {
            vty_out (vty, "Target            : %s%s", smap_get(&ovs->logrotate_config, OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET), VTY_NEWLINE);
            }
    }
    else {
        VLOG_ERR("Couldn't retrieve any logrotate columns");
        return CMD_OVSDB_FAILURE;
    }

    return CMD_SUCCESS;
}

void logrotate_vty_init()
{
    install_element(CONFIG_NODE, &configure_logrotate_period_cmd);
    install_element(CONFIG_NODE, &configure_logrotate_maxsize_cmd);
    install_element(CONFIG_NODE, &configure_logrotate_targetRemote_cmd);
    install_element(CONFIG_NODE, &configure_no_logrotate_period_cmd);
    install_element(CONFIG_NODE, &configure_no_logrotate_maxsize_cmd);
    install_element(CONFIG_NODE, &configure_no_logrotate_targetRemote_cmd);
    install_element(ENABLE_NODE, &show_logrotate_cmd);
}
