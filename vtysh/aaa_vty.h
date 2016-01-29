/* AAA CLI commands header file.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: aaa_vty.h
 *
 * Purpose:  To add declarations required for aaa_vty.c.
 */

#define SYSTEM_AAA_RADIUS               "radius"
#define SYSTEM_AAA_FALLBACK             "fallback"
#define SYSTEM_AAA_RADIUS_LOCAL         "local"
#define SYSTEM_AAA_RADIUS_AUTH          "radius_auth"
#define RADIUS_CHAP                     "chap"
#define RADIUS_PAP                      "pap"
#define OPS_TRUE_STR                        "true"
#define OPS_FALSE_STR                       "false"

#define MAX_RADIUS_SERVERS                    64
#define RADIUS_SERVER_DEFAULT_PASSKEY         "testing123-1"
#define RADIUS_SERVER_DEFAULT_PORT            1812
#define RADIUS_SERVER_DEFAULT_RETRIES         1
#define RADIUS_SERVER_DEFAULT_TIMEOUT         5

#define AUTO_PROVISIONING_ENABLE              "enable"
#define AUTO_PROVISIONING_DISABLE             "disable"

#define SSH_AUTH_ENABLE                       "true"
#define SSH_AUTH_DISABLE                      "false"

#define SSH_PUBLICKEY_AUTHENTICATION_ENABLE "ssh_publickeyauthentication_enable"
#define SSH_PASSWORD_AUTHENTICATION_ENABLE  "ssh_passkeyauthentication_enable"

void
aaa_vty_init (void);
