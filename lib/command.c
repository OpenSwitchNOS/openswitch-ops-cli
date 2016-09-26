/*
   Command interpreter routine for virtual terminal [aka TeletYpe]
   Copyright (C) 1997, 98, 99 Kunihiro Ishiguro
   Copyright (C) 2013 by Open Source Routing.
   Copyright (C) 2013 by Internet Systems Consortium, Inc. ("ISC")
   Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2, or (at your
option) any later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <zebra.h>


#include "memory.h"
#include "log.h"
#include <lib/version.h>
#include "thread.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "shash.h"
#include "workqueue.h"
#ifdef ENABLE_OVSDB
#include "lib_vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_config_context.h"
#include "vty_utils.h"
#include "openvswitch/vlog.h"
#include "vtysh/utils/tacacs_vtysh_utils.h"
#include "vtysh/utils/audit_log_utils.h"
#include "ovsdb-idl.h"
#include "vtysh/utils/vlan_vtysh_utils.h"
#include <latch.h>
VLOG_DEFINE_THIS_MODULE(vtysh_command);
extern struct ovsdb_idl *idl;
#endif

#define MAX_CMD_LEN 256
#define MAX_DYN_HELPSTR_LEN 256
/* Command vector which includes some level of command lists. Normally
   each daemon maintains each own cmdvec. */
vector cmdvec = NULL;
#define AUDIT_LOG_USER_MSG(vty, cfgdata, ret) \
    if (vty->node >= CONFIG_NODE){ \
       const char *hostname = vtysh_ovsdb_hostname_get(); \
       strcat(strcpy(op, "op=CLI:command"), " "); \
       audit_log_user_msg(op, cfgdata, (char*)hostname, ret); \
    }

struct cmd_token token_cr;
char *command_cr = NULL;

enum filter_type
{
  FILTER_RELAXED,
  FILTER_STRICT
};

enum matcher_rv
{
  MATCHER_OK,
  MATCHER_COMPLETE,
  MATCHER_INCOMPLETE,
  MATCHER_NO_MATCH,
  MATCHER_AMBIGUOUS,
  MATCHER_EXCEED_ARGC_MAX
};

#define MATCHER_ERROR(matcher_rv) \
  (   (matcher_rv) == MATCHER_INCOMPLETE \
   || (matcher_rv) == MATCHER_NO_MATCH \
   || (matcher_rv) == MATCHER_AMBIGUOUS \
   || (matcher_rv) == MATCHER_EXCEED_ARGC_MAX \
  )

/* Host information structure. */
struct host host;

/* list of callback functions for dynamic helpstr */
static struct dyn_cb_func *dyn_cb_lookup = NULL;
/* Standard command node structures. */
static struct cmd_node auth_node =
{
  AUTH_NODE,
  "Password: ",
};

static struct cmd_node view_node =
{
  VIEW_NODE,
  "%s> ",
};

static struct cmd_node restricted_node =
{
  RESTRICTED_NODE,
  "%s$ ",
};

static struct cmd_node auth_enable_node =
{
  AUTH_ENABLE_NODE,
  "Password: ",
};

static struct cmd_node enable_node =
{
  ENABLE_NODE,
  "%s# ",
};

static struct cmd_node config_node =
{
  CONFIG_NODE,
  "%s(config)# ",
  1
};


/* Default motd string. */
static const char *default_motd =
"\r\n\
Hello, this is " QUAGGA_PROGNAME " (version " QUAGGA_VERSION ").\r\n\
" GIT_INFO "\r\n";


static const struct facility_map {
  int facility;
  const char *name;
  size_t match;
} syslog_facilities[] =
  {
    { LOG_KERN, "kern", 1 },
    { LOG_USER, "user", 2 },
    { LOG_MAIL, "mail", 1 },
    { LOG_DAEMON, "daemon", 1 },
    { LOG_AUTH, "auth", 1 },
    { LOG_SYSLOG, "syslog", 1 },
    { LOG_LPR, "lpr", 2 },
    { LOG_NEWS, "news", 1 },
    { LOG_UUCP, "uucp", 2 },
    { LOG_CRON, "cron", 1 },
#ifdef LOG_FTP
    { LOG_FTP, "ftp", 1 },
#endif
    { LOG_LOCAL0, "local0", 6 },
    { LOG_LOCAL1, "local1", 6 },
    { LOG_LOCAL2, "local2", 6 },
    { LOG_LOCAL3, "local3", 6 },
    { LOG_LOCAL4, "local4", 6 },
    { LOG_LOCAL5, "local5", 6 },
    { LOG_LOCAL6, "local6", 6 },
    { LOG_LOCAL7, "local7", 6 },
    { 0, NULL, 0 },
  };

int get_list_value(const char *str, unsigned long *num_range);
void cmd_free_memory_str(char *str);

static const char *
facility_name(int facility)
{
  const struct facility_map *fm;

  for (fm = syslog_facilities; fm->name; fm++)
    if (fm->facility == facility)
      return fm->name;
  return "";
}

static int
facility_match(const char *str)
{
  const struct facility_map *fm;

  for (fm = syslog_facilities; fm->name; fm++)
    if (!strncmp(str,fm->name,fm->match))
      return fm->facility;
  return -1;
}

static int
level_match(const char *s)
{
  int level ;

  for ( level = 0 ; zlog_priority [level] != NULL ; level ++ )
    if (!strncmp (s, zlog_priority[level], 2))
      return level;
  return ZLOG_DISABLED;
}

/* This is called from main when a daemon is invoked with -v or --version. */
void
print_version (const char *progname)
{
  printf ("%s version %s\n", progname, QUAGGA_VERSION);
}


/* Utility function to concatenate argv argument into a single string
   with inserting ' ' character between each argument.  */
char *
argv_concat (const char **argv, int argc, int shift)
{
  int i;
  size_t len;
  char *str;
  char *p;

  len = 0;
  for (i = shift; i < argc; i++)
    len += strlen(argv[i])+1;
  if (!len)
    return NULL;
  p = str = XMALLOC(MTYPE_TMP, len);
  for (i = shift; i < argc; i++)
    {
      size_t arglen;
      memcpy(p, argv[i], (arglen = strlen(argv[i])));
      p += arglen;
      *p++ = ' ';
    }
  *(p-1) = '\0';
  return str;
}

/* Install top node of command vector. */
void
install_node (struct cmd_node *node,
	      int (*func) (struct vty *))
{
  vector_set_index (cmdvec, node->node, node);
  node->func = func;
  node->cmd_vector = vector_init (VECTOR_MIN_SIZE);
}

/* Breaking up string into each command piece. I assume given
   character is separated by a space character. Return value is a
   vector which includes char ** data element. */
vector
cmd_make_strvec (const char *string)
{
  const char *cp, *start;
  char *token;
  int strlen;
  vector strvec;

  if (string == NULL)
    return NULL;

  cp = string;

  /* Skip white spaces. */
  while (isspace ((int) *cp) && *cp != '\0')
    cp++;

  /* Return if there is only white spaces */
  if (*cp == '\0')
    return NULL;

  if (*cp == '!' || *cp == '#')
    return NULL;

  /* Prepare return vector. */
  strvec = vector_init (VECTOR_MIN_SIZE);

  /* Copy each command piece and set into vector. */
  while (1)
    {
      start = cp;
      while (!(isspace ((int) *cp) || *cp == '\r' || *cp == '\n') &&
	     *cp != '\0')
	cp++;
      strlen = cp - start;
      token = XMALLOC (MTYPE_STRVEC, strlen + 1);
      memcpy (token, start, strlen);
      *(token + strlen) = '\0';
      vector_set (strvec, token);

      while ((isspace ((int) *cp) || *cp == '\n' || *cp == '\r') &&
	     *cp != '\0')
	cp++;

      if (*cp == '\0')
	return strvec;
    }
}

/* Free allocated string vector. */
void
cmd_free_strvec (vector v)
{
  unsigned int i;
  char *cp;

  if (!v)
    return;

  for (i = 0; i < vector_active (v); i++)
    if ((cp = vector_slot (v, i)) != NULL)
      XFREE (MTYPE_STRVEC, cp);

  vector_free (v);
}

#ifndef ENABLE_OVSDB
struct format_parser_state
{
  vector topvect; /* Top level vector */
  vector intvect; /* Intermediate level vector, used when there's
                   * a multiple in a keyword. */
  vector curvect; /* current vector where read tokens should be
                     appended. */

  const char *string; /* pointer to command string, not modified */
  const char *cp; /* pointer in command string, moved along while
                     parsing */
  const char *dp;  /* pointer in description string, moved along while
                     parsing */
  const char *dyn_cbp;  /* pointer in dynamic callback string, moved
                           along while parsing */

  int in_keyword; /* flag to remember if we are in a keyword group */
  int in_multiple; /* flag to remember if we are in a multiple group */
  int just_read_word; /* flag to remember if the last thing we red was a
                       * real word and not some abstract token */
};
#endif

static void
format_parser_error(struct format_parser_state *state, const char *message)
{
  int offset = state->cp - state->string + 1;

  fprintf(stderr, "\nError parsing command: \"%s\"\n", state->string);
  fprintf(stderr, "                        %*c\n", offset, '^');
  fprintf(stderr, "%s at offset %d.\n", message, offset);
  fprintf(stderr, "This is a programming error. Check your DEFUNs etc.\n");
  exit(1);
}

char *
format_parser_desc_str(struct format_parser_state *state)
{
  const char *cp, *start;
  char *token;
  int strlen;

  cp = state->dp;

  if (cp == NULL)
    return NULL;

  /* Skip white spaces. */
  while (isspace ((int) *cp) && *cp != '\0')
    cp++;

  /* Return if there is only white spaces */
  if (*cp == '\0')
    return NULL;

  start = cp;

  while (!(*cp == '\r' || *cp == '\n') && *cp != '\0')
    cp++;

  strlen = cp - start;
  token = XMALLOC (MTYPE_CMD_TOKENS, strlen + 1);
  memcpy (token, start, strlen);
  *(token + strlen) = '\0';

  state->dp = cp;

  return token;
}

char *
format_parser_dyn_cb_str(struct format_parser_state *state)
{
  const char *cp, *start;
  char *token;
  int strlen;

  cp = state->dyn_cbp;

  if (cp == NULL)
    return NULL;

  /* Skip white spaces. */
  while (*cp == ' ' && *cp != '\0')
    cp++;

  /* Return if there is only white spaces */
  if (*cp == '\0')
    return NULL;

  start = cp;
  while (!(*cp == '\r' || *cp == '\n') && *cp != '\0')
    cp++;

  strlen = cp - start;
  if (strlen > 0)
  {
    token = XMALLOC (MTYPE_CMD_TOKENS, strlen + 1);
    memcpy (token, start, strlen);
    *(token + strlen) = '\0';
  }
  else
    token = NULL;

  if (*cp == '\n')
    cp++;

  state->dyn_cbp = cp;

  return token;
}

static void
format_parser_begin_keyword(struct format_parser_state *state)
{
  struct cmd_token *token;
  vector keyword_vect;

  if (state->in_keyword
      || state->in_multiple)
    format_parser_error(state, "Unexpected '{'");

  state->cp++;
  state->in_keyword = 1;

  token = XCALLOC(MTYPE_CMD_TOKENS, sizeof(*token));
  token->type = TOKEN_KEYWORD;
  token->keyword = vector_init(VECTOR_MIN_SIZE);

  keyword_vect = vector_init(VECTOR_MIN_SIZE);
  vector_set(token->keyword, keyword_vect);

  vector_set(state->curvect, token);
  state->curvect = keyword_vect;
}

static void
format_parser_begin_multiple(struct format_parser_state *state)
{
  struct cmd_token *token;

  if (state->in_keyword == 1)
    format_parser_error(state, "Keyword starting with '('");

  if (state->in_multiple)
    format_parser_error(state, "Nested group");

  state->cp++;
  state->in_multiple = 1;
  state->just_read_word = 0;

  token = XCALLOC(MTYPE_CMD_TOKENS, sizeof(*token));
  token->type = TOKEN_MULTIPLE;
  token->multiple = vector_init(VECTOR_MIN_SIZE);

  vector_set(state->curvect, token);
  if (state->curvect != state->topvect)
    state->intvect = state->curvect;
  state->curvect = token->multiple;
}

static void
format_parser_end_keyword(struct format_parser_state *state)
{
  if (state->in_multiple
      || !state->in_keyword)
    format_parser_error(state, "Unexpected '}'");

  if (state->in_keyword == 1)
    format_parser_error(state, "Empty keyword group");

  state->cp++;
  state->in_keyword = 0;
  state->curvect = state->topvect;
}

static void
format_parser_end_multiple(struct format_parser_state *state)
{
  char *dummy;

  if (!state->in_multiple)
    format_parser_error(state, "Unepexted ')'");

  if (vector_active(state->curvect) == 0)
    format_parser_error(state, "Empty multiple section");

  if (!state->just_read_word)
    {
      /* There are constructions like
       * 'show ip ospf database ... (self-originate|)'
       * in use.
       * The old parser reads a description string for the
       * word '' between |) which will never match.
       * Simulate this behvaior by dropping the next desc
       * string in such a case. */

      dummy = format_parser_desc_str(state);
      XFREE(MTYPE_CMD_TOKENS, dummy);
    }

  state->cp++;
  state->in_multiple = 0;

  if (state->intvect)
    state->curvect = state->intvect;
  else
    state->curvect = state->topvect;
}

static void
format_parser_handle_pipe(struct format_parser_state *state)
{
  struct cmd_token *keyword_token;
  vector keyword_vect;

  if (state->in_multiple)
    {
      state->just_read_word = 0;
      state->cp++;
    }
  else if (state->in_keyword)
    {
      state->in_keyword = 1;
      state->cp++;

      keyword_token = vector_slot(state->topvect,
                                  vector_active(state->topvect) - 1);
      keyword_vect = vector_init(VECTOR_MIN_SIZE);
      vector_set(keyword_token->keyword, keyword_vect);
      state->curvect = keyword_vect;
    }
  else
    {
      format_parser_error(state, "Unexpected '|'");
    }
}

void
format_parser_read_word(struct format_parser_state *state)
{
  const char *start;
  int len;
  char *cmd;
  struct cmd_token *token;
  struct dyn_cb_func * dyn_cb_temp;

  start = state->cp;

  while (state->cp[0] != '\0'
         && !strchr("\r\n(){}|", state->cp[0])
         && !isspace((int)state->cp[0]))
    state->cp++;

  len = state->cp - start;
  cmd = XMALLOC(MTYPE_CMD_TOKENS, len + 1);
  memcpy(cmd, start, len);
  cmd[len] = '\0';

  token = XCALLOC(MTYPE_CMD_TOKENS, sizeof(*token));
  token->type = TOKEN_TERMINAL;
  token->cmd = cmd;
  token->desc = format_parser_desc_str(state);

  if ((state->dyn_cbp != NULL) && (token->dyn_cb == NULL))
  {
    token->dyn_cb = format_parser_dyn_cb_str(state);

    if (token->dyn_cb != NULL)
    {
      dyn_cb_temp = dyn_cb_lookup;
      while (dyn_cb_temp != NULL)
      {
          if (!strcmp(dyn_cb_temp->funcname, token->dyn_cb)) {
              token->dyn_cb_func = dyn_cb_temp->funcptr;
              break;
          }
          dyn_cb_temp = dyn_cb_temp->next;
      }
    }
  }

  vector_set(state->curvect, token);

  if (state->in_keyword == 1)
    state->in_keyword = 2;

  state->just_read_word = 1;
}

/**
 * Parse a given command format string and build a tree of tokens from
 * it that is suitable to be used by the command subsystem.
 *
 * @param string Command format string.
 * @param descstr Description string.
 * @param dyn_cb_str Dynamic helpstr callback functions string
 * @return A vector of struct cmd_token representing the given command,
 *         or NULL on error.
 */
vector
cmd_parse_format(const char *string, const char *descstr, const char *dyn_cb_str)
{
  struct format_parser_state state;

  if (string == NULL)
    return NULL;

  memset(&state, 0, sizeof(state));
  state.topvect = state.curvect = vector_init(VECTOR_MIN_SIZE);
  state.cp = state.string = string;
  state.dp = descstr;
  state.dyn_cbp = dyn_cb_str;

  while (1)
    {
      while (isspace((int)state.cp[0]) && state.cp[0] != '\0')
        state.cp++;

      switch (state.cp[0])
        {
        case '\0':
          if (state.in_keyword
              || state.in_multiple)
            format_parser_error(&state, "Unclosed group/keyword");
          return state.topvect;
        case '{':
          format_parser_begin_keyword(&state);
          break;
        case '(':
          format_parser_begin_multiple(&state);
          break;
        case '}':
          format_parser_end_keyword(&state);
          break;
        case ')':
          format_parser_end_multiple(&state);
          break;
        case '|':
          format_parser_handle_pipe(&state);
          break;
        default:
#ifndef ENABLE_OVSDB
          format_parser_read_word(&state);
#else
          utils_format_parser_read_word(&state);
#endif
        }
    }
}

/* Return prompt character of specified node. */
const char *
cmd_prompt (enum node_type node)
{
  struct cmd_node *cnode;

  cnode = vector_slot (cmdvec, node);
  return cnode->prompt;
}

/* Install a command into a node. */
void
install_element (enum node_type ntype, struct cmd_element *cmd)
{
  struct cmd_node *cnode;

  /* cmd_init hasn't been called */
  if (!cmdvec)
    return;

  cnode = vector_slot (cmdvec, ntype);

  if (cnode == NULL)
    {
      fprintf (stderr, "Command node %d doesn't exist, please check it\n",
	       ntype);
      exit (1);
    }

  vector_set (cnode->cmd_vector, cmd);
  if (cmd->tokens == NULL)
#ifndef ENABLE_OVSDB
    cmd->tokens = cmd_parse_format(cmd->string, cmd->doc, cmd->dyn_cb_str);
#else
    cmd->tokens = utils_cmd_parse_format(cmd->string, cmd->doc, cmd->dyn_cb_str);
#endif
}

static const unsigned char itoa64[] =
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void
to64(char *s, long v, int n)
{
  while (--n >= 0)
    {
      *s++ = itoa64[v&0x3f];
      v >>= 6;
    }
}

static char *
zencrypt (const char *passwd)
{
  char salt[6];
  struct timeval tv;
  char *crypt (const char *, const char *);

  gettimeofday(&tv,0);

  to64(&salt[0], random(), 3);
  to64(&salt[3], tv.tv_usec, 3);
  salt[5] = '\0';

  return crypt (passwd, salt);
}

