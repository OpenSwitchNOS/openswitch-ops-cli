/*
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 * File: logrotate_vty.h
 *
 * Purpose: header file for logrotate_vty.c
 */

#ifndef _LOGROTATE_VTY_H
#define _LOGROTATE_VTY_H

#include "command.h"

# define LOGROTATE_CMD_STR_PERIOD        "logrotate period (daily | hourly | weekly | monthly ) "
# define LOGROTATE_NO_CMD_STR_PERIOD     "no logrotate period {hourly| weekly | monthly }"
# define LOGROTATE_CMD_STR_MAXSIZE       "logrotate maxsize <1-200>"
# define LOGROTATE_NO_CMD_STR_MAXSIZE_X  "no logrotate maxsize <1-200>"
# define LOGROTATE_NO_CMD_STR_MAXSIZE    "no logrotate maxsize"

# define LOGROTATE_CMD_STR_TARGET        "logrotate target WORD"
# define LOGROTATE_NO_CMD_STR_TARGET     "no logrotate target {WORD}"

# define LOGROTATE_HELP_STR_PERIOD      "Rotates, compresses, and transfers system logs \n" \
                                        "Logrotation period \n" \
                                        "(Default) \n" \
                                        "Rotates log files every hour \n" \
                                        "Rotates log files every week \n" \
                                        "Rotates log files every month \n"

# define LOGROTATE_NO_HELP_STR_PERIOD      NO_STR \
                                           "Rotates, compresses, and transfers system logs \n" \
                                           "Logrotation period (Default: daily)\n" \
                                           "Rotates log files every hour \n" \
                                           "Rotates log files every week \n" \
                                           "Rotates log files every month \n"



# define LOGROTATE_HELP_STR_MAXSIZE     "Rotates, compresses, and transfers system logs \n" \
                                        "Maximum file size for rotation \n" \
                                        "File size in Mega Bytes (MB).Default value is 10MB \n"

# define LOGROTATE_NO_HELP_STR_MAXSIZE_X   NO_STR \
                                           "Rotates, compresses, and transfers system logs \n" \
                                           "Maximum file size for rotation \n" \
                                           "File size in Mega Bytes (MB).Default value is 10MB \n"


# define LOGROTATE_NO_HELP_STR_MAXSIZE     NO_STR \
                                           "Rotates, compresses, and transfers system logs \n" \
                                           "Maximum file size for rotation \n"


# define LOGROTATE_HELP_STR_TARGET     "Rotates, compresses, and transfers system logs \n" \
                                       "Transfers logs to remote host \n"\
                                       "URI of the remote host. Supported values :'tftp://A.B.C.D' or 'tftp://X:X::X:X' \n"

# define LOGROTATE_NO_HELP_STR_TARGET     NO_STR \
                                          "Rotates, compresses, and transfers system logs \n" \
                                          "Transfers logs to remote host \n" \
                                          "URI of the remote host. Supported values :'tftp://A.B.C.D' or 'tftp://X:X::X:X' \n"

#define SYSTEM_LOGROTATE_CONFIG_MAP_PERIOD "period"
#define SYSTEM_LOGROTATE_CONFIG_MAP_MAXSIZE "maxsize"
#define SYSTEM_LOGROTATE_CONFIG_MAP_TARGET "target"

#define SYSTEM_LOGROTATE_CONFIG_MAP_PERIOD_DEFAULT "daily"
#define SYSTEM_LOGROTATE_CONFIG_MAP_MAXSIZE_DEFAULT "10"
#define SYSTEM_LOGROTATE_CONFIG_MAP_TARGET_DEFAULT "local"

void logrotate_vty_init();

#endif /* _LOGROTATE_VTY_H */
