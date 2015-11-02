/* vtysh utils.
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
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
 * File: vty_utils.h
 *
 * Purpose: Functions for changing help strings and type checks.
 */
#ifndef VTY_UTILS_H
#define VTY_UTILS_H 1

extern struct latch ovsdb_latch;

/* To serialize updates to OVSDB.
 * interface threads calls to update OVSDB states. */
extern pthread_mutex_t vtysh_ovsdb_mutex;

/* Macros to lock and unlock mutexes in a verbose manner. */
#define VTYSH_OVSDB_LOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_LOCK: taking lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_lock(&vtysh_ovsdb_mutex); \
}

#define VTYSH_OVSDB_UNLOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_UNLOCK: releasing lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_unlock(&vtysh_ovsdb_mutex); \
}

vector utils_cmd_parse_format(const char* string, const char* desc);

void utils_format_parser_read_word(struct format_parser_state *state);

char* utils_formate_parser_desc_str(struct format_parser_state *state);

vector utils_cmd_describe_command(vector vline, struct vty* vty, int *status);

#endif /* VTY_UTILS_H */