/* This function write configuration of this host. */
static int
config_write_host (struct vty *vty)
{
  if (host.name)
    vty_out (vty, "hostname %s%s", host.name, VTY_NEWLINE);

  if (host.encrypt)
    {
      if (host.password_encrypt)
        vty_out (vty, "password 8 %s%s", host.password_encrypt, VTY_NEWLINE);
      if (host.enable_encrypt)
        vty_out (vty, "enable password 8 %s%s", host.enable_encrypt, VTY_NEWLINE);
    }
  else
    {
      if (host.password)
        vty_out (vty, "password %s%s", host.password, VTY_NEWLINE);
      if (host.enable)
        vty_out (vty, "enable password %s%s", host.enable, VTY_NEWLINE);
    }

  if (zlog_default->default_lvl != LOG_DEBUG)
    {
      vty_out (vty, "! N.B. The 'log trap' command is deprecated.%s",
	       VTY_NEWLINE);
      vty_out (vty, "log trap %s%s",
	       zlog_priority[zlog_default->default_lvl], VTY_NEWLINE);
    }

  if (host.logfile && (zlog_default->maxlvl[ZLOG_DEST_FILE] != ZLOG_DISABLED))
    {
      vty_out (vty, "log file %s", host.logfile);
      if (zlog_default->maxlvl[ZLOG_DEST_FILE] != zlog_default->default_lvl)
	vty_out (vty, " %s",
		 zlog_priority[zlog_default->maxlvl[ZLOG_DEST_FILE]]);
      vty_out (vty, "%s", VTY_NEWLINE);
    }

  if (zlog_default->maxlvl[ZLOG_DEST_STDOUT] != ZLOG_DISABLED)
    {
      vty_out (vty, "log stdout");
      if (zlog_default->maxlvl[ZLOG_DEST_STDOUT] != zlog_default->default_lvl)
	vty_out (vty, " %s",
		 zlog_priority[zlog_default->maxlvl[ZLOG_DEST_STDOUT]]);
      vty_out (vty, "%s", VTY_NEWLINE);
    }

  if (zlog_default->maxlvl[ZLOG_DEST_MONITOR] == ZLOG_DISABLED)
    vty_out(vty,"no log monitor%s",VTY_NEWLINE);
  else if (zlog_default->maxlvl[ZLOG_DEST_MONITOR] != zlog_default->default_lvl)
    vty_out(vty,"log monitor %s%s",
	    zlog_priority[zlog_default->maxlvl[ZLOG_DEST_MONITOR]],VTY_NEWLINE);

  if (zlog_default->maxlvl[ZLOG_DEST_SYSLOG] != ZLOG_DISABLED)
    {
      vty_out (vty, "log syslog");
      if (zlog_default->maxlvl[ZLOG_DEST_SYSLOG] != zlog_default->default_lvl)
	vty_out (vty, " %s",
		 zlog_priority[zlog_default->maxlvl[ZLOG_DEST_SYSLOG]]);
      vty_out (vty, "%s", VTY_NEWLINE);
    }

  if (zlog_default->facility != LOG_DAEMON)
    vty_out (vty, "log facility %s%s",
	     facility_name(zlog_default->facility), VTY_NEWLINE);

  if (zlog_default->record_priority == 1)
    vty_out (vty, "log record-priority%s", VTY_NEWLINE);

  if (zlog_default->timestamp_precision > 0)
    vty_out (vty, "log timestamp precision %d%s",
	     zlog_default->timestamp_precision, VTY_NEWLINE);

  if (host.advanced)
    vty_out (vty, "service advanced-vty%s", VTY_NEWLINE);

  if (host.encrypt)
    vty_out (vty, "service password-encryption%s", VTY_NEWLINE);

  if (host.lines >= 0)
    vty_out (vty, "service terminal-length %d%s", host.lines,
	     VTY_NEWLINE);

  if (host.motdfile)
    vty_out (vty, "banner motd file %s%s", host.motdfile, VTY_NEWLINE);
  else if (! host.motd)
    vty_out (vty, "no banner motd%s", VTY_NEWLINE);

  return 1;
}

/* Utility function for getting command vector. */
static vector
cmd_node_vector (vector v, enum node_type ntype)
{
  struct cmd_node *cnode = vector_slot (v, ntype);
  return cnode->cmd_vector;
}

#if 0
/* Filter command vector by symbol.  This function is not actually used;
 * should it be deleted? */
static int
cmd_filter_by_symbol (char *command, char *symbol)
{
  int i, lim;

  if (strcmp (symbol, "IPV4_ADDRESS") == 0)
    {
      i = 0;
      lim = strlen (command);
      while (i < lim)
	{
	  if (! (isdigit ((int) command[i]) || command[i] == '.' || command[i] == '/'))
	    return 1;
	  i++;
	}
      return 0;
    }
  if (strcmp (symbol, "STRING") == 0)
    {
      i = 0;
      lim = strlen (command);
      while (i < lim)
	{
	  if (! (isalpha ((int) command[i]) || command[i] == '_' || command[i] == '-'))
	    return 1;
	  i++;
	}
      return 0;
    }
  if (strcmp (symbol, "IFNAME") == 0)
    {
      i = 0;
      lim = strlen (command);
      while (i < lim)
	{
	  if (! isalnum ((int) command[i]))
	    return 1;
	  i++;
	}
      return 0;
    }
  return 0;
}
#endif

/* Completion match types. */
enum match_type
{
  no_match,
  extend_match,
  ipv4_netmask_match,
  ipv4_prefix_match,
  ipv4_match,
  ipv6_prefix_match,
  ipv6_match,
#ifdef ENABLE_OVSDB
  ifname_match,
  port_match,
  vlan_match,
  mac_match,
#endif
  range_match,
  vararg_match,
  partly_match,
  range_match_comma,
  range_match_list,
  range_match_comma_list,
  exact_match
};

static enum match_type
cmd_ipv4_match (const char *str)
{
  const char *sp;
  int dots = 0, nums = 0;
  char buf[4];

  if (str == NULL)
    return partly_match;

  for (;;)
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0')
	{
	  if (*str == '.')
	    {
	      if (dots >= 3)
		return no_match;

	      if (*(str + 1) == '.')
		return no_match;

	      if (*(str + 1) == '\0')
		return partly_match;

	      dots++;
	      break;
	    }
	  if (!isdigit ((int) *str))
	    return no_match;

	  str++;
	}

      if (str - sp > 3)
	return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
	return no_match;

      nums++;

      if (*str == '\0')
	break;

      str++;
    }

  if (nums < 4)
    return partly_match;

  return exact_match;
}

static enum match_type
cmd_ipv4_prefix_match (const char *str)
{
  const char *sp;
  int dots = 0;
  char buf[4];

  if (str == NULL)
    return partly_match;

  for (;;)
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0' && *str != '/')
	{
	  if (*str == '.')
	    {
	      if (dots == 3)
		return no_match;

	      if (*(str + 1) == '.' || *(str + 1) == '/')
		return no_match;

	      if (*(str + 1) == '\0')
		return partly_match;

	      dots++;
	      break;
	    }

	  if (!isdigit ((int) *str))
	    return no_match;

	  str++;
	}

      if (str - sp > 3)
	return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
	return no_match;

      if (dots == 3)
	{
	  if (*str == '/')
	    {
	      if (*(str + 1) == '\0')
		return partly_match;

	      str++;
	      break;
	    }
	  else if (*str == '\0')
	    return partly_match;
	}

      if (*str == '\0')
	return partly_match;

      str++;
    }

  sp = str;
  while (*str != '\0')
    {
      if (!isdigit ((int) *str))
	return no_match;

      str++;
    }

  if (atoi (sp) > 32)
    return no_match;

  return exact_match;
}

static enum match_type
cmd_ipv4_netmask_match (const char *str)
{
  const char *sp;
  int dots, nums;
  char buf[4];

  if (str == NULL)
    return partly_match;

  for (dots = 0; ; )
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0' && *str != '/')
        {
          if (*str == '.')
            {
              if (dots == 3)
                return no_match;

              if (*(str + 1) == '.' || *(str + 1) == '/')
                return no_match;

              if (*(str + 1) == '\0')
                return partly_match;

              dots++;
              break;
            }

          if (!isdigit ((int) *str))
            return no_match;

          str++;
        }

      if (str - sp > 3)
        return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
        return no_match;

      if (dots == 3)
        {
          if (*str == '/')
            {
              if (*(str + 1) == '\0')
                return partly_match;

              str++;
              break;
            }
          else if (*str == '\0')
            return partly_match;
        }

      if (*str == '\0')
        return partly_match;

      str++;
    }

  for (dots = 0, nums = 0; ; )
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0')
        {
          if (*str == '.')
            {
              if (dots >= 3)
          return no_match;

              if (*(str + 1) == '.')
          return no_match;

              if (*(str + 1) == '\0')
          return partly_match;

              dots++;
              break;
            }
          if (!isdigit ((int) *str))
            return no_match;

          str++;
        }

      if (str - sp > 3)
        return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
        return no_match;

      nums++;

      if (*str == '\0')
        break;

      str++;
    }

  if (nums < 4)
    return partly_match;

  return exact_match;
}

#define IPV6_ADDR_STR		"0123456789abcdefABCDEF:.%"
#define IPV6_PREFIX_STR		"0123456789abcdefABCDEF:.%/"
#define STATE_START		1
#define STATE_COLON		2
#define STATE_DOUBLE		3
#define STATE_ADDR		4
#define STATE_DOT               5
#define STATE_SLASH		6
#define STATE_MASK		7

#ifdef HAVE_IPV6

static enum match_type
cmd_ipv6_match (const char *str)
{
  struct sockaddr_in6 sin6_dummy;
  int ret;

  if (str == NULL)
    return partly_match;

  if (strspn (str, IPV6_ADDR_STR) != strlen (str))
    return no_match;

  /* use inet_pton that has a better support,
   * for example inet_pton can support the automatic addresses:
   *  ::1.2.3.4
   */
  ret = inet_pton(AF_INET6, str, &sin6_dummy.sin6_addr);

  if (ret == 1)
    return exact_match;

  return no_match;
}

static enum match_type
cmd_ipv6_prefix_match (const char *str)
{
  int state = STATE_START;
  int colons = 0, nums = 0, double_colon = 0;
  int mask;
  const char *sp = NULL;
  char *endptr = NULL;

  if (str == NULL)
    return partly_match;

  if (strspn (str, IPV6_PREFIX_STR) != strlen (str))
    return no_match;

  while (*str != '\0' && state != STATE_MASK)
    {
      switch (state)
	{
	case STATE_START:
	  if (*str == ':')
	    {
	      if (*(str + 1) != ':' && *(str + 1) != '\0')
		return no_match;
	      colons--;
	      state = STATE_COLON;
	    }
	  else
	    {
	      sp = str;
	      state = STATE_ADDR;
	    }

	  continue;
	case STATE_COLON:
	  colons++;
	  if (*(str + 1) == '/')
	    return no_match;
	  else if (*(str + 1) == ':')
	    state = STATE_DOUBLE;
	  else
	    {
	      sp = str + 1;
	      state = STATE_ADDR;
	    }
	  break;
	case STATE_DOUBLE:
	  if (double_colon)
	    return no_match;

	  if (*(str + 1) == ':')
	    return no_match;
	  else
	    {
	      if (*(str + 1) != '\0' && *(str + 1) != '/')
		colons++;
	      sp = str + 1;

	      if (*(str + 1) == '/')
		state = STATE_SLASH;
	      else
		state = STATE_ADDR;
	    }

	  double_colon++;
	  nums += 1;
	  break;
	case STATE_ADDR:
	  if (*(str + 1) == ':' || *(str + 1) == '.'
	      || *(str + 1) == '\0' || *(str + 1) == '/')
	    {
	      if (str - sp > 3)
		return no_match;

	      for (; sp <= str; sp++)
		if (*sp == '/')
		  return no_match;

	      nums++;

	      if (*(str + 1) == ':')
		state = STATE_COLON;
	      else if (*(str + 1) == '.')
		{
		  if (colons || double_colon)
		    state = STATE_DOT;
		  else
		    return no_match;
		}
	      else if (*(str + 1) == '/')
		state = STATE_SLASH;
	    }
	  break;
	case STATE_DOT:
	  state = STATE_ADDR;
	  break;
	case STATE_SLASH:
	  if (*(str + 1) == '\0')
	    return partly_match;

	  state = STATE_MASK;
	  break;
	default:
	  break;
	}

      if (nums > 11)
	return no_match;

      if (colons > 7)
	return no_match;

      str++;
    }

  if (state < STATE_MASK)
    return partly_match;

  mask = strtol (str, &endptr, 10);
  if (*endptr != '\0')
    return no_match;

  if (mask < 0 || mask > 128)
    return no_match;

/* I don't know why mask < 13 makes command match partly.
   Forgive me to make this comments. I Want to set static default route
   because of lack of function to originate default in ospf6d; sorry
       yasu
  if (mask < 13)
    return partly_match;
*/

  return exact_match;
}

#endif /* HAVE_IPV6  */

#ifdef ENABLE_OVSDB
static int
cmd_ifname_match (const char *str)
{
  if(NULL == lib_vtysh_ovsdb_interface_match)
    return 1;

  /* check for vlan interfaces */
  if(verify_ifname((char*)str)) {
    return 0;
  }

  if(((*lib_vtysh_ovsdb_interface_match)(str)) == 0)
    return 0;

  return 1;
}

static int
cmd_port_match (const char *str)
{
  if(NULL == lib_vtysh_ovsdb_port_match)
    return 1;

  if(((*lib_vtysh_ovsdb_port_match)(str)) == 0)
    return 0;

  return 1;
}

/* Wrapper function to call mac validation func */
static int
cmd_mac_match (const char *str)
{
  if(NULL == lib_vtysh_ovsdb_mac_match)
    return 1;

  if(((*lib_vtysh_ovsdb_mac_match)(str)) == 0)
    return 0;

  return 1;
}

static int
cmd_vlan_match (const char *str)
{
  if(NULL == lib_vtysh_ovsdb_vlan_match)
    return 1;

  if(((*lib_vtysh_ovsdb_vlan_match)(str)) == 0)
    return 0;

  return 1;
}
#endif /* ENABLE_OVSDB */

#define DECIMAL_STRLEN_MAX 10

static int
cmd_range_match (const char *range, const char *str)
{
  char *p;
  char buf[DECIMAL_STRLEN_MAX + 1];
  char *endptr = NULL;
  unsigned long min, max, val;

  if (str == NULL)
    return 1;

  val = strtoul (str, &endptr, 10);
  if (*endptr != '\0')
    return 0;

  range++;
  p = strchr (range, '-');
  if (p == NULL)
    return 0;
  if (p - range > DECIMAL_STRLEN_MAX)
    return 0;
  strncpy (buf, range, p - range);
  buf[p - range] = '\0';
  min = strtoul (buf, &endptr, 10);
  if (*endptr != '\0')
    return 0;

  range = p + 1;
  p = strchr (range, '>');
  if (p == NULL)
    return 0;
  if (p - range > DECIMAL_STRLEN_MAX)
    return 0;
  strncpy (buf, range, p - range);
  buf[p - range] = '\0';
  max = strtoul (buf, &endptr, 10);
  if (*endptr != '\0')
    return 0;

  if (val < min || val > max)
    return 0;

  return 1;
}

