/* Ntp CLI commands header file
 *
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
 * File: ntp_vty.h
 *
 * Purpose:  To add declarations required for ntpd_vty.c
 */

#ifndef _NTPD_VTY_H
#define _NTPD_VTY_H

/* Structure defiitions */
typedef struct ntp_cli_ntp_server_params_s {
    bool no_form;           /* TRUE/FALSE */

    char *vrf_name;         /* VRF */
    char *server_name;	    /* FQDN or IP Address */
    char *prefer;           /* true or false */
    char *version;          /* 3 or 4 */
    char *keyid;            /* 1-65534 */
    void *key_row;/* ptr to the key entry - (ovsrec_ntp_key *) */
} ntp_cli_ntp_server_params_t;

typedef struct ntp_cli_ntp_auth_key_params_s {
    bool no_form;           /* TRUE/FALSE */

    char *key;              /* 1-65534 */
    char *md5_pwd;	    /* 8-16 chars */
} ntp_cli_ntp_auth_key_params_t;

typedef struct ntp_cli_ntp_trusted_key_params_s {
    bool no_form;           /* TRUE/FALSE */

    char *key;              /* 1-65534 */
} ntp_cli_ntp_trusted_key_params_t;

typedef struct ntp_cli_ntp_auth_enable_params_s {
    bool no_form;           /* TRUE/FALSE */
} ntp_cli_ntp_auth_enable_params_t;

/*
 * depending on the outcome of the db transaction, return
 * the appropriate value for the cli command execution.
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

/* Commonly used declarations */
#define NTP_TRUE                                        1
#define NTP_FALSE                                       0

#define NTP_TRUE_STR                                    "true"
#define NTP_FALSE_STR                                   "false"

#define NTP_DEFAULT_STR                                 "-"
#define NTP_DEFAULT_INT                                 0
#define NTP_DEFAULT_ZERO_STR                            "0"

/* NTP Help strings */
#define NTP_STR                    "NTP Client configuration\n"
#define NTP_SERVER_STR             "NTP Association configuration\n"
#define NTP_SERVER_NAME_STR        "NTP Association name or IPv4 Address\n"
#define NTP_SERVER_PREFER_STR      "NTP Association preference configuration\n"
#define NTP_SERVER_VERSION_STR     "NTP Association version configuration\n"
#define NTP_SERVER_VERSION_NUM_STR "Version can be 3 or 4\n"
#define NTP_AUTH_STR               "NTP Authentication configuration\n"
#define NTP_AUTH_ENABLE_STR        "NTP Authentication Enable/Disable\n"
#define NTP_AUTH_KEY_STR           "NTP Authentication Key configuration\n"
#define NTP_TRUST_KEY_STR          "NTP Trusted Key configuration\n"
#define NTP_MD5_STR                "MD5 Password configuration\n"
#define NTP_KEY_ID_STR             "NTP Key ID\n"
#define NTP_KEY_NUM_STR            "NTP Key Number between 1-65534\n"
#define NTP_MD5_PASSWORD_STR       "NTP MD5 Password <8-16> chars\n"
#define NTP_SHOW_STR               "Show NTP information\n"
#define NTP_SHOW_ASSOC_STR         "Show NTP Association summary\n"
#define NTP_SHOW_STATUS_STR        "Show NTP Status information\n"
#define NTP_SHOW_STATISTICS_STR    "Show NTP Statistics information\n"
#define NTP_SHOW_AUTH_KEYS_STR     "Show NTP Authentication Keys information\n"
#define NTP_SHOW_TRUST_KEYS_STR    "Show NTP Trusted Keys information\n"

#endif // _NTPD_VTY_H
