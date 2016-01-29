/* Traceroute CLI commands header file
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 */

/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file traceroute.h
 * purpose:    to add declarations required for traceroute_vty.c
 ***************************************************************************/

#ifndef _TRACEROUTE_VTY_H
#define _TRACEROUTE_VTY_H

#include <stdbool.h>

#define TRACEROUTE_STR \
"Traceroute Utility\n"
#define TRACEROUTE_IP \
"Enter IP address of the  device to traceroute\n"
#define TRACEROUTE_HOST \
"Enter Hostname of the device to traceroute\n"
#define TRACEROUTE_MAXTTL \
"Specify maximum number of hops to reach the destination <1-255>\n"
#define TRACEROUTE_MINTTL \
"Specify minimum number of hops to reach the destination <1-255>\n"
#define TRACEROUTE_TIMEOUT \
"Traceroute timeout in seconds <1-60>\n"
#define TRACEROUTE_PROBES \
"Number of Probes <1-5>\n"
#define TRACEROUTE_DSTPORT \
"Destination port <1-34000>\n"
#define TRACEROUTE_IP_OPTION \
"Specify the IP option\n"
#define TRACEROUTE_LOOSEROUTE \
"Specify the route for loose source record route\n"
#define LOOSEIP \
"Enter intermediate router's IP address for loose source routing\n"
#define MAXTTL_INPUT \
"Enter maximum TTL value (default: 30)\n"
#define MINTTL_INPUT \
"Enter minimum TTL value (default: 1)\n"
#define TIMEOUT_INPUT \
"Enter timeout value in seconds (default: 3 seconds)\n"
#define PROBES_INPUT \
"Enter probes value (default: 3)\n"
#define DSTPORT_INPUT \
"Enter destination port value (default: 33434)\n"

#define TRACEROUTE_MAX_HOSTNAME_LENGTH  256

/* traceroute options default values*/
#define TRACE_DEF_PORT                  33434
#define TRACE_DEF_PROBES                3
#define TRACE_DEF_MINTTL                1
#define TRACE_DEF_MAXTTL                30
#define TRACE_DEF_WAIT                  3

#define SWNS_EXEC                       "/sbin/ip netns exec swns"

/* default traceroute cmd */
#define TRACEROUTE4_DEF_CMD             "traceroute"
#define TRACEROUTE6_DEF_CMD             "traceroute6"

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
}arguments;

/*structure to store the traceroute tockens*/
typedef struct tracerouteEntry_t {
    bool isIpv4;
    char *tracerouteTarget;
    uint16_t tracerouteDstport;
    uint8_t tracerouteTimeout;
    uint8_t tracerouteMaxttl;
    uint8_t tracerouteMinttl;
    uint32_t tracerouteProbes;
    char *tracerouteLoosesourceIp;
} tracerouteEntry;

/*prototypes of the functions*/
void printOutput(char *);
bool traceroute_handler(tracerouteEntry *, void (*fPtr)(char *));
int decodeTracerouteParam(const char*, arguments, tracerouteEntry *);
void traceroute_vty_init(void);

#endif /* _TRACEROUTE_VTY_H */
