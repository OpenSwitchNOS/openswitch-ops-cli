/* SFTP CLI commands header file
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
 * File: sftp_vty.h
 *
 * Purpose:  To add declarations required for sftp_vty.c
 */

#ifndef _SFTP_VTY_H
#define _SFTP_VTY_H

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include <unistd.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "sftp_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include <arpa/inet.h>
#include <string.h>

/* Max lengths. */
#define MAX_USERNAME_LEN        256
#define MAX_HOSTNAME_LEN        256

/* OVSDB key-value pair. */
#define SFTP_SERVER_CONFIG      "sftp_server_enable"

#define SFTP_STR \
"SSH File Transfer Protocol\n"
#define SFTP_SERVER \
"SFTP server configuration(Default: Disable)\n"
#define ENABLE_STR \
"Enable SFTP server\n"

#define SFTP_CLIENT_STR \
"Copy data from an SFTP server\n"
#define SRC_FILENAME \
"Specify source filename for the SFTP transfer\n"
#define USERNAME_STR \
"Specify the username string (Max Length 256) on the remote system information\n"
#define HOST_IPv4 \
"Specify the host IP of the remote system (IPv4)\n"
#define HOST_IPv6 \
"Specify the host IP of the remote system (IPv6)\n"
#define HOSTNAME_STR \
"Specify the hostname string (Max Length 256) of the remote system\n"
#define DST_FILENAME \
"Specify destination filename (Default path: '/var/local/')\n"
#define DEFAULT_DST \
"/var/local/"

/* Store the details for SFTP client action. */
typedef struct sftpClient_t {
    char *userName;     /* Stores the username of remote system. */
    char *hostName;     /* Stores the hostname of remote system. */
    char *srcFile;      /* Stores the source filename for SFTP transfer. */
    char *dstFile;      /* Stores the destination path. */
    bool isInteractive; /* Flag to indicate SFTP client is
                         * interactive or non-interactive. */
} sftpClient;

void
sftp_vty_init(void);

#endif /* SFTP_VTY_H */
