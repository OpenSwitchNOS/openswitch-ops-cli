/* LED CLI commands.
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
 * File: led_vty.h
 *
 * Purpose: To add LED Platform CLI configuration and display commands.
 */

#ifndef _LED_VTY_H
#define _LED_VTY_H

#ifndef SYS_STR
#define SYS_STR		"System information\n"
#endif

#define LED_STR 	"Shows LED information\n"
#define LED_SET_STR 	"Set LED state\n"

int cli_system_no_set_led(char* sLedName);
int cli_system_get_led();
int  cli_system_set_led(char* sLedName,char* sLedState);
void led_vty_init();

#endif //_LED_VTY_H
