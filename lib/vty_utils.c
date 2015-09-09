/* vtysh utils
 * Copyright (C) 1997, 98, 99 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
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
 * File: vty_utils.c
 *
 * Purpose: Functions for changing help strings and type checks.
 */
#include "vector.h"
#include "vty.h"
#include "command.h"
#include <pthread.h>
#include "vty_utils.h"
#include "latch.h"

struct latch ovsdb_latch;

pthread_mutex_t vtysh_ovsdb_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * This command converts command string into a vector of cmd_tokens
 */
vector utils_cmd_parse_format(const char* string, const char* descstr)
{
  vector temp = cmd_parse_format(string, descstr);
  return temp;
}

/*
 * This command reads a word in the command string
 */
void utils_format_parser_read_word(struct format_parser_state *state)
{
  format_parser_read_word(state);

  static char *noHelpString = "This is the no help string for hostname";
  const char *noStr = "no";
  const char *hostnameStr = "hostname";
  struct cmd_token *token;
  int width = 0,j=0,i;

  for (i = 0; i < vector_active (state->curvect); i++)
  {
    if ((token = vector_slot (state->curvect, i)) != NULL)
      {
        int len;

        if (token->cmd[0] == '\0')
        {
          continue;
        }

        len = strlen (token->cmd);
        if (token->cmd[0] == '.')
          len--;

        if (width < len)
          width = len;
      }
  }
  for (i = 0; i < vector_active (state->curvect); i++)
  {
    if ((token = vector_slot (state->curvect, i)) != NULL)
      {
        if (token->cmd == NULL || token->cmd[0] == '\0')
          continue;
        if(token->cmd != NULL && strstr(token->cmd, noStr) != NULL)
        {
          j = 1;
        }
        if (token->cmd != NULL && strstr((token->cmd), hostnameStr) != NULL)
        {
          if (token->desc && j == 1)
          {
            token->desc = noHelpString;
          }
        }
      }
  }
}

/*
 * This command returns the help string for the word
 */
char* utils_format_parser_desc_str(struct format_parser_state *state)
{
  char* token = format_parser_desc_str(state);
  return token;
}

/*
 * This command returns the vector when '?' token is typed
 */
vector utils_cmd_describe_command(vector vline, struct vty *vty, int *status)
{
  return cmd_describe_command(vline, vty, status);
}
