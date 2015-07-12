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
 * File: vtysh_ovsdb_if.c
 *
 * Purpose: Main file for integrating vtysh with ovsdb.
 */

#include <stdio.h>


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
#include "vtysh/vtysh_ovsdb_if.h"
#include "assert.h"

typedef unsigned char boolean;

VLOG_DEFINE_THIS_MODULE(vtysh_ovsdb_if);

struct ovsdb_idl *idl;
static unsigned int idl_seqno;
static char *appctl_path = NULL;
static struct unixctl_server *appctl;
static struct ovsdb_idl_txn *txn;
static struct ovsdb_idl_txn *status_txn;

boolean exiting = false;

/*
 * Running idl run and wait to fetch the data from the DB
 */
static void
vtysh_run()
{
    while(!ovsdb_idl_has_lock(idl))
    {
        ovsdb_idl_run(idl);
        unixctl_server_run(appctl);

        ovsdb_idl_wait(idl);
        unixctl_server_wait(appctl);
    }
}

static void
bgp_ovsdb_init(struct ovsdb_idl *idl)
{
  ovsdb_idl_add_table(idl, &ovsrec_table_bgp_router);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_asn);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_router_id);
  ovsdb_idl_add_column(idl, &ovsrec_bgp_router_col_status);
  ovsdb_idl_add_table(idl, &ovsrec_table_bgp_neighbor);
  ovsdb_idl_add_table(idl, &ovsrec_table_rib);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_prefix);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_prefix_len);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_from_protocol);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_nexthop_list);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_address_family);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_sub_address_family);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_protocol_specific_data);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_flags);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_selected_for_RIB);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_distance);
  ovsdb_idl_add_column(idl, &ovsrec_rib_col_metric);


}

/*
 * Create a connection to the OVSDB at db_path and create
 * the idl cache.
 */
static void
ovsdb_init(const char *db_path)
{
    idl = ovsdb_idl_create(db_path, &ovsrec_idl_class, false, true);
    ovsdb_idl_set_lock(idl, "halon_vtysh");
    idl_seqno = ovsdb_idl_get_seqno(idl);

    /* Add hostname columns */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_hostname);

    /* Add tables and columns for LLDP configuration */
    ovsdb_idl_add_table(idl, &ovsrec_table_open_vswitch);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_cur_cfg);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_open_vswitch_col_status);

    ovsdb_idl_add_table(idl, &ovsrec_table_interface);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_lldp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_other_config);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_link_state);
    ovsdb_idl_add_column(idl, &ovsrec_interface_col_lldp_neighbor_info);

    // BGP tables
    bgp_ovsdb_init(idl);

    /* Fetch data from DB */
    vtysh_run();
}

static void
halon_vtysh_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
                    const char *argv[] OVS_UNUSED, void *exiting_)
{
    boolean *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}

/*
 * The init for the ovsdb integration called in vtysh main function
 */
void vtysh_ovsdb_init(int argc, char *argv[])
{
    int retval;
    char *ovsdb_sock;

    set_program_name(argv[0]);
    proctitle_init(argc, argv);
    fatal_ignore_sigpipe();

    ovsdb_sock = xasprintf("unix:%s/db.sock", ovs_rundir());
    ovsrec_init();

    retval = unixctl_server_create(appctl_path, &appctl);
    if(retval)
    {
        exit(EXIT_FAILURE);
    }

    unixctl_command_register("exit", "", 0, 0, halon_vtysh_exit, &exiting);

    ovsdb_init(ovsdb_sock);
    free(ovsdb_sock);

    VLOG_INFO_ONCE("Halon Vtysh OVSDB Integration has been initialized");

    return;
}

/*
 * The set command to set the hostname column in the
 * open_vswitch table from the set-hotname command
 */
void vtysh_ovsdb_hostname_set(const char* in)
{
    const struct ovsrec_open_vswitch *ovs= NULL;

    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs)
    {
        txn = ovsdb_idl_txn_create(idl);
        ovsrec_open_vswitch_set_hostname(ovs, in);
        ovsdb_idl_txn_commit(txn);
        ovsdb_idl_txn_destroy(txn);
        VLOG_INFO("We have set the hostname in the ovsdb table %s",in);
    }
    else
    {
        VLOG_ERR("We couldn't retrieve any open_vswitch table rows");
    }
}

/*
 * The get command to read from the ovsdb open_vswitch table
 * hostname column from the vtysh get-hostname command
 */
char* vtysh_ovsdb_hostname_get()
{
    const struct ovsrec_open_vswitch *ovs;
    ovsdb_idl_run(idl);
    ovsdb_idl_wait(idl);
    ovs = ovsrec_open_vswitch_first(idl);

    if(ovs)
    {
        printf("The hostname in ovsdb table is %s\n", ovs->hostname);
        VLOG_INFO("We have read the hostname %s from ovsdb", ovs->hostname);
        return ovs->hostname;
    }
    else
    {
        VLOG_ERR("We couldn't retrieve any open_vswitch table rows");
    }

    return NULL;
}

/*
 * When exiting vtysh destroy the idl cache
 */
void vtysh_ovsdb_exit(void)
{
    ovsdb_idl_destroy(idl);
}

/* This API is for fetching contents from DB to Vtysh IDL cache
   and to do initial setup before commiting changes to IDL cache.
   Keeping the return value as boolean so that we can handle any error cases
   in future. */
boolean cli_do_config_start()
{
  ovsdb_idl_run(idl);
  status_txn = ovsdb_idl_txn_create(idl);

  if(status_txn == NULL)
  {
     assert(0);
     return false;
  }
  return true;
}

/* This API is for pushing Vtysh IDL contents to DB */
boolean cli_do_config_finish()
{
  enum ovsdb_idl_txn_status status;

  status = ovsdb_idl_txn_commit(status_txn);
  ovsdb_idl_txn_destroy(status_txn);
  status_txn = NULL;

  if ((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED))
     return false;

  return true;
}


void cli_do_config_abort()
{
  ovsdb_idl_txn_destroy(status_txn);
}
