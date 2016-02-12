/* Ping CLI commands header file
 *
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
 * File: ping.h
 *
 * Purpose:  To add declarations required for ping_vty.c
 */

#ifndef _PING_H
#define _PING_H

#include <stdbool.h>

#define PING_STR            "Send ping requests to a device on the network\n"
#define PING_IP             "Enter IP address of the device to ping\n"
#define PING_HOST           "Enter Hostname of the device to ping\n"
#define PATTERN             "Enter Hexadecimal pattern (Default: AB)\n"
#define TOS                 "Type of Service \n"
#define INPUT_TOS           "Enter TOS value in the range \n"
#define TS                  "Record the intermediate router timestamp\n"
#define RECORD              "Record the intermediate router addresses\n"
#define TSONLY              "include-timestamp"
#define TSADDR              "include-timestamp-and-address"
#define RR                  "record-route"

#define PING6_STR \
"Send IPv6 ping requests to a device on the network\n"
#define PING_IP_OPT \
"Specify the IP option - Record Route or Timestamp option\n"
#define PING_DSIZE \
"Ping datagram size \n"
#define PING_INTERVAL \
"Specify the interval between successive ping requests in seconds\n"
#define PING_PATTERN \
"Specify the data pattern in hexadecimal digits to send\n"
#define PING_COUNT \
"Number of packets to send\n"
#define PING_TIMEOUT \
"Ping timeout in seconds\n"
#define TS_ADDR \
"Record the intermediate router timestamp and IP address\n"
#define INPUT_DSIZE  \
"Enter Datagram size value in the range. (Default: 100 bytes)\n"
#define INPUT_TIMEOUT \
"Enter timeout value in the range in seconds. (Default: 2 seconds)\n"
#define INPUT_INTERVAL \
"Enter interval value in the range in seconds. (Default: 1 second)\n"
#define INPUT_COUNT \
"Enter Repetition value in the range. (Default: 5)\n"

#define PING_MAX_HOSTNAME_LENGTH 256
#define MAX_PATTERN_LENGTH       16

/* ping options default values */
#define PING_DEF_TIMEOUT        2
#define PING_DEF_COUNT          5
#define PING_DEF_SIZE           100

/* default ping cmd */
#define PING4_DEF_CMD       "ping"

/* ping6 : use -n to suppress lookup of symbolic names.
Display numeric output only */
#define PING6_DEF_CMD       "ping6 -n"

/* cmd to execute in namespace swns */
#define SWNS_EXEC            "/sbin/ip netns exec swns"

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
    PING_IP_OPTION
} pingArguments;

/* structure to store the value of ping tokens */
typedef struct pingEntry_t {
    bool isIpv4;
    char *pingTarget;
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

/* prototypes of functions */
void printPingOutput (char *);
int decodeParam (const char*, pingArguments, pingEntry *);
bool ping_main (pingEntry *, void (*fPtr)(char *));
void ping_vty_init (void);

#endif /* _PING_H */
