/* Power Supply CLI commands
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
 * File: powersupply_vty.c
 *
 * Purpose:  To add power supply CLI configuration and display commands.
 */

#include "command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/powersupply_vty.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "memory.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"

VLOG_DEFINE_THIS_MODULE(vtysh_powersupply_cli);

extern struct ovsdb_idl *idl;

extern char *psu_state_string[];

/*
 * Function        : compare_psu
 * Resposibility    : Power Supply sort function for qsort
 * Parameters
 *   a   : Pointer to 1st element in the array
 *   b   : Pointer to next element in the array
 * Return      : comparative difference between names.
 */
static inline int
compare_psu(const void* a,const void* b)
{
    struct ovsrec_power_supply* s1 = (struct ovsrec_power_supply*)a;
    struct ovsrec_power_supply* s2 = (struct ovsrec_power_supply*)b;

    return (strcmp(s1->name,s2->name));
}

/*
 * Function        : format_psu_string
 * Resposibility   : Change status string in OVSDB to more
 *                   readable string
 * Parameters
 *      status  : Pointer to status string
 * Return      : Pointer to formatted status string
 */
static char*
format_psu_string(char* status)
{
    if (!status)
        return NULL;

    if (0 == strcmp(status,OVSREC_POWER_SUPPLY_STATUS_FAULT_ABSENT))
        return psu_state_string[POWER_SUPPLY_STATUS_FAULT_ABSENT];
    else if (0 == strcmp(status,OVSREC_POWER_SUPPLY_STATUS_FAULT_INPUT))
        return psu_state_string[POWER_SUPPLY_STATUS_FAULT_INPUT];
    else if (0 == strcmp(status,OVSREC_POWER_SUPPLY_STATUS_FAULT_OUTPUT))
        return psu_state_string[POWER_SUPPLY_STATUS_FAULT_OUTPUT];

    return status;
}

int
cli_system_get_psu()
{
	const struct ovsrec_power_supply* pPSU = NULL;
	struct ovsrec_power_supply* pPSUsort = NULL;
	const struct ovsrec_subsystem* pSys = NULL;

	int nPres = 0,nOK = 0,i = 0, n = 0;
	/*
	 * OPS_TODO : Support for multiple subsystem
	 */
	pSys = ovsrec_subsystem_first (idl);
	if (pSys)
		n = pSys->n_power_supplies;
	else
		return CMD_OVSDB_FAILURE;

	if (n > 0)
	{
		pPSUsort = (struct ovsrec_power_supply*)calloc(n,
				sizeof(struct ovsrec_power_supply));
		if (!pPSUsort)
			return CMD_OVSDB_FAILURE;
	}

	vty_out(vty,"%s",VTY_NEWLINE);
	vty_out(vty,"%-15s%-10s%s","Name","Status",VTY_NEWLINE);
	vty_out(vty,"%s%s","-----------------------------",VTY_NEWLINE);
	OVSREC_POWER_SUPPLY_FOR_EACH(pPSU,idl)
	{
		if (pPSU)
		{
			memcpy(pPSUsort+i,pPSU,sizeof(struct ovsrec_power_supply));
			i++;
		}
	}
	if (n > 0)
		qsort((void*)pPSUsort,n,sizeof(struct ovsrec_power_supply),
				compare_psu);

	for (i = 0; i < n ; i++)
	{
		vty_out(vty,"%-15s",(pPSUsort+i)->name);
		vty_out(vty,"%-10s",format_psu_string((pPSUsort+i)->status));
		if (0 != strcasecmp((pPSUsort+i)->status,
				OVSREC_POWER_SUPPLY_STATUS_FAULT_ABSENT))
			nPres++;
		if  (0 == strcasecmp((pPSUsort+i)->status,
				OVSREC_POWER_SUPPLY_STATUS_OK))
			nOK++;
		vty_out(vty,"%s",VTY_NEWLINE);
	}
	vty_out(vty,"%s",VTY_NEWLINE);

	if(pPSUsort)
	{
		free(pPSUsort);
		pPSUsort = NULL;
	}
	return CMD_SUCCESS;
}


DEFUN (cli_platform_show_psu,
        cli_platform_show_psu_cmd,
        "show system power-supply",
        SHOW_STR
        SYS_STR
        PSU_STR)
{
	return cli_system_get_psu();
}
/*
 * Function : powersupply_vty_init
 * Resposibility : Install the cli action routines
 */
void
powersupply_vty_init()
{
	install_element (ENABLE_NODE, &cli_platform_show_psu_cmd);
}
