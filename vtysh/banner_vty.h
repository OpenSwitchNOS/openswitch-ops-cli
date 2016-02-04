#ifndef _BANNER_VTY_H
#define _BANNER_VTY_H

#define MAX_BANNER_LENGTH 4096

/* Changing these default strings changes determines the
 * string banner restored by the "banner default" command.
 * To change the default string set on a newly created image
 * consult the OVSDB schema. Changing this to require only a
 * single change would require additional columns be added
 * to the schema.
 */
#define DEFAULT_BANNER_STR "\nWelcome to OpenSwitch\n"
#define DEFAULT_BANNER_EXEC_STR "\nPlease be responsible\n"

#define SYSTEM_OTHER_CONFIG_MAP_BANNER "banner"
#define SYSTEM_OTHER_CONFIG_MAP_BANNER_EXEC "banner_exec"

enum banner_type {
	BANNER_MOTD,
	BANNER_EXEC
};

int install_banner(enum banner_type which_banner, const char *delim);
int display_banner(enum banner_type which_banner);
void banner_vty_init(void);

#endif
