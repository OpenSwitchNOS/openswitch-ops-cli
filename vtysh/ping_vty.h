/* Ping CLI commands header file
 *
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
 * File: Ping_vty.h
 *
 * Purpose:  To add declarations required for Ping_vty.c
 */

#include <stdbool.h>
#ifndef _PING_VTY_H
#define _PING_VTY_H

#define PING_STR        	"Ping Utility\n"
#define PING_IP         	"Enter IP address of the device to ping\n"
#define PING_HOST			"Enter Hostname of the device to ping\n"
#define PING_DSIZE			"Ping datagram size <100-65468>\n"
#define PING_PATTERN 		"Specify the data pattern in hex digits to send\n"
#define PATTERN				"Enter Hex pattern\n"
#define PING_COUNT			"Number of packets to send <1-10000>\n"
#define PING_TIMEOUT		"Ping timeout in seconds <1-60>\n"
#define PING_INTERVAL		"Specify the interval between pings in seconds <1-60>\n"
#define TOS					"Type of Service <0-255>\n"
#define INPUT				"Enter a number\n"
#define PING_IP_OPTION		"Specify the IP option - Record Route or Timestamp option\n"
#define TS					"Record the intermediate router timestamp\n"
#define TS_ADDR				"Record the intermediate router timestamp and IP address\n"
#define RECORD				"Record the intermediate router addresses\n"
#define TSONLY      		"include-timestamp"
#define TSADDR      		"include-timestamp-and-address"
#define RR          		"record-route"


#define PING_MAX_HOSTNAME_LENGTH 256

/* ping options default values(cisco val) */
#define PING_DEF_TIMEOUT       	2
#define PING_DEF_COUNT      	5
#define PING_DEF_SIZE   		100

/* default ping cmd */
#define PING4_DEF_CMD "ping"
#define PING6_DEF_CMD "ping6"

/* enum for type of arguments passed through cli */
typedef enum {
	IPv4_DESTINATION,
	IPv6_DESTINATION,
	DATA_FILL,
	DATA_SIZE,
	REPETITIONS,
	INTERVAL,
	TIMEOUT,
	TYPE_OF_SERVICE,
	IP_OPTION
}arguments;

/* structure to store the value of ping tokens */
typedef struct pingEntry_t {
	bool isIpv4;
	char pingTarget[PING_MAX_HOSTNAME_LENGTH+1];
	char *pingDataFill;
    uint16_t pingDataSize;
    uint8_t pingTimeout;
	uint8_t pingInterval;
	uint16_t pingRepetitions;
	uint8_t pingTos;
	bool includeTimestamp;
	bool includeTimestampAddress;
	bool recordRoute;
} pingEntry;

void printOutput( char *);
int decodeParam(const char*, arguments , pingEntry *);
bool ping_main(pingEntry *, void (*fPtr)(char *));
void ping_vty_init (void);

#endif /* _PING_VTY_H */
