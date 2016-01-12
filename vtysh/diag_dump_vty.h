
/* Diagnostic dump CLI commands file
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
 * File: diag_dump_vty.h
 *
 * Purpose: header file for diag_dump_vty.c
 */

#ifndef DIAG_DUMP_VTY_H
#define DIAG_DUMP_VTY_H

#include "command.h"
#include "dirs.h"
#include "util.h"
#include "daemon.h"
#include "unixctl.h"

#define DIAG_DUMP_CONF        "/etc/openswitch/supportability/ops_diagdump.yaml"
#define DIAG_DUMP_CMD              "diag-dump"
#define FILE_PATH_LEN_MAX          256


#define DIAG_DUMP_STR              "Show diagnostic information\n"
#define DIAG_DUMP_LIST_STR         "Show supported features with description\n"

#define DIAG_DUMP_FEATURE          "Feature name  \n"
#define DIAG_DUMP_FEATURE_BASIC    "Basic information \n"
#define DIAG_DUMP_FEATURE_FILE     "Absolute path of file\n"




struct daemon {
   char* name;
   struct daemon* next;
};

struct feature {
   char* name;
   char* desc;
   struct daemon*   p_daemon;
   struct feature*   next;
};

enum  {
   VALUE,
   FEATURE_NAME,
   FEATURE_DESC,
   DAEMON,
   MAX_NUM_KEYS
} ;



void diag_dump_vty_init( void );
#endif /* DIAG_DUMP_VTY_H */
