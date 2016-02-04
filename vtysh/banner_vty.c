/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
#include "lib/command.h"
#include "lib/memory.h"
#include "lib/memtypes.h"
#include <readline/readline.h>
#include <string.h>
#include "vtysh.h"
#include "banner_vty.h"
#include "vtysh_ovsdb_if.h"

static int install_banner(enum banner_type which_banner, const char *delim);
static int display_banner(enum banner_type which_banner);

DEFUN (banner_motd,
       banner_motd_cmd,
       "banner (default|DELIMITER)",
       BANNER_STR
       BANNER_DEFAULT_STR
       BANNER_DELIM_STR)
{
    if (argv[0] == NULL)
    {
        return CMD_ERR_INCOMPLETE;
    }

    if (strncmp(argv[0], "default", strlen("default")) == 0)
    {
        return vtysh_ovsdb_banner_set(DEFAULT_BANNER_STR, BANNER_MOTD);
    }

    return install_banner(BANNER_MOTD, argv[0]);
}

DEFUN (banner_exec,
       banner_exec_cmd,
       "banner exec (default|DELIMITER)",
       BANNER_STR
       BANNER_EXEC_STR
       BANNER_EXEC_DEFAULT_STR
       BANNER_DELIM_STR)
{
    if (argv[0] == NULL)
    {
        return CMD_ERR_INCOMPLETE;
    }

    if (strncmp(argv[0], "default", strlen("default")) == 0)
    {
        return vtysh_ovsdb_banner_set(DEFAULT_BANNER_EXEC_STR, BANNER_EXEC);
    }

    return install_banner(BANNER_EXEC, argv[0]);
}

DEFUN (no_banner_motd,
       no_banner_motd_cmd,
       "no banner {exec}",
       NO_STR
       NO_BANNER_STR
       BANNER_EXEC_STR)
{
    if (argv[0] == NULL) /* 'exec' was not specified */
    {
        return vtysh_ovsdb_banner_set("", BANNER_MOTD);
    }
    else
    {
        return vtysh_ovsdb_banner_set("", BANNER_EXEC);
    }
}

DEFUN (show_banner_motd,
       show_banner_motd_cmd,
       "show banner {exec}",
       SHOW_STR
       SHOW_BANNER_STR
       SHOW_BANNER_EXEC_STR)
{
    if (argv[0] == NULL) /* 'exec' was not specified */
    {
        return display_banner(BANNER_MOTD);
    }
    else
    {
        return display_banner(BANNER_EXEC);
    }
}

static int
install_banner(enum banner_type which_banner, const char *delim) {
    char new_banner[MAX_BANNER_LENGTH];
    char *line_read = NULL;
    int return_code = 0;
    new_banner[0] = '\0';
    vty_out(vty, MSG_BANNER_ENTER_BANNER, VTY_NEWLINE);
    strncat(new_banner, VTY_NEWLINE, MAX_BANNER_LENGTH - strlen(new_banner));
    while((line_read = readline(">> "))) {
        if (strlen(new_banner) + strlen(line_read) < MAX_BANNER_LENGTH) {
            if (0 == strncmp(line_read, delim, 1))
            {
                break;
            }
            strncat(new_banner, line_read, MAX_BANNER_LENGTH - strlen(new_banner));
            strncat(new_banner, VTY_NEWLINE, MAX_BANNER_LENGTH - strlen(new_banner));
        }
        else {
            vty_out(vty, MSG_BANNER_TOO_LONG, VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }
    if (which_banner == BANNER_MOTD) {
        return_code = vtysh_ovsdb_banner_set(new_banner, BANNER_MOTD);
    }
    else { // which_banner == BANNER_EXEC
        return_code = vtysh_ovsdb_banner_set(new_banner, BANNER_EXEC);
    }
    return return_code;
}

static int
display_banner(enum banner_type which_banner) {
    char banner[MAX_BANNER_LENGTH];
    int return_code = 0;
    if (which_banner == BANNER_MOTD) {
        return_code = vtysh_ovsdb_banner_get(banner);
    }
    else {  // which_banner == BANNER_EXEC
        return_code = vtysh_ovsdb_banner_exec_get(banner);
    }
    vty_out (vty, "%s%s", banner, VTY_NEWLINE);
    return return_code;
}

void banner_vty_init(void) {
    install_element (CONFIG_NODE, &banner_motd_cmd);
    install_element (CONFIG_NODE, &banner_exec_cmd);
    install_element (CONFIG_NODE, &no_banner_motd_cmd);
    install_element (ENABLE_NODE, &show_banner_motd_cmd);
}
