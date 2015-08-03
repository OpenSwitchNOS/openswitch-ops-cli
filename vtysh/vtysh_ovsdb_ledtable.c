/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
*/
/****************************************************************************
 * @ingroup quagga
 *
 * @file vtysh_ovsdb_ledtable.c
 * Source for registering client callback with led table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_ledtable.h"

#define DEFAULT_LED_STATE OVSREC_LED_STATE_OFF

char ledclientname[] = "vtysh_ovsdb_ledtable_clientcallback";

/***************************************************************************
* @function      : vtysh_ovsdb_ledtable_clientcallback
* @detail    : client callback routine for LED configuration
* @parame[in]
*   p_private: Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
*          structure object
* @return : e_vtysh_ok on success
***************************************************************************/
vtysh_ret_val vtysh_ovsdb_ledtable_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    struct ovsrec_led *pLedRow = NULL;

    OVSREC_LED_FOR_EACH(pLedRow,p_msg->idl)
    {
        if(pLedRow)
        {
            /* Assuming there is no misconfiguration, state can be on|off|flashing */
            if(0 != strcasecmp(pLedRow->state,DEFAULT_LED_STATE))
            {
                vtysh_ovsdb_cli_print(p_msg,"%s %s %s", "led",pLedRow->id,pLedRow->state);
            }
        }
    }

    return e_vtysh_ok;
}

/************************************************************************
* @function : vtysh_ovsdb_init_ledtableclients
* @responsibility : Registers the client callback routines for led table
* @return : 0 on success
*************************************************************************/
int vtysh_ovsdb_init_ledtableclients()
{
    vtysh_ovsdb_client client;

    client.p_client_name = ledclientname;
    client.client_id = e_vtysh_led_table_config;
    client.p_callback = &vtysh_ovsdb_ledtable_clientcallback;

    vtysh_ovsdbtable_addclient(e_led_table,e_vtysh_led_table_config,&client);

    return e_vtysh_ok;
}