static enum match_type
cmd_word_match(struct cmd_token *token,
               enum filter_type filter,
               const char *word)
{
  const char *str;
  enum match_type match_type;

  str = token->cmd;

  if (filter == FILTER_RELAXED)
    if (!word || !strlen(word))
      return partly_match;

  if (!word)
    return no_match;

  if (CMD_VARARG(str))
    {
      return vararg_match;
    }
  else if (CMD_RANGE(str))
    {
        if ((str[1] == 'C') && (str[2] == ':'))
        {
            if (cmd_range_comma_cli_parser_validate (word, str, COMMA_OPERATOR,
                                                     NULL, NULL) == 1)
                return range_match_comma;
        }
        else if ((str[1] == 'L') && (str[2] == ':'))
        {
            if (cmd_range_comma_cli_parser_validate (word, str, RANGE_OPERATOR,
                                                     NULL, NULL) == 1)
                return range_match_list;
        }
        else if ((str[1] == 'A') && (str[2] == ':'))
        {
            if (cmd_range_comma_cli_parser_validate (word, str, BOTH_OPERATOR,
                                                     NULL, NULL) == 1)
                return range_match_comma_list;
        }
        else if (cmd_range_match(str, word))
        {
            return range_match;
        }
    }
#ifdef HAVE_IPV6
  else if (CMD_IPV6(str))
    {
      match_type = cmd_ipv6_match(word);
      if ((filter == FILTER_RELAXED && match_type != no_match)
          || (filter == FILTER_STRICT && match_type == exact_match))
        return ipv6_match;
    }
  else if (CMD_IPV6_PREFIX(str))
    {
      match_type = cmd_ipv6_prefix_match(word);
      if ((filter == FILTER_RELAXED && match_type != no_match)
          || (filter == FILTER_STRICT && match_type == exact_match))
        return ipv6_prefix_match;
    }
#endif /* HAVE_IPV6 */
  else if (CMD_IPV4(str))
    {
      match_type = cmd_ipv4_match(word);
      if ((filter == FILTER_RELAXED && match_type != no_match)
          || (filter == FILTER_STRICT && match_type == exact_match))
        return ipv4_match;
    }
  else if (CMD_IPV4_PREFIX(str))
    {
      match_type = cmd_ipv4_prefix_match(word);
      if ((filter == FILTER_RELAXED && match_type != no_match)
          || (filter == FILTER_STRICT && match_type == exact_match))
        return ipv4_prefix_match;
    }
  else if (CMD_IPV4_NETMASK(str))
    {
      match_type = cmd_ipv4_netmask_match(word);
      if ((filter == FILTER_RELAXED && match_type != no_match)
          || (filter == FILTER_STRICT && match_type == exact_match))
        return ipv4_netmask_match;
    }
#ifdef ENABLE_OVSDB
  else if (CMD_IFNAME(str))
    {
      if (strcmp (str, "IFNAME") != 0)
      {
        if (cmd_input_comma_str_is_valid(word, BOTH_OPERATOR) != COMMA_ERR)
        {
            char *buf = cmd_allocate_memory_str(word);
            char *temp = buf;
            int flag = 0;
            unsigned long min_max[2];
            unsigned long i = 0;
            char buf_temp[DECIMAL_STRLEN_MAX + 1];
            temp = strtok (temp, ",");
            while (temp)
            {
                if (strchr(temp, '-'))
                {
                    if (cmd_ifname_match (temp) != 0)
                    {
                        get_list_value (temp, min_max);
                        if (min_max[0] >= min_max[1])
                        {
                            cmd_free_memory_str (buf);
                            return no_match;
                        }
                        for (i = min_max[0]; i <= min_max[1]; i++)
                        {
                            snprintf (buf_temp, DECIMAL_STRLEN_MAX + 1, "%lu", i);
                            if (cmd_ifname_match (buf_temp) != 0)
                            {
                                flag = 1;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    if (cmd_ifname_match (temp) != 0)
                    {
                        flag = 1;
                        break;
                    }
                }
                temp = strtok (NULL, ",");
            }

            cmd_free_memory_str (buf);
            if (flag == 0)
            {
                return ifname_match;
            }
        }
      }
      else if (cmd_ifname_match (word) == 0)
        {
            return ifname_match;
        }
    }
  else if (CMD_PORT(str))
    {
      if(cmd_port_match(word) == 0)
        return port_match;
    }
  else if (CMD_VLAN(str))
    {
      if(cmd_vlan_match(word) == 0)
        return vlan_match;
    }
  else if (CMD_MAC(str))
    {
      if(0 == cmd_mac_match(word))
         return mac_match;
    }
#endif
  else if (CMD_OPTION(str) || CMD_VARIABLE(str))
    {
      return extend_match;
    }
  else
    {
      if (filter == FILTER_RELAXED && !strncmp(str, word, strlen(word)))
        {
          if (!strcmp(str, word))
            return exact_match;
          return partly_match;
        }
      if (filter == FILTER_STRICT && !strcmp(str, word))
        return exact_match;
    }

  return no_match;
}

struct cmd_matcher
{
  struct cmd_element *cmd; /* The command element the matcher is using */
  enum filter_type filter; /* Whether to use strict or relaxed matching */
  vector vline; /* The tokenized commandline which is to be matched */
  unsigned int index; /* The index up to which matching should be done */

  /* If set, construct a list of matches at the position given by index */
  enum match_type *match_type;
  vector *match;

  unsigned int word_index; /* iterating over vline */
};

static int
push_argument(int *argc, const char **argv, const char *arg)
{
  if (!arg || !strlen(arg))
    arg = NULL;

  if (!argc || !argv)
    return 0;

  if (*argc >= CMD_ARGC_MAX)
    return -1;

  argv[(*argc)++] = arg;
  return 0;
}

static void
cmd_matcher_record_match(struct cmd_matcher *matcher,
                         enum match_type match_type,
                         struct cmd_token *token)
{
  if (matcher->word_index != matcher->index)
    return;

  if (matcher->match)
    {
      if (!*matcher->match)
        *matcher->match = vector_init(VECTOR_MIN_SIZE);
      vector_set(*matcher->match, token);
    }

  if (matcher->match_type)
    {
      if (match_type > *matcher->match_type)
        *matcher->match_type = match_type;
    }
}

static int
cmd_matcher_words_left(struct cmd_matcher *matcher)
{
  return matcher->word_index < vector_active(matcher->vline);
}

static const char*
cmd_matcher_get_word(struct cmd_matcher *matcher)
{
  assert(cmd_matcher_words_left(matcher));

  return vector_slot(matcher->vline, matcher->word_index);
}

static enum matcher_rv
cmd_matcher_match_terminal(struct cmd_matcher *matcher,
                           struct cmd_token *token,
                           int *argc, const char **argv)
{
  const char *word;
  enum match_type word_match;
  static char szToken[MAX_CMD_LEN] = {0};
  struct cmd_token temptoken;
  char* temp = NULL;
  int nCopysize = 0;

  assert(token->type == TOKEN_TERMINAL);
  memset(&temptoken,0,sizeof(struct cmd_token));
  if (!cmd_matcher_words_left(matcher))
    {
      if (CMD_OPTION(token->cmd))
        return MATCHER_OK; /* missing optional args are NOT pushed as NULL */
      else
        return MATCHER_INCOMPLETE;
    }

  word = cmd_matcher_get_word(matcher);
  word_match = cmd_word_match(token, matcher->filter, word);
  if (word_match == no_match)
    return MATCHER_NO_MATCH;

  /* We have to record the input word as argument if it matched
   * against a variable. */
  if (CMD_VARARG(token->cmd)
      || CMD_VARIABLE(token->cmd))
    {
      if (push_argument(argc, argv, word))
        return MATCHER_EXCEED_ARGC_MAX;
    }
  /* For pushing complete token for '[]' tokens */
  else if (CMD_OPTION(token->cmd))
  {
    /* Before pushing the token remove '[' and ']'*/
    /* increment to point to next byte after */
    temp = strchr(token->cmd,'[') + 1;
    nCopysize = strlen(token->cmd)-2;
    strncpy(szToken,temp,nCopysize);
    memcpy(&temptoken,token,sizeof(struct cmd_token));
    temptoken.cmd = XSTRDUP(MTYPE_CMD_TOKENS,szToken);

    /* Validate and complete the tokens present inside []*/
    word_match = cmd_word_match(&temptoken, matcher->filter, word);
    /* .LINE cannot be an optional type token ([]) */
    if ((no_match == word_match) ||
        (vararg_match == word_match))
    {
       XFREE(MTYPE_CMD_TOKENS,temptoken.cmd);
       return MATCHER_NO_MATCH;
    }
    else if ((partly_match == word_match) ||
         (exact_match == word_match))
    {
       if (push_argument(argc, argv, szToken))
       {
         XFREE(MTYPE_CMD_TOKENS,temptoken.cmd);
         return MATCHER_EXCEED_ARGC_MAX;
       }
    }
    else
    {
       /* if not a fixed string then push value input by user */
       if (push_argument(argc, argv, word))
       {
         XFREE(MTYPE_CMD_TOKENS,temptoken.cmd);
         return MATCHER_EXCEED_ARGC_MAX;
       }
    }
    /* Word match should be extend_match as it is used
       to check for matched count */
    word_match = extend_match;
    XFREE(MTYPE_CMD_TOKENS,temptoken.cmd);
  }

  cmd_matcher_record_match(matcher, word_match, token);

  matcher->word_index++;

  /* A vararg token should consume all left over words as arguments */
  if (CMD_VARARG(token->cmd))
    while (cmd_matcher_words_left(matcher))
      {
        word = cmd_matcher_get_word(matcher);
        if (word && strlen(word))
          push_argument(argc, argv, word);
        matcher->word_index++;
      }

  return MATCHER_OK;
}

static enum matcher_rv
cmd_matcher_match_multiple(struct cmd_matcher *matcher,
                           struct cmd_token *token,
                           int *argc, const char **argv)
{
  enum match_type multiple_match;
  unsigned int multiple_index;
  const char *word;
  const char *arg;
  struct cmd_token *word_token;
  enum match_type word_match;
  assert(token->type == TOKEN_MULTIPLE);

  multiple_match = no_match;

  if (!cmd_matcher_words_left(matcher))
    return MATCHER_INCOMPLETE;

  word = cmd_matcher_get_word(matcher);
  for (multiple_index = 0;
       multiple_index < vector_active(token->multiple);
       multiple_index++)
    {
      word_token = vector_slot(token->multiple, multiple_index);

      word_match = cmd_word_match(word_token, matcher->filter, word);
      if (word_match == no_match)
        continue;

      cmd_matcher_record_match(matcher, word_match, word_token);

      if (word_match > multiple_match)
        {
          multiple_match = word_match;
          /* Push complete token instead of user input value
             if it's a partly match */
          if(partly_match == word_match)
                arg = word_token->cmd;
          else
                arg = word;
        }
      /* To mimic the behavior of the old command implementation, we
       * tolerate any ambiguities here :/ */
    }

  matcher->word_index++;

  if (multiple_match == no_match)
    return MATCHER_NO_MATCH;

  if (push_argument(argc, argv, arg))
    return MATCHER_EXCEED_ARGC_MAX;

  return MATCHER_OK;
}

static enum matcher_rv
cmd_matcher_read_keywords(struct cmd_matcher *matcher,
                          struct cmd_token *token,
                          vector args_vector)
{
  unsigned int i;
  unsigned long keyword_mask;
  unsigned int keyword_found;
  enum match_type keyword_match;
  enum match_type word_match;
  vector keyword_vector;
  struct cmd_token *word_token;
  const char *word;
  int keyword_argc;
  const char **keyword_argv;
  enum matcher_rv rv = MATCHER_NO_MATCH;
  keyword_mask = 0;
  while (1)
    {
      if (!cmd_matcher_words_left(matcher))
        return MATCHER_OK;

      word = cmd_matcher_get_word(matcher);

      keyword_found = -1;
      keyword_match = no_match;
      for (i = 0; i < vector_active(token->keyword); i++)
        {
          if (keyword_mask & (1 << i))
            continue;

          keyword_vector = vector_slot(token->keyword, i);
          word_token = vector_slot(keyword_vector, 0);

          word_match = cmd_word_match(word_token, matcher->filter, word);
          if (word_match == no_match)
            continue;

          cmd_matcher_record_match(matcher, word_match, word_token);

          if (word_match > keyword_match)
            {
              keyword_match = word_match;
              keyword_found = i;
            }
          else if (word_match == keyword_match)
            {
              if (matcher->word_index != matcher->index || args_vector)
                return MATCHER_AMBIGUOUS;
            }
        }

      if (keyword_found == (unsigned int)-1)
        return MATCHER_NO_MATCH;

      matcher->word_index++;

      if (matcher->word_index > matcher->index)
        return MATCHER_OK;

      keyword_mask |= (1 << keyword_found);

      if (args_vector)
        {
          keyword_argc = 0;
          keyword_argv = XMALLOC(MTYPE_TMP, (CMD_ARGC_MAX + 1) * sizeof(char*));
          /* We use -1 as a marker for unused fields as NULL might be a valid value */
          for (i = 0; i < CMD_ARGC_MAX + 1; i++)
            keyword_argv[i] = (void*)-1;
          vector_set_index(args_vector, keyword_found, keyword_argv);
        }
      else
        {
          keyword_argv = NULL;
        }

      keyword_vector = vector_slot(token->keyword, keyword_found);
      /* the keyword itself is at 0. We are only interested in the arguments,
       * so start counting at 1. */
      for (i = 1; i < vector_active(keyword_vector); i++)
        {
          word_token = vector_slot(keyword_vector, i);

          switch (word_token->type)
            {
            case TOKEN_TERMINAL:
              rv = cmd_matcher_match_terminal(matcher, word_token,
                                              &keyword_argc, keyword_argv);
              break;
            case TOKEN_MULTIPLE:
              rv = cmd_matcher_match_multiple(matcher, word_token,
                                              &keyword_argc, keyword_argv);
              break;
            case TOKEN_KEYWORD:
              assert(!"Keywords should never be nested.");
              break;
            }

          if (MATCHER_ERROR(rv))
            return rv;

          if (matcher->word_index > matcher->index)
            return MATCHER_OK;
        }
    }
  /* not reached */
}

static enum matcher_rv
cmd_matcher_build_keyword_args(struct cmd_matcher *matcher,
                               struct cmd_token *token,
                               int *argc, const char **argv,
                               vector keyword_args_vector)
{
  unsigned int i, j;
  const char **keyword_args = NULL;
  vector keyword_vector;
  struct cmd_token *word_token;
  const char *arg = NULL;
  enum matcher_rv rv;

  rv = MATCHER_OK;

  if (keyword_args_vector == NULL)
    return rv;

  for (i = 0; i < vector_active(token->keyword); i++)
    {
      keyword_vector = vector_slot(token->keyword, i);
      keyword_args = vector_lookup(keyword_args_vector, i);

      if (vector_active(keyword_vector) == 1)
        {
          /* this is a keyword without arguments */
          if (keyword_args)
            {
              word_token = vector_slot(keyword_vector, 0);
              arg = word_token->cmd;
            }
          else
            {
              arg = NULL;
            }

            if (push_argument(argc, argv, arg))
                 rv = MATCHER_EXCEED_ARGC_MAX;
        }
      else
      {
          /* this is a keyword with arguments */
          if (keyword_args)
            {
              /* the keyword was present, so just fill in the arguments */
              for (j = 0; keyword_args[j] != (void*)-1; j++)
                if (push_argument(argc, argv, keyword_args[j]))
                  rv = MATCHER_EXCEED_ARGC_MAX;
              XFREE(MTYPE_TMP, keyword_args);
            }
          else
            {
              /* the keyword was not present, insert NULL for the arguments
               * the keyword would have taken. */
              for (j = 1; j < vector_active(keyword_vector); j++)
                {
                  word_token = vector_slot(keyword_vector, j);
                  if ((word_token->type == TOKEN_TERMINAL
                       && (CMD_VARARG(word_token->cmd)
                           || CMD_VARIABLE(word_token->cmd)
                           || CMD_OPTION(word_token->cmd)))
                      || word_token->type == TOKEN_MULTIPLE)
                    {
                      if (push_argument(argc, argv, NULL))
                        rv = MATCHER_EXCEED_ARGC_MAX;
                    }
                }
            }
        }
    }
  vector_free(keyword_args_vector);
  return rv;
}

static enum matcher_rv
cmd_matcher_match_keyword(struct cmd_matcher *matcher,
                          struct cmd_token *token,
                          int *argc, const char **argv)
{
  vector keyword_args_vector;
  enum matcher_rv reader_rv;
  enum matcher_rv builder_rv;
  assert(token->type == TOKEN_KEYWORD);

  if (argc && argv)
    keyword_args_vector = vector_init(VECTOR_MIN_SIZE);
  else
    keyword_args_vector = NULL;

  reader_rv = cmd_matcher_read_keywords(matcher, token, keyword_args_vector);
  builder_rv = cmd_matcher_build_keyword_args(matcher, token, argc,
                                              argv, keyword_args_vector);
  /* keyword_args_vector is consumed by cmd_matcher_build_keyword_args */

  if (!MATCHER_ERROR(reader_rv) && MATCHER_ERROR(builder_rv))
    return builder_rv;

  return reader_rv;
}

static void
cmd_matcher_init(struct cmd_matcher *matcher,
                 struct cmd_element *cmd,
                 enum filter_type filter,
                 vector vline,
                 unsigned int index,
                 enum match_type *match_type,
                 vector *match)
{
  memset(matcher, 0, sizeof(*matcher));

  matcher->cmd = cmd;
  matcher->filter = filter;
  matcher->vline = vline;
  matcher->index = index;

  matcher->match_type = match_type;
  if (matcher->match_type)
    *matcher->match_type = no_match;
  matcher->match = match;

  matcher->word_index = 0;
}

static enum matcher_rv
cmd_element_match(struct cmd_element *cmd_element,
                  enum filter_type filter,
                  vector vline,
                  unsigned int index,
                  enum match_type *match_type,
                  vector *match,
                  int *argc,
                  const char **argv)
{
  struct cmd_matcher matcher;
  unsigned int token_index;
  enum matcher_rv rv = MATCHER_NO_MATCH;

  cmd_matcher_init(&matcher, cmd_element, filter,
                   vline, index, match_type, match);

  if (argc != NULL)
    *argc = 0;
  if (cmd_element->tokens) {
      for (token_index = 0;
           token_index < vector_active(cmd_element->tokens);
           token_index++)
     {
          struct cmd_token *token = vector_slot(cmd_element->tokens, token_index);

          switch (token->type)
         {
             case TOKEN_TERMINAL:
                 rv = cmd_matcher_match_terminal(&matcher, token, argc, argv);
                 break;
             case TOKEN_MULTIPLE:
                 rv = cmd_matcher_match_multiple(&matcher, token, argc, argv);
                 break;
             case TOKEN_KEYWORD:
                 rv = cmd_matcher_match_keyword(&matcher, token, argc, argv);
         }

         if (MATCHER_ERROR(rv))
             return rv;

         if (matcher.word_index > index)
             return MATCHER_OK;
      }
  }

  /* return MATCHER_COMPLETE if all words were consumed */
  if (matcher.word_index >= vector_active(vline))
    return MATCHER_COMPLETE;

  /* return MATCHER_COMPLETE also if only an empty word is left. */
  if (matcher.word_index == vector_active(vline) - 1
      && (!vector_slot(vline, matcher.word_index)
          || !strlen((char*)vector_slot(vline, matcher.word_index))))
    return MATCHER_COMPLETE;

  return MATCHER_NO_MATCH; /* command is too long to match */
}

/**
 * Filter a given vector of commands against a given commandline and
 * calculate possible completions.
 *
 * @param commands A vector of struct cmd_element*. Commands that don't
 *                 match against the given command line will be overwritten
 *                 with NULL in that vector.
 * @param filter Either FILTER_RELAXED or FILTER_STRICT. This basically
 *               determines how incomplete commands are handled, compare with
 *               cmd_word_match for details.
 * @param vline A vector of char* containing the tokenized commandline.
 * @param index Only match up to the given token of the commandline.
 * @param match_type Record the type of the best match here.
 * @param matches Record the matches here. For each cmd_element in the commands
 *                vector, a match vector will be created in the matches vector.
 *                That vector will contain all struct command_token* of the
 *                cmd_element which matched against the given vline at the given
 *                index.
 * @return A code specifying if an error occured. If all went right, it's
 *         CMD_SUCCESS.
 */
static int
cmd_vector_filter(vector commands,
                  enum filter_type filter,
                  vector vline,
                  unsigned int index,
                  enum match_type *match_type,
                  vector *matches)
{
  unsigned int i;
  struct cmd_element *cmd_element;
  enum match_type best_match;
  enum match_type element_match;
  enum matcher_rv matcher_rv;

  best_match = no_match;
  *matches = vector_init(VECTOR_MIN_SIZE);

  for (i = 0; i < vector_active (commands); i++)
    if ((cmd_element = vector_slot (commands, i)) != NULL)
      {
        if(cmd_element->attr & CMD_ATTR_HIDDEN)
          continue;
        vector_set_index(*matches, i, NULL);
        matcher_rv = cmd_element_match(cmd_element, filter,
                                       vline, index,
                                       &element_match,
                                       (vector*)&vector_slot(*matches, i),
                                       NULL, NULL);
        if (MATCHER_ERROR(matcher_rv))
          {
            vector_slot(commands, i) = NULL;
            if (matcher_rv == MATCHER_AMBIGUOUS)
              return CMD_ERR_AMBIGUOUS;
            if (matcher_rv == MATCHER_EXCEED_ARGC_MAX)
              return CMD_ERR_EXEED_ARGC_MAX;
          }
        else if (element_match > best_match)
          {
            best_match = element_match;
          }
      }
  *match_type = best_match;
  return CMD_SUCCESS;
}

/**
 * Check whether a given commandline is complete if used for a specific
 * cmd_element.
 *
 * @param cmd_element A cmd_element against which the commandline should be
 *                    checked.
 * @param vline The tokenized commandline.
 * @return 1 if the given commandline is complete, 0 otherwise.
 */
static int
cmd_is_complete(struct cmd_element *cmd_element,
                vector vline)
{
  enum matcher_rv rv;

  rv = cmd_element_match(cmd_element,
                         FILTER_RELAXED,
                         vline, -1,
                         NULL, NULL,
                         NULL, NULL);

  return rv;
}

/**
 * Parse a given commandline and construct a list of arguments for the
 * given command_element.
 *
 * @param cmd_element The cmd_element for which we want to construct arguments.
 * @param vline The tokenized commandline.
 * @param argc Where to store the argument count.
 * @param argv Where to store the argument list. Should be at least
 *             CMD_ARGC_MAX elements long.
 * @return CMD_SUCCESS if everything went alright, an error otherwise.
 */
static int
cmd_parse(struct cmd_element *cmd_element,
          vector vline,
          int *argc, const char **argv)
{
  enum matcher_rv rv = cmd_element_match(cmd_element,
                                         FILTER_RELAXED,
                                         vline, -1,
                                         NULL, NULL,
                                         argc, argv);
  switch (rv)
    {
    case MATCHER_COMPLETE:
      return CMD_SUCCESS;

    case MATCHER_NO_MATCH:
      return CMD_ERR_NO_MATCH;

    case MATCHER_AMBIGUOUS:
      return CMD_ERR_AMBIGUOUS;

    case MATCHER_EXCEED_ARGC_MAX:
      return CMD_ERR_EXEED_ARGC_MAX;

    default:
      return CMD_ERR_INCOMPLETE;
    }
}

/* Check ambiguous match */
static int
is_cmd_ambiguous (vector cmd_vector,
                  const char *command,
                  vector matches,
                  enum match_type type)
{
  unsigned int i;
  unsigned int j;
  const char *str = NULL;
  const char *matched = NULL;
  vector match_vector;
  struct cmd_token *cmd_token;

  if (command == NULL)
    command = "";

  for (i = 0; i < vector_active (matches); i++)
    if ((match_vector = vector_slot (matches, i)) != NULL)
      {
	int match = 0;

	for (j = 0; j < vector_active (match_vector); j++)
	  if ((cmd_token = vector_slot (match_vector, j)) != NULL)
	    {
	      enum match_type ret;

	      assert(cmd_token->type == TOKEN_TERMINAL);
	      if (cmd_token->type != TOKEN_TERMINAL)
		continue;

	      str = cmd_token->cmd;

	      switch (type)
		{
		case exact_match:
		  if (!(CMD_OPTION (str) || CMD_VARIABLE (str))
		      && strcmp (command, str) == 0)
		    match++;
		  break;
		case partly_match:
		  if (!(CMD_OPTION (str) || CMD_VARIABLE (str))
		      && strncmp (command, str, strlen (command)) == 0)
		    {
		      if (matched && strcmp (matched, str) != 0)
			return 1;	/* There is ambiguous match. */
		      else
			matched = str;
		      match++;
		    }
		  break;
		case range_match:
		  if (cmd_range_match (str, command))
		    {
		      if (matched && strcmp (matched, str) != 0)
			return 1;
		      else
			matched = str;
		      match++;
		    }
		  break;
                case range_match_comma:
                   cmd_range_comma_cli_parser_validate (command, str,
                                            COMMA_OPERATOR, &matched, &match);
                   break;
                case range_match_list:
                   cmd_range_comma_cli_parser_validate (command, str,
                                            RANGE_OPERATOR, &matched, &match);
                   break;
                case range_match_comma_list:
                   cmd_range_comma_cli_parser_validate (command, str,
                                            BOTH_OPERATOR, &matched, &match);
                   break;
#ifdef HAVE_IPV6
		case ipv6_match:
		  if (CMD_IPV6 (str))
		    match++;
		  break;
		case ipv6_prefix_match:
		  if ((ret = cmd_ipv6_prefix_match (command)) != no_match)
		    {
		      if (ret == partly_match)
			return 2;	/* There is incomplete match. */

		      match++;
		    }
		  break;
#endif /* HAVE_IPV6 */
		case ipv4_match:
		  if (CMD_IPV4 (str))
		    match++;
		  break;
		case ipv4_prefix_match:
		  if ((ret = cmd_ipv4_prefix_match (command)) != no_match)
		    {
		      if (ret == partly_match)
			return 2;	/* There is incomplete match. */

		      match++;
		    }
		  break;
    case ipv4_netmask_match:
      if ((ret = cmd_ipv4_netmask_match (command)) != no_match)
        {
          if (ret == partly_match)
            return 2; /* There is incomplete match. */

          match++;
        }
      break;
#ifdef ENABLE_OVSDB
                case ifname_match:
                  if (CMD_IFNAME(str))
                    match++;
                  break;
		case port_match:
		  if (CMD_PORT(str))
		    match++;
		  break;
		case vlan_match:
		  if (CMD_VLAN(str))
		    match++;
		  break;
                case mac_match:
		  if (CMD_MAC(str))
		    match++;
		  break;
#endif
		case extend_match:
		  if (CMD_OPTION (str) || CMD_VARIABLE (str))
		    match++;
		  break;
		case no_match:
		default:
		  break;
		}
	    }
	if (!match)
	  vector_slot (cmd_vector, i) = NULL;
      }
  return 0;
}

/* If src matches dst return dst string, otherwise return NULL */
static const char *
cmd_entry_function (const char *src, const char *dst)
{
  /* Skip variable arguments. */
  if (CMD_OPTION (dst) || CMD_VARIABLE (dst) || CMD_VARARG (dst) ||
      CMD_IPV4 (dst) || CMD_IPV4_PREFIX (dst) || CMD_RANGE (dst))
    return NULL;

  /* In case of 'command \t', given src is NULL string. */
  if (src == NULL)
    return dst;

  /* Matched with input string. */
  if (strncmp (src, dst, strlen (src)) == 0)
    return dst;

  return NULL;
}

/* If src matches dst return dst string, otherwise return NULL */
/* This version will return the dst string always if it is
   CMD_VARIABLE for '?' key processing */
static const char *
cmd_entry_function_desc (const char *src, const char *dst)
{
  if (CMD_VARARG (dst))
    return dst;

  if (CMD_RANGE (dst))
    {
      if ((dst[1] == 'C') && (dst[2] == ':'))
      {
          if (cmd_range_comma_cli_parser_validate (src, dst, COMMA_OPERATOR,
                                                   NULL, NULL) == 1)
              return dst;
      }
      else if((dst[1] == 'L') && (dst[2] == ':'))
      {
          if (cmd_range_comma_cli_parser_validate (src, dst, RANGE_OPERATOR,
                                                   NULL, NULL) == 1)
              return dst;
      }
      else if ((dst[1] == 'A') && (dst[2] == ':'))
      {
          if (cmd_range_comma_cli_parser_validate (src, dst, BOTH_OPERATOR,
                                                   NULL, NULL) == 1)
              return dst;
      }
      else if (cmd_range_match (dst, src))
      {
          return dst;
      }
      return NULL;
    }

#ifdef HAVE_IPV6
  if (CMD_IPV6 (dst))
    {
      if (cmd_ipv6_match (src))
	return dst;
      else
	return NULL;
    }

  if (CMD_IPV6_PREFIX (dst))
    {
      if (cmd_ipv6_prefix_match (src))
	return dst;
      else
	return NULL;
    }
#endif /* HAVE_IPV6 */

  if (CMD_IPV4 (dst))
    {
      if (cmd_ipv4_match (src))
	return dst;
      else
	return NULL;
    }

  if (CMD_IPV4_PREFIX (dst))
    {
      if (cmd_ipv4_prefix_match (src))
	return dst;
      else
	return NULL;
    }

  if (CMD_IPV4_NETMASK (dst))
    {
      if (cmd_ipv4_netmask_match (src))
        return dst;
      else
        return NULL;
    }

#ifdef ENABLE_OVSDB
  if (CMD_IFNAME(dst))
  {
    return dst;
  }

  if (CMD_PORT(dst))
    return dst;

  if (CMD_VLAN(dst))
    return dst;
  if (CMD_MAC(dst))
    return dst;
#endif

  /* Optional or variable commands always match on '?' */
  if (CMD_OPTION (dst) || CMD_VARIABLE (dst))
    return dst;

  /* In case of 'command \t', given src is NULL string. */
  if (src == NULL)
    return dst;

  if (strncmp (src, dst, strlen (src)) == 0)
    return dst;
  else
    return NULL;
}

/**
 * Check whether a string is already present in a vector of strings.
 * @param v A vector of char*.
 * @param str A char*.
 * @return 0 if str is already present in the vector, 1 otherwise.
 */
static int
cmd_unique_string (vector v, const char *str)
{
  unsigned int i;
  char *match;

  for (i = 0; i < vector_active (v); i++)
    if ((match = vector_slot (v, i)) != NULL)
      if (strcmp (match, str) == 0)
	return 0;
  return 1;
}

/**
 * Check whether a struct cmd_token matching a given string is already
 * present in a vector of struct cmd_token.
 * @param v A vector of struct cmd_token*.
 * @param str A char* which should be searched for.
 * @return 0 if there is a struct cmd_token* with its cmd matching str,
 *         1 otherwise.
 */
static int
desc_unique_string (vector v, const char *str)
{
  unsigned int i;
  struct cmd_token *token;

  for (i = 0; i < vector_active (v); i++)
    if ((token = vector_slot (v, i)) != NULL)
      if (strcmp (token->cmd, str) == 0)
	return 0;
  return 1;
}


char ErrDescStr[] =
            "Error: Help strings does not match for the identical tokens";
/**
 * Check the command tokens in the list v for identical commands with
 * different help strings. If so, change the error string to Error String
 */
static int
check_unique_helpstr(vector v, const char *cmdstr, const char *helpstr)
{
  unsigned int i;
  struct cmd_token *token;

  for (i = 0; i < vector_active (v); i++)
  {
    if ((token = vector_slot (v, i)) != NULL)
    {
      if (strcmp (token->cmd, cmdstr) != 0)
      {
        continue;
      }
      if (strcmp (token->desc, helpstr) != 0)
      {
        token->desc = ErrDescStr;
        return 1;
      }
    }
  }
  return 0;
}


static int
cmd_try_do_shortcut (enum node_type node, char* first_word) {
  if ( first_word != NULL &&
       node != AUTH_NODE &&
       node != VIEW_NODE &&
       node != AUTH_ENABLE_NODE &&
       node != ENABLE_NODE &&
       node != RESTRICTED_NODE &&
       0 == strcmp( "do", first_word ) )
    return 1;
  return 0;
}

static void
cmd_matches_free(vector *matches)
{
  unsigned int i;
  vector cmd_matches;

  for (i = 0; i < vector_active(*matches); i++)
    if ((cmd_matches = vector_slot(*matches, i)) != NULL)
      vector_free(cmd_matches);
  vector_free(*matches);
  *matches = NULL;
}

static int
cmd_describe_cmp(const void *a, const void *b)
{
  const struct cmd_token *first = *(struct cmd_token * const *)a;
  const struct cmd_token *second = *(struct cmd_token * const *)b;

  return strcmp(first->cmd, second->cmd);
}

static void
cmd_describe_sort(vector matchvec)
{
  qsort(matchvec->index, vector_active(matchvec),
        sizeof(void*), cmd_describe_cmp);
}

/* '?' describe command support. */
static vector
cmd_describe_command_real (vector vline, struct vty *vty, int *status)
{
  unsigned int i;
  vector cmd_vector;
#define INIT_MATCHVEC_SIZE 10
  vector matchvec;
  struct cmd_element *cmd_element;
  unsigned int index;
  int ret;
  enum match_type match;
  char *command;
  vector matches = NULL;
  vector match_vector;

  /* Set index. */
  if (vector_active (vline) == 0)
    {
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }

  index = vector_active (vline) - 1;

  /* Make copy vector of current node's command vector. */
  cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));

  /* Prepare match vector */
  matchvec = vector_init (INIT_MATCHVEC_SIZE);

  /* Filter commands and build a list how they could possibly continue. */
  for (i = 0; i <= index; i++)
    {
      command = vector_slot (vline, i);

      if (matches)
	cmd_matches_free(&matches);

      ret = cmd_vector_filter(cmd_vector,
	                      FILTER_RELAXED,
	                      vline, i,
	                      &match,
	                      &matches);

      if (ret != CMD_SUCCESS)
	{
	  vector_free (cmd_vector);
	  vector_free (matchvec);
	  cmd_matches_free(&matches);
	  *status = ret;
	  return NULL;
	}

      /* The last match may well be ambigious, so break here */
      if (i == index)
	break;

      if (match == vararg_match)
	{
	  /* We found a vararg match - so we can throw out the current matches here
	   * and don't need to continue checking the command input */
	  unsigned int j, k;

	  for (j = 0; j < vector_active (matches); j++)
	    if ((match_vector = vector_slot (matches, j)) != NULL)
	      for (k = 0; k < vector_active (match_vector); k++)
	        {
	          struct cmd_token *token = vector_slot (match_vector, k);
	          vector_set (matchvec, token);
	        }

	  *status = CMD_SUCCESS;
	  vector_set(matchvec, &token_cr);
	  vector_free (cmd_vector);
	  cmd_matches_free(&matches);
	  cmd_describe_sort(matchvec);
	  return matchvec;
	}

      ret = is_cmd_ambiguous(cmd_vector, command, matches, match);
      if (ret == 1)
	{
	  vector_free (cmd_vector);
	  vector_free (matchvec);
	  cmd_matches_free(&matches);
	  *status = CMD_ERR_AMBIGUOUS;
	  return NULL;
	}
      else if (ret == 2)
	{
	  vector_free (cmd_vector);
	  vector_free (matchvec);
	  cmd_matches_free(&matches);
	  *status = CMD_ERR_NO_MATCH;
	  return NULL;
	}
    }

  /* Make description vector. */
  for (i = 0; i < vector_active (matches); i++)
    if ((cmd_element = vector_slot (cmd_vector, i)) != NULL)
      {
        unsigned int j;
        const char *last_word;
        vector vline_trimmed;

        last_word = vector_slot(vline, vector_active(vline) - 1);
        if (last_word == NULL || !strlen(last_word))
          {
            vline_trimmed = vector_copy(vline);
            vector_unset(vline_trimmed, vector_active(vline_trimmed) - 1);

            ret = cmd_is_complete(cmd_element, vline_trimmed);
            if ((MATCHER_COMPLETE == ret)
                && desc_unique_string(matchvec, command_cr))
              {
                if (match != vararg_match)
                  vector_set(matchvec, &token_cr);
              }

            vector_free(vline_trimmed);
          }

        match_vector = vector_slot (matches, i);
        if (match_vector)
          for (j = 0; j < vector_active(match_vector); j++)
            {
              struct cmd_token *token = vector_slot(match_vector, j);
              const char *string = NULL;
              static char dyn_helpstr[MAX_DYN_HELPSTR_LEN];
              int len = 0;

              string = cmd_entry_function_desc(command, token->cmd);

              if (token->dyn_cb != NULL)
              {
                memset(dyn_helpstr, '\0', MAX_DYN_HELPSTR_LEN);
                token->dyn_cb_func(token, vty, dyn_helpstr, MAX_DYN_HELPSTR_LEN);
                len = strlen(dyn_helpstr);
                if (len > 0)
                {
                  token->desc = XREALLOC(MTYPE_CMD_TOKENS, token->desc, len + 1);
                  memcpy (token->desc, dyn_helpstr, len);
                  *(token->desc + len) = '\0';
                }
              }

              if (string && desc_unique_string(matchvec, string))
                vector_set(matchvec, token);
#ifndef NDEBUG
              else
                /* Check if there is a conflict among the helpstrings */
                check_unique_helpstr(matchvec, token->cmd, token->desc);
#endif
            }
      }
  vector_free (cmd_vector);
  cmd_matches_free(&matches);

  if (vector_slot (matchvec, 0) == NULL)
    {
      vector_free (matchvec);
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }

  *status = CMD_SUCCESS;
  cmd_describe_sort(matchvec);
  return matchvec;
}

vector
cmd_describe_command (vector vline, struct vty *vty, int *status)
{
  vector ret;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      enum node_type onode;
      vector shifted_vline;
      unsigned int index;

      onode = vty->node;
      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_describe_command_real (shifted_vline, vty, status);

      vector_free(shifted_vline);
      vty->node = onode;
      return ret;
  }


  return cmd_describe_command_real (vline, vty, status);
}


