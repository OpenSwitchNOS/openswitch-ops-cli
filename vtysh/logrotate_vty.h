/* (C) Copyright 2015 Hewlett-Packard Enterprise Development LP.
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
 * File: logrotate_vty.h
 *
 * Purpose: header file for logrotate_vty.c
 */

#ifndef _LOGROTATE_VTY_H
#define _LOGROTATE_VTY_H

#include "command.h"

# define LOGROTATE_CMD_STR_PERIOD        "logrotate period (hourly| weekly | monthly ) "
# define LOGROTATE_NO_CMD_STR_PERIOD     "no logrotate period {hourly| weekly | monthly }"
# define LOGROTATE_CMD_STR_MAXSIZE       "logrotate maxsize <1-200>"
# define LOGROTATE_NO_CMD_STR_MAXSIZE    "no logrotate maxsize {<1-200>}"

# define LOGROTATE_CMD_STR_TARGET        "logrotate target WORD"
# define LOGROTATE_NO_CMD_STR_TARGET     "no logrotate target {WORD}"

# define LOGROTATE_HELP_STR_PERIOD      "Rotates, compresses, and transfers system logs \n" \
                                        "Logrotation period \n" \
                                        "Rotates log files every hour \n" \
                                        "Rotates log files every week \n" \
                                        "Rotates log files every month \n"

# define LOGROTATE_NO_HELP_STR_PERIOD      NO_STR \
                                           "Rotates, compresses, and transfers system logs \n" \
                                           "Logrotation period \n" \
                                           "Rotates log files every hour \n" \
                                           "Rotates log files every week \n" \
                                           "Rotates log files every month \n"



# define LOGROTATE_HELP_STR_MAXSIZE     "Rotates, compresses, and transfers system logs \n" \
                                        "Maximum file size for rotation \n" \
                                        "File size in Mega Bytes (MB).Default value is 10MB \n"

# define LOGROTATE_NO_HELP_STR_MAXSIZE     NO_STR \
                                           "Rotates, compresses, and transfers system logs \n" \
                                           "Maximum file size for rotation \n" \
                                           "File size in Mega Bytes (MB).Default value is 10MB \n"


# define LOGROTATE_HELP_STR_TARGET     "Rotates, compresses, and transfers system logs \n" \
                                       "Transfers logs to remote host \n"\
                                       "URI of the remote host. Supported values :'tftp://A.B.C.D' or 'tftp://X:X::X:X' \n"

# define LOGROTATE_NO_HELP_STR_TARGET     NO_STR \
                                          "Rotates, compresses, and transfers system logs \n" \
                                          "Transfers logs to remote host \n" \
                                          "URI of the remote host. Supported values :'tftp://A.B.C.D' or 'tftp://X:X::X:X' \n"

#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD "period"
#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE "maxsize"
#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET "target"

#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT "daily"
#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT "10"
#define OPEN_VSWITCH_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT "local"

void logrotate_vty_init();

#endif /* _LOGROTATE_VTY_H */
