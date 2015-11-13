/* Traceroute CLI commands header file
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 This program is free software; you can redistribute it and/or
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
 */

/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file traceroute_vty.h
 * purpose:	to add declarations required for traceroute_vty.c
 ***************************************************************************/
#include <stdbool.h>

#ifndef _TRACEROUTE_VTY_H
#define _TRACEROUTE_VTY_H

#define TRACEROUTE_STR		 	"traceroute Utility\n"
#define TRACEROUTE_IP 		 	"Enter IP address of the  device to traceroute\n"
#define TRACEROUTE_HOST		 	"Enter Hostname of the device to ping\n"
#define TRACEROUTE_MAXTTL		"Maximum time to live <1-255>\n"
#define TRACEROUTE_MINTTL		"Minimum time to live <1-255>\n"
#define TRACEROUTE_TIMEOUT		"Traceroute timeout in seconds <1-120>\n"
#define TRACEROUTE_PROBES		"Number of Probes <1-5>\n"
#define TRACEROUTE_DSTPORT		"Destination port <1-34000>\n"
#define TRACEROUTE_IP_OPTION	"Specify the IP option\n"
#define TRACEROUTE_LOOSEROUTE	"Loose Source Route\n"
#define LOOSEIP					"Enter the ip for looseroute\n"
#define INPUT 					"Enter a number\n"

#define TRACEROUTE_MAX_HOSTNAME_LENGTH 256

/* traceroute options default values*/
#define TRACE_PORT				33434
#define TRACE_PROBES			3
#define TRACE_MINTTL			1
#define TRACE_MAXTTL			30
#define TRACE_WAIT				3

/* default traceroute cmd */
#define TRACEROUTE4_DEF_CMD 	"traceroute"
#define TRACEROUTE6_DEF_CMD 	"traceroute6"

/*defining the type of arguments passing through the cli*/
typedef enum {
    DESTINATION,
    IP_OPTION,
    DST_PORT,
	MAX_TTL,
	MIN_TTL,
	PROBES,
	TIME_OUT,
	IPV6_DESTINATION
	} arguments;

/*structure to store the traceroute tockens*/
typedef struct tracerouteEntry_t {
	bool isIpv4;
	char tracerouteTarget[TRACEROUTE_MAX_HOSTNAME_LENGTH+1];
	uint16_t tracerouteDstport;
	uint8_t tracerouteTimeout;
	uint8_t tracerouteMaxttl;
	uint8_t tracerouteMinttl;
	uint32_t tracerouteProbes;
	char tracerouteLoosesourceIp[TRACEROUTE_MAX_HOSTNAME_LENGTH+1];
} tracerouteEntry;

/*prototypes of the functions*/
void printOutput( char *);
int traceroute_handler(tracerouteEntry *,void (*fPtr)(char *));
int validateTracerouteTarget(const char* value,arguments type,tracerouteEntry* p);
void traceroute_vty_init (void);

#endif /* _TRACEROUTE_VTY_H */