/* Check LCD of matched command. */
static int
cmd_lcd (char **matched)
{
  int i;
  int j;
  int lcd = -1;
  char *s1, *s2;
  char c1, c2;

  if (matched[0] == NULL || matched[1] == NULL)
    return 0;

  for (i = 1; matched[i] != NULL; i++)
    {
      s1 = matched[i - 1];
      s2 = matched[i];

      for (j = 0; (c1 = s1[j]) && (c2 = s2[j]); j++)
	if (c1 != c2)
	  break;

      if (lcd < 0)
	lcd = j;
      else
	{
	  if (lcd > j)
	    lcd = j;
	}
    }
  return lcd;
}

static int
cmd_complete_cmp(const void *a, const void *b)
{
  const char *first = *(char * const *)a;
  const char *second = *(char * const *)b;

  if (!first)
    {
      if (!second)
        return 0;
      return 1;
    }
  if (!second)
    return -1;

  return strcmp(first, second);
}

static void
cmd_complete_sort(vector matchvec)
{
  qsort(matchvec->index, vector_active(matchvec),
        sizeof(void*), cmd_complete_cmp);
}

/* Command line completion support. */
static char **
cmd_complete_command_real (vector vline, struct vty *vty, int *status)
{
  unsigned int i;
  vector cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));
#define INIT_MATCHVEC_SIZE 10
  vector matchvec;
  unsigned int index;
  char **match_str;
  struct cmd_token *token;
  char *command;
  int lcd;
  vector matches = NULL;
  vector match_vector;

  if (vector_active (vline) == 0)
    {
      vector_free (cmd_vector);
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }
  else
    index = vector_active (vline) - 1;

  /* First, filter by command string */
  for (i = 0; i <= index; i++)
    {
      command = vector_slot (vline, i);
      enum match_type match;
      int ret;

      if (matches)
        cmd_matches_free(&matches);

      /* First try completion match, if there is exactly match return 1 */
      ret = cmd_vector_filter(cmd_vector,
	                      FILTER_RELAXED,
	                      vline, i,
	                      &match,
	                      &matches);

      if (ret != CMD_SUCCESS)
	{
	  vector_free(cmd_vector);
	  cmd_matches_free(&matches);
	  *status = ret;
	  return NULL;
	}

      /* Break here - the completion mustn't be checked to be non-ambiguous */
      if (i == index)
	break;

      /* If there is exact match then filter ambiguous match else check
	 ambiguousness. */
      ret = is_cmd_ambiguous (cmd_vector, command, matches, match);
      if (ret == 1)
	{
	  vector_free (cmd_vector);
	  cmd_matches_free(&matches);
	  *status = CMD_ERR_AMBIGUOUS;
	  return NULL;
	}
      /*
	   else if (ret == 2)
	   {
	   vector_free (cmd_vector);
	   cmd_matches_free(&matches);
	   *status = CMD_ERR_NO_MATCH;
	   return NULL;
	   }
	 */
    }

  /* Prepare match vector. */
  matchvec = vector_init (INIT_MATCHVEC_SIZE);

  /* Build the possible list of continuations into a list of completions */
  for (i = 0; i < vector_active (matches); i++)
    if ((match_vector = vector_slot (matches, i)))
      {
	const char *string;
	unsigned int j;

	for (j = 0; j < vector_active (match_vector); j++)
	  if ((token = vector_slot (match_vector, j)))
		{
		  if ((string =
		       cmd_entry_function (vector_slot (vline, index),
					   token->cmd)))
		    if (cmd_unique_string (matchvec, string))
		      vector_set (matchvec, XSTRDUP (MTYPE_TMP, string));
		}
      }

  /* We don't need cmd_vector any more. */
  vector_free (cmd_vector);
  cmd_matches_free(&matches);

  /* No matched command */
  if (vector_slot (matchvec, 0) == NULL)
    {
      vector_free (matchvec);

      /* In case of 'command \t' pattern.  Do you need '?' command at
         the end of the line. */
      if (vector_slot (vline, index) == '\0')
	*status = CMD_ERR_NOTHING_TODO;
      else
	*status = CMD_ERR_NO_MATCH;
      return NULL;
    }

  /* Only one matched */
  if (vector_slot (matchvec, 1) == NULL)
    {
      match_str = (char **) matchvec->index;
      vector_only_wrapper_free (matchvec);
      *status = CMD_COMPLETE_FULL_MATCH;
      return match_str;
    }
  /* Make it sure last element is NULL. */
  vector_set (matchvec, NULL);

  /* Check LCD of matched strings. */
  if (vector_slot (vline, index) != NULL)
    {
      lcd = cmd_lcd ((char **) matchvec->index);

      if (lcd)
	{
	  int len = strlen (vector_slot (vline, index));

	  if (len < lcd)
	    {
	      char *lcdstr;

	      lcdstr = XMALLOC (MTYPE_TMP, lcd + 1);
	      memcpy (lcdstr, matchvec->index[0], lcd);
	      lcdstr[lcd] = '\0';

	      /* match_str = (char **) &lcdstr; */

	      /* Free matchvec. */
	      for (i = 0; i < vector_active (matchvec); i++)
		{
		  if (vector_slot (matchvec, i))
		    XFREE (MTYPE_TMP, vector_slot (matchvec, i));
		}
	      vector_free (matchvec);

	      /* Make new matchvec. */
	      matchvec = vector_init (INIT_MATCHVEC_SIZE);
	      vector_set (matchvec, lcdstr);
	      match_str = (char **) matchvec->index;
	      vector_only_wrapper_free (matchvec);

	      *status = CMD_COMPLETE_MATCH;
	      return match_str;
	    }
	}
    }

  match_str = (char **) matchvec->index;
  cmd_complete_sort(matchvec);
  vector_only_wrapper_free (matchvec);
  *status = CMD_COMPLETE_LIST_MATCH;
  return match_str;
}

char **
cmd_complete_command (vector vline, struct vty *vty, int *status)
{
  char **ret;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      enum node_type onode;
      vector shifted_vline;
      unsigned int index;

      onode = vty->node;
      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_complete_command_real (shifted_vline, vty, status);

      vector_free(shifted_vline);
      vty->node = onode;
      return ret;
  }


  return cmd_complete_command_real (vline, vty, status);
}

/* return parent node */
/* MUST eventually converge on CONFIG_NODE */
enum node_type
node_parent ( enum node_type node )
{
  enum node_type ret;

  assert (node > CONFIG_NODE);

  switch (node)
    {
    case BGP_VPNV4_NODE:
    case BGP_IPV4_NODE:
    case BGP_IPV4M_NODE:
    case BGP_IPV6_NODE:
    case BGP_IPV6M_NODE:
      ret = BGP_NODE;
      break;
    case KEYCHAIN_KEY_NODE:
      ret = KEYCHAIN_NODE;
      break;
    default:
      ret = CONFIG_NODE;
    }

  return ret;
}

/*
 * Purpose : Autocomplete the command tokens in matching case.
 * Return  : return cmd_string contains the complete command tokens.
 * Example : User endered command - "ip add 1.2.3.1/24"
 *           This function returns  "ip address 1.2.3.1/24"
*/
static char *
autocomplete_command_tokens(struct cmd_element *matched_element, vector vline,
                            char *cmd_string)
{
  char *cmd_token = NULL;
  const char *usr_token = NULL;
  struct cmd_matcher matcher;
  unsigned int token_index=0;
  int flag;
  unsigned int i;
  enum match_type word_match;
  struct cmd_token *word_token;
  const char *word;
  vector keyword_vector;
  char *line_token = ".LINE";

  cmd_matcher_init(&matcher, matched_element, 0,
                   vline, -1, NULL, NULL);

  for (token_index= 0; token_index < vector_active(matched_element->tokens);
       token_index++){
    flag = 1;
    struct cmd_token *token = vector_slot(matched_element->tokens, token_index);

    switch (token->type)
       {
             case TOKEN_TERMINAL:
                 if (!cmd_matcher_words_left(&matcher))
                      break;
                 word = cmd_matcher_get_word(&matcher);
                 cmd_token = token->cmd;
                 goto JUMP;
             case TOKEN_MULTIPLE:
                 if (!cmd_matcher_words_left(&matcher))
                       break;
                 word = cmd_matcher_get_word(&matcher);
                 for (i = 0; i < vector_active(token->multiple); i++)
                 {
                     word_token = vector_slot(token->multiple, i);

                     word_match = cmd_word_match(word_token, 0, word);
                     if (word_match == no_match)
                           continue;

                     if (word_match > no_match)
                        cmd_token = word_token->cmd;
                 }
                 goto JUMP;
             case TOKEN_KEYWORD:
                  while (1){
                    flag = 1;
                    if (!cmd_matcher_words_left(&matcher))
                      break;

                    word = cmd_matcher_get_word(&matcher);
                    for (i = 0; i < vector_active(token->keyword); i++)
                    {
                      keyword_vector = vector_slot(token->keyword, i);
                      word_token = vector_slot(keyword_vector, 0);

                      word_match = cmd_word_match(word_token, 0, word);
                      if (word_match == no_match)
                        continue;

                      if (word_match > no_match)
                         cmd_token = word_token->cmd;
                    }
                    JUMP:
                      if ((cmd_token != NULL) && !(strcmp(cmd_token, "alias")))
                         break;

                      char *matched_string;
                      matched_string = cmd_token;
                      usr_token = word;
                      while (*usr_token != '\0'){
                        if (*usr_token != *cmd_token){
                          flag = 0;
                          break;
                        }
                        usr_token++;
                        cmd_token++;
                      }

                     if (flag == 1)
                       strcat(strcat(cmd_string, matched_string), " ");
                     else
                       strcat(strcat(cmd_string, word), " ");

                     matcher.word_index++;
                     if (token->type != TOKEN_KEYWORD)
                       break;
                   }
       }
       if ((cmd_token != NULL) && !(strcmp(cmd_token, "alias")))
         break;
  }
  if (!(strcmp(cmd_token, line_token)))
  {
    for (; matcher.word_index < vector_active(vline); matcher.word_index++)
      strcat(strcat(cmd_string, vline->index[matcher.word_index]), " ");
  }
  return cmd_string;
}

