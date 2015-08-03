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
 * @file vtysh_ovsdb_subsystemntable.c
 * Source for registering client callback with subsystem table.
 *
 ***************************************************************************/

#include <zebra.h>

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "fan_vty.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_subsystemtable.h"


char subsysclientname[] = "vtysh_ovsdb_subsystemtable_clientcallback";

static vtysh_ret_val vtysh_ovsdb_subsystemtable_parse_othercfg(const struct smap *subsystemrow_config, vtysh_ovsdb_cbmsg *p_msg)
{
    const char *data = NULL;
    if(NULL == subsystemrow_config)
    {
        return e_vtysh_error;
    }
    data = smap_get(subsystemrow_config, FAN_SPEED_OVERRIDE_STR);
    if(data)
    {
        if(!(VTYSH_STR_EQ(data, "normal")))
            vtysh_ovsdb_cli_print(p_msg, "fan-speed %s",data);
    }
    return e_vtysh_error;
}


vtysh_ret_val vtysh_ovsdb_subsystemtable_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_subsystem *subsysrow;
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_subsystemtable_clientcallback entered");
    subsysrow = ovsrec_subsystem_first(p_msg->idl);
    if(subsysrow)
    {
    /* parse other config param */
        vtysh_ovsdb_subsystemtable_parse_othercfg(&subsysrow->other_config, p_msg);
    }
    return e_vtysh_error;
}


vtysh_ret_val vtysh_ovsdb_init_subsystemtableclients()
{
    vtysh_ovsdb_client client;
    client.p_client_name = subsysclientname;
    client.client_id = e_vtysh_subsystem_table_config;
    client.p_callback = &vtysh_ovsdb_subsystemtable_clientcallback;
    vtysh_ovsdbtable_addclient(e_subsystem_table, e_vtysh_subsystem_table_config, &client);
    return e_vtysh_ok;
}
