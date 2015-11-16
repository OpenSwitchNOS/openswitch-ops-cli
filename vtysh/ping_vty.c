/* PING CLI commands
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
 * File: ping_vty.c
 *
 * Purpose: To add Ping CLI commands.
 */

#include <stdlib.h>
#include <stdbool.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "ping.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(ping_vty);

/*-----------------------------------------------------------------------------
| Name : printOutput
| Responsibility : Display ping output
| Parameters : char* buff : pointer to char
| Return : void
-----------------------------------------------------------------------------*/
void printOutput( char * buff)
{
    vty_out (vty,"%s %s", buff, VTY_NEWLINE);
}

/*-----------------------------------------------------------------------------
| Name : decodeParam
| Responsibility : Validate input parameters and store in pingEntry structure
| Parameters : const char* value : input argument
|              argument type : ping token whose value is to be stored
               pingEntry *p : pointer to structure
| Return : CMD_SUCCESS for success , CMD_WARNING for failure
-----------------------------------------------------------------------------*/
int decodeParam(const char* value,arguments type, pingEntry* p)
{
    struct in_addr addr,addr6;
    int length =0,i;
    char *store;

    switch(type)
    {
        case IPv4_DESTINATION :
        {
            /* Validations for IPV4 Address/Hostname */
            if (isalpha((int) *value))
            {
                if(strlen((char*)value) > PING_MAX_HOSTNAME_LENGTH)
                {
                    vty_out (vty, "Invalid Hostname : Hostname length should be less \
                                   than 256 characters %s", VTY_NEWLINE);
                    return CMD_WARNING;
                }
                p->isIpv4 = true;
                p->pingTarget = (char*)value;
            }
            else if (inet_pton(AF_INET, (char*)value, &addr) <= 0)
            {
                vty_out (vty, "Invalid IPV4 Address %s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                p->isIpv4 = true;
                p->pingTarget = (char*)value;
            }
            break;
        }
        case IPv6_DESTINATION :
        {
            /* validation for IPV6 Address/Hostname */
            if (isalpha((int) *value))
            {
                if(strlen((char*)value) > PING_MAX_HOSTNAME_LENGTH)
                {
                    vty_out (vty, "Invalid Hostname : Hostname length should be less \
                                   than 256 characters %s", VTY_NEWLINE);
                    return CMD_WARNING;
                }
                p->pingTarget = (char*)value;
            }
            else if (inet_pton(AF_INET6, (char*)value, &addr6) <= 0)
            {
                vty_out (vty, "Invalid IPV6 Address %s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                p->pingTarget = (char*)value;
            }
            break;
        }
        case DATA_SIZE :
        {
            /* Store value of Datasize */
            if(value)
                p->pingDataSize = atoi(value);
            break;
        }
        case DATA_FILL :
        {
            /* Store value of Datapattern */
            if(value)
            {
                length = strlen((char*)value);
                store = (char*)value;
                for(i=0;store[i]!='\0';i++)
                {
                    if((isxdigit(store[i])) == 0)
                    {
                        vty_out (vty, "pattern for datafill should be in \
                            hexadecimal only %s", VTY_NEWLINE);
                        return CMD_WARNING;
                    }
                    else
                        p->pingDataFill = (char*)value;
                }
            }
            break;
        }
        case REPETITIONS :
        {
            /* Store value of Repetitions(count) */
            if(value)
                p->pingRepetitions = atoi(value);
            break;
        }
        case INTERVAL :
        {
            /* Store value of Interval */
            if(value)
                p->pingInterval = atoi(value);
            break;
        }
        case TIMEOUT :
        {
            /* Store value of ping timeout */
            if(value)
                p->pingTimeout = atoi(value);
            break;
        }
        case TYPE_OF_SERVICE :
        {
            /* Store value of ping Tos */
            if(value)
                p->pingTos = atoi(value);
            break;
        }
        case IP_OPTION :
        {
            /* Store value of ping Ip-option */
            if(value)
            {
                if (strcmp(TSONLY, (char*)value) == 0)
                    p->includeTimestamp = true;
                else if (strcmp(TSADDR, (char*)value) == 0)
                    p->includeTimestampAddress = true;
                else if (strcmp(RR, (char*)value) == 0)
                    p->recordRoute = true;
            }
            break;
        }
        default : break;
    }
    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for ping IPV4
| Responsibility : Display ping IPV4 parameters to the user,perform
| validation of input parameters and pass it to handler
-----------------------------------------------------------------------------*/
DEFUN (cli_ping,
       cli_ping_cmd,
    " ping ( A.B.C.D | WORD )"
    " { datagram-size <100-65468> | data-fill STR | repetitions <1-10000> | interval <1-60> |"
    "  timeout <1-60> |  tos <0-255> | ip-option (include-timestamp |include-timestamp-and-address"
    " |record-route )}",
    PING_STR
    PING_IP
    PING_HOST
    PING_DSIZE
    INPUT_DSIZE
    PING_PATTERN
    PATTERN
    PING_COUNT
    INPUT_COUNT
    PING_INTERVAL
    INPUT_INTERVAL
    PING_TIMEOUT
    INPUT_TIMEOUT
    TOS
    INPUT_TOS
    PING_IP_OPTION
    TS
    TS_ADDR
    RECORD
    )
{
    pingEntry p;
    memset (&p, 0, sizeof (struct pingEntry_t));

    /* decode token IPV4 Address/Hostname */
    if (decodeParam(argv[0],IPv4_DESTINATION,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IPV4 Address/Hostname failed");
        return CMD_WARNING;
    }

    /* decode token datagram-size */
    if (decodeParam(argv[1],DATA_SIZE,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token datagram-size failed");
        return CMD_WARNING;
    }

    /* decode token data-fill */
    if(decodeParam(argv[2],DATA_FILL,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token data-fill failed");
        return CMD_WARNING;
    }

    /* decode token repetitions */
    if(decodeParam(argv[3],REPETITIONS,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token repetition failed");
        return CMD_WARNING;
    }

    /* decode token interval */
    if (decodeParam(argv[4],INTERVAL,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token interval failed");
        return CMD_WARNING;
    }

    /* decode token timeout */
    if(decodeParam(argv[5],TIMEOUT,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token timeout failed");
        return CMD_WARNING;
    }

    /* decode token tos */
    if(decodeParam(argv[6],TYPE_OF_SERVICE,&p) != CMD_SUCCESS)
    {
       VLOG_ERR("Decoding of token tos failed");
        return CMD_WARNING;
    }

    /* decode token ip-options */
    if(decodeParam(argv[7],IP_OPTION,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token ip-option failed");
        return CMD_WARNING;
    }

    /* Wrapper for popen, output is printed by printOutput function */
    if(!ping_main(&p,printOutput))
    {
       VLOG_ERR("Call to handler failed");
       return CMD_WARNING;
    }

    return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
| Defun for ping IPV6
| Responsibility : Display ping IPV6 parameters to the user and perform
| validation of input parameters
-----------------------------------------------------------------------------*/
DEFUN (cli_ping6,
       cli_ping6_cmd,
    " ping6 ( X:X::X:X | WORD )"
    " { datagram-size <100-65468> | data-fill STR | repetitions <1-10000> "
    " | interval <1-60> }",
    PING_STR
    PING_IP
    PING_HOST
    PING_DSIZE
    INPUT_DSIZE
    PING_PATTERN
    PATTERN
    PING_COUNT
    INPUT_COUNT
    PING_INTERVAL
    INPUT_INTERVAL
    )
{
    pingEntry p;
    int ret_code=CMD_SUCCESS;
    memset (&p, 0, sizeof (struct pingEntry_t));

   /* decode token IPV6 Address/Hostname */
    if (decodeParam(argv[0],IPv6_DESTINATION,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token IPV6 Address/Hostname failed");
        return CMD_WARNING;
    }

   /* decode token datagram-size */
    if (decodeParam(argv[1],DATA_SIZE,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token datagram-size failed");
        return CMD_WARNING;
    }

    /* decode token data-fill */
    if(decodeParam(argv[2],DATA_FILL,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token data-fill failed");
        return CMD_WARNING;
    }

    /* decode token repetitions */
    if(decodeParam(argv[3],REPETITIONS,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token repetition failed");
        return CMD_WARNING;
    }

    /* decode token interval */
    if (decodeParam(argv[4],INTERVAL,&p) != CMD_SUCCESS)
    {
        VLOG_ERR("Decoding of token interval failed");
        return CMD_WARNING;
    }

    /* Wrapper for popen, output is printed by printOutput function */
    if(!ping_main(&p,printOutput))
    {
       VLOG_ERR("Call to handler failed");
       return CMD_WARNING;
    }

    return CMD_SUCCESS;
}

 /* Install Ping related vty commands. */
void
ping_vty_init (void)
{
    install_element (ENABLE_NODE, &cli_ping_cmd);
    install_element (ENABLE_NODE, &cli_ping6_cmd);
}