int check_cmd_authorization(char *tac_command)
{

  int count = 0;
  const struct ovsrec_tacacs_server *server_row = NULL;
  const struct ovsrec_aaa_server_group *group_row = NULL;
  const struct ovsrec_system *ovs = ovsrec_system_first(idl);
  const struct ovsrec_aaa_server_group_prio *group_prio_list = NULL;
  struct shash sorted_tacacs_servers;
  const struct shash_node **nodes = NULL;
  int count_servers = 0;
  int idx = 0;
  int iter = 0;
  char *passkey = NULL;
  char *timeout_str = NULL;
  int timeout = 0;
  char *tty = "mytty";
  char *remote_addr = "";
  char *service = "shell";
  char *protocol = "ip";
  int tac_author_status = EXIT_OK;
  bool by_default_priority = false;
  struct passwd *pw;
  /* get the logged in user.*/
  pw = getpwuid( getuid());

  group_prio_list = ovsrec_aaa_server_group_prio_first(idl);

  if (group_prio_list == NULL)
  {
    VLOG_DBG("Could not fetch the priority list for tacacs command authorization");
    return EXIT_FAIL;
  }

  /* count the number of groups configured for authorization */
  count = group_prio_list->n_authorization_group_prios;

  /* check if the number of groups configured is more than one(default) and first group is not "none" */
  if (count >= 1 && (strcmp(group_prio_list->value_authorization_group_prios[0]->group_name, TAC_NONE_GROUP) != 0))
  {
    for (iter = 0; iter < count; iter ++)
    {
      /* get the group information */
      group_row = group_prio_list->value_authorization_group_prios[iter];

      if (strcmp(group_row->group_name, TAC_NONE_GROUP) != 0)
      {
        /* initialize the list*/
        shash_init(&sorted_tacacs_servers);
        /* Add servers to list if the it is added to group configured for authorization */
        OVSREC_TACACS_SERVER_FOR_EACH(server_row, idl)
        {
          if (server_row->group == group_row)
          {
            shash_add(&sorted_tacacs_servers, server_row->ip_address, (void *)server_row);
          }
          else if (strcmp(group_row->group_name, TAC_DEFAULT_GROUP) == 0)
          {
            shash_add(&sorted_tacacs_servers, server_row->ip_address, (void *)server_row);
          }
        }

        /* if the groups is default sort by default priority else by group priority */
        if (strcmp(group_row->group_name, TAC_DEFAULT_GROUP) == 0)
        {
          by_default_priority = true;
        }

        /* get the list sorted , true = default and  false = group priority*/
        nodes = sort_tacacs_server(&sorted_tacacs_servers, by_default_priority);

        if (nodes == NULL)
        {
           shash_destroy(&sorted_tacacs_servers);
           VLOG_DBG("Failed to get the sorted tacacs server list");
           return EXIT_FAIL;
        }

        /* get the number of servers in the list */
        count_servers = shash_count(&sorted_tacacs_servers);
        /* send the cmd for authorization to all the servers in the list*/
        for(idx = 0; idx < count_servers; idx++)
        {
          server_row = (const struct ovsrec_tacacs_server *)nodes[idx]->data;
          /* check if timeout is set by user or get global value*/
          if (server_row->timeout)
          {
            timeout = *(server_row->timeout);
          }
	  else
          {
            timeout_str = strdup(smap_get(&ovs->aaa, SYSTEM_AAA_TACACS_TIMEOUT));
            timeout = atoi(timeout_str);
          }
          /* check if passkey is set by user or get global value*/
          if (server_row->passkey)
          {
            strncpy(passkey, server_row->passkey, strlen(server_row->passkey));
            passkey[strlen(server_row->passkey)+1] = '\0';
          }
          else
          {
            passkey =  strdup(smap_get(&ovs->aaa, SYSTEM_AAA_TACACS_PASSKEY));
          }

          /*send the command for authorization*/
          tac_author_status = tac_cmd_author_ptr(server_row->ip_address, passkey, pw->pw_name,
                                                 tty, remote_addr, service, protocol, tac_command,
                                                 timeout, true, NULL, NULL, NULL);
          /* if authorized, return */
          if (tac_author_status == EXIT_OK)
          {
            VLOG_DBG(" %s command authorized for %s\n", tac_command, pw->pw_name);
            free(nodes);
            shash_destroy(&sorted_tacacs_servers);
            return tac_author_status;
          }
	  /*if not authorized, return */
	  else if (tac_author_status == EXIT_FAIL)
          {
            free(nodes);
            shash_destroy(&sorted_tacacs_servers);
            return tac_author_status;
          }
        }
        /*
         * Did not get a valid reply from the servers in this group.
         * destroy the list and free the node,
         * so that it can be initialized for different group.
         */
        free(nodes);
        shash_destroy(&sorted_tacacs_servers);
      }
      else
      {
        return EXIT_OK;
      }
    }
    /*
     * if we are at this stage that means user is not authorized to execute this command
     * in any of the servers.We need to check if the last option for authorization is
     * not none. if not, we should not let the user execute this command.
     */
    if (strcmp(group_prio_list->value_authorization_group_prios[count]->group_name, TAC_NONE_GROUP) != 0)
    {
      tac_author_status = EXIT_FAIL;
      VLOG_DBG("Did not recieve valid authorization response from any of the configured servers");
    }
  }
  return tac_author_status;
}

/* Execute command by argument vline vector. */
static int
cmd_execute_command_real (vector vline,
			  enum filter_type filter,
			  struct vty *vty,
			  struct cmd_element **cmd)
{
  unsigned int i;
  unsigned int index;
  vector cmd_vector;
  struct cmd_element *cmd_element;
  struct cmd_element *matched_element;
  unsigned int matched_count, incomplete_count;
  int argc;
  const char *argv[CMD_ARGC_MAX];
  enum match_type match = 0;
  char *command;
  int ret;
  vector matches;
  char op[MAX_OP_DESC_LEN];
  char *cfgdata;
  char cmd_string[MAX_CFGDATA_LEN] = "";
  char *tac_command = NULL;
  int tac_author_return = 0;
  bool ready = false;

  /* Make copy of command elements. */
  cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));

  for (index = 0; index < vector_active (vline); index++)
    {
      command = vector_slot (vline, index);
      ret = cmd_vector_filter(cmd_vector,
			      filter,
			      vline, index,
			      &match,
			      &matches);

      if (ret != CMD_SUCCESS)
	{
	  cmd_matches_free(&matches);
	  return ret;
	}

      if (match == vararg_match)
	{
	  cmd_matches_free(&matches);
	  break;
	}

      ret = is_cmd_ambiguous (cmd_vector, command, matches, match);
      cmd_matches_free(&matches);

      if (ret == 1)
	{
	  vector_free(cmd_vector);
	  return CMD_ERR_AMBIGUOUS;
	}
      else if (ret == 2)
	{
	  vector_free(cmd_vector);
	  return CMD_ERR_NO_MATCH;
	}
    }

  /* Check matched count. */
  matched_element = NULL;
  matched_count = 0;
  incomplete_count = 0;

  for (i = 0; i < vector_active (cmd_vector); i++)
    if ((cmd_element = vector_slot (cmd_vector, i)))
    {
        if(cmd_element->attr & CMD_ATTR_NOT_ENABLED)
        {
          continue;
        }
        ret = cmd_is_complete(cmd_element, vline);
        if (MATCHER_COMPLETE == ret)
        {
          matched_element = cmd_element;
          matched_count++;
        }
        else if (MATCHER_INCOMPLETE == ret)
        {
          incomplete_count++;
        }
    }

  /* Finish of using cmd_vector. */
  vector_free (cmd_vector);

  /* To execute command, matched_count must be 1. */
  if (matched_count == 0)
    {
      if (incomplete_count)
	return CMD_ERR_INCOMPLETE;
      else
	return CMD_ERR_NO_MATCH;
    }

  if (matched_count > 1)
    return CMD_ERR_AMBIGUOUS;

  ret = cmd_parse(matched_element, vline, &argc, argv);
  if (ret != CMD_SUCCESS)
    return ret;

  /* Complete the command tokens when command match is OK.*/
  if (vty->node >= CONFIG_NODE && ret == CMD_SUCCESS)
    cfgdata = autocomplete_command_tokens(matched_element, vline, cmd_string);

  /* For vtysh execution. */
  if (cmd)
    *cmd = matched_element;

  /* copy the command to be sent for cmd authorization */
  tac_command = (char*)matched_element->string;
  /* if the command is exit or end, allow it*/
  if ((strcmp(tac_command, EXIT_CMD) != 0) &&
      (strcmp(tac_command, END_CMD) != 0))
  {
    /* check if the system and db is ready */
    VTYSH_OVSDB_LOCK;
    ready = vtysh_chk_for_system_configured_db_is_ready();
    VTYSH_OVSDB_UNLOCK;
    if (ready == true)
    {
      /* send command for authorization */
      tac_author_return = check_cmd_authorization(tac_command);
      if (tac_author_return == EXIT_FAIL)
      {
        vty_out(vty, "Cannot execute command. Command not allowed.\n");
        return CMD_ERR_NOTHING_TODO;
      }
    }
  }

  if (matched_element->daemon)
    return CMD_SUCCESS_DAEMON;
  vty->buf = (char*)matched_element->string;
  vty->length = strlen(matched_element->string);
  /* Execute matched command. */
  if(((matched_element->attr) & CMD_ATTR_NOLOCK) == 0)
  {
      VLOG_DBG("Setting the latch");
      latch_set(&ovsdb_latch);
      struct range_list *temp = vty->index_list;
      static char ifnumber[MAX_IFNAME_LENGTH + 1];
      if (temp != NULL)
      {
          while (temp != NULL)
          {
              VTYSH_OVSDB_LOCK;
              strcpy(ifnumber, temp->value);
              vty->index = ifnumber;
              if ((((matched_element->attr) & CMD_ATTR_NON_IDL_CMD) == CMD_ATTR_NON_IDL_CMD)
                     || (vtysh_chk_for_system_configured_db_is_ready() == true)) {
                  ret = (*matched_element->func) (matched_element, vty, 0, argc, argv);
              } else {
                  vty_out(vty, "System is not ready. Please retry after few seconds..%s", VTY_NEWLINE);
              }
              AUDIT_LOG_USER_MSG(vty, cfgdata, ret);
              temp = temp->link;
              VTYSH_OVSDB_UNLOCK;
              if (vty->index == NULL)
                  break;
          }
      }
      else
      {
          VTYSH_OVSDB_LOCK;
          if ((((matched_element->attr) & CMD_ATTR_NON_IDL_CMD) == CMD_ATTR_NON_IDL_CMD)
                  || (vtysh_chk_for_system_configured_db_is_ready() == true)) {
              ret = (*matched_element->func) (matched_element, vty, 0, argc, argv);
          } else {
              vty_out(vty, "System is not ready. Please retry after few seconds..%s", VTY_NEWLINE);
          }
          AUDIT_LOG_USER_MSG(vty, cfgdata, ret);
          VTYSH_OVSDB_UNLOCK;
      }
  }
  else
  {
      struct range_list *temp = vty->index_list;
      static char ifnumber[MAX_IFNAME_LENGTH];
      if (temp != NULL)
      {
          while (temp != NULL)
          {
              strcpy(ifnumber, temp->value);
              vty->index = ifnumber;
              if (((matched_element->attr) & CMD_ATTR_NON_IDL_CMD) == CMD_ATTR_NON_IDL_CMD) {
                  ready = true;
              } else {
                  VTYSH_OVSDB_LOCK;
                  ready = vtysh_chk_for_system_configured_db_is_ready();
                  VTYSH_OVSDB_UNLOCK;
             }

              if (ready  == true) {
                  ret = (*matched_element->func) (matched_element, vty, 0, argc, argv);
              } else {
                  vty_out(vty, "System is not ready. Please retry after few seconds..%s", VTY_NEWLINE);
              }
              AUDIT_LOG_USER_MSG(vty, cfgdata, ret);
              temp = temp->link;
              if (vty->index == NULL)
                  break;
          }
      }
      else
      {
          if (((matched_element->attr) & CMD_ATTR_NON_IDL_CMD) == CMD_ATTR_NON_IDL_CMD) {
              ready = true;
          } else {
              VTYSH_OVSDB_LOCK;
              ready = vtysh_chk_for_system_configured_db_is_ready();
              VTYSH_OVSDB_UNLOCK;
          }
          if (ready == true) {
              ret = (*matched_element->func) (matched_element, vty, 0, argc, argv);
          } else {
              vty_out(vty, "System is not ready. Please retry after few seconds..%s", VTY_NEWLINE);
          }
          AUDIT_LOG_USER_MSG(vty, cfgdata, ret);
      }
  }
  return ret;
}

/**
 * Execute a given command, handling things like "do ..." and checking
 * whether the given command might apply at a parent node if doesn't
 * apply for the current node.
 *
 * @param vline Command line input, vector of char* where each element is
 *              one input token.
 * @param vty The vty context in which the command should be executed.
 * @param cmd Pointer where the struct cmd_element of the matched command
 *            will be stored, if any. May be set to NULL if this info is
 *            not needed.
 * @param vtysh If set != 0, don't lookup the command at parent nodes.
 * @return The status of the command that has been executed or an error code
 *         as to why no command could be executed.
 */
int
cmd_execute_command (vector vline, struct vty *vty, struct cmd_element **cmd,
		     int vtysh) {
  int ret, saved_ret, tried = 0;
  enum node_type onode, try_node;

  onode = try_node = vty->node;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      struct range_list* temp = vty->index_list;
      vty->index_list = NULL;
      vector shifted_vline;
      unsigned int index;

      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_execute_command_real (shifted_vline, FILTER_RELAXED, vty, cmd);

      vector_free(shifted_vline);
      vty->node = onode;
      vty->index_list = temp;
      return ret;
  }


  saved_ret = ret = cmd_execute_command_real (vline, FILTER_RELAXED, vty, cmd);

  if (vtysh)
    return saved_ret;

  /* This assumes all nodes above CONFIG_NODE are childs of CONFIG_NODE */
  while ( ret != CMD_SUCCESS && ret != CMD_WARNING
	  && vty->node > CONFIG_NODE )
    {
      try_node = node_parent(try_node);
      vty->node = try_node;
      ret = cmd_execute_command_real (vline, FILTER_RELAXED, vty, cmd);
      tried = 1;
      if (ret == CMD_SUCCESS || ret == CMD_WARNING)
	{
	  /* succesfull command, leave the node as is */
	  return ret;
	}
    }
  /* no command succeeded, reset the vty to the original node and
     return the error for this node */
  if ( tried )
    vty->node = onode;
  return saved_ret;
}

/**
 * Execute a given command, matching it strictly against the current node.
 * This mode is used when reading config files.
 *
 * @param vline Command line input, vector of char* where each element is
 *              one input token.
 * @param vty The vty context in which the command should be executed.
 * @param cmd Pointer where the struct cmd_element* of the matched command
 *            will be stored, if any. May be set to NULL if this info is
 *            not needed.
 * @return The status of the command that has been executed or an error code
 *         as to why no command could be executed.
 */
int
cmd_execute_command_strict (vector vline, struct vty *vty,
			    struct cmd_element **cmd)
{
  return cmd_execute_command_real(vline, FILTER_STRICT, vty, cmd);
}

/* Configration make from file. */
int
config_from_file (struct vty *vty, FILE *fp, unsigned int *line_num)
{
  int ret;
  *line_num = 0;
  vector vline;

  while (fgets (vty->buf, VTY_BUFSIZ, fp))
    {
      ++(*line_num);
      vline = cmd_make_strvec (vty->buf);

      /* In case of comment line */
      if (vline == NULL)
	continue;
      /* Execute configuration command : this is strict match */
      ret = cmd_execute_command_strict (vline, vty, NULL);

      /* Try again with setting node to CONFIG_NODE */
      while (ret != CMD_SUCCESS && ret != CMD_WARNING
	     && ret != CMD_ERR_NOTHING_TODO && vty->node != CONFIG_NODE)
	{
	  vty->node = node_parent(vty->node);
	  ret = cmd_execute_command_strict (vline, vty, NULL);
	}

      cmd_free_strvec (vline);

      if (ret != CMD_SUCCESS && ret != CMD_WARNING
	  && ret != CMD_ERR_NOTHING_TODO)
	return ret;
    }
  return CMD_SUCCESS;
}


int cmd_try_execute_command (struct vty *vty, char *buf)
{
  int ret;
  vector vline;
  unsigned int i;
  unsigned int index;
  vector cmd_vector;
  struct cmd_element *cmd_element;
  unsigned int matched_count, incomplete_count;
  enum match_type match = 0;
  char *command;
  vector matches;

  /* Split readline string up into the vector */
  vline = cmd_make_strvec (buf);

  if (vline == NULL)
    return CMD_SUCCESS;

  /* Make copy of command elements. */
  cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));

  for (index = 0; index < vector_active (vline); index++)
  {
    command = vector_slot (vline, index);
    ret = cmd_vector_filter(cmd_vector,
        FILTER_RELAXED,
        vline, index,
        &match,
        &matches);

    if (ret != CMD_SUCCESS)
    {
      cmd_matches_free(&matches);
      vector_free(cmd_vector);
      return ret;
    }

    if (match == vararg_match)
    {
      cmd_matches_free(&matches);
      break;
    }

    ret = is_cmd_ambiguous (cmd_vector, command, matches, match);
    cmd_matches_free(&matches);

    if (ret == 1)
    {
      vector_free(cmd_vector);
      return CMD_ERR_AMBIGUOUS;
    }
    else if (ret == 2)
    {
      vector_free(cmd_vector);
      return CMD_ERR_NO_MATCH;
    }
  }

  /* Check matched count. */
  matched_count = 0;
  incomplete_count = 0;

  for (i = 0; i < vector_active (cmd_vector); i++)
     if ((cmd_element = vector_slot (cmd_vector, i)))
     {
        if(cmd_element->attr & CMD_ATTR_NOT_ENABLED)
        {
           continue;
        }
        ret = cmd_is_complete(cmd_element, vline);
        if (MATCHER_COMPLETE == ret)
        {
           matched_count++;
        }
        else if (MATCHER_INCOMPLETE == ret)
        {
           incomplete_count++;
        }
     }

     /* Finish of using cmd_vector. */
  vector_free (cmd_vector);

  if (matched_count == 0)
  {
    if (incomplete_count)
      return CMD_ERR_INCOMPLETE;
    else
      return CMD_ERR_NO_MATCH;
  }

  if (matched_count > 1)
    return CMD_ERR_AMBIGUOUS;

  return CMD_COMPLETE_MATCH;

}


