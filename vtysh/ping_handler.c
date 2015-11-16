#include<stdint.h>
#include<stdio.h>
#include "ping_vty.h"

/*---------------------------------------------------------------------------------------
| Name : ping_main
| Responsibility : Based on values passed in the structure, construct ping command
|                  and perform ping functionality
| Parameters : pingEntry* p : pointer to ping structure,
|              void (*fptr)(char *buff): function pointer for display purpose
| Return : returns true on successful execution, false for any error encountered
----------------------------------------------------------------------------------------*/
bool ping_main(pingEntry *p,void (*fPtr)(char *buff))
{
	char output[BUFSIZ],buffer[BUFSIZ];
	char *target = buffer;
	int len=0;
    FILE *fp=NULL;

    if(!p || !fPtr)
        return false;

    /* Append default cmd either ping4 or ping6 */
	if(p->isIpv4)
		len += sprintf(target+len, "%s ", PING4_DEF_CMD);
	else
        len += sprintf(target+len, "%s ", PING6_DEF_CMD);

    /* Append Target address */
	if(p->pingTarget)
		len += sprintf(target+len, "%s", p->pingTarget);

    /* Append value repetitions(count) */
    if(!p->pingRepetitions)
		p->pingRepetitions = PING_DEF_COUNT;

	len += sprintf(target+len, " -c %d", p->pingRepetitions);

    /* Append value of packet size */
    if(!p->pingDataSize)
        p->pingDataSize = PING_DEF_SIZE;

    len += sprintf(target+len, " -s %d", p->pingDataSize);

    /* Append value of Interval */
    if(p->pingInterval)
        len += sprintf(target+len, " -i %d", p->pingInterval);

    /* Append value of datafill */
    if(p->pingDataFill)
        len += sprintf(target+len, " -p %s", p->pingDataFill);

    /* Ping4 options */
	if(p->isIpv4)
	{
        /* Append value of timeout */
        if(!p->pingTimeout)
			p->pingTimeout = PING_DEF_TIMEOUT;
		len += sprintf(target+len, " -W %d", p->pingTimeout);

        /* Append value of tos */
		if(p->pingTos)
            len += sprintf(target+len, " -T %d", p->pingTos);

        /* Ping4 ip-options */
        if(p->includeTimestamp)
            len += sprintf(target+len, " --ip-timestamp=tsonly ");
        else if (p->includeTimestampAddress)
            len += sprintf(target+len, " --ip-timestamp=tsaddr ");
        else if (p->recordRoute)
            len += sprintf(target+len, " -R ");
	}

    fp = popen(buffer,"r");
	if(fp)
	{
        while ( fgets( output, BUFSIZ, fp ) != NULL )
            (*fPtr)(output);
	}
	else
    {
	    (*fPtr)("Failed to Open pipe stream");
	    return false;
	}
	pclose(fp);
    return true;
}
