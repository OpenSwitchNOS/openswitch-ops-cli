/* PING CLI commands
 *
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 * File: ping_vty.c
 *
 * Purpose: To add Ping CLI commands.
 */

#include <stdlib.h>
#include <stdbool.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "ping_vty.h"

/*-----------------------------------------------------------------------------
| Name : printOutput
| Responsibility : Display ping output
| Parameters : char* b : pointer to char
| Return : void
-----------------------------------------------------------------------------*/
void printOutput( char * buff)
{
	vty_out(vty,"%s %s", buff,VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Name : decodeParam
| Responsibility : Validate input parameters and store in pingEntry structure
| Parameters : const char* value : input argument array
|			   type : 1 for ipv4 , 0 for ipv6
			   pingEntry *p : pointer to structure
| Return : CMD_SUCCESS for success , CMD_WARNING for failure
-----------------------------------------------------------------------------*/
int decodeParam(const char* value[], int type ,pingEntry *p)
{
	struct in_addr addr,addr6;
	int length,i;
	char *store;

    switch(type)
    {
		case 0:
		{
			/* validation for IPv6 address/Hostname */
			if (isalpha((int) *value[0]))
            {
				if(strlen((char*)value[0]) > PING_MAX_HOSTNAME_LENGTH)
                {
					vty_out (vty, "hostname length should be less than 256 chars %s",VTY_NEWLINE);
					return CMD_WARNING;
				}
				strcpy(p->pingTarget, (char*)value[0]);
			}
			else if (inet_pton(AF_INET6, (char*)value[0], &addr6) <= 0)
            {
				vty_out (vty, "Invalid ipv6 address/Hostname %s",VTY_NEWLINE);
                return CMD_WARNING;
            }
			else
            {
				strcpy(p->pingTarget, (char*)value[0]);
			}
			break;
		}
		case 1:
		{
			/* Validations for IPv4 address/Hostname */
			if (isalpha((int) *value[0]))
            {
				if(strlen((char*)value[0]) > PING_MAX_HOSTNAME_LENGTH)
                {
					vty_out (vty, "hostname length should be less than 256 chars %s",VTY_NEWLINE);
					return CMD_WARNING;
				}
				p->isIpv4 = 1;
				strcpy(p->pingTarget, (char*)value[0]);
			}
			else if (inet_pton(AF_INET, (char*)value[0], &addr) <= 0)
            {
				vty_out (vty, "Invalid ipv4 address/Hostname %s",VTY_NEWLINE);
				return CMD_WARNING;
            }
			else
            {
				p->isIpv4 = 1;
				strcpy(p->pingTarget, (char*)value[0]);
			}
			break;
		}
		default : break;
	}

	/* common parameters for both ping4 and ping6 */

	/* Store value of Datasize */
	if(value[1])
        p->pingDataSize = atoi(value[1]);

    /* Store value of Datapattern */
    if(value[2])
    {
		length = strlen((char*)value[2]);
		store = (char*)value[2];
		p->pingDataFill = (char*)malloc(length+1);
		for(i=0;store[i]!='\0';i++)
		{
			if((isxdigit(store[i])) == 0)
				{
					vty_out (vty, "pattern for datafill should be in hex only %s",VTY_NEWLINE);
					return CMD_WARNING;
				}
				else
					strcpy(p->pingDataFill, (char*)value[2]);
		}
	}

    /* Store value of Repetitions(count) */
	if(value[3])
		p->pingRepetitions = atoi(value[3]);

	/* Store value of Interval */
	if(value[4])
        p->pingInterval = atoi(value[4]);
}

/*-----------------------------------------------------------------------------
| Defun for ping ipv4
| Responsibility : Display ping ipv4 parameters to the user,perform
| validation of input parameters and pass it to handler
-----------------------------------------------------------------------------*/
DEFUN (cli_ping,
       cli_ping_cmd,
	"ping ( A.B.C.D | WORD )"
	" { datagram-size <100-65468> | data-fill STR | repetitions <1-10000> | interval <1-60> |"
	"  timeout <1-60> |  tos <0-255> | ip-option (include-timestamp |include-timestamp-and-address |record-route )}",
    PING_STR
	PING_IP
	PING_HOST
	PING_DSIZE
    INPUT
	PING_PATTERN
	PATTERN
	PING_COUNT
    INPUT
	PING_INTERVAL
    INPUT
	PING_TIMEOUT
	INPUT
	TOS
	INPUT
	IP_OPTION
	TS
	TS_ADDR
	RECORD
	)
{
	pingEntry p;
	memset (&p, 0, sizeof (struct pingEntry_t));
	int ret_code=CMD_SUCCESS;

    /* Validations for Input parameters */
	ret_code=decodeParam(argv,1,&p);
	if( ret_code == CMD_WARNING )
		return CMD_WARNING;

    /* Store value of ping timeout */
    if(argv[5])
        p.pingTimeout = atoi(argv[5]);

    /* Store value of ping Tos */
    if(argv[6])
        p.pingTos = atoi(argv[6]);

    /* Store value of ping Ip-option */
    if(argv[7])
    {
        if (strcmp(TSONLY, (char*)argv[7]) == 0)
            p.includeTimestamp = 1;
		else if (strcmp(TSADDR, (char*)argv[7]) == 0)
            p.includeTimestampAddress = 1;
		else if (strcmp(RR, (char*)argv[7]) == 0)
            p.recordRoute = 1;
    }

	/* Wrapper for popen, output is printed by printOutput function */
	if(!ping_main(&p,printOutput))
		return CMD_WARNING;
	else
		return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for ping ipv6
| Responsibility : Display ping ipv6 parameters to the user and perform
| validation of input parameters
-----------------------------------------------------------------------------*/
DEFUN (cli_ping6,
       cli_ping6_cmd,
	"ping6 ( X:X::X:X | WORD )"
	" { datagram-size <100-65468> | data-fill STR | repetitions <1-10000> | interval <1-60> }",
    PING_STR
	PING_IP
	PING_HOST
	PING_DSIZE
    INPUT
    PING_PATTERN
	PATTERN
	PING_COUNT
    INPUT
	PING_INTERVAL
    INPUT
	)
{
    pingEntry p;
	int ret_code=CMD_SUCCESS;
    memset (&p, 0, sizeof (struct pingEntry_t));

	/* Validations for Input parameters */
    ret_code=decodeParam(argv,0,&p);
    if( ret_code == CMD_WARNING )
		return CMD_WARNING;

    if(!ping_main(&p,printOutput))
        return CMD_WARNING;
	else
		return CMD_SUCCESS;
}

 /* Install Ping related vty commands. */
void
ping_vty_init (void)
{
    install_element (ENABLE_NODE, &cli_ping_cmd);
	install_element (ENABLE_NODE, &cli_ping6_cmd);
}