/* Configration from terminal */
DEFUN (config_terminal,
       config_terminal_cmd,
       "configure terminal",
       "Configuration from vty interface\n"
       "Configuration terminal\n")
{
  if (vty_config_lock (vty))
    vty->node = CONFIG_NODE;
  else
    {
      vty_out (vty, "VTY configuration is locked by other VTY%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  return CMD_SUCCESS;
}

/* Enable command */
DEFUN (enable,
       config_enable_cmd,
       "enable",
       "Turn on privileged mode command\n")
{
  /* If enable password is NULL, change to ENABLE_NODE */
  if ((host.enable == NULL && host.enable_encrypt == NULL) ||
      vty->type == VTY_SHELL_SERV)
    vty->node = ENABLE_NODE;
  else
    vty->node = AUTH_ENABLE_NODE;

  return CMD_SUCCESS;
}

/* Disable command */
DEFUN (disable,
       config_disable_cmd,
       "disable",
       "Turn off privileged mode command\n")
{
  if (vty->node == ENABLE_NODE)
    vty->node = VIEW_NODE;
  return CMD_SUCCESS;
}

/* Down vty node level. */
DEFUN_NON_IDL (config_exit,
               config_exit_cmd,
               "exit",
               "Exit current mode and down to previous mode\n")
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
    case RESTRICTED_NODE:
      if (vty_shell (vty))
	exit (0);
      else
	vty->status = VTY_CLOSE;
      break;
    case CONFIG_NODE:
      vty->index_list = cmd_free_memory_range_list (vty->index_list);
      vty->node = ENABLE_NODE;
      vty_config_unlock (vty);
      break;
    case AAA_SERVER_GROUP_NODE:
    case INTERFACE_NODE:
    case VLAN_NODE:
    case MGMT_INTERFACE_NODE:
#ifdef ENABLE_OVSDB
    case VLAN_INTERFACE_NODE:
    case DHCP_SERVER_NODE:
    case TFTP_SERVER_NODE:
    case ACCESS_LIST_NODE:
    case MIRROR_NODE:
#endif
    case LINK_AGGREGATION_NODE:
    case QOS_QUEUE_PROFILE_NODE:
    case QOS_SCHEDULE_PROFILE_NODE:
    case ZEBRA_NODE:
    case BGP_NODE:
    case RIP_NODE:
    case RIPNG_NODE:
    case BABEL_NODE:
    case OSPF_NODE:
    case OSPF6_NODE:
    case ISIS_NODE:
    case KEYCHAIN_NODE:
    case MASC_NODE:
    case RMAP_NODE:
    case PIM_NODE:
    case VTY_NODE:
      vty->node = CONFIG_NODE;
      break;
    case BGP_VPNV4_NODE:
    case BGP_IPV4_NODE:
    case BGP_IPV4M_NODE:
    case BGP_IPV6_NODE:
    case BGP_IPV6M_NODE:
      vty->node = BGP_NODE;
      break;
    case KEYCHAIN_KEY_NODE:
      vty->node = KEYCHAIN_NODE;
      break;
    default:
      break;
    }
    vty->index = NULL;
    vty->index_list = cmd_free_memory_range_list (vty->index_list);;
  return CMD_SUCCESS;
}

/* quit is alias of exit. */
ALIAS (config_exit,
       config_quit_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

/* End of configuration. */
DEFUN_NON_IDL (config_end,
               config_end_cmd,
               "end",
               "End current mode and change to enable mode.")
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
    case RESTRICTED_NODE:
      /* Nothing to do. */
      break;
    case CONFIG_NODE:
    case AAA_SERVER_GROUP_NODE:
    case INTERFACE_NODE:
    case VLAN_NODE:
    case LINK_AGGREGATION_NODE:
    case QOS_QUEUE_PROFILE_NODE:
    case QOS_SCHEDULE_PROFILE_NODE:
    case MGMT_INTERFACE_NODE:
#ifdef ENABLE_OVSDB
    case VLAN_INTERFACE_NODE:
    case DHCP_SERVER_NODE:
    case TFTP_SERVER_NODE:
    case ACCESS_LIST_NODE:
    case MIRROR_NODE:
#endif
    case ZEBRA_NODE:
    case RIP_NODE:
    case RIPNG_NODE:
    case BABEL_NODE:
    case BGP_NODE:
    case BGP_VPNV4_NODE:
    case BGP_IPV4_NODE:
    case BGP_IPV4M_NODE:
    case BGP_IPV6_NODE:
    case BGP_IPV6M_NODE:
    case RMAP_NODE:
    case OSPF_NODE:
    case OSPF6_NODE:
    case ISIS_NODE:
    case KEYCHAIN_NODE:
    case KEYCHAIN_KEY_NODE:
    case MASC_NODE:
    case PIM_NODE:
    case VTY_NODE:
      vty_config_unlock (vty);
      vty->node = ENABLE_NODE;
      break;
    default:
      break;
    }
  vty->index_list = cmd_free_memory_range_list (vty->index_list);
  return CMD_SUCCESS;
}

/* Show version. */
#ifndef ENABLE_OVSDB
DEFUN (show_version,
       show_version_cmd,
       "show version",
       SHOW_STR
       "Displays zebra version\n")
{
  vty_out (vty, "%s %s (%s).%s", QUAGGA_PROGNAME, QUAGGA_VERSION,
	   host.name?host.name:"", VTY_NEWLINE);
  vty_out (vty, "%s%s", GIT_INFO, VTY_NEWLINE);

  return CMD_SUCCESS;
}
#else
DEFUN (show_version,
       show_version_cmd,
       "show version",
       SHOW_STR
       SHOW_VERSION_STR)
{
  vty_out (vty, "%s %s%s", vtysh_ovsdb_os_name_get(),
           vtysh_ovsdb_switch_version_get(), VTY_NEWLINE);
  return CMD_SUCCESS;
}
#endif /* ENABLE_OVSDB */

/* Show version detail. */
DEFUN (show_version_detail,
       show_version_detail_cmd,
       "show version detail",
       SHOW_STR
       SHOW_VERSION_STR
       SHOW_VERSION_DETAIL_STR)
{
  vtysh_ovsdb_show_version_detail();
  return CMD_SUCCESS;
}

/* Show version detail OPS. */
DEFUN (show_version_detail_ops,
       show_version_detail_ops_cmd,
       "show version detail ops",
       SHOW_STR
       SHOW_VERSION_STR
       SHOW_VERSION_DETAIL_STR
       SHOW_VERSION_DETAIL_OPS_STR)
{
  vtysh_ovsdb_show_version_detail_ops();
  return CMD_SUCCESS;
}

/* Help display function for all node. */
DEFUN (config_help,
       config_help_cmd,
       "help",
       "Description of the interactive help system\n")
{
  vty_out (vty,
	   "Quagga VTY provides advanced help feature.  When you need help,%s\
anytime at the command line please press '?'.%s\
%s\
If nothing matches, the help list will be empty and you must backup%s\
 until entering a '?' shows the available options.%s\
Two styles of help are provided:%s\
1. Full help is available when you are ready to enter a%s\
command argument (e.g. 'show ?') and describes each possible%s\
argument.%s\
2. Partial help is provided when an abbreviated argument is entered%s\
   and you want to know what arguments match the input%s\
   (e.g. 'show me?'.)%s%s", VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE);
  return CMD_SUCCESS;
}

/* Help display function for all node. */
DEFUN (config_list,
       config_list_cmd,
       "list",
       "Print command list\n")
{
  unsigned int i;
  struct cmd_node *cnode = vector_slot (cmdvec, vty->node);
  struct cmd_element *cmd;

  for (i = 0; i < vector_active (cnode->cmd_vector); i++)
    if ((cmd = vector_slot (cnode->cmd_vector, i)) != NULL
        && !(cmd->attr & CMD_ATTR_DEPRECATED
             || cmd->attr & CMD_ATTR_HIDDEN))
      vty_out (vty, "  %s%s", cmd->string,
	       VTY_NEWLINE);
  return CMD_SUCCESS;
}

