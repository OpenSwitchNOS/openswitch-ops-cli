/* SFTP CLI commands
 *
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
 * File: sftp_vty.c
 *
 * Purpose:  To add SFTP CLI configuration and display commands.
 */
#include "sftp_vty.h"

static int show_sftp_server (void);
static int sftp_server_enable_disable (bool);

extern struct ovsdb_idl *idl;
VLOG_DEFINE_THIS_MODULE(vtysh_sftp_cli);

/*-----------------------------------------------------------------------------
| Function : show_sftp_server
| Responsibility : To show the current SFTP server status.
| Return : On success returns CMD_SUCCESS,
|          On failure returns CMD_OVSDB_FAILURE
-----------------------------------------------------------------------------*/
static int
show_sftp_server (void)
{
    const struct ovsrec_system *row = NULL;
    char *status = NULL;

    row = ovsrec_system_first(idl);
    if (!row) {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    vty_out(vty, "%sSFTP server configuration %s",
                  VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "-------------------------%s",
                  VTY_NEWLINE);

    status = (char *)smap_get(&row->other_config, SFTP_SERVER_CONFIG);
    if (status == NULL) {
        vty_out(vty, "SFTP server : Disabled %s", VTY_NEWLINE);

        return CMD_SUCCESS;
    } else {
        if (!strcmp(status, "true")) {
            vty_out(vty, "SFTP server : Enabled %s", VTY_NEWLINE);
        } else if (!strcmp(status, "false")) {
            vty_out(vty, "SFTP server : Disabled %s", VTY_NEWLINE);
        }
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Function : sftp_server_enable_disable
| Responsibility : To enable/disable SFTP server.
| Parameters :
|     bool enable: If true, enable SFTP server and if false
|                  disable the SFTP server.
| Return : On success returns CMD_SUCCESS,
|          On failure returns CMD_OVSDB_FAILURE
-----------------------------------------------------------------------------*/
static int
sftp_server_enable_disable (bool enable)
{
    const struct ovsrec_system *row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    struct smap smap;

    if (status_txn == NULL)
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    row = ovsrec_system_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort(status_txn);
        return CMD_OVSDB_FAILURE;
    }

    smap_clone(&smap, &row->other_config);

    if (enable) {
        smap_replace(&smap, SFTP_SERVER_CONFIG, "true");
    } else {
        smap_replace(&smap, SFTP_SERVER_CONFIG, "false");
    }

    ovsrec_system_set_other_config(row, &smap);

    txn_status = cli_do_config_finish(status_txn);
    smap_destroy(&smap);

    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/* SFTP server enable config. */
DEFUN ( cli_sftp_server_enable,
        cli_sftp_server_enable_cmd,
        "sftp server enable",
        SFTP_STR
        SFTP_SERVER
        ENABLE_STR )
{
      return sftp_server_enable_disable(1);
}

/* SFTP server enable config disable. */
DEFUN ( cli_sftp_server_disable,
        cli_sftp_server_disable_cmd,
        "no sftp server enable",
        NO_STR
        SFTP_STR
        SFTP_SERVER
        ENABLE_STR )
{
      return sftp_server_enable_disable(0);
}

/* Show SFTP server config. */
DEFUN ( cli_show_sftp_server,
        cli_show_sftp_server_cmd,
        "show sftp server",
        SHOW_STR
        SFTP_STR
        SFTP_SERVER )
{
      return show_sftp_server();
}

/* Install SFTP VTY commands. */
void
sftp_vty_init (void)
{
    install_element(ENABLE_NODE, &cli_show_sftp_server_cmd);
    install_element(CONFIG_NODE, &cli_sftp_server_enable_cmd);
    install_element(CONFIG_NODE, &cli_sftp_server_disable_cmd);
}
