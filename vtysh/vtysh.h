/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
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
 */

#ifndef VTYSH_H
#define VTYSH_H

#define VTYSH_ZEBRA  0x01
#define VTYSH_RIPD   0x02
#define VTYSH_RIPNGD 0x04
#define VTYSH_OSPFD  0x08
#define VTYSH_OSPF6D 0x10
#define VTYSH_BGPD   0x20
#define VTYSH_ISISD  0x40
#define VTYSH_BABELD  0x80
#define VTYSH_PIMD   0x100
#define VTYSH_ALL	  VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_RIPNGD|VTYSH_OSPFD|VTYSH_OSPF6D|VTYSH_BGPD|VTYSH_ISISD|VTYSH_BABELD|VTYSH_PIMD
#define VTYSH_RMAP	  VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_RIPNGD|VTYSH_OSPFD|VTYSH_OSPF6D|VTYSH_BGPD|VTYSH_BABELD
#define VTYSH_INTERFACE	  VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_RIPNGD|VTYSH_OSPFD|VTYSH_OSPF6D|VTYSH_ISISD|VTYSH_BABELD|VTYSH_PIMD

/* vtysh local configuration file. */
#define VTYSH_DEFAULT_CONFIG "vtysh.conf"

#ifdef ENABLE_OVSDB

#define VTYSH_MAX_ALIAS_SUPPORTED 50
#define VTYSH_MAX_ALIAS_DEF_LEN   30
#define VTYSH_MAX_ALIAS_DEF_LEN_WITH_ARGS   40
#define VTYSH_MAX_ALIAS_LIST_LEN 400

struct vtysh_alias_data {
   char alias_def_str[VTYSH_MAX_ALIAS_DEF_LEN];
   char alias_list_str[VTYSH_MAX_ALIAS_LIST_LEN];
   struct cmd_element alias_cmd_element;
   char alias_def_str_with_args[VTYSH_MAX_ALIAS_DEF_LEN_WITH_ARGS];
   struct cmd_element alias_cmd_element_with_args;

};

#define VTYSH_ALIAS_CMD_HELPSTRING            "Execute \"show aliases\" to list the command list\nArguments to replace $1, $2 etc.\n"
#define VTYSH_ERROR_MAX_ALIASES_EXCEEDED      "Max number of aliases supported exceeded\n"
#define VTYSH_ERROR_MAX_ALIAS_LEN_EXCEEDED    "Max length exceeded\n"
#define VTYSH_ERROR_ALIAS_NAME_ALREADY_EXISTS "Alias command name can not be same as an already existing token\n"
#define VTYSH_ERROR_ALIAS_NOT_FOUND           "Alias (%s) not configured\n"
#define VTYSH_ERROR_ALIAS_LOOP_ALIAS          "Alias name can not be part of its own definition\n"

#define VTYSH_CONSOLE_LENGTH 80

#define OVSDB_TXN_CREATE_ERROR "Couldn't create the OVSDB transaction."
#define OVSDB_ROW_FETCH_ERROR  "Couldn't fetch row from the DB."
#define OVSDB_TXN_COMMIT_ERROR "Committing transaction to DB failed."

extern int vtysh_alias_callback(struct cmd_element *self, struct vty *vty, int vty_flags, int argc, const char *argv[]);

extern int enable_mininet_test_prompt;
#endif

void vtysh_init_vty (void);
void vtysh_init_cmd (void);
extern int vtysh_connect_all (const char *optional_daemon_name);
void vtysh_readline_init (void);
void vtysh_user_init (void);

int vtysh_execute (const char *);
int vtysh_execute_no_pager (const char *);

char *vtysh_prompt (void);

void vtysh_config_write (void);

int vtysh_config_from_file (struct vty *, FILE *);

int vtysh_read_config (char *);

void vtysh_config_parse (char *);

void vtysh_config_dump (FILE *);

void vtysh_config_init (void);

void vtysh_pager_init (void);

int execute_command (const char *, int, const char *arg[]);


/* Child process execution flag. */
extern int execute_flag;

extern struct vty *vty;

#endif /* VTYSH_H */
