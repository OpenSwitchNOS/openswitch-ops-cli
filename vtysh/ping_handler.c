/* PING Handler file
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
 * File: ping_handler.c
 *
 * Purpose: Implemtation of ping and ping6 functionality.
 */

#include <stdint.h>
#include <stdio.h>
#include "ping.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(ping_handler);

/*---------------------------------------------------------------------------------------
| Name : ping_main
| Responsibility : Based on values passed in the structure, construct ping command
|                  and perform ping functionality
| Parameters : pingEntry* p : pointer to ping structure,
|              void (*fptr)(char *buff): function pointer for display purpose
| Return : returns true on successful execution, false for any error encountered
----------------------------------------------------------------------------------------*/
bool ping_main (pingEntry *p, void (*fPtr)(char *buff))
{
    char output[BUFSIZ], buffer[BUFSIZ];
    char *target = buffer;
    int len = 0;
    FILE *fp = NULL;

    if (!fPtr)
    {
        VLOG_ERR("function pointer passed is null");
        return false;
    }
    if (!p)
    {
        VLOG_ERR("Pointer to ping structure is null");
        return false;
    }

    len += sprintf(target+len, "%s ", SWNS_EXEC);

    /* Append default cmd either ping4 or ping6 */
    if (p->isIpv4)
        len += sprintf(target+len, "%s ", PING4_DEF_CMD);
    else
        len += sprintf(target+len, "%s ", PING6_DEF_CMD);

    /* Append Target address */
    if (p->pingTarget)
        len += sprintf(target+len, "%s", p->pingTarget);

    /* Append value repetitions(count) */
    if (!p->pingRepetitions)
        p->pingRepetitions = PING_DEF_COUNT;

    len += sprintf(target+len, " -c %d", p->pingRepetitions);

    /* Append value of packet size */
    if (!p->pingDataSize)
        p->pingDataSize = PING_DEF_SIZE;

    len += sprintf(target+len, " -s %d", p->pingDataSize);

    /* Append value of Interval */
    if (p->pingInterval)
        len += sprintf(target+len, " -i %d", p->pingInterval);

    /* Append value of datafill */
    if (p->pingDataFill)
        len += sprintf(target+len, " -p %s", p->pingDataFill);

    /* Ping4 options */
    if (p->isIpv4)
    {
        /* Append value of timeout */
        if (!p->pingTimeout)
            p->pingTimeout = PING_DEF_TIMEOUT;
        len += sprintf(target+len, " -W %d", p->pingTimeout);

        /* Append value of tos */
        if (p->pingTos)
            len += sprintf(target+len, " -T %d", p->pingTos);

        /* Ping4 ip-options */
        if (p->includeTimestamp)
            len += sprintf(target+len, " --ip-timestamp=tsonly ");
        else if (p->includeTimestampAddress)
            len += sprintf(target+len, " --ip-timestamp=tsaddr ");
        else if (p->recordRoute)
            len += sprintf(target+len, " -R ");
    }

    fp = popen(buffer,"r");
    if (fp)
    {
        while ( fgets( output, BUFSIZ, fp ) != NULL )
            (*fPtr)(output);
    }
    else
    {
        VLOG_ERR("Failed to open pipe stream");
        return false;
    }
    pclose(fp);
    return true;
}