/* Write current configuration into file. */
DEFUN (config_write_file,
       config_write_file_cmd,
       "write file",
       "Write running configuration to memory, network, or terminal\n"
       "Write to configuration file\n")
{
  unsigned int i;
  int fd;
  struct cmd_node *node;
  char *config_file;
  char *config_file_tmp = NULL;
  char *config_file_sav = NULL;
  int ret = CMD_WARNING;
  struct vty *file_vty;

  /* Check and see if we are operating under vtysh configuration */
  if (host.config == NULL)
    {
      vty_out (vty, "Can't save to configuration file, using vtysh.%s",
	       VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Get filename. */
  config_file = host.config;

  config_file_sav =
    XMALLOC (MTYPE_TMP, strlen (config_file) + strlen (CONF_BACKUP_EXT) + 1);
  strcpy (config_file_sav, config_file);
  strcat (config_file_sav, CONF_BACKUP_EXT);


  config_file_tmp = XMALLOC (MTYPE_TMP, strlen (config_file) + 8);
  snprintf (config_file_tmp,strlen (config_file) + 8, "%s.XXXXXX", config_file);

  /* Open file to configuration write. */
  fd = mkstemp (config_file_tmp);
  if (fd < 0)
    {
      vty_out (vty, "Can't open configuration file %s.%s", config_file_tmp,
	       VTY_NEWLINE);
      goto finished;
    }

  /* Make vty for configuration file. */
  file_vty = vty_new ();
  file_vty->fd = fd;
  file_vty->type = VTY_FILE;

  /* Config file header print. */
  vty_out (file_vty, "!\n! Zebra configuration saved from vty\n!   ");
  vty_time_print (file_vty, 1);
  vty_out (file_vty, "!\n");

  for (i = 0; i < vector_active (cmdvec); i++)
    if ((node = vector_slot (cmdvec, i)) && node->func)
      {
	if ((*node->func) (file_vty))
	  vty_out (file_vty, "!\n");
      }
  vty_close (file_vty);

  if (unlink (config_file_sav) != 0)
    if (errno != ENOENT)
      {
	vty_out (vty, "Can't unlink backup configuration file %s.%s", config_file_sav,
		 VTY_NEWLINE);
        goto finished;
      }
  if (link (config_file, config_file_sav) != 0)
    {
      vty_out (vty, "Can't backup old configuration file %s.%s", config_file_sav,
	        VTY_NEWLINE);
      goto finished;
    }
  sync ();
  if (unlink (config_file) != 0)
    {
      vty_out (vty, "Can't unlink configuration file %s.%s", config_file,
	        VTY_NEWLINE);
      goto finished;
    }
  if (link (config_file_tmp, config_file) != 0)
    {
      vty_out (vty, "Can't save configuration file %s.%s", config_file,
	       VTY_NEWLINE);
      goto finished;
    }
  sync ();

  if (chmod (config_file, CONFIGFILE_MASK) != 0)
    {
      vty_out (vty, "Can't chmod configuration file %s: %s (%d).%s",
	config_file, safe_strerror(errno), errno, VTY_NEWLINE);
      goto finished;
    }

  vty_out (vty, "Configuration saved to %s%s", config_file,
	   VTY_NEWLINE);
  ret = CMD_SUCCESS;

finished:
  unlink (config_file_tmp);
  XFREE (MTYPE_TMP, config_file_tmp);
  XFREE (MTYPE_TMP, config_file_sav);
  return ret;
}

ALIAS (config_write_file,
       config_write_cmd,
       "write",
       "Write running configuration to memory, network, or terminal\n")

ALIAS (config_write_file,
       config_write_memory_cmd,
       "write memory",
       "Write running configuration to memory, network, or terminal\n"
       "Write configuration to the file (same as write file)\n")

ALIAS (config_write_file,
       copy_runningconfig_startupconfig_cmd,
       "copy running-config startup-config",
       "Copy configuration\n"
       "Copy running config to... \n"
       "Copy running config to startup config (same as write file)\n")

/* Write current configuration into the terminal. */
DEFUN (config_write_terminal,
       config_write_terminal_cmd,
       "write terminal",
       "Write running configuration to memory, network, or terminal\n"
       "Write to terminal\n")
{
  unsigned int i;
  struct cmd_node *node;

  if (vty->type == VTY_SHELL_SERV)
    {
      for (i = 0; i < vector_active (cmdvec); i++)
	if ((node = vector_slot (cmdvec, i)) && node->func && node->vtysh)
	  {
	    if ((*node->func) (vty))
	      vty_out (vty, "!%s", VTY_NEWLINE);
	  }
    }
  else
    {
      vty_out (vty, "%sCurrent configuration:%s", VTY_NEWLINE,
	       VTY_NEWLINE);
      vty_out (vty, "!%s", VTY_NEWLINE);

      for (i = 0; i < vector_active (cmdvec); i++)
	if ((node = vector_slot (cmdvec, i)) && node->func)
	  {
	    if ((*node->func) (vty))
	      vty_out (vty, "!%s", VTY_NEWLINE);
	  }
      vty_out (vty, "end%s",VTY_NEWLINE);
    }
  return CMD_SUCCESS;
}

#ifndef ENABLE_OVSDB
/* Write current configuration into the terminal. */
ALIAS (config_write_terminal,
       show_running_config_cmd,
       "show running-config",
       SHOW_STR
       "running configuration\n")

/* Write startup configuration into the terminal. */
DEFUN (show_startup_config,
       show_startup_config_cmd,
       "show startup-config",
       SHOW_STR
       "Contents of startup configuration\n")
{
  char buf[BUFSIZ];
  FILE *confp;

  confp = fopen (host.config, "r");
  if (confp == NULL)
    {
      vty_out (vty, "Can't open configuration file [%s]%s",
	       host.config, VTY_NEWLINE);
      return CMD_WARNING;
    }

  while (fgets (buf, BUFSIZ, confp))
    {
      char *cp = buf;

      while (*cp != '\r' && *cp != '\n' && *cp != '\0')
	cp++;
      *cp = '\0';

      vty_out (vty, "%s%s", buf, VTY_NEWLINE);
    }

  fclose (confp);

  return CMD_SUCCESS;
}
#endif

#ifndef ENABLE_OVSDB
/* Hostname configuration */
DEFUN (config_hostname,
       hostname_cmd,
       "hostname WORD",
       "Set system's network name\n"
       "This system's network name\n")
{
  if (!isalpha((int) *argv[0]))
    {
      vty_out (vty, "Please specify string starting with alphabet%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (host.name)
    XFREE (MTYPE_HOST, host.name);

  host.name = XSTRDUP (MTYPE_HOST, argv[0]);
  return CMD_SUCCESS;
}

DEFUN (config_no_hostname,
       no_hostname_cmd,
       "no hostname [HOSTNAME]",
       NO_STR
       HOSTNAME_NO_STR
       "Host name of this router\n")
{
  if (host.name)
    XFREE (MTYPE_HOST, host.name);
  host.name = NULL;
  return CMD_SUCCESS;
}
#else
#define MAX_DOMAINNAME_LEN 32
extern char *temp_prompt;
extern void  vtysh_ovsdb_hostname_set(const char * in);
extern const char* vtysh_ovsdb_hostname_get();
extern void  vtysh_ovsdb_domainname_set(const char * in);
extern const char* vtysh_ovsdb_domainname_get();

/* CLI for hostname configuration */
DEFUN (config_hostname,
       hostname_cmd,
       "hostname HOSTNAME",
       HOSTNAME_SET_STR
       "Hostname string(Max Length 32), first letter must be alphabet\n")
{
    if(strlen (argv[0]) > MAX_HOSTNAME_LEN)
    {
        vty_out (vty, "Specify string of max %d character.\n", MAX_HOSTNAME_LEN);
        return CMD_SUCCESS;
    }
    if (!isalpha((int) *argv[0]))
    {
        vty_out (vty, "Please specify string starting with alphabet.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    vtysh_ovsdb_hostname_set(argv[0]);
    temp_prompt = NULL;
    return CMD_SUCCESS;
}

/* CLI for dislplaying hostname */
DEFUN (config_show_hostname,
       show_hostname_cmd,
       "show hostname",
       SHOW_STR
       HOSTNAME_GET_STR)
{
    const char* hostname = NULL;
    hostname = vtysh_ovsdb_hostname_get();
    if (hostname)
    {
        vty_out (vty, "%s%s", hostname, VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "%s%s", "", VTY_NEWLINE);
    }
    return CMD_SUCCESS;
}

/* CLI for resetting hostname without argument  */
DEFUN (config_no_hostname,
       no_hostname_cmd,
       "no hostname",
       NO_STR
       HOSTNAME_NO_STR)
{
    vtysh_ovsdb_hostname_set("");
    temp_prompt = NULL;
    return CMD_SUCCESS;
}

/* CLI for resetting hostname with argument */
DEFUN (config_no_hostname_arg,
       no_hostname_cmd_arg,
       "no hostname HOSTNAME",
       NO_STR
       HOSTNAME_NO_STR
       "Hostname string(Max Length 32), first letter must be alphabet\n")
{
    temp_prompt = NULL;
    return vtysh_ovsdb_hostname_reset(argv[0]);
}

/* Domain name configuration */
DEFUN (config_domainname,
       domainname_cmd,
       "domain-name WORD",
       DOMAINNAME_SET_STR
       "Domain name string(Max Length 32), first letter must be an alphabet\n")
{

    if (strlen (argv[0]) > MAX_DOMAINNAME_LEN)
    {
        vty_out (vty, "Specify string of max %d characters.%s",
                                    MAX_DOMAINNAME_LEN, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    if (!isalpha((int) *argv[0]))
    {
        vty_out (vty, "Please specify string starting with an alphabet.%s",
                                    VTY_NEWLINE);
        return CMD_SUCCESS;
    }
    vtysh_ovsdb_domainname_set(argv[0]);
    return CMD_SUCCESS;
}

/* CLI for dislplaying domain name */
DEFUN (config_show_domainname,
       show_domainname_cmd,
       "show domain-name",
       SHOW_STR
       DOMAINNAME_GET_STR)
{
    const char* domainname = NULL;
    domainname = vtysh_ovsdb_domainname_get();
    if (domainname)
    {
        vty_out (vty, "%s%s", domainname, VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "%s%s", "", VTY_NEWLINE);
    }
    return CMD_SUCCESS;
}

/* CLI for resetting domain name, without argument  */
DEFUN (config_no_domainname,
       no_domainname_cmd,
       "no domain-name",
       NO_STR
       DOMAINNAME_NO_STR)
{
       vtysh_ovsdb_domainname_set("");
       return CMD_SUCCESS;
}
/* CLI for resetting domain name with argument */
DEFUN (config_no_domainname_arg,
       no_domainname_cmd_arg,
       "no domain-name WORD",
       NO_STR
       DOMAINNAME_NO_STR
       "Domain name string(Max Length 32), first letter must be an alphabet\n")
{
    return vtysh_ovsdb_domainname_reset(argv[0]);
}

#endif //ENABLE_OVSDB

/* VTY interface password set. */
DEFUN (config_password, password_cmd,
       "password (8|) WORD",
       "Assign the terminal connection password\n"
       "Specifies a HIDDEN password will follow\n"
       "dummy string \n"
       "The HIDDEN line password string\n")
{
  /* Argument check. */
  if (argc == 0)
    {
      vty_out (vty, "Please specify password.%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (argc == 2)
    {
      if (*argv[0] == '8')
	{
	  if (host.password)
	    XFREE (MTYPE_HOST, host.password);
	  host.password = NULL;
	  if (host.password_encrypt)
	    XFREE (MTYPE_HOST, host.password_encrypt);
	  host.password_encrypt = XSTRDUP (MTYPE_HOST, argv[1]);
	  return CMD_SUCCESS;
	}
      else
	{
	  vty_out (vty, "Unknown encryption type.%s", VTY_NEWLINE);
	  return CMD_WARNING;
	}
    }

  if (!isalnum ((int) *argv[0]))
    {
      vty_out (vty,
	       "Please specify string starting with alphanumeric%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (host.password)
    XFREE (MTYPE_HOST, host.password);
  host.password = NULL;

  if (host.encrypt)
    {
      if (host.password_encrypt)
	XFREE (MTYPE_HOST, host.password_encrypt);
      host.password_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (argv[0]));
    }
  else
    host.password = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;
}

ALIAS (config_password, password_text_cmd,
       "password LINE",
       "Assign the terminal connection password\n"
       "The UNENCRYPTED (cleartext) line password\n")

/* VTY enable password set. */
DEFUN (config_enable_password, enable_password_cmd,
       "enable password (8|) WORD",
       "Modify enable password parameters\n"
       "Assign the privileged level password\n"
       "Specifies a HIDDEN password will follow\n"
       "dummy string \n"
       "The HIDDEN 'enable' password string\n")
{
  /* Argument check. */
  if (argc == 0)
    {
      vty_out (vty, "Please specify password.%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Crypt type is specified. */
  if (argc == 2)
    {
      if (*argv[0] == '8')
	{
	  if (host.enable)
	    XFREE (MTYPE_HOST, host.enable);
	  host.enable = NULL;

	  if (host.enable_encrypt)
	    XFREE (MTYPE_HOST, host.enable_encrypt);
	  host.enable_encrypt = XSTRDUP (MTYPE_HOST, argv[1]);

	  return CMD_SUCCESS;
	}
      else
	{
	  vty_out (vty, "Unknown encryption type.%s", VTY_NEWLINE);
	  return CMD_WARNING;
	}
    }

  if (!isalnum ((int) *argv[0]))
    {
      vty_out (vty,
	       "Please specify string starting with alphanumeric%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (host.enable)
    XFREE (MTYPE_HOST, host.enable);
  host.enable = NULL;

  /* Plain password input. */
  if (host.encrypt)
    {
      if (host.enable_encrypt)
	XFREE (MTYPE_HOST, host.enable_encrypt);
      host.enable_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (argv[0]));
    }
  else
    host.enable = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;
}

ALIAS (config_enable_password,
       enable_password_text_cmd,
       "enable password LINE",
       "Modify enable password parameters\n"
       "Assign the privileged level password\n"
       "The UNENCRYPTED (cleartext) 'enable' password\n")

/* VTY enable password delete. */
DEFUN (no_config_enable_password, no_enable_password_cmd,
       "no enable password",
       NO_STR
       "Modify enable password parameters\n"
       "Assign the privileged level password\n")
{
  if (host.enable)
    XFREE (MTYPE_HOST, host.enable);
  host.enable = NULL;

  if (host.enable_encrypt)
    XFREE (MTYPE_HOST, host.enable_encrypt);
  host.enable_encrypt = NULL;

  return CMD_SUCCESS;
}

DEFUN (service_password_encrypt,
       service_password_encrypt_cmd,
       "service password-encryption",
       "Set up miscellaneous service\n"
       "Enable encrypted passwords\n")
{
  if (host.encrypt)
    return CMD_SUCCESS;

  host.encrypt = 1;

  if (host.password)
    {
      if (host.password_encrypt)
	XFREE (MTYPE_HOST, host.password_encrypt);
      host.password_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (host.password));
    }
  if (host.enable)
    {
      if (host.enable_encrypt)
	XFREE (MTYPE_HOST, host.enable_encrypt);
      host.enable_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (host.enable));
    }

  return CMD_SUCCESS;
}

DEFUN (no_service_password_encrypt,
       no_service_password_encrypt_cmd,
       "no service password-encryption",
       NO_STR
       "Set up miscellaneous service\n"
       "Enable encrypted passwords\n")
{
  if (! host.encrypt)
    return CMD_SUCCESS;

  host.encrypt = 0;

  if (host.password_encrypt)
    XFREE (MTYPE_HOST, host.password_encrypt);
  host.password_encrypt = NULL;

  if (host.enable_encrypt)
    XFREE (MTYPE_HOST, host.enable_encrypt);
  host.enable_encrypt = NULL;

  return CMD_SUCCESS;
}

DEFUN (config_terminal_length, config_terminal_length_cmd,
       "terminal length <0-512>",
       "Set terminal line parameters\n"
       "Set number of lines on a screen\n"
       "Number of lines on screen (0 for no pausing)\n")
{
  int lines;
  char *endptr = NULL;

  lines = strtol (argv[0], &endptr, 10);
  if (lines < 0 || lines > 512 || *endptr != '\0')
    {
      vty_out (vty, "length is malformed%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  vty->lines = lines;

  return CMD_SUCCESS;
}

DEFUN (config_terminal_no_length, config_terminal_no_length_cmd,
       "terminal no length",
       "Set terminal line parameters\n"
       NO_STR
       "Set number of lines on a screen\n")
{
  vty->lines = -1;
  return CMD_SUCCESS;
}

DEFUN (service_terminal_length, service_terminal_length_cmd,
       "service terminal-length <0-512>",
       "Set up miscellaneous service\n"
       "System wide terminal length configuration\n"
       "Number of lines of VTY (0 means no line control)\n")
{
  int lines;
  char *endptr = NULL;

  lines = strtol (argv[0], &endptr, 10);
  if (lines < 0 || lines > 512 || *endptr != '\0')
    {
      vty_out (vty, "length is malformed%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  host.lines = lines;

  return CMD_SUCCESS;
}

DEFUN (no_service_terminal_length, no_service_terminal_length_cmd,
       "no service terminal-length [<0-512>]",
       NO_STR
       "Set up miscellaneous service\n"
       "System wide terminal length configuration\n"
       "Number of lines of VTY (0 means no line control)\n")
{
  host.lines = -1;
  return CMD_SUCCESS;
}

DEFUN_HIDDEN (do_echo,
	      echo_cmd,
	      "echo .MESSAGE",
	      "Echo a message back to the vty\n"
	      "The message to echo\n")
{
  char *message;

  vty_out (vty, "%s%s", ((message = argv_concat(argv, argc, 0)) ? message : ""),
	   VTY_NEWLINE);
  if (message)
    XFREE(MTYPE_TMP, message);
  return CMD_SUCCESS;
}

DEFUN (config_logmsg,
       config_logmsg_cmd,
       "logmsg "LOG_LEVELS" .MESSAGE",
       "Send a message to enabled logging destinations\n"
       LOG_LEVEL_DESC
       "The message to send\n")
{
  int level;
  char *message;

  if ((level = level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;

  zlog(NULL, level, "%s", ((message = argv_concat(argv, argc, 1)) ? message : ""));
  if (message)
    XFREE(MTYPE_TMP, message);
  return CMD_SUCCESS;
}

DEFUN (show_logging,
       show_logging_cmd,
       "show logging",
       SHOW_STR
       "Show current logging configuration\n")
{
  struct zlog *zl = zlog_default;

  vty_out (vty, "Syslog logging: ");
  if (zl->maxlvl[ZLOG_DEST_SYSLOG] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s, facility %s, ident %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_SYSLOG]],
	     facility_name(zl->facility), zl->ident);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Stdout logging: ");
  if (zl->maxlvl[ZLOG_DEST_STDOUT] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_STDOUT]]);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Monitor logging: ");
  if (zl->maxlvl[ZLOG_DEST_MONITOR] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_MONITOR]]);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "File logging: ");
  if ((zl->maxlvl[ZLOG_DEST_FILE] == ZLOG_DISABLED) ||
      !zl->fp)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s, filename %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_FILE]],
	     zl->filename);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Protocol name: %s%s",
  	   zlog_proto_names[zl->protocol], VTY_NEWLINE);
  vty_out (vty, "Record priority: %s%s",
  	   (zl->record_priority ? "enabled" : "disabled"), VTY_NEWLINE);
  vty_out (vty, "Timestamp precision: %d%s",
	   zl->timestamp_precision, VTY_NEWLINE);

  return CMD_SUCCESS;
}

DEFUN (config_log_stdout,
       config_log_stdout_cmd,
       "log stdout",
       "Logging control\n"
       "Set stdout logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, zlog_default->default_lvl);
  return CMD_SUCCESS;
}

DEFUN (config_log_stdout_level,
       config_log_stdout_level_cmd,
       "log stdout "LOG_LEVELS,
       "Logging control\n"
       "Set stdout logging level\n"
       LOG_LEVEL_DESC)
{
  int level;

  if ((level = level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, level);
  return CMD_SUCCESS;
}

DEFUN (no_config_log_stdout,
       no_config_log_stdout_cmd,
       "no log stdout [LEVEL]",
       NO_STR
       "Logging control\n"
       "Cancel logging to stdout\n"
       "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, ZLOG_DISABLED);
  return CMD_SUCCESS;
}

DEFUN (config_log_monitor,
       config_log_monitor_cmd,
       "log monitor",
       "Logging control\n"
       "Set terminal line (monitor) logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, zlog_default->default_lvl);
  return CMD_SUCCESS;
}

DEFUN (config_log_monitor_level,
       config_log_monitor_level_cmd,
       "log monitor "LOG_LEVELS,
       "Logging control\n"
       "Set terminal line (monitor) logging level\n"
       LOG_LEVEL_DESC)
{
  int level;

  if ((level = level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, level);
  return CMD_SUCCESS;
}

DEFUN (no_config_log_monitor,
       no_config_log_monitor_cmd,
       "no log monitor [LEVEL]",
       NO_STR
       "Logging control\n"
       "Disable terminal line (monitor) logging\n"
       "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, ZLOG_DISABLED);
  return CMD_SUCCESS;
}

static int
set_log_file(struct vty *vty, const char *fname, int loglevel)
{
  int ret;
  char *p = NULL;
  const char *fullpath;

  /* Path detection. */
  if (! IS_DIRECTORY_SEP (*fname))
    {
      char cwd[MAXPATHLEN+1];
      cwd[MAXPATHLEN] = '\0';

      if (getcwd (cwd, MAXPATHLEN) == NULL)
        {
          zlog_err ("config_log_file: Unable to alloc mem!");
          return CMD_WARNING;
        }

      if ( (p = XMALLOC (MTYPE_TMP, strlen (cwd) + strlen (fname) + 2))
          == NULL)
        {
          zlog_err ("config_log_file: Unable to alloc mem!");
          return CMD_WARNING;
        }
      snprintf (p,strlen (cwd) + strlen (fname) + 2, "%s/%s", cwd, fname);
      fullpath = p;
    }
  else
    fullpath = fname;

  ret = zlog_set_file (NULL, fullpath, loglevel);

  if (p)
    XFREE (MTYPE_TMP, p);

  if (!ret)
    {
      vty_out (vty, "can't open logfile %s\n", fname);
      return CMD_WARNING;
    }

  if (host.logfile)
    XFREE (MTYPE_HOST, host.logfile);

  host.logfile = XSTRDUP (MTYPE_HOST, fname);

  return CMD_SUCCESS;
}

DEFUN (config_log_file,
       config_log_file_cmd,
       "log file FILENAME",
       "Logging control\n"
       "Logging to file\n"
       "Logging filename\n")
{
  return set_log_file(vty, argv[0], zlog_default->default_lvl);
}

DEFUN (config_log_file_level,
       config_log_file_level_cmd,
       "log file FILENAME "LOG_LEVELS,
       "Logging control\n"
       "Logging to file\n"
       "Logging filename\n"
       LOG_LEVEL_DESC)
{
  int level;

  if ((level = level_match(argv[1])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  return set_log_file(vty, argv[0], level);
}

DEFUN (no_config_log_file,
       no_config_log_file_cmd,
       "no log file [FILENAME]",
       NO_STR
       "Logging control\n"
       "Cancel logging to file\n"
       "Logging file name\n")
{
  zlog_reset_file (NULL);

  if (host.logfile)
    XFREE (MTYPE_HOST, host.logfile);

  host.logfile = NULL;

  return CMD_SUCCESS;
}

ALIAS (no_config_log_file,
       no_config_log_file_level_cmd,
       "no log file FILENAME LEVEL",
       NO_STR
       "Logging control\n"
       "Cancel logging to file\n"
       "Logging file name\n"
       "Logging level\n")

DEFUN (config_log_syslog,
       config_log_syslog_cmd,
       "log syslog",
       "Logging control\n"
       "Set syslog logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, zlog_default->default_lvl);
  return CMD_SUCCESS;
}

DEFUN (config_log_syslog_level,
       config_log_syslog_level_cmd,
       "log syslog "LOG_LEVELS,
       "Logging control\n"
       "Set syslog logging level\n"
       LOG_LEVEL_DESC)
{
  int level;

  if ((level = level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, level);
  return CMD_SUCCESS;
}

DEFUN_DEPRECATED (config_log_syslog_facility,
		  config_log_syslog_facility_cmd,
		  "log syslog facility "LOG_FACILITIES,
		  "Logging control\n"
		  "Logging goes to syslog\n"
		  "(Deprecated) Facility parameter for syslog messages\n"
		  LOG_FACILITY_DESC)
{
  int facility;

  if ((facility = facility_match(argv[0])) < 0)
    return CMD_ERR_NO_MATCH;

  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, zlog_default->default_lvl);
  zlog_default->facility = facility;
  return CMD_SUCCESS;
}

DEFUN (no_config_log_syslog,
       no_config_log_syslog_cmd,
       "no log syslog [LEVEL]",
       NO_STR
       "Logging control\n"
       "Cancel logging to syslog\n"
       "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, ZLOG_DISABLED);
  return CMD_SUCCESS;
}

ALIAS (no_config_log_syslog,
       no_config_log_syslog_facility_cmd,
       "no log syslog facility "LOG_FACILITIES,
       NO_STR
       "Logging control\n"
       "Logging goes to syslog\n"
       "Facility parameter for syslog messages\n"
       LOG_FACILITY_DESC)

DEFUN (config_log_facility,
       config_log_facility_cmd,
       "log facility "LOG_FACILITIES,
       "Logging control\n"
       "Facility parameter for syslog messages\n"
       LOG_FACILITY_DESC)
{
  int facility;

  if ((facility = facility_match(argv[0])) < 0)
    return CMD_ERR_NO_MATCH;
  zlog_default->facility = facility;
  return CMD_SUCCESS;
}

DEFUN (no_config_log_facility,
       no_config_log_facility_cmd,
       "no log facility [FACILITY]",
       NO_STR
       "Logging control\n"
       "Reset syslog facility to default (daemon)\n"
       "Syslog facility\n")
{
  zlog_default->facility = LOG_DAEMON;
  return CMD_SUCCESS;
}

DEFUN_DEPRECATED (config_log_trap,
		  config_log_trap_cmd,
		  "log trap "LOG_LEVELS,
		  "Logging control\n"
		  "(Deprecated) Set logging level and default for all destinations\n"
		  LOG_LEVEL_DESC)
{
  int new_level ;
  int i;

  if ((new_level = level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;

  zlog_default->default_lvl = new_level;
  for (i = 0; i < ZLOG_NUM_DESTS; i++)
    if (zlog_default->maxlvl[i] != ZLOG_DISABLED)
      zlog_default->maxlvl[i] = new_level;
  return CMD_SUCCESS;
}

DEFUN_DEPRECATED (no_config_log_trap,
		  no_config_log_trap_cmd,
		  "no log trap [LEVEL]",
		  NO_STR
		  "Logging control\n"
		  "Permit all logging information\n"
		  "Logging level\n")
{
  zlog_default->default_lvl = LOG_DEBUG;
  return CMD_SUCCESS;
}

DEFUN (config_log_record_priority,
       config_log_record_priority_cmd,
       "log record-priority",
       "Logging control\n"
       "Log the priority of the message within the message\n")
{
  zlog_default->record_priority = 1 ;
  return CMD_SUCCESS;
}

DEFUN (no_config_log_record_priority,
       no_config_log_record_priority_cmd,
       "no log record-priority",
       NO_STR
       "Logging control\n"
       "Do not log the priority of the message within the message\n")
{
  zlog_default->record_priority = 0 ;
  return CMD_SUCCESS;
}

DEFUN (config_log_timestamp_precision,
       config_log_timestamp_precision_cmd,
       "log timestamp precision <0-6>",
       "Logging control\n"
       "Timestamp configuration\n"
       "Set the timestamp precision\n"
       "Number of subsecond digits\n")
{
  if (argc != 1)
    {
      vty_out (vty, "Insufficient arguments%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  VTY_GET_INTEGER_RANGE("Timestamp Precision",
  			zlog_default->timestamp_precision, argv[0], 0, 6);
  return CMD_SUCCESS;
}

DEFUN (no_config_log_timestamp_precision,
       no_config_log_timestamp_precision_cmd,
       "no log timestamp precision",
       NO_STR
       "Logging control\n"
       "Timestamp configuration\n"
       "Reset the timestamp precision to the default value of 0\n")
{
  zlog_default->timestamp_precision = 0 ;
  return CMD_SUCCESS;
}

/* The following functions are remnants from Quagga */
/* See ../bannery_vty.c for banner code */
DEFUN (banner_motd_file,
       banner_motd_file_cmd,
       "banner motd file [FILE]",
       "Set banner\n"
       "Banner for motd\n"
       "Banner from a file\n"
       "Filename\n")
{
  if (host.motdfile)
    XFREE (MTYPE_HOST, host.motdfile);
  host.motdfile = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;
}

DEFUN (banner_motd_default,
       banner_motd_default_cmd,
       "banner motd default",
       "Set banner string\n"
       "Strings for motd\n"
       "Default string\n")
{
  host.motd = default_motd;
  return CMD_SUCCESS;
}

DEFUN (no_banner_motd,
       no_banner_motd_cmd,
       "no banner motd",
       NO_STR
       "Set banner string\n"
       "Strings for motd\n")
{
  host.motd = NULL;
  if (host.motdfile)
    XFREE (MTYPE_HOST, host.motdfile);
  host.motdfile = NULL;
  return CMD_SUCCESS;
}

/* Set config filename.  Called from vty.c */
void
host_config_set (char *filename)
{
  if (host.config)
    XFREE (MTYPE_HOST, host.config);
  host.config = XSTRDUP (MTYPE_HOST, filename);
}

void
install_default (enum node_type node)
{
  install_element (node, &config_exit_cmd);
  install_element (node, &config_quit_cmd);
  install_element (node, &config_end_cmd);
  install_element (node, &config_help_cmd);
  install_element (node, &config_list_cmd);

  install_element (node, &config_write_terminal_cmd);
  install_element (node, &config_write_file_cmd);
  install_element (node, &config_write_memory_cmd);
  install_element (node, &config_write_cmd);
#ifndef ENABLE_OVSDB
  install_element (node, &show_running_config_cmd);
#endif
}

/* Initialize command interface. Install basic nodes and commands. */
void
cmd_init (int terminal)
{
  command_cr = XSTRDUP(MTYPE_CMD_TOKENS, "<cr>");
  token_cr.type = TOKEN_TERMINAL;
  token_cr.cmd = command_cr;
  token_cr.desc = XSTRDUP(MTYPE_CMD_TOKENS, "");

  /* Allocate initial top vector of commands. */
  cmdvec = vector_init (VECTOR_MIN_SIZE);

  /* Default host value settings. */
  host.name = NULL;
  host.password = NULL;
  host.enable = NULL;
  host.logfile = NULL;
  host.config = NULL;
  host.lines = -1;
  host.motd = default_motd;
  host.motdfile = NULL;

  /* Install top nodes. */
  install_node (&view_node, NULL);
  install_node (&enable_node, NULL);
  install_node (&auth_node, NULL);
  install_node (&auth_enable_node, NULL);
  install_node (&restricted_node, NULL);
  install_node (&config_node, config_write_host);

  /* Each node's basic commands. */
  install_element (VIEW_NODE, &show_version_cmd);
  install_element (VIEW_NODE, &show_version_detail_cmd);
  install_element (VIEW_NODE, &show_version_detail_ops_cmd);
  if (terminal)
    {
      install_element (VIEW_NODE, &config_list_cmd);
      install_element (VIEW_NODE, &config_exit_cmd);
      install_element (VIEW_NODE, &config_quit_cmd);
      install_element (VIEW_NODE, &config_help_cmd);
      install_element (VIEW_NODE, &config_enable_cmd);
      install_element (VIEW_NODE, &config_terminal_length_cmd);
      install_element (VIEW_NODE, &config_terminal_no_length_cmd);
      install_element (VIEW_NODE, &show_logging_cmd);
      install_element (VIEW_NODE, &echo_cmd);

      install_element (RESTRICTED_NODE, &config_list_cmd);
      install_element (RESTRICTED_NODE, &config_exit_cmd);
      install_element (RESTRICTED_NODE, &config_quit_cmd);
      install_element (RESTRICTED_NODE, &config_help_cmd);
      install_element (RESTRICTED_NODE, &config_enable_cmd);
      install_element (RESTRICTED_NODE, &config_terminal_length_cmd);
      install_element (RESTRICTED_NODE, &config_terminal_no_length_cmd);
      install_element (RESTRICTED_NODE, &echo_cmd);
    }

  if (terminal)
    {
      install_default (ENABLE_NODE);
      install_element (ENABLE_NODE, &config_disable_cmd);
      install_element (ENABLE_NODE, &config_terminal_cmd);
      install_element (ENABLE_NODE, &copy_runningconfig_startupconfig_cmd);
    }
#ifndef ENABLE_OVSDB
  install_element (ENABLE_NODE, &show_startup_config_cmd);
#endif
  install_element (ENABLE_NODE, &show_version_cmd);
  install_element (ENABLE_NODE, &show_version_detail_cmd);
  install_element (ENABLE_NODE, &show_version_detail_ops_cmd);

  if (terminal)
    {
      install_element (ENABLE_NODE, &config_terminal_length_cmd);
      install_element (ENABLE_NODE, &config_terminal_no_length_cmd);
      install_element (ENABLE_NODE, &show_logging_cmd);
      install_element (ENABLE_NODE, &echo_cmd);
      install_element (ENABLE_NODE, &config_logmsg_cmd);

      install_default (CONFIG_NODE);
    }

  install_element (CONFIG_NODE, &hostname_cmd);
  install_element (ENABLE_NODE, &show_hostname_cmd);
  install_element (CONFIG_NODE, &no_hostname_cmd);
  install_element (CONFIG_NODE, &no_hostname_cmd_arg);
  install_element (CONFIG_NODE, &domainname_cmd);
  install_element (ENABLE_NODE, &show_domainname_cmd);
  install_element (CONFIG_NODE, &no_domainname_cmd);
  install_element (CONFIG_NODE, &no_domainname_cmd_arg);

  if (terminal)
    {
      install_element (CONFIG_NODE, &password_cmd);
      install_element (CONFIG_NODE, &password_text_cmd);
      install_element (CONFIG_NODE, &enable_password_cmd);
      install_element (CONFIG_NODE, &enable_password_text_cmd);
      install_element (CONFIG_NODE, &no_enable_password_cmd);

      install_element (CONFIG_NODE, &config_log_stdout_cmd);
      install_element (CONFIG_NODE, &config_log_stdout_level_cmd);
      install_element (CONFIG_NODE, &no_config_log_stdout_cmd);
      install_element (CONFIG_NODE, &config_log_monitor_cmd);
      install_element (CONFIG_NODE, &config_log_monitor_level_cmd);
      install_element (CONFIG_NODE, &no_config_log_monitor_cmd);
      install_element (CONFIG_NODE, &config_log_file_cmd);
      install_element (CONFIG_NODE, &config_log_file_level_cmd);
      install_element (CONFIG_NODE, &no_config_log_file_cmd);
      install_element (CONFIG_NODE, &no_config_log_file_level_cmd);
      install_element (CONFIG_NODE, &config_log_syslog_cmd);
      install_element (CONFIG_NODE, &config_log_syslog_level_cmd);
      install_element (CONFIG_NODE, &config_log_syslog_facility_cmd);
      install_element (CONFIG_NODE, &no_config_log_syslog_cmd);
      install_element (CONFIG_NODE, &no_config_log_syslog_facility_cmd);
      install_element (CONFIG_NODE, &config_log_facility_cmd);
      install_element (CONFIG_NODE, &no_config_log_facility_cmd);
      install_element (CONFIG_NODE, &config_log_trap_cmd);
      install_element (CONFIG_NODE, &no_config_log_trap_cmd);
      install_element (CONFIG_NODE, &config_log_record_priority_cmd);
      install_element (CONFIG_NODE, &no_config_log_record_priority_cmd);
      install_element (CONFIG_NODE, &config_log_timestamp_precision_cmd);
      install_element (CONFIG_NODE, &no_config_log_timestamp_precision_cmd);
      install_element (CONFIG_NODE, &service_password_encrypt_cmd);
      install_element (CONFIG_NODE, &no_service_password_encrypt_cmd);
      install_element (CONFIG_NODE, &banner_motd_default_cmd);
      install_element (CONFIG_NODE, &banner_motd_file_cmd);
      install_element (CONFIG_NODE, &no_banner_motd_cmd);
      install_element (CONFIG_NODE, &service_terminal_length_cmd);
      install_element (CONFIG_NODE, &no_service_terminal_length_cmd);

      install_element (VIEW_NODE, &show_thread_cpu_cmd);
      install_element (ENABLE_NODE, &show_thread_cpu_cmd);
      install_element (RESTRICTED_NODE, &show_thread_cpu_cmd);

      install_element (ENABLE_NODE, &clear_thread_cpu_cmd);
      install_element (VIEW_NODE, &show_work_queues_cmd);
      install_element (ENABLE_NODE, &show_work_queues_cmd);
    }
  srand(time(NULL));
}

static void
cmd_terminate_token(struct cmd_token *token)
{
  unsigned int i, j;
  vector keyword_vect;

  if (token->multiple)
    {
      for (i = 0; i < vector_active(token->multiple); i++)
        cmd_terminate_token(vector_slot(token->multiple, i));
      vector_free(token->multiple);
      token->multiple = NULL;
    }

  if (token->keyword)
    {
      for (i = 0; i < vector_active(token->keyword); i++)
        {
          keyword_vect = vector_slot(token->keyword, i);
          for (j = 0; j < vector_active(keyword_vect); j++)
            cmd_terminate_token(vector_slot(keyword_vect, j));
          vector_free(keyword_vect);
        }
      vector_free(token->keyword);
      token->keyword = NULL;
    }

  XFREE(MTYPE_CMD_TOKENS, token->cmd);
  XFREE(MTYPE_CMD_TOKENS, token->desc);
  XFREE(MTYPE_CMD_TOKENS, token->dyn_cb);

  XFREE(MTYPE_CMD_TOKENS, token);
}

struct cmd_element *
cmd_terminate_element(struct cmd_element *cmd)
{
  unsigned int i;

  if (cmd->tokens == NULL)
    return cmd;

  for (i = 0; i < vector_active(cmd->tokens); i++)
    cmd_terminate_token(vector_slot(cmd->tokens, i));

  vector_free(cmd->tokens);
  cmd->tokens = NULL;
  cmd = NULL;
  return cmd;
}

void
cmd_terminate ()
{
  unsigned int i, j;
  struct cmd_node *cmd_node;
  struct cmd_element *cmd_element;
  vector cmd_node_v;

  if (cmdvec)
    {
      for (i = 0; i < vector_active (cmdvec); i++)
        if ((cmd_node = vector_slot (cmdvec, i)) != NULL)
          {
            cmd_node_v = cmd_node->cmd_vector;

            for (j = 0; j < vector_active (cmd_node_v); j++)
              if ((cmd_element = vector_slot (cmd_node_v, j)) != NULL)
                cmd_terminate_element(cmd_element);

            vector_free (cmd_node_v);
          }

      vector_free (cmdvec);
      cmdvec = NULL;
    }

  if (command_cr)
    XFREE(MTYPE_CMD_TOKENS, command_cr);
  if (token_cr.desc)
    XFREE(MTYPE_CMD_TOKENS, token_cr.desc);
  if (host.name)
    XFREE (MTYPE_HOST, host.name);
  if (host.password)
    XFREE (MTYPE_HOST, host.password);
  if (host.password_encrypt)
    XFREE (MTYPE_HOST, host.password_encrypt);
  if (host.enable)
    XFREE (MTYPE_HOST, host.enable);
  if (host.enable_encrypt)
    XFREE (MTYPE_HOST, host.enable_encrypt);
  if (host.logfile)
    XFREE (MTYPE_HOST, host.logfile);
  if (host.motdfile)
    XFREE (MTYPE_HOST, host.motdfile);
  if (host.config)
    XFREE (MTYPE_HOST, host.config);
}

/*
 * This function will remove the cli installed node/element based on the
 * type which is passed by caller.
 */

void
cmd_terminate_node_element (void *del_ptr, enum data_type del_type)
{
  unsigned int i, j;
  struct cmd_node *cmd_node;
  struct cmd_element *cmd_element;
  vector cmd_node_v;

  if (cmdvec)
  {
      for (i = 0; i < vector_active (cmdvec); i++)
        if ((cmd_node = vector_slot (cmdvec, i)) != NULL)
        {
            if ((del_type == NODE) && (cmd_node == del_ptr)) {
                    cmd_node_v = cmd_node->cmd_vector;
                    for (j = 0; j < vector_active (cmd_node_v); j++) {
                      if ((cmd_element = vector_slot (cmd_node_v, j)) != NULL) {
                                vector_slot (cmd_node_v, j) = cmd_terminate_element(cmd_element);
                          }
                      }
                vector_free (cmd_node_v);
                cmdvec->index[i] = NULL;
                break;

            } else if ((del_type == ELEMENT)) {
                cmd_node_v = cmd_node->cmd_vector;
                for (j = 0; j < vector_active (cmd_node_v); j++) {
                        if ((cmd_element = vector_slot (cmd_node_v, j)) != NULL) {
                                if (cmd_element == del_ptr) {
                                        vector_slot (cmd_node_v, j) = cmd_terminate_element(cmd_element);
                                        break;
                                }
                        }
                }

            }
       }
   }
}
/*
 * Function : install_dyn_helpstr_funcptr
 * Responsibility : install dynamic helpstring callback function in global list.
 * Parameters : char *funcname : dynamic helpstring callback function name.
 *              void (*funcptr): callback function pointer.
 * Return : void.
 */
void
install_dyn_helpstr_funcptr(char *funcname,
                   void (*funcptr)(struct cmd_token *token, struct vty *vty, \
                            char * const dyn_helpstr_ptr, int max_strlen))
{
    /* allocate node */
    struct dyn_cb_func* new_node =
                    (struct dyn_cb_func*) malloc(sizeof(struct dyn_cb_func));
    /* put in the data  */
    new_node->funcname  = funcname;
    new_node->funcptr =  funcptr;
    new_node->next = dyn_cb_lookup;
    dyn_cb_lookup = new_node;

    return;
}

#define COMMA_ERR  1
#define COMMA_STR_VALID 0

/*
 * Function : cmd_allocate_memory_str
 * Responsibility : allocate memory and copy string for temporary storage.
 * Parameters : char *str : string whose temporary storage is required.
 * Return : char * : temporary storage for str.
 */
char *
cmd_allocate_memory_str(const char *str)
{
    char *buf = NULL;
    buf = (char*)malloc(strlen(str) + 1);
    if (buf)
        strcpy (buf, str);
    return buf;
}

/*
 * Function : cmd_free_memory_str
 * Responsibility : free the memory block allocated for string.
 * Parameters : char *str : string pointer pointing to memory block.
 * Return : void.
 */
void
cmd_free_memory_str(char *str)
{
    free(str);
}

/*
 * Function : get_list_value
 * Responsibility : get the two values from string str and store it in
                    num_range array.
 * Parameters : char *str : string containing two values separated by '-' sign.
 *              unsigned long *num_range : array of long to store the
                                           two values obtained string str.
 * Return : int : 1, if command executed successfully and num_range array
                     is populated with two values.
 *                0, otherwise.
 */
int
get_list_value(const char *str, unsigned long *num_range)
{
    char *endptr = NULL;
    char *p;
    char buf[DECIMAL_STRLEN_MAX + 1];
    unsigned long start, end;

    p = strchr (str, '-');
    if (p == NULL)
        return 0;
    strncpy (buf, str, (p - str));
    buf[p - str] = '\0';
    start = strtoul (buf, &endptr, 10);
    if (*endptr != '\0')
        return 0;
    str = (p + 1);
    if (*str == '\0')
        return 0;
    end = strtoul (str, &endptr, 10);
    if (*endptr != '\0')
        return 0;
    num_range[0] = start;
    num_range[1] = end;
    if(start >= end)
        return 0;
    return 1;
}

/*
 * Function : cmd_input_comma_str_is_valid
 * Responsibility : validation of input string for correct format.
 * Parameters : char *str : input string.
 *              enum cli_int_type type : type of input allowed in particular
                                         command.
 * Return : int : 1, COMMA_STR_VALID if input string is in correct format.
 *                0, COMMA_ERR if input string is not in correct format.
 */
int
cmd_input_comma_str_is_valid(const char *str, enum cli_int_type type)
{
    const char *p = str;

    if (str == NULL)
        return COMMA_ERR;

    if (*p < '0' || *p > '9')
        return COMMA_ERR;

    p++;
    if (*p == '\0' && type == RANGE_OPERATOR)
    {
        return COMMA_ERR;
    }

    while (*p)
    {
        if ( ((type == BOTH_OPERATOR) && ((*p == ',' || *p == '-'))) ||
             ((type == COMMA_OPERATOR) && (*p == ',')) ||
             ((type == RANGE_OPERATOR) && (*p == '-')) )
        {
            if ((*(p + 1) >= '0') && (*(p + 1) <= '9'))
            {
                p += 2;
                continue;
            }
            else
            {
                return COMMA_ERR;
            }
        }
        if (*p >= '0' && *p <= '9')
        {
            p++;
            continue;
        }
        return COMMA_ERR;
    }
    return COMMA_STR_VALID;
}

/*
 * Function : cmd_input_is_within_range
 * Responsibility : validation of input string so that each value lies within
                    range of values allowed in particular command.
 * Parameters : const char *str : input string entered.
 *              const char *range : string containing allowed min-max values
                                    for particular command.
 * Return : int : 1, COMMA_STR_VALID if input string is within allowed range.
 *                0, COMMA_ERR if input string is not within allowed range.
 */
int
cmd_input_is_within_range(const char *str, const char *range)
{
    if (str == NULL || range == NULL)
        return COMMA_ERR;

    unsigned long min_max[2];
    unsigned long temp_min_max[2];
    unsigned long val;

    char *tmp = NULL;
    char *temp_free = NULL;
    char *endptr = NULL;

    char *buf = cmd_allocate_memory_str(range);
    char *first_ptr  = strchr(buf, '<');
    char *second_ptr = strchr(buf, '>');

    first_ptr++;
    *second_ptr = '\0';
    get_list_value (first_ptr, min_max);
    cmd_free_memory_str (buf);

    temp_free = tmp = cmd_allocate_memory_str (str);

    tmp = strtok (tmp, ",");
    while (tmp)
    {
        if (strchr (tmp, '-'))
        {
            get_list_value (tmp, temp_min_max);
            if (temp_min_max[0] < min_max[0] || temp_min_max[0] > min_max[1])
            {
                cmd_free_memory_str (temp_free);
                return COMMA_ERR;
            }
            if (temp_min_max[1] < min_max[0] || temp_min_max[1] > min_max[1])
            {
                cmd_free_memory_str (temp_free);
                return COMMA_ERR;
            }
            if (temp_min_max[0] >= temp_min_max[1])
            {
                cmd_free_memory_str (temp_free);
                return COMMA_ERR;
            }
        }
        else
        {
            val = strtoul (tmp, &endptr, 10);
            if (*endptr != '\0')
            {
                cmd_free_memory_str (temp_free);
                return COMMA_ERR;
            }

            if (val < min_max[0] || val  > min_max[1])
            {
                cmd_free_memory_str (temp_free);
                return COMMA_ERR;
            }
        }
        tmp = strtok (NULL, ",");
    }
    cmd_free_memory_str (temp_free);
    return COMMA_STR_VALID;
}

/*
 * Function : cmd_free_memory_range_list
 * Responsibility : free memory block used to store range list.
 * Parameters : struct range_list *list : pointer holding the range list.
 * Return : NULL.
 */
struct range_list *
cmd_free_memory_range_list(struct range_list *list)
{
    while(list != NULL)
    {
        struct range_list *temp = list;
        list = list->link;
        free(temp);
    }
    return NULL;
}

/*
 * Function : cmd_insert_value_list
 * Responsibility : append/create the list with the value from str.
 * Parameters : struct range_list *node : pointer holding the range list.
 *              char *str : string containing input value.
 * Return : struct range_list * : list generated by adding the str.
 */
struct range_list*
cmd_insert_value_list(struct range_list *node, const char *str)
{
    struct range_list *temp = NULL;
    if (node == NULL)
    {
        temp = (struct range_list*)malloc (sizeof(struct range_list));
        if (temp)
        {
            strcpy (temp->value, str);
            temp->link = NULL;
            return temp;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        node->link = cmd_insert_value_list (node->link, str);
    }
    return node;
}

/*
 * Function :  cmd_get_list_from_range_str
 * Responsibility : give range list of all values from input string which
                    contain input in form of range (A-B).
 * Parameters : char *value : string containing input value.
 *              int flag_intf : identifying input is interface number or
                                normal integer.
 * Return : struct range_list * : list generated by adding the str.
 */
struct range_list*
cmd_get_list_from_range_str (const char *str_ptr, int flag_intf)
{
    unsigned long i;
    const char *tmp = NULL;
    unsigned long num[2];
    struct range_list *node = NULL;
    char buf[DECIMAL_STRLEN_MAX + 1];

    if (strchr (str_ptr, '-'))
    {
        tmp = str_ptr;
        if ((flag_intf == 1) && (cmd_ifname_match (tmp) == 0))
        {
            node = cmd_insert_value_list (node, tmp);
        }
        else if (get_list_value (tmp, num) == 1)
        {
            for(i = num[0]; i <= num[1]; i++)
            {
                snprintf(buf, DECIMAL_STRLEN_MAX + 1, "%lu", i);
                node = cmd_insert_value_list (node, buf);
            }
        }
        else
        {
            return NULL;
        }
    }
    return node;
}

/*
 * Function :  cmd_get_range_value
 * Responsibility : give range list of all values from input string.
 * Parameters : char *value : string containing input value.
 *              int flag_intf : identifying input is interface number or
                                normal integer.
 * Return : struct range_list * : list generated from input string value.
 */
struct range_list*
cmd_get_range_value (const char *value, int flag_intf)
{
    struct range_list *node = NULL;
    struct range_list *temp, *temp_node = NULL;
    char *str_ptr = (char*)value;
    char *tmp = NULL;
    if (strchr (str_ptr, ','))
    {
        tmp = strtok (str_ptr, ",");
        while (tmp != NULL)
        {
            if (strchr (tmp, '-'))
            {
                if (node == NULL)
                {
                    node = cmd_get_list_from_range_str (tmp, flag_intf);
                }
                else
                {
                    temp_node = node;
                    temp = cmd_get_list_from_range_str (tmp, flag_intf);
                    if (temp != NULL)
                    {
                         while (temp_node->link != NULL)
                             temp_node = temp_node->link;
                         temp_node->link = temp;
                    }
                }
            }
            else
            {
                node = cmd_insert_value_list (node, tmp);
            }
            tmp = strtok (NULL, ",");
        }
    }
    else if (strchr (str_ptr, '-'))
    {
       node = cmd_get_list_from_range_str (str_ptr, flag_intf);
       if (node == NULL)
           return NULL;
    }
    else
    {
        node = cmd_insert_value_list (node, str_ptr);
    }
    return node;
}

/*
 * Function :  cmd_input_range_match
 * Responsibility : check whether input string is valid and values are within
                    range.
 * Parameters : const char *range : allowed min-max values for particular
                                    command.
 *              const char *str : string containing input value.
 *              enum cli_int_type type : type of input allowed in particular
                                         command.
 * Return : int : 1, if input string is valid and within range.
 *                0, otherwise.
 */
int
cmd_input_range_match(const char *range, const char *str, enum cli_int_type type)
{
    if (str == NULL)
        return 1;
    if (cmd_input_comma_str_is_valid(str, type) != COMMA_ERR)
    {
        if ((type == COMMA_OPERATOR) || (type == BOTH_OPERATOR))
        {
            if (cmd_input_is_within_range (str, range) == COMMA_ERR)
                return 0;
        }
        return 1;
    }
    else
        return 0;
}

/*
 * Function :  cmd_range_comma_cli_parser_validate
 * Responsibility : check word match and for ambiguous command.
 * Parameters : const char *src : input string.
 *              const char *dst : string containing help string of allowed
                                  values.
 *              enum cli_int_type type : type of input allowed in particular
                                         command.
 *              const char **matched : reference for matched string.
 *              int *match : reference for match variable.
 * Return : int : 1, if input string is valid and within range.
 *                0, otherwise.
 */
int
cmd_range_comma_cli_parser_validate(const char* src, const char* dst,
                                    enum cli_int_type type,
                                    const char **matched, int *match)
{
    char *buf = cmd_allocate_memory_str(dst);
    buf[2] = '<';
    if (cmd_input_range_match((buf + 2), src, type) == 1)
    {
        if (match != NULL)
        {
            *matched = dst;
            *match = *match + 1;
        }
        cmd_free_memory_str(buf);
        return 1;
    }
    cmd_free_memory_str(buf);
    return 0;
}
