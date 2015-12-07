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

static int sftp_client_copy (sftpClient *sc);

extern struct ovsdb_idl *idl;
VLOG_DEFINE_THIS_MODULE(vtysh_sftp_cli);

/*-----------------------------------------------------------------------------
| Function : sftp_client_copy
| Responsibility : To handle SFTP client copy operation.
| Parameters :
|     sftpClient *sc: Pointer to hold all the information needed to perform
|                     SFTP copy.
| Return : On success returns CMD_SUCCESS,
|          On failure returns CMD_WARNING
-----------------------------------------------------------------------------*/
static int
sftp_client_copy (sftpClient *sc)
{
    int ret = CMD_SUCCESS;
    int len = 0, argc = 2;
    int cmd_len = 0;
    char *dest = NULL;
    char *command = NULL;
    char *arguments[argc];

    if (!sc)
    {
        VLOG_ERR("Structure pointer passed "
                 "is null %s %d\n",__FILE__,__LINE__);
        return CMD_WARNING;
    }

    cmd_len = strlen(sc->userName) + strlen(sc->hostName)
              + strlen(sc->srcFile);
    command = (char*)malloc ((cmd_len+3)*sizeof(char));

    if (!command)
    {
        VLOG_ERR("Malloc returned null %s %d",__FILE__,__LINE__);
        return CMD_WARNING;
    }

    len += sprintf(command+len, "%s@", sc->userName);
    if (sc->isInteractive)
    {
        /* SFTP client for interative mode. */
        len += sprintf(command+len, "%s", sc->hostName);
        argc = 1;
    }
    else
    {
        /* SFTP client for non-interactive mode. */
        len += sprintf(command+len, "%s:", sc->hostName);
        len += sprintf(command+len, "%s", sc->srcFile);

        /* If no destination location is mentioned then
         * DEFAULT_DST is the destination. */
        if (sc->dstFile)
        {
            dest = sc->dstFile;
        }
        else
        {
            dest = DEFAULT_DST;
        }
        arguments[1] = dest;
    }

    arguments[0] = command;
    execute_command("sftp", argc, (const char **)arguments);

    free (command);
    return ret;
}

/* SFTP client interactive copy. */
DEFUN ( cli_sftp_interactive,
        cli_sftp_interactive_cmd,
        "copy sftp WORD (A.B.C.D | X:X::X:X | WORD)",
        COPY_STR
        SFTP_CLIENT_STR
        USERNAME_STR
        HOST_IPv4
        HOST_IPv6
        HOSTNAME_STR )
{
    sftpClient sclient;
    memset(&sclient, 0, sizeof(sftpClient));

    /* Validation of input params. */
    if (strlen((char*)argv[0]) > MAX_USERNAME_LEN)
    {
        vty_out(vty, "Username should be less than %d "
                     "characters%s",MAX_USERNAME_LEN, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    else
    {
        sclient.userName = (char *)argv[0];
    }

    if (isalpha((int) *argv[1]))
    {
        if (strlen((char*)argv[1]) < MAX_HOSTNAME_LEN)
        {
            sclient.hostName = (char *)argv[1];
        }
        else
        {
            vty_out(vty, "Hostname should be less than %d characters "
                         "or invalid host IP%s",MAX_HOSTNAME_LEN, VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }
    else
    {
        sclient.hostName = (char *)argv[1];
    }

    sclient.srcFile = "";
    sclient.dstFile = NULL;
    sclient.isInteractive = true;

    return sftp_client_copy(&sclient);
}

/* SFTP client non-interactive copy. */
DEFUN ( cli_sftp_non_interactive_copy,
        cli_sftp_non_interactive_cmd,
        "copy sftp WORD (A.B.C.D | X:X::X:X | WORD) WORD [WORD]",
        COPY_STR
        SFTP_CLIENT_STR
        USERNAME_STR
        HOST_IPv4
        HOST_IPv6
        HOSTNAME_STR
        SRC_FILENAME
        DST_FILENAME )
{
    sftpClient sclient;
    memset(&sclient, 0, sizeof(sftpClient));

    /* Validation of input params. */
    if (strlen((char*)argv[0]) > MAX_USERNAME_LEN)
    {
        vty_out(vty, "Username should be less than %d "
                     "characters%s",MAX_USERNAME_LEN, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    else
    {
        sclient.userName = (char *)argv[0];
    }

    if (isalpha((int) *argv[1]))
    {
        if (strlen((char*)argv[1]) < MAX_HOSTNAME_LEN)
        {
            sclient.hostName = (char *)argv[1];
        }
        else
        {
            vty_out(vty, "Hostname should be less than %d characters "
                         "or invalid host IP%s",MAX_HOSTNAME_LEN, VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }
    else
    {
        sclient.hostName = (char *)argv[1];
    }

    sclient.srcFile = (char *)argv[2];
    sclient.dstFile = (char *)argv[3];
    sclient.isInteractive = false;

    return sftp_client_copy(&sclient);
}

/* Install SFTP VTY commands. */
void
sftp_vty_init (void)
{
    install_element(ENABLE_NODE, &cli_sftp_non_interactive_cmd);
    install_element(ENABLE_NODE, &cli_sftp_interactive_cmd);
}
