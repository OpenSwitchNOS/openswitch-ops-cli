/* TRACEROUTE CLI commands
 *
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
#include "traceroute.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(traceroute_vty);

/*-----------------------------------------------------------------------------
| Name : printOutput
| Responsibility : Display traceroute output
| Parameters : char* buff : pointer to char
| Return : void
-----------------------------------------------------------------------------*/
void printOutput( char * buff)
{
    vty_out(vty, "%s %s", buff, VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Name : decodeTracerouteParam
| Responsibility : Validate input parameters and store in tracerouteEntry structure
| Parameters : const char* value : pointer to char
|              type : enum of arguments
               tracerouteEntry *p : pointer to structure
| Return : CMD_SUCCESS for success , CMD_WARNING for failure
-----------------------------------------------------------------------------*/
int decodeTracerouteParam(const char* value, arguments type, tracerouteEntry* p)
{
    struct in_addr addr,addr6;
    int length = 0;

    switch(type)
    {
        case DESTINATION:
        {
            /* validation for IPv4 address/Hostname */
            p->isIpv4 = true;
            if (isalpha((int) *value))
            {
                if(strlen((char*)value) > TRACEROUTE_MAX_HOSTNAME_LENGTH)
                {
                    vty_out (vty, "Invalid hostname."
                    "Length must be less than %d characters."
                    " %s", TRACEROUTE_MAX_HOSTNAME_LENGTH, VTY_NEWLINE);
                    return CMD_WARNING;
                }
                p->tracerouteTarget = (char*)value;
            }
            else if (inet_pton(AF_INET, (char*)value, &addr) <= 0)
            {
                vty_out (vty, "Invalid IPv4 address. %s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                p->tracerouteTarget = (char*)value;
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
                    vty_out (vty, "Invalid hostname."
                    "Length must be less than %d characters."
                    " %s", TRACEROUTE_MAX_HOSTNAME_LENGTH, VTY_NEWLINE);
                    return CMD_WARNING;
                }
                p->tracerouteTarget = (char*)value;
            }
            else if (inet_pton(AF_INET6, (char*)value, &addr6) <= 0)
            {
                vty_out (vty, "Invalid IPv6 address. %s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                p->tracerouteTarget = (char*)value;
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
                    vty_out (vty, "Invalid looseroute IP address. %s", VTY_NEWLINE);
                    return CMD_WARNING;
                }
                else
                {
                    p->tracerouteLoosesourceIp = (char*)value;
                }
            }
            break;
        }

        /* common parameters for both traceroute4 and traceroute6 */

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
    "traceroute ( A.B.C.D | WORD ) { dstport <1-34000> | maxttl <1-255> | "
    "minttl <1-255> | probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_DSTPORT
    DSTPORT_INPUT
    TRACEROUTE_MAXTTL
    MAXTTL_INPUT
    TRACEROUTE_MINTTL
    MINTTL_INPUT
    TRACEROUTE_PROBES
    PROBES_INPUT
    TRACEROUTE_TIMEOUT
    TIMEOUT_INPUT
    )
{
    tracerouteEntry p;
    int ret = CMD_SUCCESS;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* decode token IPv4 address/Hostname */
    if (decodeTracerouteParam(argv[0], DESTINATION, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IPv4 Address/Hostname failed");
        return CMD_SUCCESS;
    }
    /* decode token destination port */
    if (decodeTracerouteParam(argv[1], DST_PORT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token destination port failed");
        return CMD_SUCCESS;
    }

    /* decode token maximum ttl */
    if (decodeTracerouteParam(argv[2], MAX_TTL, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token maximum ttl failed");
        return CMD_SUCCESS;
    }

    /* decode token minimum ttl */
    if (decodeTracerouteParam(argv[3], MIN_TTL, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token minimum ttl failed");
        return CMD_SUCCESS;
    }

    /* decode token probes */
    if (decodeTracerouteParam(argv[4], PROBES, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token probes failed");
        return CMD_SUCCESS;
    }

    /* decode token timeout */
    if (decodeTracerouteParam(argv[5], TIME_OUT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token timeout failed");
        return CMD_SUCCESS;
    }

    /* handler for popen, output is printed by printOutput function */
    if(!traceroute_handler(&p, printOutput))
    {
        VLOG_ERR("Call to handler failed");
        return CMD_SUCCESS;
    }

    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for traceroute IPv4 ip-option loosesourceroute
| Responsibility : Display traceroute IPv4 parameters to the user,perform
| validation of input parameters and pass it to handler
-----------------------------------------------------------------------------*/
DEFUN (cli_traceroute_ipoption,
       cli_traceroute_ipoption_cmd,
    "traceroute ( A.B.C.D | WORD ) ip-option loosesourceroute A.B.C.D "
    "{ dstport <1-34000> | maxttl <1-255> | minttl <1-255> | "
    "probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_IP_OPTION
    TRACEROUTE_LOOSEROUTE
    LOOSEIP
    TRACEROUTE_DSTPORT
    DSTPORT_INPUT
    TRACEROUTE_MAXTTL
    MAXTTL_INPUT
    TRACEROUTE_MINTTL
    MINTTL_INPUT
    TRACEROUTE_PROBES
    PROBES_INPUT
    TRACEROUTE_TIMEOUT
    TIMEOUT_INPUT
    )
{
    tracerouteEntry p;
    int ret = CMD_SUCCESS;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* decode token IPv4 address/Hostname */
    if (decodeTracerouteParam(argv[0], DESTINATION, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IPv4 Address/Hostname failed");
        return CMD_SUCCESS;
    }

    /* decode token IP-option */
    if (decodeTracerouteParam(argv[1], IP_OPTION, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IP-option failed");
        return CMD_SUCCESS;
    }

    /* decode token destination port */
    if (decodeTracerouteParam(argv[2], DST_PORT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token destination port failed");
        return CMD_SUCCESS;
    }

    /* decode token maximum ttl */
    if (decodeTracerouteParam(argv[3], MAX_TTL, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token maximum ttl failed");
        return CMD_SUCCESS;
    }

    /* decode token minimum ttl */
    if (decodeTracerouteParam(argv[4], MIN_TTL, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token minimum ttl failed");
        return CMD_SUCCESS;
    }

    /* decode token probes */
    if (decodeTracerouteParam(argv[5], PROBES, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token probes failed");
        return CMD_SUCCESS;
    }

    /* decode token timeout */
    if (decodeTracerouteParam(argv[6], TIME_OUT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token timeout failed");
        return CMD_SUCCESS;
    }

    /* handler for popen, output is printed by printOutput function */
    if(!traceroute_handler(&p, printOutput))
    {
        VLOG_ERR("Call to handler failed");
        return CMD_SUCCESS;
    }

    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for traceroute IPv6
| Responsibility : Display traceroute Ipv6 parameters to the user and perform
| validation of input parameters
-----------------------------------------------------------------------------*/
DEFUN (cli_traceroute6,
       cli_traceroute6_cmd,
    "traceroute6 ( X:X::X:X | WORD ) { dstport <1-34000> | maxttl <1-255> | "
    "probes <1-5>| timeout <1-120>} ",
    TRACEROUTE_STR
    TRACEROUTE_IP
    TRACEROUTE_HOST
    TRACEROUTE_DSTPORT
    DSTPORT_INPUT
    TRACEROUTE_MAXTTL
    MINTTL_INPUT
    TRACEROUTE_PROBES
    PROBES_INPUT
    TRACEROUTE_TIMEOUT
    TIMEOUT_INPUT
    )
{
    int ret = CMD_SUCCESS;
    tracerouteEntry p;
    memset (&p, 0, sizeof (struct tracerouteEntry_t));

    /* decode token IPv6 address/Hostname */
    if (decodeTracerouteParam(argv[0], IPV6_DESTINATION, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IPv6 Address/Hostname failed");
        return CMD_SUCCESS;
    }

    /* decode token destination port */
    if (decodeTracerouteParam(argv[1], DST_PORT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token destination port failed");
        return CMD_SUCCESS;
    }

    /* decode token maximum ttl */
    if (decodeTracerouteParam(argv[2], MAX_TTL, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token maximum ttl failed");
        return CMD_SUCCESS;
    }

    /* decode token probes */
    if (decodeTracerouteParam(argv[3], PROBES, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token probes failed");
        return CMD_SUCCESS;
    }

    /* decode token timeout */
    if (decodeTracerouteParam(argv[4], TIME_OUT, &p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token timeout failed");
        return CMD_SUCCESS;
    }

   /* handler for popen, output is printed by printOutput function */
    if(!traceroute_handler(&p, printOutput))
    {
        VLOG_ERR("Call to handler failed");
        return CMD_SUCCESS;
    }

    return CMD_SUCCESS;
}

/* Install traceroute related vty commands */
void
traceroute_vty_init (void)
{
   install_element (ENABLE_NODE, &cli_traceroute_cmd);
   install_element (ENABLE_NODE, &cli_traceroute6_cmd);
   install_element (ENABLE_NODE, &cli_traceroute_ipoption_cmd);
}
