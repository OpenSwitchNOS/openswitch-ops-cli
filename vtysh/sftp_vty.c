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
 * Purpose:  To add sftp CLI configuration and display commands.
 */
#include "sftp_vty.h"

static int show_sftp_server (void);
static int sftp_server_enable_disable (const char *status);

extern struct ovsdb_idl *idl;
VLOG_DEFINE_THIS_MODULE(vtysh_sftp_cli);

/* handler to show the current sftp server status */
static int
show_sftp_server (void)
{
    const struct ovsrec_system *row = NULL;
    char *status = NULL;

    row = ovsrec_system_first(idl);
    if (!row) {
        vty_out(vty, "Error: Could not fetch default system data.%s",
                     VTY_NEWLINE);
        VLOG_ERR("%s System table did not have any rows. Ideally it "
                 "should have just one entry.", __func__);
        vty_out(vty, "%s", VTY_NEWLINE);

        return CMD_OVSDB_FAILURE;
    }

    vty_out(vty, "%sSFTP server configuration %s",
                  VTY_NEWLINE, VTY_NEWLINE);
    vty_out(vty, "-------------------------%s",
                  VTY_NEWLINE);

    status = (char *)smap_get(&row->aaa, SFTP_SERVER_CONFIG);
    if (status == NULL) {
        vty_out(vty, "SFTP server : Disabled%s", VTY_NEWLINE);

        return CMD_SUCCESS;
    } else {
        if (!strcmp(status, SFTP_SERVER_ENABLE)) {
            vty_out(vty, "SFTP server : Enabled%s", VTY_NEWLINE);
        } else if (!strcmp(status, SFTP_SERVER_DISABLE)) {
            vty_out(vty, "SFTP server : Disabled%s", VTY_NEWLINE);
        }
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}

/* handler to enable/disable SFTP server configurations */
static int
sftp_server_enable_disable (const char *status)
{
    const struct ovsrec_system *row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    struct smap smap_aaa;

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

    smap_clone(&smap_aaa, &row->aaa);

    if (strcmp(SFTP_SERVER_ENABLE, status) == 0)
    {
        smap_replace(&smap_aaa, SFTP_SERVER_CONFIG, status);
    }
    else if (strcmp(SFTP_SERVER_DISABLE, status) == 0)
    {
        smap_replace(&smap_aaa, SFTP_SERVER_CONFIG, status);
    }

    ovsrec_system_set_aaa(row, &smap_aaa);

    txn_status = cli_do_config_finish(status_txn);
    smap_destroy(&smap_aaa);

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

/* SFTP server enable config */
DEFUN ( cli_sftp_server_enable,
        cli_sftp_server_enable_cmd,
        "sftp server enable",
        SFTP_STR
        SFTP_SERVER
        ENABLE_STR )
{
      return sftp_server_enable_disable(SFTP_SERVER_ENABLE);
}

/* SFTP server enable config disable */
DEFUN ( cli_sftp_server_disable,
        cli_sftp_server_disable_cmd,
        "no sftp server enable",
        NO_STR
        SFTP_STR
        SFTP_SERVER
        ENABLE_STR )
{
      return sftp_server_enable_disable(SFTP_SERVER_DISABLE);
}

DEFUN ( cli_show_sftp_server,
        cli_show_sftp_server_cmd,
        "show sftp server",
        SHOW_STR
        SFTP_STR
        SFTP_SERVER )
{
      return show_sftp_server();
}

/* install SFTP VTY commands */
void
sftp_vty_init (void)
{
        install_element(ENABLE_NODE, &cli_show_sftp_server_cmd);
        install_element(CONFIG_NODE, &cli_sftp_server_enable_cmd);
        install_element(CONFIG_NODE, &cli_sftp_server_disable_cmd);
}
