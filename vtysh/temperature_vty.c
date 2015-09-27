/* Temperature CLI commands
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
 * File: temperature_vty.c
 *
 * Purpose: To add temperature CLI configuration and display commands
 */

#include <sys/wait.h>
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "temperature_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"

VLOG_DEFINE_THIS_MODULE(vtysh_temperature_cli);
extern struct ovsdb_idl *idl;

/*
 * Function     : vtysh_ovsdb_show_temp_sensor
 * Responsibility : display temperature sensor information
 * Parameters
 *    detail   : boolean param to decide whether detailed description or brief
 description
 */

static void
vtysh_ovsdb_show_temp_sensor (boolean detail)
{
    const struct ovsrec_temp_sensor *row;
    OVSREC_TEMP_SENSOR_FOR_EACH (row, idl)
    {
        if (row)
        {
            if (!detail)
            {
                vty_out (vty,"%-10s%-15.2f%-15s%-10s%s",
                        row->name,((row->temperature)/1000.0),
                        row->status, row->fan_state,VTY_NEWLINE);
            }
            else
            {
                vty_out(vty,"%-26s:%s %s","Name",row->name,VTY_NEWLINE);
                vty_out(vty,"%-26s:%s %s","Location",row->location,
                        VTY_NEWLINE);
                vty_out(vty,"%-26s:%s %s","Status",row->status,VTY_NEWLINE);
                vty_out(vty,"%-26s:%s %s",
                        "Fan-state",row->fan_state,VTY_NEWLINE);
                vty_out(vty,"%-26s:%.2f%s",
                        "Current temperature(in C)",
                        ((row->temperature)/1000.0),VTY_NEWLINE);
                vty_out(vty,"%-26s:%.2f%s",
                        "Minimum temperature(in C)",
                        ((row->min)/1000.0),VTY_NEWLINE);
                vty_out(vty,"%-26s:%.2f%s",
                        "Maximum temperature(in C)",
                        ((row->max)/1000.0),VTY_NEWLINE);
                vty_out(vty,"%s",VTY_NEWLINE);
            }
        }
        else
        {
            VLOG_ERR("Unable to retrieve Temp_sensor table rows");
        }
    }
}
DEFUN (vtysh_show_system_temperature_detail,
        vtysh_show_system_temperature_detail_cmd,
        "show system temperature detail",
        SHOW_STR
        SYS_STR
        TEMP_STR
        TEMP_DETAIL_STR)
{
    vty_out(vty,"%s%s","Detailed temperature information",VTY_NEWLINE);
    vty_out(vty,"---------------------------------------------------%s",
            VTY_NEWLINE);
    vtysh_ovsdb_show_temp_sensor (true);
    return CMD_SUCCESS;
}

DEFUN (vtysh_show_system_temperature,
        vtysh_show_system_temperature_cmd,
        "show system temperature",
        SHOW_STR
        SYS_STR
        TEMP_STR)
{
    vty_out(vty,"%s%s","Temperature information",VTY_NEWLINE);
    vty_out(vty,"---------------------------------------------------%s",
            VTY_NEWLINE);
    vty_out(vty,"%-12s%-9s%s"," ","Current",VTY_NEWLINE);
    vty_out(vty,"%-10s%-15s%-15s%-10s%s","Name","temperature",
            "Status","Fan state",VTY_NEWLINE);
    vty_out(vty,"%-12s%-6s%s"," ","(in C)",VTY_NEWLINE);
    vty_out(vty,"---------------------------------------------------%s",
            VTY_NEWLINE);
    vtysh_ovsdb_show_temp_sensor (false);
    return CMD_SUCCESS;
}
/*
 * Function     : platform_vty_init
 * Responsibility : install all the CLIs in the respective contexts.
 */

void
temperature_vty_init (void)
{
    install_element (VIEW_NODE, &vtysh_show_system_temperature_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_temperature_cmd);
    install_element (VIEW_NODE, &vtysh_show_system_temperature_detail_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_temperature_detail_cmd);
}
