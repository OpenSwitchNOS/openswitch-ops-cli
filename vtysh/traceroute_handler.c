/* Traceroute CLI commands handler file
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
 * File: traceroute_handler.c
 *
 * Purpose: To perform traceroute functionality
 */

#include <stdint.h>
#include <stdio.h>
#include "traceroute.h"
#include "command.h"
#include "vtysh/vtysh.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(traceroute_handler);

/*---------------------------------------------------------------------------------------
| Name : traceroute_handler
| Responsibility :construct traceroute command and perform traceroute functionality
| Parameters : tracerouteEntry* p : pointer to traceroute structure,
|              void (*fptr)(char *buff): function pointer for display purpose
| Return : returns true on successful execution, false for any error encountered
----------------------------------------------------------------------------------------*/
bool traceroute_handler(tracerouteEntry *p, void (*fPtr)(char *buff))
{
    char output[BUFSIZ],buffer[BUFSIZ];
    char *target = buffer;
    int len = 0;
    FILE *fp = NULL;

    if(!fPtr)
    {
        VLOG_ERR("function pointer passed is null");
        return false;
    }
    if(!p)
    {
        VLOG_ERR("Pointer to traceroute structure is null");
        return false;
    }

    /*
     * As of now, traceroute is executed from swns namespace.
     * swns namespace is accessible only by root users.
     * Fix is added to display proper error message for other users.
     * This is a temporary fix, fix will be removed once RBAC is implemented.
     */

    fp = popen("whoami", "r");

    if (fp)
    {
        fgets(output, BUFSIZ, fp);
        pclose(fp);
        fp = NULL;

        /* Removing new line at the end of output */
        output[strlen(output)-1] = '\0';
        if (strcmp(output, "root") != 0)
        {
            (*fPtr)("Traceroute is supported only for root users.");
            return true;
        }
    }

    /*
     * Executing the command in the "swns" namespace, as the
     * interfaces visible in vtysh are from "swns".
     */
    len += sprintf(target+len, "%s ", SWNS_EXEC);

    /* Append default cmd either traceroute4 or traceroute6 */
    if(p->isIpv4)
    {
        len += sprintf(target+len, "%s ", TRACEROUTE4_DEF_CMD);
    }
    else
    {
        len += sprintf(target+len, "%s ", TRACEROUTE6_DEF_CMD);
    }

    /* Append Target address */
    if(p->tracerouteTarget)
    {
        len += sprintf(target+len, "%s", p->tracerouteTarget);
    }
    /* Append the value of destination port */
    if(!p->tracerouteDstport)
    {
        p->tracerouteDstport = TRACE_DEF_PORT;
    }
    len += sprintf(target+len, " -p %d", p->tracerouteDstport);

    /* Append the value of max TTL */
    if(!p->tracerouteMaxttl)
    {
        p->tracerouteMaxttl = TRACE_DEF_MAXTTL;
    }
    len += sprintf(target+len, " -m %d", p->tracerouteMaxttl);

    /* Append the value of probes */
    if(!p->tracerouteProbes)
    {
        p->tracerouteProbes = TRACE_DEF_PROBES;
    }
    len += sprintf(target+len, " -q %d", p->tracerouteProbes);

    /* Append the value of wait time */
    if(!p->tracerouteTimeout)
    {
        p->tracerouteTimeout = TRACE_DEF_WAIT;
    }
    len += sprintf(target+len, " -w %d", p->tracerouteTimeout);

    /* Traceroute4 options */
    if(p->isIpv4)
    {
        /* Append the value of min TTL */
        if(!p->tracerouteMinttl)
        {
            p->tracerouteMinttl = TRACE_DEF_MINTTL;
        }
        len += sprintf(target+len, " -f %d", p->tracerouteMinttl);

        /* Append the IP of loosesourceroute */
        if(p->tracerouteLoosesourceIp)
        {
            len += sprintf(target+len, " -g %s", p->tracerouteLoosesourceIp);
        }
    }
    fp = popen(buffer,"w");

    if(fp)
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
