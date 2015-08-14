/* AAA CLI commands header file
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: aaa_vty.h
 *
 * Purpose:  To add declarations required for aaa_vty.c
 */

#define OPEN_VSWITCH_AAA_RADIUS               "radius"
#define OPEN_VSWITCH_AAA_FALLBACK             "fallback"
#define OPEN_VSWITCH_AAA_RADIUS_LOCAL         "local"
#define HALON_TRUE_STR                        "true"
#define HALON_FALSE_STR                       "false"

#define MAX_RADIUS_SERVERS                    64
#define RADIUS_SERVER_DEFAULT_PASSKEY         "testing123-1"
#define RADIUS_SERVER_DEFAULT_PORT            1812
#define RADIUS_SERVER_DEFAULT_RETRIES         1
#define RADIUS_SERVER_DEFAULT_TIMEOUT         5

void
aaa_vty_init (void);
