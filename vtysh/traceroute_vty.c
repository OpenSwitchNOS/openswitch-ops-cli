/* TRACEROUTE CLI commands
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
 * File: traceroute_vty.c
 *
 * Purpose: To add Traceroute CLI commands.
 */

#include <stdlib.h>
#include <stdbool.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "traceroute_vty.h"

/*-----------------------------------------------------------------------------
| Name : printOutput
| Responsibility : Display traceroute output
| Parameters : char* b : pointer to char
| Return : void
-----------------------------------------------------------------------------*/
void printOutput( char * buff)
{
    vty_out(vty,"%s %s", buff,VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Name : validateTracerouteTarget
| Responsibility : Validate input parameters and store in tracerouteEntry structure
| Parameters : const char* value : input argument
|               type : enum of arguments
               tracerouteEntry *p : pointer to structure
| Return : CMD_SUCCESS for success , CMD_WARNING for failure
-----------------------------------------------------------------------------*/
int validateTracerouteTarget(const char* value,arguments type, tracerouteEntry* p)
{
    struct in_addr addr,addr6;

    switch(type)
    {
        case DESTINATION:
        {
            /* validation for IPv4 address/Hostname */
            if (isalpha((int) *value))
            {
                if(strlen((char*)value) > TRACEROUTE_MAX_HOSTNAME_LENGTH)
                {
                    vty_out (vty, "hostname should be less than 256 chars %s",VTY_NEWLINE);
                    return CMD_WARNING;
                }
            p->isIpv4 = 1;
            strcpy(p->tracerouteTarget, (char*)value);
            }
            else if (inet_pton(AF_INET, (char*)value, &addr) <= 0)
            {
                vty_out (vty, "Invalid ipv4 address %s",VTY_NEWLINE);
                return CMD_WARNING;
                }
            else
            {
                p->isIpv4 = 1;
                strcpy(p->tracerouteTarget, (char*)value);
            }
            break;
        }

        case IPV6_DESTINATION:
        {
            /* validation for IPv6 address/Hostname */
            if (isalpha((int) *value))
            {
                if(strlen((char*)value) > TRACEROUTE_MAX_HOSTNAME_LENGTH)
                {
                    vty_out (vty, "hostname should be less than 256 chars %s",VTY_NEWLINE);
                    return CMD_WARNING;
                }
                strcpy(p->tracerouteTarget, (char*)value);
            }
            else if (inet_pton(AF_INET, (char*)value, &addr6) <= 0)
            {
                vty_out (vty, "Invalid ipv6 address %s",VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                strcpy(p->tracerouteTarget, (char*)value);
            }
            break;
        }

        case IP_OPTION:
        {
            /* Validations of loosesourceroute ip-option for IPv4 address/Hostname */
            if(value)
            {
                if (inet_pton(AF_INET, (char*)value, &addr) <= 0)
                {
                vty_out (vty, "Invalid looseroute IP address address %s",VTY_NEWLINE);
                return CMD_WARNING;
                }
                else
                {
                    strcpy(p->tracerouteLoosesourceIp, (char*)value);
                }
            }
            break;
        }

        /* common parameters for both ping4 and ping6 */

        case DST_PORT:
        {
            /* Store value of destination port */
            if(value)
            {
                p->tracerouteDstport = atoi(value);
            }
            break;
        }

        case MAX_TTL:
        {
            /* Store value of maximum ttl */
            if(value)
            {
                p->tracerouteMaxttl = atoi(value);
            }
            break;
        }

        case MIN_TTL:
        {
            /* Store value of minimum ttl */
            if(value)
            {
                p->tracerouteMinttl = atoi(value);
            }
            break;
        }

        case PROBES:
        {
            /* Store value of number of probes */
            if(value)
            {
                p->tracerouteProbes = atoi(value);
            }
            break;
        }

        case TIME_OUT:
        {
            /* Store value of wait time */
            if(value)
            {
                p->tracerouteTimeout = atoi(value);
            }
            break;
        }

        default : break;
    }

return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for traceroute ipv4
| Responsibility : Display traceroute ipv4 parameters to the user,perform
| validation of input parameters and pass it to handler
-----------------------------------------------------------------------------*/
DEFUN (cli_traceroute,
       cli_traceroute_cmd,
    "traceroute ( A.B.C.D | WORD ) { dstport <1-34000> | maxttl <1-255> | minttl <1-255> | probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_DSTPORT
    INPUT
    TRACEROUTE_MAXTTL
    INPUT
    TRACEROUTE_MINTTL
    INPUT
    TRACEROUTE_PROBES
    INPUT
    TRACEROUTE_TIMEOUT
    INPUT
    )
{
    tracerouteEntry p;
    int ret = CMD_SUCCESS;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* Validations for Input parameters */
    ret = validateTracerouteTarget(argv[0],DESTINATION,&p);
    ret = validateTracerouteTarget(argv[1],DST_PORT,&p);
    ret = validateTracerouteTarget(argv[2],MAX_TTL,&p);
    ret = validateTracerouteTarget(argv[3],MIN_TTL,&p);
    ret = validateTracerouteTarget(argv[4],PROBES,&p);
    ret = validateTracerouteTarget(argv[5],TIME_OUT,&p);

    /* handler for popen, output is printed by printOutput function */
    if(!traceroute_handler(&p,printOutput))
    {
        ret = CMD_WARNING;
        return ret;
    }
    else
    {
        ret = CMD_SUCCESS;
        return ret;
    }
}

/*-----------------------------------------------------------------------------
| Defun for traceroute ipv4 ip-option loosesourceroute
| Responsibility : Display traceroute ipv4 parameters to the user,perform
| validation of input parameters and pass it to handler
-----------------------------------------------------------------------------*/
DEFUN (cli_traceroute_ipoption,
       cli_traceroute_ipoption_cmd,
    "traceroute ( A.B.C.D | WORD ) ip-option loosesourceroute A.B.C.D { dstport <1-34000> | maxttl <1-255> | minttl <1-255> | probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_IP_OPTION
    TRACEROUTE_LOOSEROUTE
    LOOSEIP
    TRACEROUTE_DSTPORT
    INPUT
    TRACEROUTE_MAXTTL
    INPUT
    TRACEROUTE_MINTTL
    INPUT
    TRACEROUTE_PROBES
    INPUT
    TRACEROUTE_TIMEOUT
    INPUT
    )
{
    tracerouteEntry p;
    int ret = CMD_SUCCESS;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* Validations for Input parameters */
    ret = validateTracerouteTarget(argv[0],DESTINATION,&p);
    ret = validateTracerouteTarget(argv[1],IP_OPTION,&p);
    ret = validateTracerouteTarget(argv[2],DST_PORT,&p);
    ret = validateTracerouteTarget(argv[3],MAX_TTL,&p);
    ret = validateTracerouteTarget(argv[4],MIN_TTL,&p);
    ret = validateTracerouteTarget(argv[5],PROBES,&p);
    ret = validateTracerouteTarget(argv[6],TIME_OUT,&p);

    if(!traceroute_handler(&p,printOutput))
    {
        ret = CMD_WARNING;
        return ret;
    }
    else
    {
        ret = CMD_SUCCESS;
        return ret;
    }
}

/*-----------------------------------------------------------------------------
| Defun for traceroute ipv6
| Responsibility : Display traceroute ipv6 parameters to the user and perform
| validation of input parameters
-----------------------------------------------------------------------------*/
DEFUN (cli_traceroute6,
       cli_traceroute6_cmd,
    "traceroute6 ( X:X::X:X | WORD ) { dstport <1-34000> | maxttl <1-255> | minttl <1-255> | probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_DSTPORT
    INPUT
    TRACEROUTE_MAXTTL
    INPUT
    TRACEROUTE_MINTTL
    INPUT
    TRACEROUTE_PROBES
    INPUT
    TRACEROUTE_TIMEOUT
    INPUT
    )
{
    int ret = CMD_SUCCESS;
    tracerouteEntry p;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* Validations for Input parameters */
    ret = validateTracerouteTarget(argv[0],IPV6_DESTINATION,&p);
    ret = validateTracerouteTarget(argv[1],DST_PORT,&p);
    ret = validateTracerouteTarget(argv[2],MAX_TTL,&p);
    ret = validateTracerouteTarget(argv[3],MIN_TTL,&p);
    ret = validateTracerouteTarget(argv[4],PROBES,&p);
    ret = validateTracerouteTarget(argv[5],TIME_OUT,&p);

   /* handler for popen, output is printed by printOutput function */
    if(!traceroute_handler(&p,printOutput))
    {
        ret = CMD_WARNING;
        return ret;
    }
    else
    {
        ret = CMD_SUCCESS;
        return ret;
    }
}

/* Install traceroute related vty commands. */
void
traceroute_vty_init (void)
{
   install_element (ENABLE_NODE, &cli_traceroute_cmd);
   install_element (ENABLE_NODE, &cli_traceroute6_cmd);
   install_element (ENABLE_NODE, &cli_traceroute_ipoption_cmd);
}
