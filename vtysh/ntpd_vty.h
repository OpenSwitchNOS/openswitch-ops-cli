/* Ntpd CLI commands header file
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: ntpd_vty.h
 *
 * Purpose:  To add declarations required for ntpd_vty.c
 */

#ifndef _NTPD_VTY_H
#define _NTPD_VTY_H

#ifndef SHOW_STR
#define SHOW_STR "Show information\n"
#endif

#define NTPD_STR "Ntpd information\n"

#ifndef STATUS_STR
#define STATUS_STR "Status information\n"
#endif

/* Function declarations */
void ntpd_vty_init (void);

/* Structure defiitions */
typedef struct ntp_cli_ntp_server_params_s
{
    bool no_form;           /* TRUE/FALSE */

    char *server_name;	    /* FQDN or IP Address */
    bool prefer;            /* TRUE/FALSE */
    unsigned char version;  /* 3 or 4 */
    unsigned int keyid;     /* 1-65535 */
} ntp_cli_ntp_server_params_t;

/*
** depending on the outcome of the db transaction, return
** the appropriate value for the cli command execution.
*/
inline static int
config_finish_result (enum ovsdb_idl_txn_status status)
{
    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED)) {
        return CMD_SUCCESS;
    }
    return CMD_WARNING;
}

/********************** standard database txn operations **********************/

#define START_DB_TXN(txn)                                       \
    do {                                                        \
        txn = cli_do_config_start();                            \
        if (txn == NULL) {                                      \
            vty_out(vty, "ovsdb_idl_txn_create failed: %s: %d\n",   \
                    __FILE__, __LINE__);                            \
            cli_do_config_abort(txn);                               \
            return CMD_OVSDB_FAILURE;                               \
        }                                                           \
    } while (0)

#define END_DB_TXN(txn)                                   \
    do {                                                  \
        enum ovsdb_idl_txn_status status;                 \
        status = cli_do_config_finish(txn);               \
        return config_finish_result(status);                \
    } while (0)

#define ERRONEOUS_DB_TXN(txn, error_message)                        \
    do {                                                            \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction failed: %s: %d -- %s\n", \
                __FILE__, __LINE__, error_message);                 \
        return CMD_WARNING;                                         \
    } while (0)

/* used when NO error is detected but still need to terminate */
#define ABORT_DB_TXN(txn, message)                             \
    do {                                                       \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction aborted: %s: %d, %s\n",  \
                __FILE__, __LINE__, message);                       \
        return CMD_SUCCESS;                                         \
    } while (0)

/********************** standard database txn operations **********************/

#endif // _NTPD_VTY_H
