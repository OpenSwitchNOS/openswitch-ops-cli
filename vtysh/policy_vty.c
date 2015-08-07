/*
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
 * File: policy_vty.c
 *
 * Purpose: This file contains implementation of router policy CLI commands
 */

#include <stdio.h>
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "memory.h"
#include "command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "log.h"
#include "lib/prefix.h"
#include "lib/routemap.h"
#include "lib/plist.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "util.h"

extern struct ovsdb_idl *idl;

#define NET_BUFSZ    19

VLOG_DEFINE_THIS_MODULE(policy_vty);

struct lookup_entry {
   char *cli_cmd;
   char *table_key;
};

const struct lookup_entry match_table[]={
  {"ip address prefix-list", "prefix_list"},
  {NULL, NULL},
};

const struct lookup_entry set_table[]={
  {"community", "community"},
  {"metric", "metric"},
  {NULL, NULL},
};

/*
 * Map 'CLI command argument list' to 'smap key'
 * Input
 * cmd - lookup command
 * lookup_table - match/set table that maps cmds to keys
 * Return value - key on match, otherwise NULL
 */
char *policy_cmd_to_key_lookup(const char *cmd, const struct lookup_entry *lookup_table)
{
        int i;

        for (i=0; lookup_table[i].cli_cmd; i++) {
            if (strcmp(cmd, lookup_table[i].cli_cmd) == 0)
               return lookup_table[i].table_key;
        }

        return NULL;
}

/*
 * Map 'smap key' to 'CLI command argument list'
 * Input
 * key - lookup key
 * lookup_table - match/set table that maps cmds to keys
 * Return value - cli cmd on match, otherwise NULL
 */
char *policy_key_to_cmd_lookup(const char *key, const struct lookup_entry *lookup_table)
{
        int i;

        for (i=0; lookup_table[i].cli_cmd; i++) {
            if (strcmp(key, lookup_table[i].table_key) == 0)
               return lookup_table[i].cli_cmd;
        }

        return NULL;
}

/*
 * IP Address Prefix List
 */
static int
policy_set_prefix_list_in_ovsdb (struct vty *vty, afi_t afi, const char *name,
                         const char *seq, const char *typestr,
                         const char *prefix, const char *ge, const char *le)

