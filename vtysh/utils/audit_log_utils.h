/*
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
/****************************************************************************
 *
 * @file audit_log_utils.h
 ***************************************************************************/

#ifndef _AUDIT_LOG__UTILS_H
#define _AUDIT_LOG__UTILS_H

#ifndef NULL
#define NULL 0
#endif
#define MAX_OP_DESC_LEN 20
#define MAX_CFGDATA_LEN 1000

void audit_log_user_msg(char *op, const char *cfgdata, char *hostname, int result);
extern int audit_fd;
static char *replace_space(char *str);
#endif
