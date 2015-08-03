/* System CLI commands.
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
 * File: system_vty.h
 *
 * Purpose: To add system CLI configuration and display commands.
 */

#ifndef _SYSTEM_VTY_H
#define _SYSTEM_VTY_H

#ifndef SYS_STR
#define SYS_STR		"System information\n"
#endif

typedef enum
{
	CLI_FAN,
	CLI_PSU,
	CLI_LED,
	CLI_TEMP
};

int cli_system_get_all();
void system_vty_init();

#endif //_SYSTEM_VTY_H
