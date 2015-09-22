/* System LED CLI commands
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
 * File: led_vty.c
 *
 * Purpose:  To add system LED CLI configuration and display commands.
 */

#include "command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/led_vty.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "memory.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"

VLOG_DEFINE_THIS_MODULE (vtysh_led_cli);

extern struct ovsdb_idl *idl;

const char *led_state_strings[] = {
    OVSREC_LED_STATE_FLASHING,          /*!< LED state "flashing" */
    OVSREC_LED_STATE_OFF,               /*!< LED state "off" */
    OVSREC_LED_STATE_ON                 /*!< LED state "on" */
};

/*
 * Function    : lookup_led
 * Resposibility  : Lookup for led using name
 * Parameter
 *     name    : Pointer to name of led character string
 * Return  : Pointer to ovsrec_led structure object
 */
static const struct ovsrec_led *
lookup_led (const char *name)
{
    const static struct ovsrec_led *led;

    OVSREC_LED_FOR_EACH (led, idl) {
        if (strcmp(led->id, name) == 0) {
            return((struct ovsrec_led *)led);
        }
    }

    return (NULL);
}

/*
 * Function        : cli_system_get_led
 * Resposibility      : Get system led information from idl
 * Return      : 0 on success 1 otherwise
 */

int
cli_system_get_led ()
{
    const struct ovsrec_led* pLed = NULL;
    const struct ovsrec_subsystem* pSys = NULL;

    pSys = ovsrec_subsystem_first (idl);

    vty_out(vty,"%-15s%-10s%-10s%s","Name","State","Status",VTY_NEWLINE);
	vty_out(vty,"%s%s","-----------------------------------",VTY_NEWLINE);

    if (pSys->n_leds)
    {
        OVSREC_LED_FOR_EACH (pLed,idl)
        {
            if (pLed)
            {
                vty_out(vty,"%-15s",pLed->id);
                vty_out(vty,"%-10s",pLed->state);
                vty_out(vty,"%-10s",pLed->status);
            }
            vty_out(vty,"%s",VTY_NEWLINE);
        }
    }

    return CMD_SUCCESS;
}

/*
 * Function        : cli_system_set_led
 * Resposibility      : Set system led state
 * Parameters
 *  sLedName: Pointer to led name string
 *  sLedState: Pointer to led state string
 * Return      : 0 on success 1 otherwise
 */

int
cli_system_set_led (char* sLedName,char* sLedState)
{
    const struct ovsrec_led* pOvsLed = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    pOvsLed = lookup_led (sLedName);

    if (pOvsLed)
    {
        status_txn = cli_do_config_start();
        if (status_txn != NULL)
        {
            ovsrec_led_set_state (pOvsLed, sLedState);
            status = cli_do_config_finish (status_txn);
            if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
            {
                return CMD_SUCCESS;
            }
            else
            {
                VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
                return CMD_OVSDB_FAILURE;
            }
        }
        else
        {
            VLOG_ERR("Unable to acquire transaction");
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
        }

    }
    else
    {
		vty_out(vty,"Cannot find LED%s",VTY_NEWLINE);
    }
    return CMD_SUCCESS;
}

/*
 * Func        : cli_system_no_set_led
 * Resposibility      : Set system led state to default
 * Parameters
 *      sLedName: Pointer to led name string
 * Return      : 0 on success 1 otherwise
 */

int
cli_system_no_set_led (char* sLedName)
{
    const struct ovsrec_led* pOvsLed = NULL;
    struct ovsdb_idl_txn* status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    pOvsLed = lookup_led(sLedName);

    if (pOvsLed)
    {
        status_txn = cli_do_config_start();
	if (status_txn != NULL)
	{
           ovsrec_led_set_state (pOvsLed, OVSREC_LED_STATE_OFF);
           status = cli_do_config_finish(status_txn);
           if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
           {
              return CMD_SUCCESS;
           }
           else
           {
              VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
              return CMD_OVSDB_FAILURE;
           }
	}
	else
	{
           VLOG_ERR("Unable to acquire transaction");
           cli_do_config_abort(status_txn);
           return CMD_OVSDB_FAILURE;
	}

    }
    else
    {
        vty_out(vty,"Cannot find LED%s",VTY_NEWLINE);
    }

    return CMD_SUCCESS;

}


/*
 * Action routines for LED related CLIs
 */
DEFUN (cli_platform_show_led,
		cli_platform_show_led_cmd,
		"show system led",
		SHOW_STR
		SYS_STR
		LED_STR)
{
    return cli_system_get_led();
}

DEFUN (cli_platform_set_led,
        cli_platform_set_led_cmd,
        "led WORD (on|off|flashing)",
        LED_SET_STR
        "Name of LED e.g. <base-loc> for locator LED\n"
        "Switch on the LED\n"
        "Switch off the LED (Default)\n"
        "Blink the LED\n")
{
    return cli_system_set_led (CONST_CAST(char*,argv[0]),
			CONST_CAST(char*,argv[1]));
}


DEFUN (no_cli_platform_set_led,
        no_cli_platform_set_led_cmd,
        "no led WORD",
        NO_STR
        LED_SET_STR
        "Name of LED e.g. <base-loc> for locator LED\n")
{
    return cli_system_no_set_led (CONST_CAST(char*,argv[0]));
}

/*
 * Function        : led_vty_init
 * Resposibility     : Install the cli action routines
 */
void
led_vty_init()
{
    install_element (ENABLE_NODE, &cli_platform_show_led_cmd);
    install_element (VIEW_NODE, &cli_platform_show_led_cmd);
    install_element (CONFIG_NODE, &cli_platform_set_led_cmd);
    install_element (CONFIG_NODE, &no_cli_platform_set_led_cmd);
}