{
    static struct ovsdb_idl_txn *policy_txn=NULL;
    struct ovsrec_prefix_list *policy_row;
    const struct ovsrec_prefix_list *policy_row_const;
    struct ovsrec_prefix_list_entries  *policy_entry_row;
    const struct ovsrec_prefix_list_entries  *policy_entry_row_const;
    enum ovsdb_idl_txn_status status;
    int ret_status = 0;
    int policy_name_found = 0;

    int ret;
    enum prefix_list_type type;
    struct prefix_list *plist;
    struct prefix_list_entry *pentry;
    struct prefix_list_entry *dup;
    struct prefix p;
    int any = 0;
    int seqnum = -1;
    int lenum = 0;
    int genum = 0;

    /* Sequential number. */
    if (seq)
        seqnum = atoi (seq);

    /* ge and le number */
    if (ge)
        genum = atoi (ge);
    if (le)
        lenum = atoi (le);

    /* Check filter type. */
    if (strncmp ("permit", typestr, 1) == 0)
        type = PREFIX_PERMIT;
    else if (strncmp ("deny", typestr, 1) == 0)
        type = PREFIX_DENY;
    else
    {
        vty_out (vty, "%% prefix type must be permit or deny%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    /* "any" is special token for matching any IPv4 addresses.  */
    if (afi == AFI_IP)
    {
        if (strncmp ("any", prefix, strlen (prefix)) == 0)
        {
            ret = str2prefix_ipv4 ("0.0.0.0/0", (struct prefix_ipv4 *) &p);
            genum = 0;
            lenum = IPV4_MAX_BITLEN;
            any = 1;
        }
        else
            ret = str2prefix_ipv4 (prefix, (struct prefix_ipv4 *) &p);

        if (ret <= 0)
        {
            vty_out (vty, "%% Malformed IPv4 prefix%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    ovsdb_idl_run(idl);

    policy_txn = ovsdb_idl_txn_create(idl);
    if (policy_txn == NULL) {
         vty_out (vty, "%% Prefix list transaction creation failed%s", VTY_NEWLINE);
         VLOG_ERR("Prefix list transaction creation failed");
         return TXN_ERROR;
    }

    /*
     * If 'name' row already exists get a row structure pointer
     */
    OVSREC_PREFIX_LIST_FOR_EACH(policy_row_const, idl) {
        if (strcmp(policy_row->name, name) == 0) {
            policy_name_found = 1;
            policy_row = (struct ovsrec_prefix_list *)policy_row_const;
            break;
        }
    }

    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!policy_name_found) {
        policy_row = ovsrec_prefix_list_insert(policy_txn);
        ovsrec_prefix_list_set_name(policy_row, name);
    }

    policy_name_found = 0;
    OVSREC_PREFIX_LIST_ENTRIES_FOR_EACH(policy_entry_row_const, idl) {
        if (policy_entry_row->sequence == seqnum) {
            policy_name_found = 1;
            policy_entry_row = (struct ovsrec_prefix_list_entries *)policy_entry_row_const;
            break;
        }
    }

    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!policy_name_found) {
        policy_entry_row = ovsrec_prefix_list_entries_insert(policy_txn);
    }

    ovsrec_prefix_list_entries_set_sequence(policy_entry_row, seqnum);
    ovsrec_prefix_list_entries_set_action(policy_entry_row, typestr);
    ovsrec_prefix_list_entries_set_prefix(policy_entry_row, prefix);
    ovsrec_prefix_list_entries_set_prefix_list(policy_entry_row, policy_row);

    status = ovsdb_idl_txn_commit_block(policy_txn);
    ovsdb_idl_txn_destroy(policy_txn);
    VLOG_DBG("%s Commit Status : %s", __FUNCTION__,
              ovsdb_idl_txn_status_to_string(status));
    ret_status = ((status == TXN_SUCCESS) && (status == TXN_UNCHANGED));
    return ret_status;
}

DEFUN (ip_prefix_list_seq,
       ip_prefix_list_seq_cmd,
       "ip prefix-list WORD seq <1-4294967295> (deny|permit) (A.B.C.D/M|any)",
       IP_STR
       PREFIX_LIST_STR
       "Name of a prefix list\n"
       "sequence number of an entry\n"
       "Sequence number\n"
       "Specify packets to reject\n"
       "Specify packets to forward\n"
       "IP prefix <network>/<length>, e.g., 35.0.0.0/8\n"
       "Any prefix match. Same as \"0.0.0.0/0 le 32\"\n")
{
  return policy_set_prefix_list_in_ovsdb (vty, AFI_IP, argv[0], argv[1], argv[2],
                                  argv[3], NULL, NULL);
}

/*
 * Route Map
 */
static int
policy_set_route_map_in_ovsdb (struct vty *vty, const char *name,
                         const char *typestr, const char *seq)
{
  static struct ovsdb_idl_txn *policy_txn=NULL;
  struct ovsrec_route_map *rt_map_row;
  const struct ovsrec_route_map *rt_map_row_const;
  struct ovsrec_route_map_entries  *rt_map_entry_row;
  const struct ovsrec_route_map_entries  *rt_map_entry_row_const;
  enum ovsdb_idl_txn_status status;
  int ret_status = 0;
  int policy_name_found = 0;

  int permit;
  unsigned long pref;
  struct route_map *map;
  struct route_map_index *index;
  char *endptr = NULL;

  /* Permit check. */
  if (strncmp (typestr, "permit", strlen (typestr)) == 0)
    permit = RMAP_PERMIT;
  else if (strncmp (typestr, "deny", strlen (typestr)) == 0)
    permit = RMAP_DENY;
  else
    {
      vty_out (vty, "the third field must be [permit|deny]%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Preference check. */
  pref = strtoul (seq, &endptr, 10);
  if (pref == ULONG_MAX || *endptr != '\0')
    {
      vty_out (vty, "the fourth field must be positive integer%s",
               VTY_NEWLINE);
      return CMD_SUCCESS;
    }

  if (pref == 0 || pref > 65535)
    {
      vty_out (vty, "the fourth field must be <1-65535>%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

    ovsdb_idl_run(idl);

    policy_txn = ovsdb_idl_txn_create(idl);
    if (policy_txn == NULL) {
         VLOG_ERR("Route map transaction creation failed");
            return TXN_ERROR;
    }

    /*
     * If 'name' row already exists get a row structure pointer
     */
    OVSREC_ROUTE_MAP_FOR_EACH(rt_map_row_const, idl) {
        if (strcmp(rt_map_row->name, name) == 0) {
            policy_name_found = 1;
            rt_map_row = (struct ovsrec_route_map *)rt_map_row_const;
            break;
        }
    }

    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!policy_name_found) {
        rt_map_row = ovsrec_route_map_insert(policy_txn);
        ovsrec_route_map_set_name(rt_map_row, name);
    }

    /*
     * create a empty row, it will be used as uuid, refer to from another table
     */
    policy_name_found = 0;
    OVSREC_ROUTE_MAP_ENTRIES_FOR_EACH(rt_map_entry_row_const, idl) {
        if (rt_map_entry_row->preference== pref) {
             policy_name_found = 1;
             rt_map_entry_row = (struct ovsrec_route_map_entries *)rt_map_entry_row_const;
             break;
        }
    }

    if (!policy_name_found) {
        rt_map_entry_row = ovsrec_route_map_entries_insert(policy_txn);
    }

    ovsrec_route_map_entries_set_preference(rt_map_entry_row, pref);
    ovsrec_route_map_entries_set_action(rt_map_entry_row, typestr);
    ovsrec_route_map_entries_set_route_map(rt_map_entry_row, rt_map_row);
    vty->index = rt_map_entry_row;
    vty->node = RMAP_NODE;

    status = ovsdb_idl_txn_commit_block(policy_txn);
    ovsdb_idl_txn_destroy(policy_txn);
    VLOG_DBG("%s Commit Status : %s", __FUNCTION__,
              ovsdb_idl_txn_status_to_string(status));
    ret_status = ((status == TXN_SUCCESS) && (status == TXN_UNCHANGED));
    return ret_status;
}

DEFUN (route_map,
       rt_map_cmd,
       "route-map WORD (deny|permit) <1-65535>",
       "Create route-map or enter route-map command mode\n"
       "Route map tag\n"
       "Route map denies set operations\n"
       "Route map permits set operations\n"
       "Sequence to insert to/delete from existing route-map entry\n")
{

    return policy_set_route_map_in_ovsdb (vty, argv[0], argv[1], argv[2]);
}

static int
policy_set_route_map_description_in_ovsdb (struct vty *vty, const char *description)
{
    static struct ovsdb_idl_txn *policy_txn=NULL;
    struct ovsrec_route_map_entries  *rt_map_entry_row;
    enum ovsdb_idl_txn_status status;
    int ret_status = 0;

    ovsdb_idl_run(idl);
    policy_txn = ovsdb_idl_txn_create(idl);
    if (policy_txn == NULL) {
         VLOG_ERR("Route map description transaction creation failed");
            return TXN_ERROR;
    }

    rt_map_entry_row = vty->index;
    ovsrec_route_map_entries_set_description(rt_map_entry_row, description);

    status = ovsdb_idl_txn_commit_block(policy_txn);
    ovsdb_idl_txn_destroy(policy_txn);
    VLOG_DBG("%s Commit Status : %s", __FUNCTION__,
              ovsdb_idl_txn_status_to_string(status));
    ret_status = ((status == TXN_SUCCESS) && (status == TXN_UNCHANGED));
    return ret_status;
}

DEFUN (rmap_description,
       rmap_description_cmd,
       "description .LINE",
       "Route-map comment\n"
       "Comment describing this route-map rule\n")
{
    return policy_set_route_map_description_in_ovsdb(vty, argv_concat(argv, argc, 0));
}

static int
policy_set_route_map_match_in_ovsdb (struct vty *vty,
                               struct ovsrec_route_map_entries  *rt_map_entry_row,
                               const char *command, const char *arg)
{
    static struct ovsdb_idl_txn *policy_txn=NULL;
    enum ovsdb_idl_txn_status status;
    struct smap smap_match;
    int ret_status = 0;
    char *table_key;

    table_key = policy_cmd_to_key_lookup(command, match_table);
    if (table_key == NULL) {
         VLOG_ERR("Route map match wrong key - %s", command);
            return TXN_ERROR;
    }

    ovsdb_idl_run(idl);
    policy_txn = ovsdb_idl_txn_create(idl);
    if (policy_txn == NULL) {
         VLOG_ERR("Route map match transaction creation failed");
            return TXN_ERROR;
    }

    smap_clone(&smap_match, &rt_map_entry_row->match);
    smap_replace(&smap_match, table_key, arg);
    ovsrec_route_map_entries_set_match(rt_map_entry_row, &smap_match);
    smap_destroy(&smap_match);

    status = ovsdb_idl_txn_commit_block(policy_txn);
    ovsdb_idl_txn_destroy(policy_txn);
    VLOG_DBG("%s Commit Status : %s", __FUNCTION__,
              ovsdb_idl_txn_status_to_string(status));
    ret_status = ((status == TXN_SUCCESS) && (status == TXN_UNCHANGED));
    return ret_status;
}

DEFUN (match_ip_address_prefix_list,
       match_ip_address_prefix_list_cmd,
       "match ip address prefix-list WORD",
       MATCH_STR
       IP_STR
       "Match address of route\n"
       "Match entries of prefix-lists\n"
       "IP prefix-list name\n")
{
  return policy_set_route_map_match_in_ovsdb (vty, vty->index, "ip address prefix-list", argv[0]);
}

static int
policy_set_route_map_set_in_ovsdb (struct vty *vty,
                   struct ovsrec_route_map_entries  *rt_map_entry_row,
                   const char *command, const char *arg)

{
    static struct ovsdb_idl_txn *policy_txn=NULL;
    enum ovsdb_idl_txn_status status;
    struct smap smap_set;
    int ret_status = 0;
    char *table_key;

    table_key = policy_cmd_to_key_lookup(command, set_table);
    if (table_key == NULL) {
         VLOG_ERR("Route map set wrong key - %s", command);
            return TXN_ERROR;
    }

    ovsdb_idl_run(idl);
    policy_txn = ovsdb_idl_txn_create(idl);
    if (policy_txn == NULL) {
         VLOG_ERR("Route map description transaction creation failed");
            return TXN_ERROR;
    }

    smap_clone(&smap_set, &rt_map_entry_row->set);
    smap_replace(&smap_set, table_key, arg);
    ovsrec_route_map_entries_set_set(rt_map_entry_row, &smap_set);
    smap_destroy(&smap_set);

    status = ovsdb_idl_txn_commit_block(policy_txn);
    ovsdb_idl_txn_destroy(policy_txn);
    VLOG_DBG("%s Commit Status : %s", __FUNCTION__,
              ovsdb_idl_txn_status_to_string(status));
    ret_status = ((status == TXN_SUCCESS) && (status == TXN_UNCHANGED));
    return ret_status;
}

DEFUN (set_metric,
       set_metric_cmd,
       "set metric <0-4294967295>",
       SET_STR
       "Metric value for destination routing protocol\n"
       "Metric value\n")
{
  return policy_set_route_map_set_in_ovsdb (vty, vty->index, "metric", argv[0]);
}

static void policy_vty_init(void)
{
    install_element (CONFIG_NODE, &ip_prefix_list_seq_cmd);
    install_element (CONFIG_NODE, &rt_map_cmd);
    install_element (RMAP_NODE, &rmap_description_cmd);
    install_element (RMAP_NODE, &match_ip_address_prefix_list_cmd);
    install_element (RMAP_NODE, &set_metric_cmd);
}
