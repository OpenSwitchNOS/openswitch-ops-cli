#include "lib/command.h"
#include "lib/memory.h"
#include "lib/memtypes.h"
#include <readline/readline.h>
#include <string.h>
#include "vtysh.h"
#include "banner_vty.h"
#include "vtysh_ovsdb_if.h"

DEFUN (banner_motd,
       banner_motd_cmd,
       "banner (default|DELIMETER)",
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
       "banner exec (default|DELIMETER)",
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

int install_banner(enum banner_type which_banner, const char *delim) {
    char new_banner[MAX_BANNER_LENGTH];
    char *line_read = NULL;
    int return_code = 0;
    new_banner[0] = '\0';
    vty_out(vty, "Enter a new banner, when you are done enter a new line "
            "containing only your chosen delimeter.");
    vty_out(vty, VTY_NEWLINE);
    strcat(new_banner, VTY_NEWLINE);
    while((line_read = readline(">> "))) {
        if (strlen(new_banner) + strlen(line_read) < MAX_BANNER_LENGTH) {
            if (0 == strcmp(line_read, delim))
            {
                break;
            }
            strcat(new_banner, line_read);
            strcat(new_banner, VTY_NEWLINE);
        }
        else {
            vty_out(vty, "Banner exceeds maximum allowable length.%s", VTY_NEWLINE);
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

int display_banner(enum banner_type which_banner) {
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
