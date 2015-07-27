/* FAN CLI commands file
 *
 * Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 * File: fan_vty.c
 *
 * Purpose:  To add fan CLI configuration and display commands.
 */


#include <sys/wait.h>
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "fan_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"

VLOG_DEFINE_THIS_MODULE(vtysh_fan_cli);
extern struct ovsdb_idl *idl;
/*-----------------------------------------------------------------------------
| @function       : vtysh_ovsdb_fan_speed_set
| @responsibility : set other_config:fan_speed_override in subsystem table
| @params
|    speed        : speed to be set
|    set          : boolean param to set or unset the speed
-----------------------------------------------------------------------------*/

static int vtysh_ovsdb_fan_speed_set( char* speed, boolean set )
{
    const struct ovsrec_subsystem *row = NULL;
    struct smap smap_other_config;

    if(!cli_do_config_start())
    {
        VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
        return CMD_OVSDB_FAILURE;
    }
    row = ovsrec_subsystem_first(idl);
    if(!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        cli_do_config_abort();
        return CMD_OVSDB_FAILURE;
    }
    smap_clone(&smap_other_config, &row->other_config);

    if(set && speed)
        smap_replace(&smap_other_config, FAN_SPEED_OVERRIDE_STR, speed);
    else
        smap_remove(&smap_other_config,FAN_SPEED_OVERRIDE_STR);
    ovsrec_subsystem_set_other_config(row,&smap_other_config);
    smap_destroy(&smap_other_config);
    if(cli_do_config_finish())
        return CMD_SUCCESS;
    else
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

DEFUN ( vtysh_fan_speed,
        vtysh_fan_speed_cmd,
        "fan-speed (slow | normal | medium | fast | max)",
        FAN_SET_STR
        " \n"
        " \n"
        " \n"
        " \n"
        " \n")
{
    return vtysh_ovsdb_fan_speed_set(argv[0], true);
}

DEFUN ( vtysh_no_fan_speed,
        vtysh_no_fan_speed_cmd,
        "no fan-speed",
        NO_STR
        FAN_SET_STR)
{
    return vtysh_ovsdb_fan_speed_set(NULL,false );
}

/*-----------------------------------------------------------------------------
| @function        : vtysh_ovsdb_fan_show
| @responsibility  : display fan table entries
-----------------------------------------------------------------------------*/

static void  vtysh_ovsdb_fan_show()
{
    const struct ovsrec_fan *row = NULL;
    const struct ovsrec_subsystem *subsysrow = NULL;
    const char* override = NULL ;
    int64_t rpm = 0 ;
    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);
    subsysrow = ovsrec_subsystem_first(idl);
    /* HALON_TODO: there could be a case where multiple subsystem rows could be present.
       In such case we may need to check for respective row of subsystem table*/
    if(subsysrow)
    {
        override = smap_get(&subsysrow->other_config, "fan_speed_override");
        OVSREC_FAN_FOR_EACH(row, idl)
        {
            if(row)
            {
                struct ovsdb_datum *at = ovsrec_fan_get_rpm(row,OVSDB_TYPE_INTEGER);
                if(at)
                    rpm=at->keys->integer;
                vty_out (vty,"%-13s%-7s%-15s%-14s%-8lld%s",
                 row->name,row->speed,((0 == strncmp(row->direction , "f2b",3))?"front-to-back":"back-to-front"),row->status,rpm,VTY_NEWLINE);
            }
            else
                VLOG_ERR("Couldn't retrieve any fan table rows");
        }
        vty_out(vty,"------------------------------------------------------%s",VTY_NEWLINE);
        if(override)
            vty_out(vty,"Fan speed override is set to : %s%s", override,VTY_NEWLINE);
        else
            vty_out(vty,"Fan speed override is not configured %s",VTY_NEWLINE);
        vty_out(vty,"------------------------------------------------------%s",VTY_NEWLINE);
    }
    else
    {
        VLOG_ERR("Couldn't retrieve any subsystem table rows");
    }
}


DEFUN (vtysh_show_system_fan,
       vtysh_show_system_fan_cmd,
       "show system fan",
       SHOW_STR
       SYS_STR
       FAN_STR)
{
    vty_out(vty,"%s%s","Fan information",VTY_NEWLINE);
    vty_out(vty,"------------------------------------------------------%s",VTY_NEWLINE);
    vty_out(vty,"%-13s%-7s%-15s%-14s%-8s%s","Name","Speed","Direction","Status","RPM",VTY_NEWLINE);
    vty_out(vty,"------------------------------------------------------%s",VTY_NEWLINE);
    vtysh_ovsdb_fan_show();
    return CMD_SUCCESS;
}



/*-----------------------------------------------------------------------------
| @function       : platform_vty_init
| @responsibility : install all the CLIs in the respective contexts.
-----------------------------------------------------------------------------*/

void fan_vty_init (void)
{
    install_element (VIEW_NODE, &vtysh_show_system_fan_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_fan_cmd);
    install_element (CONFIG_NODE, &vtysh_fan_speed_cmd);
    install_element (CONFIG_NODE, &vtysh_no_fan_speed_cmd);
}
