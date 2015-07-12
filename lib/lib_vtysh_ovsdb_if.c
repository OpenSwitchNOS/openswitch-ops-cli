/* Vtysh daemon ovsdb integration.
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
 * File: lib_vtysh_ovsdb_if.c
 *
 * Purpose: Library functions for integrating vtysh with ovsdb.
 */

#include <stdio.h>
#include <regex.h>

#include "vswitch-idl.h"
#include "util.h"
#include "unixctl.h"
#include "config.h"
#include "command-line.h"
#include "daemon.h"
#include "dirs.h"
#include "fatal-signal.h"
#include "poll-loop.h"
#include "timeval.h"
#include "openvswitch/vlog.h"
#include "coverage.h"

#include "openhalon-idl.h"
#include "lib_vtysh_ovsdb_if.h"

typedef unsigned char boolean;

VLOG_DEFINE_THIS_MODULE(lib_vtysh_ovsdb_if);

static struct ovsdb_idl *idl;
static unsigned int idl_seqno;
static char *appctl_path = NULL;
static struct unixctl_server *appctl;
static struct ovsdb_idl_txn *txn;

boolean exiting = false;

/*
 * Running idl run and wait to fetch the data from the DB
 */
static void
lib_vtysh_run()
{
    while(!ovsdb_idl_has_lock(idl))
    {
        ovsdb_idl_run(idl);
        unixctl_server_run(appctl);

        ovsdb_idl_wait(idl);
        unixctl_server_wait(appctl);
    }
}

/*
 * Create a connection to the OVSDB at db_path and create
 * the idl cache.
 */
static void
lib_ovsdb_init(const char *db_path)
{
    idl = ovsdb_idl_create(db_path, &ovsrec_idl_class, false, true);
    ovsdb_idl_set_lock(idl, "lib_halon_vtysh");
    idl_seqno = ovsdb_idl_get_seqno(idl);

    ovsdb_idl_add_table(idl, &ovsrec_table_interface);
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_table(idl, &ovsrec_table_vlan);

    ovsdb_idl_add_column(idl, &ovsrec_interface_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_name);

    lib_vtysh_run();
}

static void
halon_lib_vtysh_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
                    const char *argv[] OVS_UNUSED, void *exiting_)
{
    boolean *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}

/*
 * The init for the ovsdb integration called in vtysh main function
 */
void lib_vtysh_ovsdb_init(int argc, char *argv[])
{
    int retval;
    char *ovsdb_sock;

    set_program_name(argv[0]);
    proctitle_init(argc, argv);
    fatal_ignore_sigpipe();

    ovsdb_sock = xasprintf("unix:%s/db.sock", ovs_rundir());
    ovsrec_init();

    lib_ovsdb_init(ovsdb_sock);
    free(ovsdb_sock);

    VLOG_INFO_ONCE("Halon Vtysh LIB OVSDB Integration has been initialized");

    return;
}

/*
 * Check if the input string is a valid interface in the
 * OVSDB table
 */
int lib_vtysh_ovsdb_interface_match(const char *str)
{
    if(!str)
        return 1;

    struct ovsrec_interface *row, *next;
    lib_vtysh_run();

    OVSREC_INTERFACE_FOR_EACH_SAFE(row, next, idl)
    {
        if( strcmp(str,row->name) == 0)
            return 0;
    }

    return 1;
}

/*
 * Check if the input string is a valid port in the
 * OVSDB table
 */
int lib_vtysh_ovsdb_port_match(const char *str)
{
    if(!str)
        return 1;

    struct ovsrec_port *row, *next;
    lib_vtysh_run();

    OVSREC_PORT_FOR_EACH_SAFE(row, next, idl)
    {
        if (strcmp(str, row->name) == 0)
            return 0;
    }

    return 1;
}

/*
 * Check if the input string is a valid vlan in the
 * OVSDB table
 */
int lib_vtysh_ovsdb_vlan_match(const char *str)
{
    if(!str)
        return 1;

    struct ovsrec_vlan *row, *next;
    lib_vtysh_run();

    OVSREC_VLAN_FOR_EACH_SAFE(row, next, idl)
    {
        if (strcmp(str, row->name) == 0)
            return 0;
    }

    return 1;
}

/*
 * Check if the input string matches the given regex
 */
int lib_vtysh_regex_match(const char *regString, const char *inp)
{
    if(!inp || !regString)
        return 1;

    regex_t regex;
    int ret;
    char msgbuf[100];

    ret = regcomp(&regex, regString, 0);
    if(ret)
    {
        VLOG_ERR("Could not compile regex\n");
        return 1;
    }

    ret = regexec(&regex, inp, 0, NULL, 0);
    if (!ret)
    {
        return 0;
    }
    else if(ret == REG_NOMATCH)
    {
        return REG_NOMATCH;
    }
    else
    {
        regerror(ret, &regex, msgbuf, sizeof(msgbuf));
        VLOG_ERR("Regex match failed: %s\n", msgbuf);
    }

    return 1;
}

/*
 * When exiting vtysh destroy the idl cache
 */
void lib_vtysh_ovsdb_exit(void)
{
    ovsdb_idl_destroy(idl);
}
