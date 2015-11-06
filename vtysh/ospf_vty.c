
/* OSPF CLI implementation with OPS vtysh.
 *
 * Copyright (C) 2000 Kunihiro Ishiguro
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
 * File: ospf_vty.c
 *
 * Purpose: This file contains implementation of all OSPF configuration
 */
#include <stdio.h>
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "log.h"
#include "ospf_vty.h"
#include "vswitch-idl.h"
#include "openvswitch/vlog.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "util.h"
#include "prefix.h"
#include "sockunion.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/ospf_vty.h"
#include <lib/version.h>
#include "getopt.h"
#include "memory.h"
#include "vtysh/vtysh_user.h"
#include "ovsdb-idl.h"
#include "lib/prefix.h"
#include "lib/routemap.h"
#include "lib/plist.h"
#include "lib/libospf.h"
#include "openswitch-idl.h"
#include "openswitch-dflt.h"



extern struct ovsdb_idl *idl;

VLOG_DEFINE_THIS_MODULE(ospf_vty);


/* Function to verify if the input string is in IP format. */
static bool
ospf_string_is_an_ip_addr(const char *string)
{
    union sockunion su;
    return (str2sockunion(string, &su) >= 0);
}

/*Funtion to get the statistics from neighbor table. */
int
ospf_get_statistics_from_neighbor(const struct ovsrec_ospf_neighbor *
                             ovs_ospf_neighbor, const char *key)
{
    int i = 0;

    VLOG_DBG("ospf nbr statistics key=%s\n", key);
    for (i = 0; i <ovs_ospf_neighbor->n_nbr_statistics; i++) {
        if (!strcmp(ovs_ospf_neighbor->key_nbr_statistics[i], key))
            return ovs_ospf_neighbor->value_nbr_statistics[i];
    }
    return 0;
}

struct timeval
tv_adjust (struct timeval a)
{
  while (a.tv_usec >= 1000000)
    {
      a.tv_usec -= 1000000;
      a.tv_sec++;
    }

  while (a.tv_usec < 0)
    {
      a.tv_usec += 1000000;
      a.tv_sec--;
    }

  return a;
}

struct timeval
tv_sub (struct timeval a, struct timeval b)
{
  struct timeval ret;

  ret.tv_sec = a.tv_sec - b.tv_sec;
  ret.tv_usec = a.tv_usec - b.tv_usec;

  return tv_adjust (ret);
}

void ospf_clock (struct timeval *tv)
{
    quagga_gettime(QUAGGA_CLK_MONOTONIC, tv);
}


const char *
ospf_timeval_convert (struct timeval *t, char *buf, size_t size)
{
  /* Making formatted timer strings. */
#define MINUTE_IN_SECONDS   60
#define HOUR_IN_SECONDS     (60*MINUTE_IN_SECONDS)
#define DAY_IN_SECONDS      (24*HOUR_IN_SECONDS)
#define WEEK_IN_SECONDS     (7*DAY_IN_SECONDS)
unsigned long w, d, h, m, s, ms, us;

  if (!t)
    return "inactive";

  w = d = h = m = s = ms = us = 0;
  memset (buf, 0, size);

  us = t->tv_usec;
  if (us >= 1000)
    {
      ms = us / 1000;
      us %= 1000;
    }

  if (ms >= 1000)
    {
      t->tv_sec += ms / 1000;
      ms %= 1000;
    }

  if (t->tv_sec > WEEK_IN_SECONDS)
    {
      w = t->tv_sec / WEEK_IN_SECONDS;
      t->tv_sec -= w * WEEK_IN_SECONDS;
    }

  if (t->tv_sec > DAY_IN_SECONDS)
    {
      d = t->tv_sec / DAY_IN_SECONDS;
      t->tv_sec -= d * DAY_IN_SECONDS;
    }

  if (t->tv_sec >= HOUR_IN_SECONDS)
    {
      h = t->tv_sec / HOUR_IN_SECONDS;
      t->tv_sec -= h * HOUR_IN_SECONDS;
    }

  if (t->tv_sec >= MINUTE_IN_SECONDS)
    {
      m = t->tv_sec / MINUTE_IN_SECONDS;
      t->tv_sec -= m * MINUTE_IN_SECONDS;
    }

  if (w > 99)
    snprintf (buf, size, "%ldw%1ldd", w, d);
  else if (w)
    snprintf (buf, size, "%ldw%1ldd%02ldh", w, d, h);
  else if (d)
    snprintf (buf, size, "%1ldd%02ldh%02ldm", d, h, m);
  else if (h)
    snprintf (buf, size, "%ldh%02ldm%02lds", h, m, t->tv_sec);
  else if (m)
    snprintf (buf, size, "%ldm%02lds", m, t->tv_sec);
  else if (ms)
    snprintf (buf, size, "%ld.%03lds", t->tv_sec, ms);
  else
    snprintf (buf, size, "%ld usecs", t->tv_usec);

  return buf;
}

const char *
ospf_neighbor_time_print (const char *val, char *buf, int size)
{
    char *result;
    int64_t ms = 0;
    struct timeval tv_adjust, tv_val, tv_due;

    if (!val)
        return "inactive";

    /*  Do the timer manipulation. */
    ms = (int64_t)atol(val);

    tv_val.tv_sec = ms/1000;
    tv_val.tv_usec = 1000 * (ms%1000);

    ospf_clock(&tv_adjust);

    if ((tv_adjust.tv_sec > tv_val.tv_sec) ||
        ((tv_val.tv_sec <= 0) && (tv_adjust.tv_usec > tv_val.tv_usec)))
    {
        return "now";
    }

    tv_due = tv_sub(tv_val, tv_adjust);

    ospf_timeval_convert(&tv_due, buf, size);

    return buf;
}


const char *
ospf_timer_adjust (const char *val, char *buf, int size)
{
    int64_t ms = 0;
    struct timeval tv_adj, tv_val, tv_due;

    if (!val)
        return "inactive";

    /*  Do the timer manipulation. */
    ms = (int64_t)atol(val);

    tv_val.tv_sec = ms/1000;
    tv_val.tv_usec = 1000 * (ms%1000);

    ospf_clock(&tv_adj);

    if ((tv_adj.tv_sec > tv_val.tv_sec) ||
        ((tv_val.tv_sec <= 0) && (tv_adj.tv_usec > tv_val.tv_usec)))
    {
        return "now";
    }

    tv_due = tv_sub(tv_val, tv_adj);

    memset (buf, 0, size);
    snprintf(buf, size, "%d.%ds", tv_due.tv_sec, tv_due.tv_usec);
    return buf;
}

char *
ospf_options_print (int64_t options)
{
  static char buf[OSPF_OPTION_STR_MAXLEN];

  snprintf (buf, OSPF_OPTION_STR_MAXLEN, "*|%s|%s|%s|%s|%s|%s|*",
           (options & OSPF_OPTION_O) ? "O" : "-",
           (options & OSPF_OPTION_DC) ? "DC" : "-",
           (options & OSPF_OPTION_EA) ? "EA" : "-",
           (options & OSPF_OPTION_NP) ? "N/P" : "-",
           (options & OSPF_OPTION_MC) ? "MC" : "-",
           (options & OSPF_OPTION_E) ? "E" : "-");

  return buf;
}

/*
 * Find the vrf with matching the name.
 */
static const struct ovsrec_vrf*
ospf_get_ovsrec_vrf_with_name(char *name)
{
    const struct ovsrec_vrf* vrf_row = NULL;

    if(!name)
        return NULL;

    OVSREC_VRF_FOR_EACH(vrf_row, idl)
    {
        if(strcmp(vrf_row->name, name) == 0)
        {
            return vrf_row;
        }
    }

    return NULL;
}

/* Remove the area row matching the area id and remove reference from the router table. */
void
ospf_area_remove_from_router(const struct ovsrec_ospf_router *ospf_router_row,
                              int64_t area_id)
{
    int64_t *area;
    struct ovsrec_ospf_area **area_list;
    int i = 0, j;

    /* Remove OSPF_area table reference in OSPF_Router table. */
    area = xmalloc(sizeof(int64_t) * (ospf_router_row->n_areas - 1));
    area_list = xmalloc(sizeof * ospf_router_row->key_areas *
                              (ospf_router_row->n_areas - 1));
    for (i = 0, j = 0; i < ospf_router_row->n_areas; i++) {
        if(ospf_router_row->key_areas[i] !=  area_id) {
            area[j] = ospf_router_row->key_areas[i];
            area_list[j] = ospf_router_row->value_areas[i];
            j++;
        }
    }
    ovsrec_ospf_router_set_areas(ospf_router_row, area, area_list,
                               (ospf_router_row->n_areas - 1));

    free(area);
    free(area_list);
}


/* Enable the OSPF in global context */
static int
ospf_global_enable(const int64_t enable_flg)
{
    const struct ovsrec_system *row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn;
    char *ospf_feature_str = OSPF_FEATURE;

    row = ovsrec_system_first(idl);

    if (!row)
    {
        VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
        return CMD_OVSDB_FAILURE;
    }

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    ovsrec_system_set_enable_features(row, &ospf_feature_str, &enable_flg,
                               NUM_OF_FEATURES);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

}

/*
 * Find the ospf router with matching instance id
 */
static const struct ovsrec_ospf_router *
get_ovsrec_ospf_router_with_instance_id(const struct ovsrec_vrf *vrf_row,
                                              int64_t instance_tag)
{
    int i = 0;
    for (i = 0; i < vrf_row->n_ospf_routers; i++) {
        if (vrf_row->key_ospf_routers[i] == instance_tag) {
            return vrf_row->value_ospf_routers[i];
        }
    }

    return NULL;
}

/* Check if any non default values is present in neighbor table.
    If present then return false. Else true. */
int
ospf_is_neighbor_tbl_empty(const struct
                                  ovsrec_ospf_neighbor*ospf_nbr_row)
{
    char *val;

    if (ospf_nbr_row->nbr_router_id != 0)
        return false;

    if (ospf_nbr_row->n_nbr_if_addr > 0)
        return false;

    if (ospf_nbr_row->nbma_nbr)
        return false;

    return true;
}

void
ospf_neighbor_remove_from_interface(const struct
                            ovsrec_ospf_interface *interface_row,
                            const struct ovsrec_ospf_neighbor *ospf_nbr_row)
{
    struct ovsrec_ospf_neighbor **ospf_nbr_list;
    int i = 0, j = 0;

    /* Insert OSPF_Router table reference in VRF table. */
    ospf_nbr_list = xmalloc(sizeof * interface_row->neighbors *
                    (interface_row->n_neighbors - 1));
    for (i = 0; i < interface_row->n_neighbors; i++)
    {
        if (interface_row->neighbors[i] != ospf_nbr_row)
        {
            ospf_nbr_list[j] = interface_row->neighbors[i];
            j++;
        }
    }

    /* Update the reference. */
    ospf_nbr_list[interface_row->n_neighbors] =
                      CONST_CAST(struct ovsrec_ospf_neighbor*, ospf_nbr_row);
    ovsrec_ospf_interface_set_neighbors(interface_row, ospf_nbr_list,
                               (interface_row->n_neighbors - 1));

    free(ospf_nbr_list);
}


/* Add the router row to the VRF table. */
void
ospf_router_insert_to_vrf(const struct ovsrec_vrf *vrf_row,
                         const struct ovsrec_ospf_router *ospf_router_row,
                         int64_t instance_tag)
{
    int64_t *instance_list;
    struct ovsrec_ospf_router **ospf_routers_list;
    int i = 0;

    /* Insert OSPF_Router table reference in VRF table. */
    instance_list = xmalloc(sizeof(int64_t) * (vrf_row->n_ospf_routers + 1));
    ospf_routers_list = xmalloc(sizeof * vrf_row->key_ospf_routers *
                              (vrf_row->n_ospf_routers + 1));
    for (i = 0; i < vrf_row->n_ospf_routers; i++) {
        instance_list[i] = vrf_row->key_ospf_routers[i];
        ospf_routers_list[i] = vrf_row->value_ospf_routers[i];
    }

    /* Update the reference. */
    instance_list[vrf_row->n_ospf_routers] = instance_tag;
    ospf_routers_list[vrf_row->n_ospf_routers] =
                      CONST_CAST(struct ovsrec_ospf_router *, ospf_router_row);
    ovsrec_vrf_set_ospf_routers(vrf_row, instance_list, ospf_routers_list,
                               (vrf_row->n_ospf_routers + 1));
    free(instance_list);
    free(ospf_routers_list);
}

/* Set the OSPF_Router table values to default values. */
void
ospf_router_tbl_default(const struct ovsrec_ospf_router *ospf_router_row)
{
    struct smap smap = SMAP_INITIALIZER(&smap);
    const bool passive_intf_default = false;
    int64_t values[OSPF_NUM_SPF_KEYS];
    char *keys[OSPF_NUM_SPF_KEYS];
    int64_t lsa_values[OSPF_NUM_LSA_TIMER_KEYS];
    char *lsa_keys[OSPF_NUM_LSA_TIMER_KEYS];
    int64_t stub_values[OSPF_NUM_LSA_TIMER_KEYS];
    char *stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_MAX];


    if (ospf_router_row == NULL)
    {
        smap_destroy(&smap);
        return;
    }

    // Router Id
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_ROUTER_ID_STATIC, OSPF_ROUTER_ID_STATIC_DEFAULT);
    ovsrec_ospf_router_set_router_id(ospf_router_row, &smap);

    //default_information
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_DEFAULT_INFO_ORIG, OSPF_DEFAULT_INFO_ORIG_DEFAULT);
    smap_add(&smap, OSPF_KEY_ALWAYS, OSPF_ALWAYS_DEFAULT);
    ovsrec_ospf_router_set_default_information(ospf_router_row, &smap);

    //passive-interface-default
    ovsrec_ospf_router_set_passive_interface_default(ospf_router_row,
                                                    &passive_intf_default, 1);

    //SPF config
    keys[OSPF_SPF_DELAY]        = OSPF_KEY_SPF_DELAY;
    values[OSPF_SPF_DELAY]      = OSPF_SPF_DELAY_DEFAULT;
    keys[OSPF_SPF_HOLD_TIME]    = OSPF_KEY_SPF_HOLD_TIME;
    values[OSPF_SPF_HOLD_TIME]  = OSPF_SPF_HOLDTIME_DEFAULT;
    keys[OSPF_SPF_MAX_WAIT]     = OSPF_KEY_SPF_MAX_WAIT;
    values[OSPF_SPF_MAX_WAIT]   = OSPF_SPF_MAX_HOLDTIME_DEFAULT;
    ovsrec_ospf_router_set_spf_config(ospf_router_row, keys, values,
                                      OSPF_NUM_SPF_KEYS);

    // Restart-config
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_RESTART_ENABLE_GRACEFUL,
                OSPF_RESTART_ENABLE_GRACEFUL_DEFAULT);
    smap_add(&smap, OSPF_KEY_RESTART_HELPER_DISABLE,
                OSPF_RESTART_HELPER_DISABLE_DEFAULT);
    smap_add(&smap, OSPF_KEY_RESTART_PLANNED_ONLY,
                OSPF_RESTART_PLANNED_ONLY_DEFAULT);
    smap_add(&smap, OSPF_KEY_RESTART_INTERVAL,
                OSPF_RESTART_INTERVAL_DEFAULT);
    smap_add(&smap, OSPF_KEY_RESTART_STRICT_LSA_CHECKING,
                OSPF_RESTART_STRICT_LSA_CHECKING_DEFAULT);
    ovsrec_ospf_router_set_restart_config(ospf_router_row, &smap);


    // Router config
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_ROUTER_DEFAULT_METRIC,
                OSPF_RESTART_ROUTER_DEFAULT_METRIC_DEFAULT);
    smap_add(&smap, OSPF_KEY_AUTO_COST_REF_BW,
                OSPF_AUTO_COST_REF_BW_DEFAULT);
    smap_add(&smap, OSPF_KEY_MAX_PATHS, OSPF_MAX_PATHS_DEFAULT);
    smap_add(&smap, OSPF_KEY_LOG_ADJACENCY_CHGS,
                OSPF_LOG_ADJACENCY_CHGS_DEFAULT);
    smap_add(&smap, OSPF_KEY_LOG_ADJACENCY_DETAIL,
                OSPF_LOG_ADJACENCY_DETAIL_DEFAULT);
    smap_add(&smap, OSPF_KEY_RFC1583_COMPATIBLE,
                OSPF_RFC1583_COMPATIBLE_DEFAULT);
    smap_add(&smap, OSPF_KEY_ENABLE_OPAQUE_LSA,
                OSPF_ENABLE_OPAQUE_LSA_DEFAULT);
    ovsrec_ospf_router_set_router_config(ospf_router_row, &smap);

    //Stub router configurations
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_ADMIN]    =
                                                OSPF_KEY_ROUTER_STUB_ADMIN;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_ADMIN]  =
                                                OSPF_ROUTER_STUB_ADMIN_DEFAULT;
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_EXT_LSA]    =
                                                OSPF_KEY_ROUTER_STUB_EXT_LSA;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_EXT_LSA]  =
                                              OSPF_ROUTER_STUB_EXT_LSA_DEFAULT;
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_INCLUDE_STUB]    =
                                                OSPF_KEY_ROUTER_STUB_INCLUDE;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_INCLUDE_STUB] =
                                              OSPF_ROUTER_STUB_INCLUDE_DEFAULT;
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP]      =
                                            OSPF_KEY_ROUTER_STUB_STARTUP;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP]    =
                                            OSPF_ROUTER_STUB_STARTUP_DEFAULT;
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP_WAITBGP]   =
                                            OSPF_KEY_ROUTER_STUB_STARTUP_WAIT;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP_WAITBGP] =
                                         OSPF_ROUTER_STUB_STARTUP_WAIT_DEFAULT;
    stub_keys[OSPF_ROUTER_STUB_ROUTER_CONFIG_SUMMARY_LSA]    =
                                         OSPF_KEY_ROUTER_STUB_SUMMARY_LSA;
    stub_values[OSPF_ROUTER_STUB_ROUTER_CONFIG_SUMMARY_LSA] =
                                        OSPF_ROUTER_STUB_SUMMARY_LSA_DEFAULT;
    ovsrec_ospf_router_set_stub_router_config(ospf_router_row, stub_keys,
                                              stub_values, OSPF_ROUTER_STUB_ROUTER_CONFIG_MAX);

    //LSA timers
    lsa_keys[OSPF_LSA_ARRIVAL_INTERVAL]    = OSPF_KEY_ARRIVAL_INTERVAL;
    lsa_values[OSPF_LSA_ARRIVAL_INTERVAL]  = OSPF_LSA_ARRIVAL_INTERVAL_DEFAULT;
    lsa_keys[OSPF_LSA_GROUP_PACING]    = OSPF_KEY_LSA_GROUP_PACING;
    lsa_values[OSPF_LSA_GROUP_PACING]  = OSPF_LSA_GROUP_PACING_DEFAULT;
    lsa_keys[OSPF_LSA_START_TIME]      = OSPF_KEY_LSA_START_TIME;
    lsa_values[OSPF_LSA_START_TIME]    = OSPF_LSA_START_TIME_DEFAULT;
    lsa_keys[OSPF_LSA_HOLD_INTERVAL]   = OSPF_KEY_LSA_HOLD_INTERVAL;
    lsa_values[OSPF_LSA_HOLD_INTERVAL] = OSPF_LSA_HOLD_INTERVAL_DEFAULT;
    lsa_keys[OSPF_LSA_MAX_DELAY]    = OSPF_KEY_LSA_MAX_DELAY;
    lsa_values[OSPF_LSA_MAX_DELAY]  = OSPF_LSA_MAX_DELAY_DEFAULT;
    ovsrec_ospf_router_set_lsa_timers_config(ospf_router_row, lsa_keys,
                                        lsa_values, OSPF_NUM_LSA_TIMER_KEYS);

    ovsrec_ospf_router_set_enable_instance(ospf_router_row, true);

    smap_destroy(&smap);
}

/* "router ospf" command handler. */
static int
ospf_router_cmd_execute(char *vrf_name, int64_t instance_tag)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn = NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF not found");
    }
    /* See if it already exists. */
    ospf_router_row = get_ovsrec_ospf_router_with_instance_id(vrf_row,
                                                              instance_tag);

    /* If does not exist, create a new one. */
    if (ospf_router_row == NULL)
    {
        ospf_router_row = ovsrec_ospf_router_insert(ospf_router_txn);
        ospf_router_insert_to_vrf(vrf_row, ospf_router_row, instance_tag);
        ospf_router_tbl_default(ospf_router_row);
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);
}

/* "no router ospf" command handler. */
void
ospf_router_remove_from_vrf(const struct ovsrec_vrf *vrf_row,
                           int64_t instance_id)
{
    int64_t *instance_list;
    struct ovsrec_ospf_router **ospf_routers_list;
    int i, j;

    /* Remove OSPF_Router table reference in VRF table. */
    instance_list = xmalloc(sizeof(int64_t) * (vrf_row->n_ospf_routers - 1));
    ospf_routers_list = xmalloc(sizeof * vrf_row->key_ospf_routers *
                              (vrf_row->n_ospf_routers - 1));

    for (i = 0, j = 0; i < vrf_row->n_ospf_routers; i++)
    {
        if (vrf_row->key_ospf_routers[i] != instance_id)
        {
            instance_list[j] = vrf_row->key_ospf_routers[i];
            ospf_routers_list[j] = vrf_row->value_ospf_routers[i];
            j++;
        }
    }

    ovsrec_vrf_set_ospf_routers(vrf_row, instance_list, ospf_routers_list,
                               (vrf_row->n_ospf_routers - 1));

    free(instance_list);
    free(ospf_routers_list);
}

/* Remove the interface row matching the interface name and remove the reference from
     area table. */
void
ospf_interface_remove_from_area(const struct ovsrec_ospf_area *area_row,
                                      char *ifname)
{
    char **interface_list;
    struct ovsrec_ospf_interface **ospf_interface_list;
    int i, j;

    /* Remove OSPF_Router table reference in VRF table. */
    interface_list = xmalloc(sizeof(int64_t) * (area_row->n_ospf_interfaces - 1));
    ospf_interface_list = xmalloc(sizeof * area_row->key_ospf_interfaces *
                              (area_row->n_ospf_interfaces - 1));
    for (i = 0, j = 0; i < area_row->n_ospf_interfaces; i++)
    {
        if (strcmp(area_row->key_ospf_interfaces[i],  ifname) != 0)
        {
            interface_list[j] = area_row->key_ospf_interfaces[i];
            ospf_interface_list[j] = area_row->value_ospf_interfaces[i];
            j++;
        }
    }
    ovsrec_ospf_area_set_ospf_interfaces(area_row, interface_list, ospf_interface_list,
                               (area_row->n_ospf_interfaces - 1));
    free(interface_list);
    free(ospf_interface_list);
}


/* "no router ospf" command handler. */
static int
ospf_no_router_cmd_execute(char *vrf_name, int64_t instance_id)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    const struct ovsrec_ospf_interface *interface_row = NULL;
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    int i,j,k;
    bool delete_flag = true;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not found");
    }

    /* See if it already exists. */
    ospf_router_row = get_ovsrec_ospf_router_with_instance_id(vrf_row,
                                                              instance_id);

    /* If does not exist, nothing to delete. */
    if (ospf_router_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    /* Go through the router row and decide to delete interface and neighbors */
    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        area_row = ospf_router_row->value_areas[i];
        for (j = 0; j < area_row->n_ospf_interfaces; j++)
        {
            interface_row = area_row->value_ospf_interfaces[j];
            for (k = 0; k < interface_row->n_neighbors; k++)
            {
                /* Delete the neighbor */
                ospf_neighbor_remove_from_interface(interface_row,
                                                    ospf_nbr_row);
                ovsrec_ospf_neighbor_delete(ospf_nbr_row);
            }

            ospf_interface_remove_from_area(area_row, area_row->key_ospf_interfaces[j]);
            ovsrec_ospf_interface_delete(interface_row);
        }

       ospf_area_remove_from_router(ospf_router_row,
                                    ospf_router_row->key_areas[i]);

       ovsrec_ospf_area_delete(area_row);
    }

        /* Delete the ospf row for matching instance id. */
        ovsrec_ospf_router_delete(ospf_router_row);
        ospf_router_remove_from_vrf(vrf_row, instance_id);


    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);
}

/* "router-id <id> command handler. " */
static int
ospf_router_id_cmd_execute(char *vrf_name, char *router_ip_addr)
{
    int ret;
    struct in_addr id;
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);

    smap_init(&smap);
    memset (&id, 0, sizeof (struct in_addr));

    /* convert the router id format to integer. */
    if (ospf_string_is_an_ip_addr(router_ip_addr))
    {
        ret = inet_aton (router_ip_addr, &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed OSPF router identifier.%s", VTY_NEWLINE);
            smap_destroy(&smap);
            return CMD_WARNING;
        }

        /* Start of transaction. */
        OSPF_START_DB_TXN(ospf_router_txn);

        VLOG_DBG("vty_index for router_id: %ld\n",(int64_t)vty->index);

        vrf_row = ospf_get_ovsrec_vrf_with_name(vrf_name);
        if (vrf_row == NULL) {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not found.");
        }

        /* See if it already exists. */
        ospf_router_row =
        get_ovsrec_ospf_router_with_instance_id(vrf_row, (int64_t)vty->index);

        /* If does not exist, nothing to modify. */
        if (ospf_router_row == NULL) {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not found.");
        } else {
            /* Set the router-id with matching instance id. */
            smap_add(&smap, OSPF_KEY_ROUTER_ID_VAL, inet_ntoa(id));
            smap_add(&smap, OSPF_KEY_ROUTER_ID_STATIC, "true");
            ovsrec_ospf_router_set_router_id(ospf_router_row, &smap);
        }

        smap_destroy(&smap);

        /* End of transaction. */
        OSPF_END_DB_TXN(ospf_router_txn);
    }
    else
    {
        vty_out (vty, "Malformed OSPF router identifier.%s", VTY_NEWLINE);
        smap_destroy(&smap);
        return CMD_WARNING;
    }

}

/* "no router-id <id> command handler." */
static int
ospf_no_router_id_cmd_execute(char *vrf_name)
{
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);

    smap_init(&smap);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    VLOG_DBG("vty_index for router_id: %ld\n",(int64_t)vty->index);

    vrf_row = ospf_get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not found.");
    }
    /* See if it already exists. */
    ospf_router_row = get_ovsrec_ospf_router_with_instance_id(vrf_row,
                                                    (int64_t)vty->index);

    /* If does not exist, nothing to modify. */
    if (ospf_router_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not found.");
    } else {
        /* Unset the router-id with matching asn. */
        smap_remove(&smap, OSPF_KEY_ROUTER_ID_VAL);
        smap_add(&smap, OSPF_KEY_ROUTER_ID_STATIC, "false");
        ovsrec_ospf_router_set_router_id(ospf_router_row, &smap);
    }

    smap_destroy(&smap);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);
}

/* "enable" command in router context - handler. */
static int
ospf_router_enable_cmd_execute(char *vrf_name, int enable_flag)
{
    int ret;
    struct in_addr id;
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    VLOG_DBG("vty_index for router_id: %ld\n",(int64_t)vty->index);

    vrf_row = ospf_get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not found.");
    }

    /* See if it already exists. */
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, (int64_t)vty->index);

    /* If does not exist, nothing to modify. */
    if (ospf_router_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not found.");
    } else {
        /* Set the enable flag. */
        ovsrec_ospf_router_set_enable_instance(ospf_router_row, enable_flag);
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);
}


/* Default the values if OSPF_interface_Config table. */
void
ospf_interface_if_config_tbl_default(
                  const struct ovsrec_ospf_interface_config *ospf_if_config_row)
{
    struct smap smap = SMAP_INITIALIZER(&smap);
    char show_str[10];

    if (ospf_if_config_row == NULL)
    {
        smap_destroy(&smap);
        return;
    }

    smap_init(&smap);

    snprintf(show_str, sizeof(show_str), "%d", OSPF_TRANSMIT_DELAY_DEFAULT);
    smap_add(&smap, OSPF_KEY_TRANSMIT_DELAY, (const char *)show_str);

    snprintf(show_str, sizeof(show_str), "%d", OSPF_RETRANSMIT_INTERVAL_DEFAULT);
    smap_add(&smap, OSPF_KEY_RETRANSMIT_INTERVAL, (const char *)show_str);

    snprintf(show_str, sizeof(show_str), "%d", OSPF_HELLO_INTERVAL_DEFAULT);
    smap_add(&smap, OSPF_KEY_HELLO_INTERVAL, (const char *)show_str);

    snprintf(show_str, sizeof(show_str), "%d", OSPF_ROUTER_DEAD_INTERVAL_DEFAULT);
    smap_add(&smap, OSPF_KEY_DEAD_INTERVAL, (const char *)show_str);

    ovsrec_ospf_interface_config_set_ospf_intervals(ospf_if_config_row, &smap);

    ovsrec_ospf_interface_config_set_priority(ospf_if_config_row,OSPF_ROUTER_PRIORITY_DEFAULT);

    ovsrec_ospf_interface_config_set_mtu_ignore(ospf_if_config_row,OSPF_MTU_IGNORE_DEFAULT);

    ovsrec_ospf_interface_config_set_passive_interface(ospf_if_config_row,
                                                OSPF_PASSIVE_INTERFACE_DEFAULT);

    smap_destroy(&smap);
}


/* Get the area row matching the area id from the OSPF_Router table.*/
struct ovsrec_ospf_area *
ospf_area_get(const struct ovsrec_ospf_router *ospf_router_row,int area_id)
{
    struct ovsrec_ospf_area *area_row = NULL;
    int area;
    int i;

    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        area = ospf_router_row->key_areas[i];

        if (area == area_id)
        {
            area_row = ospf_router_row->value_areas[i];
            break;
        }
    }

    return area_row;
}

/* Insert the area row into the OSPF_Router table. */
void
ospf_area_insert_to_router(const struct ovsrec_ospf_router *ospf_router_row,
                         const struct ovsrec_ospf_area *area_row,
                         int64_t area_id)
{
    int64_t *area;
    struct ovsrec_ospf_area **area_list;
    int i = 0;

    /* Insert OSPF_Area table reference in OSPF_Router table. */
    area = xmalloc(sizeof(int64_t) * (ospf_router_row->n_areas + 1));
    area_list = xmalloc(sizeof * ospf_router_row->key_areas *
                              (ospf_router_row->n_areas + 1));
    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        area[i] = ospf_router_row->key_areas[i];
        area_list[i] = ospf_router_row->value_areas[i];
    }
    area[ospf_router_row->n_areas] = area_id;
    area_list[ospf_router_row->n_areas] =
                        CONST_CAST(struct ovsrec_ospf_area *, area_row);
    ovsrec_ospf_router_set_areas(ospf_router_row, area, area_list,
                               (ospf_router_row->n_areas + 1));

    free(area);
    free(area_list);
}


/* Get the interface row from the area table. */
struct ovsrec_ospf_interface *
ospf_area_interface_get(const struct ovsrec_ospf_area *area_row,
                               char *ifname)
{
    struct ovsrec_ospf_interface *interface_row = NULL;
    int i;
    char *interface_name;

    for (i = 0; i < area_row->n_ospf_interfaces; i++)
    {
        interface_name = area_row->key_ospf_interfaces[i];

        if (strcmp(interface_name, ifname) == 0)
        {
            interface_row = area_row->value_ospf_interfaces[i];
            break;
        }
    }

    return interface_row;

}

/* Set the values in the area table to default. */
void ospf_area_tbl_default(const struct ovsrec_ospf_area *area_row)
{
    struct smap smap = SMAP_INITIALIZER(&smap);

    if (area_row == NULL)
    {
        smap_destroy(&smap);
        return;
    }

    smap_init(&smap);

    smap_add(&smap, OSPF_KEY_AREA_TYPE, OSPF_AREA_TYPE_DEFAULT);
    smap_add(&smap, OSPF_KEY_AREA_NO_SUMMARY, OSPF_AREA_NO_SUMMARYDEFAULT);
    smap_add(&smap, OSPF_KEY_AREA_STUB_DEFAULT_COST,
                    OSPF_AREA_STUB_DEFAULT_COST);
    smap_add(&smap, OSPF_KEY_AREA_NSSA_TRANSLATOR_ROLE,
                    OSPF_AREA_NSSA_TRANSLATOR_ROLE);

    ovsrec_ospf_area_set_area_config(area_row,&smap);

    smap_destroy(&smap);
}


/* Set the reference to the interface row to the area table. */
void
ospf_area_set_interface(const struct ovsrec_ospf_area *area_row,
                         const struct ovsrec_ospf_interface *interface_row,
                         char *ifname)
{
    char **interface_list;
    struct ovsrec_ospf_interface **ospf_interface_list;
    int i = 0;

    /* Insert OSPF_Interface table reference in OSPF_Area table. */
    interface_list = xmalloc(sizeof(int64_t) * (area_row->n_ospf_interfaces+ 1));
    ospf_interface_list = xmalloc(sizeof * area_row->key_ospf_interfaces *
                              (area_row->n_ospf_interfaces + 1));
    for (i = 0; i < area_row->n_ospf_interfaces; i++) {
        interface_list[i] = area_row->key_ospf_interfaces[i];
        ospf_interface_list[i] = area_row->value_ospf_interfaces[i];
    }
    interface_list[area_row->n_ospf_interfaces] = ifname;
    ospf_interface_list[area_row->n_ospf_interfaces] =
                        CONST_CAST(struct ovsrec_ospf_interface *, interface_row);
    ovsrec_ospf_area_set_ospf_interfaces(area_row, interface_list, ospf_interface_list,
                               (area_row->n_ospf_interfaces + 1));

    free(interface_list);
    free(ospf_interface_list);
}


void
ospf_network_insert_ospf_router(const struct ovsrec_ospf_router *
                                             ospf_router_row,
                                             const int64_t area_id,
                                             const char *network_range)
{
    int64_t *area_list;
    char **network_list;
    int i = 0;

    network_list =
        xmalloc(OSPF_NETWORK_RANGE_LEN * (ospf_router_row->n_networks + 1));
    area_list =
        xmalloc(sizeof *ospf_router_row->value_networks *
                              (ospf_router_row->n_networks + 1));
    for (i = 0; i < ospf_router_row->n_networks; i++) {
        network_list[i] =
            ospf_router_row->key_networks[i];
        area_list[i] =
            ospf_router_row->value_networks[i];
    }
    network_list[ospf_router_row->n_networks] =
                                             CONST_CAST(char *, network_range);
    area_list[ospf_router_row->n_networks] = area_id;

    ovsrec_ospf_router_set_networks(ospf_router_row,
                                        network_list,
                                        area_list,
                                        (ospf_router_row->n_networks +
                                        1));
    free(network_list);
    free(area_list);
}


void
ospf_network_remove_from_ospf_router(
                        const struct ovsrec_ospf_router *ospf_router_row,
                        const int64_t area_id)
{
    int64_t *area_list;
    char **network_list;
    int i, j;

    network_list =
        xmalloc(80 * (ospf_router_row->n_networks - 1));
    area_list =
        xmalloc(sizeof * ospf_router_row->value_networks *
                (ospf_router_row->n_networks - 1));

    for (i = 0, j = 0; i < ospf_router_row->n_networks; i++) {
        if (ospf_router_row->value_networks[i] == area_id) {
            network_list[j] =
                ospf_router_row->key_networks[i];
            area_list[j] =
                ospf_router_row->value_networks[i];
            j++;
        }
    }

    ovsrec_ospf_router_set_networks(ospf_router_row,
                                        network_list,
                                        area_list,
                                        (ospf_router_row->n_networks -
                                        1));
    free(network_list);
    free(area_list);
}


static int
ospf_router_area_id_cmd_execute(bool no_flag, int instance_id,
                                const char *network_range, int64_t area_id)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    int64_t area;
    int i = 0, j = 0;

    /* TO DO: Check the validity of network range.*/

    OVSREC_OSPF_ROUTER_FOR_EACH(ospf_router_row,idl)
    {
        for (i = 0; i < ospf_router_row->n_networks; i++)
        {
            if ((strcmp(ospf_router_row->key_networks[i], network_range) == 0)
                && (ospf_router_row->value_networks[i] != area_id))
            {
                if(no_flag == false)
                {
                    OSPF_ABORT_DB_TXN(ospf_router_txn, "Remove the existing"
                        " area id before configuring a new one.");
                }
                else
                {
                    OSPF_ABORT_DB_TXN(ospf_router_txn, "Area id is not "
                        " configured.");
                }

                return CMD_WARNING;
            }
        }
    }

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not found.");
    }

    /* See if it already exists. */
    ospf_router_row = get_ovsrec_ospf_router_with_instance_id(vrf_row,
                                                              instance_id);

    /* If does not exist, create a new one. */
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not found.");
    }

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    if (!no_flag)
    {
        ospf_network_insert_ospf_router(ospf_router_row, area_id, network_range);
    }
    else
    {
        ospf_network_remove_from_ospf_router(ospf_router_row, area_id);
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

}


/* Check if any non default values is present in router table.
    If present then return false. Else true. */
int
ospf_is_router_tbl_empty(const struct ovsrec_ospf_router *ospf_router_row)
{
    const char *val;

    // Enable instance
    if (ospf_router_row->enable_instance != false)
        return false;

    // Router id
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && ( strcmp(val, OSPF_DEFAULT_STR) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_STATIC);
    if (val && ( strcmp(val, OSPF_ROUTER_ID_STATIC_DEFAULT) != 0))
        return false;

    // Area
    if (ospf_router_row->n_areas > 0)
        return false;

    // Distance
    val = smap_get(&ospf_router_row->distance, OSPF_KEY_DISTANCE_ALL);
    if (val && (atoi(val) != 0))
        return false;

    val = smap_get(&ospf_router_row->distance, OSPF_KEY_DISTANCE_EXTERNAL);
    if (val && (atoi(val) != 0))
        return false;

    val = smap_get(&ospf_router_row->distance, OSPF_KEY_DISTANCE_INTRA_AREA);
    if (val && (atoi(val) != 0))
        return false;

    val = smap_get(&ospf_router_row->distance, OSPF_KEY_DISTANCE_INTER_AREA);
    if (val && (atoi(val) != 0))
        return false;

    //Default information
    val = smap_get(&ospf_router_row->default_information, OSPF_KEY_DEFAULT_INFO_ORIG);
    if (val && ( strcmp(val, OSPF_DEFAULT_INFO_ORIG_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->default_information, OSPF_KEY_ALWAYS);
    if (val && ( strcmp(val, OSPF_ALWAYS_DEFAULT) != 0))
        return false;

    // Stub router config
    if (ospf_router_row->n_stub_router_config > 0)
        return false;

    if(ospf_router_row->passive_interface_default)
        return false;

    if (ospf_router_row->n_spf_config > 0)
        return false;

    if (ospf_router_row->n_summary_addresses > 0)
        return false;

    // Restart config
    val = smap_get(&ospf_router_row->restart_config, OSPF_KEY_RESTART_ENABLE_GRACEFUL);
    if (val && ( strcmp(val, OSPF_RESTART_ENABLE_GRACEFUL_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->restart_config, OSPF_KEY_RESTART_HELPER_DISABLE);
    if (val && ( strcmp(val, OSPF_RESTART_HELPER_DISABLE_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->restart_config, OSPF_KEY_RESTART_PLANNED_ONLY);
    if (val && ( strcmp(val, OSPF_RESTART_PLANNED_ONLY_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->restart_config, OSPF_KEY_RESTART_INTERVAL);
    if (val && ( strcmp(val, OSPF_RESTART_INTERVAL_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->restart_config, OSPF_KEY_RESTART_STRICT_LSA_CHECKING);
    if (val && ( strcmp(val, OSPF_RESTART_STRICT_LSA_CHECKING_DEFAULT) != 0))
        return false;

    // Router config
    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_ROUTER_DEFAULT_METRIC);
    if (val && ( strcmp(val, OSPF_RESTART_ROUTER_DEFAULT_METRIC_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_AUTO_COST_REF_BW);
    if (val && ( strcmp(val, OSPF_AUTO_COST_REF_BW_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_MAX_PATHS);
    if (val && ( strcmp(val, OSPF_MAX_PATHS_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_LOG_ADJACENCY_CHGS);
    if (val && ( strcmp(val, OSPF_LOG_ADJACENCY_CHGS_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_LOG_ADJACENCY_DETAIL);
    if (val && ( strcmp(val, OSPF_LOG_ADJACENCY_DETAIL_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_RFC1583_COMPATIBLE);
    if (val && ( strcmp(val, OSPF_RFC1583_COMPATIBLE_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_router_row->router_config, OSPF_KEY_ENABLE_OPAQUE_LSA);
    if (val && ( strcmp(val, OSPF_ENABLE_OPAQUE_LSA_DEFAULT) != 0))
        return false;

    // LSA timers
    if (ospf_router_row->n_lsa_timers_config > 0)
        return false;

    return true;
}

/* Check if any non default values is present in area table.
    If present then return false. Else true. */
int
ospf_is_area_tbl_empty(const struct ovsrec_ospf_area *ospf_area_row)
{
    const char *val;

    if (ospf_area_row->n_ospf_interfaces > 0)
        return false;

    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AREA_TYPE);
    if (val && ( strcmp(val, OSPF_AREA_TYPE_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AREA_NO_SUMMARY);
    if (val && ( strcmp(val, OSPF_AREA_NO_SUMMARYDEFAULT) != 0))
        return false;

    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AREA_STUB_DEFAULT_COST);
    if (val && ( strcmp(val, OSPF_AREA_STUB_DEFAULT_COST) != 0))
        return false;

    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AREA_NSSA_TRANSLATOR_ROLE);
    if (val && ( strcmp(val, OSPF_AREA_NSSA_TRANSLATOR_ROLE) != 0))
        return false;

    if (ospf_area_row->n_vlinks > 0)
        return false;

    if (ospf_area_row->n_area_range_addresses > 0)
        return false;

    val = smap_get(&ospf_area_row->authentication, OSPF_KEY_AUTH_CONF_TYPE);
    if (val && ( strcmp(val, OSPF_STRING_NULL) != 0))
        return false;

    val = smap_get(&ospf_area_row->authentication, OSPF_KEY_AUTH_CONF_KEY);
    if (val && ( strcmp(val, OSPF_STRING_NULL) != 0))
        return false;

    return true;
}

/* Check if any non default values is present in interface table.
    If present then return false. Else true. */
int
ospf_is_interface_tbl_empty(const struct
                                 ovsrec_ospf_interface *ospf_interface_row)
{
    char *val;

    if (strcmp(ospf_interface_row->name, "") != 0)
        return false;

    if (ospf_interface_row->port)
        return false;

    if (ospf_interface_row->vlink)
        return false;

    return true;
}

/* Check if any non default values is present in interface config table.
    If present then return false. Else true. */
int
ospf_is_interface_config_tbl_empty(const struct
                              ovsrec_ospf_interface_config *ospf_if_config_row)
{
    const char *val;
    int value;

    val = smap_get(&ospf_if_config_row->ospf_intervals, OSPF_KEY_TRANSMIT_DELAY);
    if (val && ( atoi(val) != OSPF_TRANSMIT_DELAY_DEFAULT))
        return false;

    val = smap_get(&ospf_if_config_row->ospf_intervals,
                   OSPF_KEY_RETRANSMIT_INTERVAL);
    if (val && ( atoi(val) != OSPF_RETRANSMIT_INTERVAL_DEFAULT))
        return false;

    val = smap_get(&ospf_if_config_row->ospf_intervals, OSPF_KEY_HELLO_INTERVAL);
    if (val && ( atoi(val) != OSPF_HELLO_INTERVAL_DEFAULT))
        return false;

    val = smap_get(&ospf_if_config_row->ospf_intervals, OSPF_KEY_DEAD_INTERVAL);
    if (val && ( atoi(val) != OSPF_ROUTER_DEAD_INTERVAL_DEFAULT))
        return false;

    value = ospf_if_config_row->priority;
    if (value != OSPF_ROUTER_PRIORITY_DEFAULT)
        return false;

    value = ospf_if_config_row->mtu_ignore;
    if (value != OSPF_MTU_IGNORE_DEFAULT)
        return false;

    if((ospf_if_config_row->passive_interface) &&
        (strcmp(ospf_if_config_row->passive_interface,
                OSPF_PASSIVE_INTERFACE_DEFAULT) != 0))
        return false;

    val = smap_get(&ospf_if_config_row->authentication, OSPF_KEY_AUTH_CONF_TYPE);
    if (val && (strcmp(val, OSPF_STRING_NULL)) != 0 )
        return false;

    val = smap_get(&ospf_if_config_row->authentication, OSPF_KEY_AUTH_CONF_KEY);
    if (val && (strcmp(val, OSPF_STRING_NULL)) != 0 )
        return false;

    if(ospf_if_config_row->n_nbma_nbr > 0)
        return false;


    return true;
}


/* Display one row of OSPF_Area. */
void
ospf_one_area_show(struct vty *vty,int64_t area_id,
                             const struct ovsrec_ospf_area *ospf_area_row)
{
    char area_str[OSPF_SHOW_STR_LEN];
    const char *val = NULL;
    int number_lsa = 0;

    /* Area id */
    OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
    if (area_id == 0)
    {
        vty_out(vty, "  Area ID:  %s (Backbone)%s", area_str, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Area ID:  %s %s", area_str, VTY_NEWLINE);
    }

    /* Number of interfaces */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_ACTIVE_INTERFACE);
    if(val)
    {
        vty_out(vty, "    Number of interfaces in this area: Total: %d,"
         " Active:%s %s", ospf_area_row->n_ospf_interfaces, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of interfaces in this area: Total: %d,"
            " Active:0 %s", ospf_area_row->n_ospf_interfaces, VTY_NEWLINE);
    }

    /* Fully adjacent neighbors */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_FULL_NEIGHBORS);
    if(val)
    {
        vty_out(vty, "    Number of fully adjacent neighbors in this area: %d%s",
                atoi(val), VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of fully adjacent neighbors in this area: 0%s",
                 VTY_NEWLINE);
    }

    /* auth type */
    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AUTH_CONF_TYPE);
    if(val)
    {
        vty_out(vty, "    Area has %s authentication: %s",
        strcmp(val, OVSREC_OSPF_AREA_AUTHENTICATION_MD5)== 0 ? "message digest": \
        strcmp(val, OVSREC_OSPF_AREA_AUTHENTICATION_TEXT) == 0 ? "text" : "no",
        VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Area has no authentication %s", VTY_NEWLINE);
    }

    /* SPF timestamp */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_SPF_LAST_RUN);
    if(val)
    {
        vty_out(vty, "    SPF algorithm last executed ago: %s%s",
                    val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    SPF algorithm last executed : 0ms ago%s",
                 VTY_NEWLINE);
    }

    /* SPF alg executed times */
    if ((ospf_area_row->key_area_statistics) &&
        (ospf_area_row->key_area_statistics[OSPF_AREA_STATISTICS_SPF_CALC]))
    {
       vty_out(vty, "    SPF algorithm executed %d times%s",
            ospf_area_row->value_area_statistics[OSPF_AREA_STATISTICS_SPF_CALC],
            VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    SPF algorithm executed 0 times%s",
             VTY_NEWLINE);
    }

    /* Number of LSA */
    number_lsa = ospf_area_row->n_abr_summary_lsas      \
                + ospf_area_row->n_asbr_summary_lsas    \
                + ospf_area_row->n_as_nssa_lsas         \
                + ospf_area_row->n_network_lsas         \
                + ospf_area_row->n_opaque_area_lsas     \
                + ospf_area_row->n_router_lsas          \
                + ospf_area_row->n_opaque_area_lsas     \
                + ospf_area_row->n_opaque_link_lsas;                    \

    vty_out(vty, "    Number of LSA %d %s", number_lsa, VTY_NEWLINE);

    /* Router LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_ROUTER_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of router LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_router_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of router LSA %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_router_lsas, VTY_NEWLINE);
    }

    /* Network LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_NETWORK_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of network LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_network_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of network LSA %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_network_lsas, VTY_NEWLINE);
    }

    /* ABR Summary LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_ABR_SUMMARY_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of ABR summary LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_abr_summary_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of ABR summary LSA %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_abr_summary_lsas, VTY_NEWLINE);
    }

    /* ASBR Summary LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_ASBR_SUMMARY_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of ASBR summary LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_asbr_summary_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of ASBR summary LSA %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_asbr_summary_lsas, VTY_NEWLINE);
    }

    /* NSSA LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_NSSA_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of NSSA LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_as_nssa_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of NSSA LSA %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_as_nssa_lsas, VTY_NEWLINE);
    }

    /* Opaque link LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_OPAQUE_LINK_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of opaque link LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_opaque_link_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of opaque link %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_opaque_link_lsas, VTY_NEWLINE);
    }

    /* Opaque area LSA */
    val = smap_get(&ospf_area_row->area_status, OSPF_KEY_AREA_OPAQUE_AREA_CHKSUM);
    if (val)
    {
        vty_out(vty, "    Number of opaque area LSA %d. Checksum Sum 0x%s %s",
                ospf_area_row->n_opaque_area_lsas, val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of opaque area %d. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_opaque_area_lsas, VTY_NEWLINE);
    }

}

/* Display the area table rows. */
void
ospf_area_show(struct vty *vty,
                        const struct ovsrec_ospf_router *ospf_router_row)
{
    const struct ovsrec_ospf_area *ospf_area_row = NULL;
    int i = 0;

    /* Loop through the areas and print them one at a time. */
    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        ospf_one_area_show(vty, ospf_router_row->key_areas[i],
                              ospf_router_row->value_areas[i]);

    }
}


void
ospf_border_router_show()
{

    /* This command will be implemented when "show ip route " command is implemented. */
}


void
ospf_ip_router_show(int br_flag)
{
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct smap smap = SMAP_INITIALIZER(&smap);
    const char *val = NULL;

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);

    /* See if it already exists. */
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, 1);
    if (ospf_router_row == NULL)
        return;

    /* Display only border router information */
    if (br_flag == true)
    {
        ospf_border_router_show();
    }
    /* Display all the router information */
    else
    {
        /* Router id */
        val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
        if (val && (strcmp(val, OSPF_DEFAULT_STR) != 0))
            vty_out(vty, "  OSPF Routing Process, Router ID:  %s%s",
                    val, VTY_NEWLINE);
        else
            vty_out(vty, "  OSPF Routing Process, Router ID:  %s%s",
                    " ", VTY_NEWLINE);

        vty_out(vty, "  This implementation conforms to RFC2328%s",
                VTY_NEWLINE);

        /* RFC 1583 compatibility */
        val = smap_get(&ospf_router_row->router_status, OSPF_KEY_RFC1583_COMPATIBLE);
        if (val)
        {
            vty_out(vty, "  RFC1583 Compatibility flag is %s%s",
                    (strcmp(val,"false") == 0)?"disabled":"enabled", VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  RFC1583 Compatibility flag is %s%s",
                                "disabled", VTY_NEWLINE);
        }

        /* Opaque capability */
        val = smap_get(&ospf_router_row->router_status, OSPF_KEY_CAPABILITY_OPAQUE);
        if (val)
        {
            vty_out(vty, "  Opaque Capability flag is %s%s",
                    (strcmp(val,"false") == 0)?"disabled":"enabled", VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  Opaque Capability flag is %s%s",
                                "disabled", VTY_NEWLINE);
        }

        /* Stub router support */
        val = smap_get(&ospf_router_row->router_status, OSPF_KEY_ENABLE_STUB_ROUTER_SUPPORT);
        if (val)
        {
            vty_out(vty, "  Stub router advertisement is %s%s",
                 (strcmp(val,"false") == 0)?"not configured":"configured",
                 VTY_NEWLINE);

        /* Stub Startup  time */
        if(ospf_router_row->key_stub_router_config[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP])
        {
           vty_out(vty, "      Enabled for %ds after start-up%s",
                ospf_router_row->value_stub_router_config[OSPF_ROUTER_STUB_ROUTER_CONFIG_STARTUP],
                VTY_NEWLINE);
            }
        }
        else
        {
            vty_out(vty, "  Stub router advertisement is %s%s",
                    "not configured", VTY_NEWLINE);
        }

        /* SPF scheduling delay */
        if(ospf_router_row->key_spf_config[OSPF_SPF_DELAY])
        {
            vty_out(vty, "  Initial SPF scheduling delay %d millisec(s)%s",
                 ospf_router_row->value_spf_config[OSPF_SPF_DELAY],
                 VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  Initial SPF scheduling delay %d millisec(s)%s",
                                 0, VTY_NEWLINE);
        }

        /* Minimum hold time */
        if(ospf_router_row->key_spf_config[OSPF_SPF_HOLD_TIME])
        {
            vty_out(vty, "  Minimum hold time between consecutive SPFs %d millisec(s)%s",
                 ospf_router_row->value_spf_config[OSPF_SPF_HOLD_TIME],
                 VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  Minimum hold time between consecutive SPFs %d millisec(s)%s",
                                 0, VTY_NEWLINE);
        }

        /* Maximum hold time */
        if(ospf_router_row->key_spf_config[OSPF_SPF_MAX_WAIT])
        {
            vty_out(vty, "  Maximum hold time between consecutive SPFs %d millisec(s)%s",
                 ospf_router_row->value_spf_config[OSPF_SPF_MAX_WAIT],
                 VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  Maximum hold time between consecutive SPFs %d millisec(s)%s",
                                 0, VTY_NEWLINE);
        }

        /* SPF hold multiplier */
        val = smap_get(&ospf_router_row->router_status, OSPF_KEY_SPF_HOLD_MULTIPLIER);
        if (val)
        {
            vty_out(vty, "  Hold time multiplier is currently %s%s",
                 val,
                 VTY_NEWLINE);
        }

        /* ABR type */
        val = smap_get(&ospf_router_row->router_status, OSPF_KEY_ROUTER_STATUS_ABR);
        if (val)
        {
            vty_out(vty, "  This router is an ABR. %s", VTY_NEWLINE);
        }
        else
        {
            val = smap_get(&ospf_router_row->router_status, OSPF_KEY_ROUTER_STATUS_ASBR);
            if (val)
            {
                vty_out(vty, "  This router is an ASBR, %s", VTY_NEWLINE);
            }
        }

        /* Number of external LSA */
        vty_out(vty, "  Number of external LSA %d. Checksum Sum 0x00000000%s",
                        ospf_router_row->n_as_ext_lsas, VTY_NEWLINE);

        /* Number of opaque AS LSA */
        vty_out(vty, "  Number of opaque AS LSA %d. Checksum Sum 0x00000000%s",
                        ospf_router_row->n_opaque_as_lsas, VTY_NEWLINE);

        /* Number of areas */
        vty_out(vty, "  Number of areas attached to this router: %d%s",
                        ospf_router_row->n_areas, VTY_NEWLINE);

        /* Adjacency logging */
        val = smap_get(&ospf_router_row->router_config, OSPF_KEY_LOG_ADJACENCY_CHGS);
        if (val)
        {
            vty_out(vty, "  All adjacency changes are %s%s",
                 (strcmp(val,"false") == 0)?"not logged":"logged",
                 VTY_NEWLINE);
        }
        else
        {
            vty_out(vty, "  All adjacency changes are %s%s",
                                "not logged", VTY_NEWLINE);
        }

        ospf_area_show(vty, ospf_router_row);
    }

}

char *
ospf_interface_type_convert(const char *intf_type)
{
    if(strcmp(intf_type, "ospf_iftype_none") == 0)
        return "NONE";
    else if(strcmp(intf_type, "ospf_iftype_broadcast") == 0)
            return "BROADCAST";
    else if(strcmp(intf_type, "ospf_iftype_pointopoint") == 0)
            return "POINT TO POINT";
    else if(strcmp(intf_type, "ospf_iftype_nbma") == 0)
            return "NBMA";
    else if(strcmp(intf_type, "ospf_iftype_pointomultipoint") == 0)
            return "POINT TO MULTIPOINT";
    else if(strcmp(intf_type, "ospf_iftype_virtuallink") == 0)
            return "VIRTUAL LINK";
    else if(strcmp(intf_type, "ospf_iftype_loopback") == 0)
            return "LOOPBACK";
    else
        return "NONE";
}

/* Function to convert the interface FSM to printable string. */
char *
ospf_ifsm_convert(const char *if_state)
{
    if(strcmp(if_state, "depend_upon") == 0)
        return "Depend Upon";
    else if(strcmp(if_state, "down") == 0)
            return "Down";
    else if(strcmp(if_state, "loopback") == 0)
            return "Loopback";
    else if(strcmp(if_state, "point_to_point") == 0)
            return "Point-to_point";
    else if(strcmp(if_state, "dr_other") == 0)
            return "DR other";
    else if(strcmp(if_state, "dr_backup") == 0)
            return "DR Backup";
    else if(strcmp(if_state, "dr") == 0)
            return "DR";
    else
        return "NONE";
}

void
ospf_interface_one_row_print(struct vty *vty,const char* ifname,
                        const struct ovsrec_ospf_interface *ospf_interface_row)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *interface_row = NULL;
    const struct ovsrec_ospf_area *ospf_area_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_ospf_interface_config *ospf_intf_config_row = NULL;
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    int i, j, n_adjacent_nbrs = 0;
    int64_t area_id = 0, router_id = 0;
    const char *val = NULL; int value;
    bool is_dr_present = false;
    bool is_bdr_present = false;
    bool is_present = false;
    const struct ovsrec_vrf *vrf_row;
    int instance_id = 1;
    char timebuf[OSPF_TIME_SIZE];

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, instance_id);

    // Get the port row.
    port_row = ospf_interface_row->port;
    ospf_intf_config_row = port_row->ospf_if_config;

    /* Get the interface row for the interface name passed. */
    OVSREC_INTERFACE_FOR_EACH(interface_row, idl)
    {
        if (strcmp(interface_row->name, ifname) == 0)
            break;
    }

    if (strcmp(interface_row->name, ifname) != 0)
    {
        vty_out(vty, "Interface name %s is not present.%s", ifname, VTY_NEWLINE);
        return;
    }

    OVSREC_OSPF_AREA_FOR_EACH(ospf_area_row, idl)
    {
        for (j = 0; j < ospf_area_row->n_ospf_interfaces; j++)
        {
            if (strcmp(ospf_area_row->key_ospf_interfaces[j], ifname) == 0)
            {
                is_present = true;
                break;
            }
        }
        if (is_present == true)
            break;
    }

    /* Get the area id from the router table */
    if ((strcmp(ospf_area_row->key_ospf_interfaces[j], ifname) == 0) && (ospf_router_row != NULL))
    {
        for (i = 0; i < ospf_router_row->n_areas; i++)
        {
            if (ospf_router_row->value_areas[i] == ospf_area_row)
            {
                area_id = ospf_router_row->key_areas[i];
                break;
            }
        }
    }
    else
    {
        ospf_area_row = NULL;
        ospf_router_row = NULL;
    }

    /* Ifname and state */
    vty_out(vty, "  %s is %s%s","not logged", ifname,
                interface_row->admin_state);

    /* MTU, Admin and oper states */
    if(interface_row->mtu)
    {
        vty_out(vty, " MTU %s bytes,", interface_row->mtu);
    }
    if(interface_row->link_speed)
    {
        vty_out(vty, " BW %s Mbps", interface_row->link_speed);
    }
    if(interface_row->admin_state)
    {
        vty_out(vty, "  <%s,%s,", interface_row->admin_state, "BROADCAST");

        if(interface_row->link_state)
        {
            vty_out(vty, " %s", interface_row->link_state);
        }
        vty_out(vty, " >");
    }
    vty_out(vty, "%s", VTY_NEWLINE);

    /* IP */
    if (port_row->ip4_address) {
        vty_out(vty, "  Internet address %s", port_row->ip4_address);
    }

    /* Broadcast  -- TBD Is this  column name="ospf_if_config" key="intf_type*/
    vty_out(vty, "  Broadcast %s", "");

    if(ospf_area_row != NULL)
    {
        vty_out(vty, "Area %d", area_id);
    }

    val = smap_get(&ospf_area_row->area_config, OSPF_KEY_AREA_TYPE);
    if(val && ( strcmp(val, OSPF_AREA_TYPE_NSSA) != 0))
    {
        vty_out(vty, " [NSSA]");
    }
    else if(val && ( strcmp(val, OSPF_AREA_TYPE_STUB) != 0))
        {
            vty_out(vty, " [Stub]");
        }
    vty_out(vty, "%s", VTY_NEWLINE);

    /* MTU ignore */
    value = ospf_intf_config_row->mtu_ignore;
    if (value)
    {
        vty_out(vty, "  MTU mismatch detection: not enabled%s", VTY_NEWLINE);
    }
    else
        vty_out(vty, "  MTU mismatch detection: enabled%s", VTY_NEWLINE);

    /* Router ID */
    if (ospf_router_row != NULL)
    {
        val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
        if(val)
        {
            vty_out(vty, "  Router ID : %s,", val);
            router_id = atoi(val);
        }
        else
        {
            vty_out(vty, "  Router ID : %s,", OSPF_DEFAULT_STR);
            router_id = 0;
        }
    }
    else
    {
        vty_out(vty, "  Router ID : %s,", OSPF_DEFAULT_STR);
        router_id = 0;
    }

    /* Interface type */
    val = ospf_intf_config_row->interface_type;
    if (val)
    {
        vty_out(vty, " Network Type %s,",ospf_interface_type_convert(val));
    }
    else
    {
        vty_out(vty, " Network Type NONE,");
    }

    /* cost */
    value = ospf_intf_config_row->if_out_cost;
    if (value)
    {
        vty_out(vty, " Cost: %s %s",val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, " Cost: %s %s","0", VTY_NEWLINE);
    }

    /* Transmit delay */
    val = smap_get(&ospf_intf_config_row->ospf_intervals,
                    OSPF_KEY_TRANSMIT_DELAY);
    if (val)
    {
        vty_out(vty, "  Transmit Delay is %s sec,",val);
    }
    else
    {
        vty_out(vty, "  Transmit Delay is %d sec,", OSPF_TRANSMIT_DELAY_DEFAULT);
    }

    /* State */
    vty_out(vty, " State %s,",ospf_ifsm_convert(ospf_interface_row->ifsm_state));

    /* Priority */
    value = ospf_intf_config_row->priority;
    vty_out(vty, " Priority %d %s",value, VTY_NEWLINE);

    /* Parse through the neighbor table and get information */
    is_dr_present = false;
    is_bdr_present = false;
    n_adjacent_nbrs = 0;
    for (j = 0; j < ospf_interface_row->n_neighbors; j++)
    {
        char show_str[OSPF_SHOW_STR_LEN];

        ospf_nbr_row = ospf_interface_row->neighbors[j];
        /* DR */
        if(((!is_dr_present) || (!is_bdr_present))
            && (ospf_nbr_row->nbr_router_id == router_id))
        {
            if(!is_dr_present && (ospf_nbr_row->dr)
                && ospf_nbr_row->nbr_if_addr)
            {
                memset(show_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->dr);
                vty_out(vty, "  Designated Router (ID) %s,", show_str);

                memset(show_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->nbr_if_addr);
                vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
                is_dr_present = true;
            }

            /* BDR */
            if(!is_bdr_present && (ospf_nbr_row->bdr)
                && ospf_nbr_row->nbr_if_addr)
            {
                memset(show_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->bdr);
                vty_out(vty, "  Backup Designated Router (ID) %s,", show_str);

                memset(show_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->nbr_if_addr);
                vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
                is_dr_present = true;
            }

        }

        /* Count number of adjacent neighbor */
        if(strcmp(ospf_nbr_row->nfsm_state, OSPF_NEIGHBOR_FSM_FULL) == 0)
        {
            n_adjacent_nbrs++;
        }
    }

    if(is_dr_present || is_bdr_present)
    {
        vty_out(vty, "  Multicast group memberships: OSPFAllRouters "   \
            "OSPFDesignatedRouters %s", VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Multicast group memberships: OSPFAllRouters %s",
                    VTY_NEWLINE);
    }

    /* Timer intervals */
    // Hello
    val = smap_get(&ospf_intf_config_row->ospf_intervals, OSPF_KEY_HELLO_INTERVAL);
    if (val)
    {
        vty_out(vty, "  Timer intervals configured, Hello %s", val);
    }
    else
    {
        vty_out(vty, "  Timer intervals configured, Hello %d",
                    OSPF_HELLO_INTERVAL_DEFAULT);
    }

    // Dead
    val = smap_get(&ospf_intf_config_row->ospf_intervals, OSPF_KEY_DEAD_INTERVAL);
    if (val)
    {
        vty_out(vty, " Dead %s", val);
    }
    else
    {
        vty_out(vty, " Dead %d", OSPF_HELLO_INTERVAL_DEFAULT);
    }

    // Wait
    val = smap_get(&ospf_intf_config_row->ospf_intervals, OSPF_KEY_DEAD_INTERVAL);
    if (val)
    {
        vty_out(vty, " wait %s", val);
    }
    else
    {
        vty_out(vty, " wait %d", OSPF_ROUTER_DEAD_INTERVAL_DEFAULT);
    }

    // Retransmit
    val = smap_get(&ospf_intf_config_row->ospf_intervals, OSPF_KEY_RETRANSMIT_INTERVAL);
    if (val)
    {
        vty_out(vty, " Retransmit %s%s", val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, " Retransmit %d%s", OSPF_RETRANSMIT_INTERVAL_DEFAULT,
                VTY_NEWLINE);
    }

    // Hello due in
    val =smap_get(&ospf_interface_row->if_status, OSPF_KEY_INTERFACE_ACTIVE);
    if (val)
    {
        if (atoi(val) == true)
        {
            val = smap_get(&ospf_interface_row->if_status, OSPF_KEY_HELLO_DUE);
            if (val)
            {
                vty_out(vty, "    Hello due in  %s %s",
                        ospf_timer_adjust(val, timebuf, sizeof(timebuf)),
                        VTY_NEWLINE);
            }
            else
            {
                vty_out(vty, "    Hello due in  %ds %s", 0, VTY_NEWLINE);
            }
        }
        else
        {
            vty_out(vty, "    no Hello (Passive Interface) %s", VTY_NEWLINE);
        }
    }
    else // Interface is not active.
    {
        vty_out(vty, "    no Hello (Passive Interface) %s", VTY_NEWLINE);
    }

    // Neighbor count and Adjacent neighbor count
    if (ospf_interface_row->n_neighbors)
        vty_out(vty, "  Neighbor Count is %s, Adjacent neighbor count is %d%s",
            ospf_interface_row->n_neighbors, n_adjacent_nbrs, VTY_NEWLINE);
    else
        vty_out(vty, "  Neighbor Count is 0, Adjacent neighbor count is %d%s",
                n_adjacent_nbrs, VTY_NEWLINE);

}

void
ospf_ip_router_interface_show(const char* ifname)
{
    const struct ovsrec_ospf_interface *ospf_interface_row;

    if (ifname != NULL)
    {
        /* Print interface details for the interface name passed. */
        OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            if (strcmp(ospf_interface_row->name, ifname) == 0)
                ospf_interface_one_row_print(vty, ifname, ospf_interface_row);
        }
    }
    else
    {
        /* Print all the interfaces present */
        OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            ospf_interface_one_row_print(vty, ospf_interface_row->name,
                                         ospf_interface_row);
        }
    }
}

void
ospf_neighbor_one_row_print(
                                const struct ovsrec_ospf_neighbor *ospf_nbr_row,
                                bool all_flag)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_interface *interface_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    int instance_id = 1, j;
    const char *val = NULL;
    struct in_addr id;
    int ret;
    char show_str[OSPF_SHOW_STR_LEN];
    char state_str[OSPF_SHOW_STR_LEN];
    char timebuf[OSPF_TIME_SIZE];
    bool is_present = false;

    memset (&id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, instance_id);

    // Get router id from the OSPF_Router table.
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && ( strcmp(val, OSPF_DEFAULT_STR) != 0))
    {
        /* Convert the router id to integer. */
        ret = inet_aton (val, &id);
        if (!ret || (id.s_addr == 0))
        {
            VLOG_DBG("Could not display nbr. Router id - %s", val);
            return;
        }

        /* Check if the neighbor row is self entry. */
        if (id.s_addr == ospf_nbr_row->nbr_router_id)
            return;
    }

    /* Dont print the "down" neighbors if "all" was not entered. */
    if((strcmp(ospf_nbr_row->nfsm_state, OSPF_NEIGHBOR_FSM_DOWN) == 0) &&
       (all_flag != true))
        return;

    /* Nbr router id */
    memset(show_str,'\0', OSPF_SHOW_STR_LEN);
    OSPF_IP_STRING_CONVERT(show_str, ospf_nbr_row->nbr_router_id);

    /* Priority and State. */
    memset(state_str,'\0', OSPF_SHOW_STR_LEN);
    if ((ospf_nbr_row->dr) && (*ospf_nbr_row->dr == id.s_addr))
    {
        snprintf(state_str, OSPF_SHOW_STR_LEN, "%s/%s", ospf_nbr_row->nfsm_state, "DR");
    }
    else if ((ospf_nbr_row->bdr) && (*ospf_nbr_row->bdr == id.s_addr))
    {
        snprintf(state_str, OSPF_SHOW_STR_LEN, "%s/%s", ospf_nbr_row->nfsm_state, "BDR");
    }
    else
    {
        snprintf(state_str, OSPF_SHOW_STR_LEN, "%s/%s", ospf_nbr_row->nfsm_state, "DROther");
    }

    vty_out(vty, "%-15s %3d %-15s ", show_str, ospf_nbr_row->priority,
            state_str);

    val = smap_get(&ospf_nbr_row->nbr_status, OSPF_KEY_NEIGHBOR_DEAD_TIMER_DUE);
    vty_out (vty, "%9s ", ospf_timer_adjust(val, timebuf, sizeof(timebuf)));

    if (ospf_nbr_row->nbr_if_addr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->nbr_if_addr);
        vty_out (vty, "%-15s ", show_str);
    }
    else
        vty_out (vty, "%-15s ", "null");

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(interface_row, idl)
    {
        for (j = 0; j < interface_row->n_neighbors; j++)
        {
            if (interface_row->neighbors[j] == ospf_nbr_row)
            {
                port_row = interface_row->port;
                snprintf(show_str, OSPF_SHOW_STR_LEN, "%s:%s",
                         interface_row->name,
                        port_row->ip4_address);
                vty_out (vty, "%-20s ", show_str);
                is_present = true;
                break;
            }
        }
    }

    /* This condition should not occur, as this would mean we have nbr without matching
            interface. */
    if (is_present == false)
    {
        vty_out (vty, "%-20s ", "NULL");
    }

    vty_out (vty, "%5ld %5ld %5d%s",
        ospf_get_statistics_from_neighbor(ospf_nbr_row,
        OSPF_KEY_NEIGHBOR_LS_RE_TRANSMIT_CNT),
        ospf_get_statistics_from_neighbor(ospf_nbr_row,
        OSPF_KEY_NEIGHBOR_LS_REQUEST_CNT),
        ospf_get_statistics_from_neighbor(ospf_nbr_row,
        OSPF_KEY_NEIGHBOR_DB_SUMMARY_CNT), VTY_NEWLINE);

}

void
ospf_neighbor_one_row_detail_print(
                                const struct ovsrec_ospf_neighbor *ospf_nbr_row,
                                bool all_flag)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_interface *interface_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    int instance_id = 1, i= 0, j, k;
    const char *val = NULL;
    struct in_addr id;
    int ret;
    char show_str[OSPF_SHOW_STR_LEN];
    char state_str[OSPF_SHOW_STR_LEN];
    char timebuf[OSPF_TIME_SIZE];
    bool is_present = false;

    memset (&id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, instance_id);

    // Get router id from the OSPF_Router table.
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && ( strcmp(val, OSPF_DEFAULT_STR) != 0))
    {
        /* Convert the router id to integer. */
        ret = inet_aton (val, &id);
        if (!ret || (id.s_addr == 0))
        {
            VLOG_DBG("Could not display nbr. Router id - %s", val);
            return;
        }

        /* Check if the neighbor row is self entry. */
        if (id.s_addr == ospf_nbr_row->nbr_router_id)
            return;
    }

    /* Dont print the "down" neighbors if "all" was not entered. */
    if((strcmp(ospf_nbr_row->nfsm_state, OSPF_NEIGHBOR_FSM_DOWN) == 0) &&
       (all_flag != true))
        return;

    /* Nbr router id */
    memset(show_str,'\0', OSPF_SHOW_STR_LEN);
    OSPF_IP_STRING_CONVERT(show_str, ospf_nbr_row->nbr_router_id);
    vty_out (vty, "Neighbor %s", show_str);

    if (ospf_nbr_row->nbr_if_addr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->nbr_if_addr);
        vty_out (vty, ",  interface address %s", show_str);
    }

    vty_out(vty, "%s", VTY_NEWLINE);

    /* Get the matching interface row */
    is_present = false;
    OVSREC_OSPF_INTERFACE_FOR_EACH(interface_row, idl)
    {
        for (i = 0; i < interface_row->n_neighbors; i++)
        {
            if (interface_row->neighbors[i] == ospf_nbr_row)
            {
                is_present = true;
                break;
            }
        }
        if (is_present)
            break;
    }

    is_present = false;
    /* Get the matching area row */
    OVSREC_OSPF_AREA_FOR_EACH(area_row, idl)
    {
        for(i = 0; i< area_row->n_ospf_interfaces; i++)
        {
            if (area_row->value_ospf_interfaces[i] == interface_row)
            {
                is_present = true;
                break;
            }
        }

        if (is_present)
            break;
    }

    is_present = false;
    for(j =0; j< ospf_router_row->n_areas; j++)
    {
        if (ospf_router_row->value_areas[j] == area_row)
        {
            vty_out (vty, "    In the area %d ",
                     ospf_router_row->key_areas[j]);
            break;
        }
    }

    if (area_row)
    {
        val = smap_get(&area_row->area_config, OSPF_KEY_AREA_TYPE);
        if(val && ( strcmp(val, OSPF_AREA_TYPE_NSSA) != 0))
        {
            vty_out(vty, " [NSSA] ");
        }
        else if(val && ( strcmp(val, OSPF_AREA_TYPE_STUB) != 0))
        {
            vty_out(vty, " [Stub] ");
        }
    }

    if (interface_row)
    {
        vty_out(vty, "via interface %s%s", interface_row->name,
                VTY_NEWLINE);
    }

    vty_out (vty, "    Neighbor priority is %d, State is %s, %d state changes%s",
             ospf_nbr_row->priority, ospf_nbr_row->nfsm_state,
             ospf_get_statistics_from_neighbor(ospf_nbr_row,
             OSPF_KEY_NEIGHBOR_STATE_CHG_CNT),
             VTY_NEWLINE);

    /* Neighbor uptime */
    val = smap_get(&ospf_nbr_row->nbr_status,
                   OSPF_KEY_NEIGHBOR_LAST_UP_TIMESTAMP);
    vty_out (vty, "    Neighbor is up for %s%s",
                     ospf_neighbor_time_print(val, timebuf, sizeof(timebuf)),
                     VTY_NEWLINE);

    if (ospf_nbr_row->dr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->dr);
        vty_out (vty, "    DR is %s ", show_str);
    }

    if (ospf_nbr_row->bdr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, *ospf_nbr_row->bdr);
        vty_out (vty, ",BDR is %s ", show_str);
    }
    vty_out (vty, "%s", VTY_NEWLINE);
    /* SUGANYA should we decode the options too */
    vty_out (vty, "    Options %d (%s)%s", ospf_nbr_row->nbr_options,
             ospf_options_print(ospf_nbr_row->nbr_options), VTY_NEWLINE);

    val = smap_get(&ospf_nbr_row->nbr_status, OSPF_KEY_NEIGHBOR_DEAD_TIMER_DUE);
    vty_out (vty, "    Dead timer due in %s %s ",
             ospf_timer_adjust(val, timebuf, sizeof(timebuf)), VTY_NEWLINE);

    vty_out (vty, "   Database Summary List %d%s",
                ospf_get_statistics_from_neighbor(ospf_nbr_row,
                OSPF_KEY_NEIGHBOR_DB_SUMMARY_CNT), VTY_NEWLINE);

    vty_out (vty, "    Link State Request List %d %s",
            ospf_get_statistics_from_neighbor(ospf_nbr_row,
                            OSPF_KEY_NEIGHBOR_LS_REQUEST_CNT), VTY_NEWLINE);

    vty_out (vty, "    Link State Retransmission List %d %s",
            ospf_get_statistics_from_neighbor(ospf_nbr_row,
                        OSPF_KEY_NEIGHBOR_LS_RE_TRANSMIT_CNT),
             VTY_NEWLINE);

}

void
ospf_ip_router_neighbor_detail_show(const char* ifname,
                                             int nbr_id,
                                             bool all_flag)
{
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;


    /* Print all the rows */
    OVSREC_OSPF_NEIGHBOR_FOR_EACH(ospf_nbr_row, idl)
    {
        ospf_neighbor_one_row_detail_print(ospf_nbr_row, all_flag);
    }
}

static void
show_ip_ospf_neighbour_header (struct vty *vty)
{
  vty_out (vty, "%s%15s %3s %-15s %9s %-15s %-20s %5s %5s %5s%s",
           VTY_NEWLINE,
           "Neighbor ID", "Pri", "State", "Dead Time",
           "Address", "Interface", "RXmtL", "RqstL", "DBsmL",
           VTY_NEWLINE);
}

void
ospf_ip_router_neighbor_show(const char* ifname,
                                     int nbr_id,
                                     bool all_flag)
{
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;

    /* Print the header */
    show_ip_ospf_neighbour_header(vty);

    if (ifname != NULL)
    {
        /* Print interface details for the interface name passed. */
        OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            if (strcmp(ospf_interface_row->name, ifname) == 0)
            {
                //ospf_neighbor_one_row_print(ifname, ospf_interface_row);
            }
        }
    }
    else if (nbr_id != 0)
    {
        /* Print all the interfaces present */
        OVSREC_OSPF_NEIGHBOR_FOR_EACH(ospf_nbr_row, idl)
        {

        }
    }
    else
    {
        /* Print all the rows */
        OVSREC_OSPF_NEIGHBOR_FOR_EACH(ospf_nbr_row, idl)
        {
            ospf_neighbor_one_row_print(ospf_nbr_row, all_flag);
        }
    }
}

void
ospf_running_config_show()
{
    const struct ovsrec_vrf *ovs_vrf = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    int i = 0, j = 0;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];

    OVSREC_VRF_FOR_EACH(ovs_vrf, idl)
    {
        for (j = 0; j < ovs_vrf->n_ospf_routers; j++)
        {
            vty_out (vty, "%s%s", "router ospf", VTY_NEWLINE);

            ospf_router_row = ovs_vrf->value_ospf_routers[j];

            /* enable */
            if (!ospf_router_row->enable_instance)
            {
                vty_out(vty, "%4s%s%s", "", "no enable", VTY_NEWLINE);
            }

            /* Router id */
            val = smap_get((const struct smap *)&ospf_router_row->router_id,
                            OSPF_KEY_ROUTER_ID_VAL);
            if (val && (strcmp(val, OSPF_DEFAULT_STR) != 0))
                vty_out(vty, "%4s%s %s%s", "",
                        "router-id", val, VTY_NEWLINE);

            /* network <range> area <area-id>*/
            while (i < ospf_router_row->n_networks)
            {
                memset(area_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(area_str, ntohl(ospf_router_row->value_networks[i]));
                vty_out(vty, "%4snetwork %s area %s%s", "",
                                      ospf_router_row->key_networks[i],
                                      area_str,
                                      VTY_NEWLINE);
                i++;
            }
        }
    }

    return;
}

DEFUN(cli_ospf_router,
      cli_ospf_router_cmd,
      "router ospf",
      ROUTER_STR
      OSPF_CONF_STR)
{
    int instance_tag = 1;

    /* When VRF and instance is supported proper values will be passed */
    if (ospf_router_cmd_execute(DEFAULT_VRF_NAME, instance_tag) == CMD_SUCCESS)
    {
        /* Get the context from previous command for sub-commands. */
        vty->node = OSPF_NODE;
        vty->index = (void*) instance_tag;
    }

    return CMD_SUCCESS;
}

DEFUN(cli_no_ospf_router,
      cli_no_ospf_router_cmd,
      "no router ospf",
      NO_STR
      ROUTER_STR
      OSPF_CONF_STR)
{
    /* When VRF and instance is supported proper values will be passed */
    ospf_no_router_cmd_execute(DEFAULT_VRF_NAME, 1);
    return CMD_SUCCESS;
}

DEFUN(cli_ospf_global_enable,
      cli_ospf_global_enable_cmd,
      "ospf enable",
      OSPF_CONF_STR
      ENABLE_STR)
{
    ospf_global_enable(true);
    return CMD_SUCCESS;
}

DEFUN(cli_no_ospf_global_enable,
      cli_no_ospf_global_enable_cmd,
      "no ospf enable",
      NO_STR
      OSPF_CONF_STR
      ENABLE_STR)
{
    ospf_global_enable(false);
    return CMD_SUCCESS;
}

/*  router-id. */
DEFUN(cli_ospf_router_id,
      cli_ospf_router_id_cmd,
      "router-id A.B.C.D",
      "Configure router identifier for the OSPF process\n"
      "OSPF router-id in IP address format\n")
{
    return ospf_router_id_cmd_execute(DEFAULT_VRF_NAME, CONST_CAST(char*, argv[0]));
}

/* no  router-id. */
DEFUN(cli_no_ospf_router_id,
         cli_no_ospf_router_id_cmd,
         "no router-id",
         NO_STR
         "Configure router identifier for the OSPF process\n")
{
    return ospf_no_router_id_cmd_execute(DEFAULT_VRF_NAME);
}

/* enable */
DEFUN(cli_ospf_enable_instance,
         cli_ospf_enable_instance_cmd,
         "enable",
         "Enable the OSPF process\n")
{
    return ospf_router_enable_cmd_execute(DEFAULT_VRF_NAME, true);
}

/* no enable */
DEFUN(cli_no_ospf_enable_instance,
         cli_no_ospf_enable_instance_cmd,
         "no enable",
         NO_STR
         "Enable the OSPF process\n")
{
    return ospf_router_enable_cmd_execute(DEFAULT_VRF_NAME, false);
}

/* network area <area-id> */
DEFUN(cli_ospf_router_network_area_id,
         cli_ospf_router_network_area_id_cmd,
         "network A.B.C.D/M area " OSPF_CMD_AS_RANGE,
         OSPF_NETWORK_STR
         OSPF_NETWORK_RANGE_STR
         OSPF_AREA_STR
         OSPF_AREA_RANGE)
{
    int ret;
    struct in_addr id;
    char ip_str[OSPF_SHOW_STR_LEN];
    int instance_id = 1;
    int64_t area_id = 0;

    area_id = htonl(atoi(argv[1]));

    return ospf_router_area_id_cmd_execute(false, instance_id, argv[0], area_id);
}

/* network <network-range> area <area-ip> */
DEFUN(cli_ospf_router_network_area_ip,
         cli_ospf_router_network_area_ip_cmd,
         "network A.B.C.D/M area A.B.C.D",
         OSPF_NETWORK_STR
         OSPF_NETWORK_RANGE_STR
         OSPF_AREA_STR
         OSPF_AREA_IP_STR)

{
    int ret;
    struct in_addr id;
    int instance_id = 1;

    memset (&id, 0, sizeof (struct in_addr));

    if(ospf_string_is_an_ip_addr(argv[1]))
    {
        ret = inet_aton (argv[1], &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    return ospf_router_area_id_cmd_execute(false, instance_id, argv[0],id.s_addr);
}


/* no network area <area-id> */
DEFUN(cli_ospf_router_no_network_area_id,
         cli_ospf_router_no_network_area_id_cmd,
         "no network A.B.C.D/M area " OSPF_CMD_AS_RANGE,
         NO_STR
         OSPF_NETWORK_STR
         OSPF_NETWORK_RANGE_STR
         OSPF_AREA_STR
         OSPF_AREA_RANGE)
{
    int ret;
    struct in_addr id;
    char ip_str[OSPF_SHOW_STR_LEN];
    int instance_id = 1;

    memset (&id, 0, sizeof (struct in_addr));

    OSPF_IP_STRING_CONVERT(ip_str, atoi(argv[1]));

    ret = inet_aton (ip_str, &id);
    if (!ret || (id.s_addr == 0))
    {
        vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    return ospf_router_area_id_cmd_execute(true, instance_id, argv[0],
                                           id.s_addr);
}

/* no network <network-range> area <area-ip> */
DEFUN(cli_ospf_router_no_network_area_ip,
         cli_ospf_router_no_network_area_ip_cmd,
         "no network A.B.C.D/M area A.B.C.D",
         NO_STR
         OSPF_NETWORK_STR
         OSPF_NETWORK_RANGE_STR
         OSPF_AREA_STR
         OSPF_AREA_IP_STR)

{
    int ret;
    struct in_addr id;
    int instance_id = 1;

    memset (&id, 0, sizeof (struct in_addr));

    if(ospf_string_is_an_ip_addr(argv[1]))
    {
        ret = inet_aton (argv[1], &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    return ospf_router_area_id_cmd_execute(true, instance_id, argv[0],
                                           id.s_addr);
}


DEFUN (cli_ip_ospf_show,
          cli_ip_ospf_show_cmd,
          "show ip ospf",
          SHOW_STR
          IP_STR
          OSPF_STR)
{
    ospf_ip_router_show(false);
}

DEFUN (cli_ip_ospf_br_show,
          cli_ip_ospf_br_show_cmd,
          "show ip ospf border-routers",
          SHOW_STR
          IP_STR
          OSPF_STR
          BORDER_ROUTER_STR)
{
    ospf_ip_router_show(true);
}

DEFUN (cli_ip_ospf_interface_show,
          cli_ip_ospf_interface_show_cmd,
          "show ip ospf interface [IFNAME]",
          SHOW_STR
          IP_STR
          OSPF_STR
          INTERFACE_STR
          IFNAME_STR)
{
    if (argc == 1)
        ospf_ip_router_interface_show(argv[0]);
    else
        ospf_ip_router_interface_show(NULL);

}


DEFUN (cli_ip_ospf_neighbor_show,
          cli_ip_ospf_neighbor_show_cmd,
          "show ip ospf neighbor {all}",
          SHOW_STR
          IP_STR
          OSPF_STR
          NEIGHBOR_STR
          ALL_STR)
{
    if (argv[0])
        ospf_ip_router_neighbor_show(NULL, 0, true);
    else
        ospf_ip_router_neighbor_show(NULL, 0, false);
}

DEFUN (cli_ip_ospf_neighbor_detail_show,
          cli_ip_ospf_neighbor_detail_show_cmd,
          "show ip ospf neighbor detail {all}",
          SHOW_STR
          IP_STR
          OSPF_STR
          NEIGHBOR_STR
          DETAIL_STR
          ALL_STR)
{
    if (argv[0])
        ospf_ip_router_neighbor_detail_show(NULL, 0, true);
    else
        ospf_ip_router_neighbor_detail_show(NULL, 0, false);
}

DEFUN (cli_ip_ospf_running_config_show,
          cli_ip_ospf_running_config_show_cmd,
          "show running-config router ospf",
          SHOW_STR
          RUNNING_CONFIG_STR
          ROUTER_SHOW_STR
          OSPF_STR)
{
    ospf_running_config_show();
}

#ifndef OSPF_DEBUG

/******************************************************************************/
/*                      DEBUG CODE - START                                                                          */
/*                      This will be retained till the end of development                                      */
/******************************************************************************/

/* "ip ospf router area <area-id>" command handler.*/
static int
ospf_router_interface_area_add(int instance_tag, int area_id)
{
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface_config *ospf_if_config_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    const struct ovsdb_datum *if_config = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    int64_t values[OSPF_NUM_AREA_KEYS];
    char *keys[OSPF_NUM_AREA_KEYS];
    int i, j;
    int64_t area;
    char *interface_name;
    bool is_present = false;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    char *ifname = "1";


    OVSREC_OSPF_ROUTER_FOR_EACH(ospf_router_row,idl)
    {
    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        area = ospf_router_row->key_areas[i];

        if (area != area_id)
        {
            area_row = ospf_router_row->value_areas[i];
            for (j = 0; j < area_row->n_ospf_interfaces; j++)
            {
                interface_name = area_row->key_ospf_interfaces[j];
                if (strcmp(interface_name, ifname) == 0)
                {
                        OSPF_ABORT_DB_TXN(ospf_router_txn, "Remove the existing"
                            " area id before configuring new one");
                    return CMD_WARNING;
                    }
                }
            }
        }
    }

    port_row = ovsrec_port_first(idl);
    if (NULL == port_row)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "No port to add ospf "
                        "interface configurations.");
        return CMD_OVSDB_FAILURE;
    }

    OVSREC_PORT_FOR_EACH(port_row, idl)
    {
        for (i = 0; i < port_row->n_interfaces; i++)
        {
            if (strcmp(port_row->name, ifname) == 0)
            {
                is_present = true;
                break;
            }
        }
        if (is_present == true)
            break;
    }

    if (is_present == false)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "Interface not present in the port.");
        return CMD_OVSDB_FAILURE;
    }

    /* Dont allow configurations  in LACP port */
    if(port_row->n_interfaces > 1)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF configuration not allowed "
                        "in LACP port.");
        return CMD_WARNING;
    }

    ospf_if_config_row = port_row->ospf_if_config;
    /* If does not exist, create a new one. */
    if (ospf_if_config_row == NULL)
    {
        ospf_if_config_row = ovsrec_ospf_interface_config_insert(ospf_router_txn);
        ovsrec_port_set_ospf_if_config(port_row, ospf_if_config_row);

        /* Set the default values in OSPF_Interface_Config table */
        ospf_interface_if_config_tbl_default(ospf_if_config_row);
    }

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "no vrf found");
    }

    /* See if it already exists. */
    ospf_router_row = get_ovsrec_ospf_router_with_instance_id(vrf_row,
                                                              instance_tag);

    /* If does not exist, create a new one. */
    if (ospf_router_row == NULL)
    {
        ospf_router_row = ovsrec_ospf_router_insert(ospf_router_txn);
        ospf_router_insert_to_vrf(vrf_row, ospf_router_row, instance_tag);
        ospf_router_tbl_default(ospf_router_row);
    }

    /* Area table */
    area_row = ospf_area_get(ospf_router_row, area_id);
    if (area_row == NULL)
    {
        area_row = ovsrec_ospf_area_insert(ospf_router_txn);
        ospf_area_insert_to_router(ospf_router_row, area_row, area_id);
        ospf_area_tbl_default(area_row);
    }

    /* Update the area table with the interface information */
    ospf_interface_row = ospf_area_interface_get(area_row, ifname);
    if (ospf_interface_row == NULL)
    {
        ospf_interface_row = ovsrec_ospf_interface_insert(ospf_router_txn);
        ospf_area_set_interface(area_row, ospf_interface_row, ifname);
        ovsrec_ospf_interface_set_ifsm_state(ospf_interface_row,
                                             OSPF_INTERFACE_IFSM_DEPEND_ON);
        ovsrec_ospf_interface_set_name(ospf_interface_row, ifname);
    }

    ovsrec_ospf_interface_set_port(ospf_interface_row, port_row);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);
}

/*Funtion to get the statistics from neighbor table. */
int
ospf_set_nbr_statistics(const struct ovsrec_ospf_neighbor *
                             ovs_ospf_neighbor, const char *key, int64_t cnt)
{
    int i = 0;

    VLOG_DBG("key=%s\n", key);
    for (i = 0; i <ovs_ospf_neighbor->n_nbr_statistics; i++) {
        if (!strcmp(ovs_ospf_neighbor->key_nbr_statistics[i], key))
            ovs_ospf_neighbor->value_nbr_statistics[i] = cnt;
    }
    return 0;
}


void
ospf_neighbor_insert_to_interface(const struct ovsrec_ospf_interface *interface_row,
                         const struct ovsrec_ospf_neighbor *ospf_nbr_row)
{
    struct ovsrec_ospf_neighbor **ospf_nbr_list;
    int i = 0;

    /* Insert OSPF_Router table reference in VRF table. */
    ospf_nbr_list = xmalloc(sizeof * interface_row->neighbors * (interface_row->n_neighbors + 1));
    for (i = 0; i < interface_row->n_neighbors; i++) {
        ospf_nbr_list[i] = interface_row->neighbors[i];
    }

    /* Update the reference. */
    ospf_nbr_list[interface_row->n_neighbors] =
                      CONST_CAST(struct ovsrec_ospf_neighbor*, ospf_nbr_row);
    ovsrec_ospf_interface_set_neighbors(interface_row, ospf_nbr_list,
                               (interface_row->n_neighbors + 1));

    free(ospf_nbr_list);
}

static int
ospf_neighbor_add(bool self, bool dr_flag, bool bdr)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_interface *interface_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    int instance_id = 1, j;
    const char *val = NULL;
    struct in_addr id;
    int ret;
    char show_str[OSPF_SHOW_STR_LEN];
    char state_str[OSPF_SHOW_STR_LEN];
    char timebuf[OSPF_TIME_SIZE];
    bool is_present = false;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    int64_t dr;
    int k, i;
    struct smap smap = SMAP_INITIALIZER(&smap);

    smap_init(&smap);

    memset (&id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_ovsrec_vrf_with_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    get_ovsrec_ospf_router_with_instance_id(vrf_row, instance_id);


    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    ospf_nbr_row = ovsrec_ospf_neighbor_insert(ospf_router_txn);

    /* Router id */
    if (self == false)
        val = "1.1.1.2";
    else
        val = "1.1.1.1";

    inet_aton (val, &id);
    ovsrec_ospf_neighbor_set_nbr_router_id(ospf_nbr_row, id.s_addr);

    /* Set DR */
    if (dr_flag)
        val = "1.1.1.1";
    else
        val = "1.1.1.3";

    inet_aton (val, &id);
    dr = id.s_addr;

    ovsrec_ospf_neighbor_set_dr(ospf_nbr_row, &dr, 1);

    /* Set BDR */

    if (bdr)
        val = "1.1.1.1";
    else
        val = "1.1.1.5";

    inet_aton (val, &id);
    dr = id.s_addr;

    ovsrec_ospf_neighbor_set_bdr(ospf_nbr_row, &dr, 1);

    /* NFSM state */
    ovsrec_ospf_neighbor_set_nfsm_state(ospf_nbr_row, "full");

    /* Priority */
    ovsrec_ospf_neighbor_set_priority(ospf_nbr_row, 120);

    /* Nbr timer due */
    //smap_add((struct smap *)&ospf_nbr_row->nbr_status, OSPF_KEY_NEIGHBOR_DEAD_TIMER_DUE, "10");

    /* Interface Address */
    val = "5.5.5.5";
    inet_aton (val, &id);
    dr = id.s_addr;
    //ovsrec_ospf_neighbor_set_nbr_if_addr(ospf_nbr_row, &dr, 1);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(interface_row, idl)
    {
        if (strcmp(interface_row->name, "1") == 0)
        {
            ospf_neighbor_insert_to_interface(interface_row, ospf_nbr_row);
            //ovsrec_ospf_interface_set_neighbors(interface_row, &ospf_nbr_row,
            //                   (interface_row->n_neighbors + 1));
            break;
        }
    }

    ovsrec_ospf_neighbor_set_nbr_options(ospf_nbr_row, 72);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

}


DEFUN (cli_ip_ospf_neighbor_add,
          cli_ip_ospf_neighbor_add_cmd,
          "debug ip ospf tables add",
          DEBUG_STR
          IP_STR
          OSPF_STR
          NEIGHBOR_STR
          "Add entries\n")
{
    ospf_router_interface_area_add(1, 500);
    ospf_neighbor_add(true, false, false);
    ospf_neighbor_add(false, true, false);
    ospf_neighbor_add(false, false, true);
}

DEFUN (cli_ip_ospf_tmr_test,
          cli_ip_ospf_tmr_test_cmd,
          "debug ospf nbr_tmr <1-4294967295>",
          DEBUG_STR
          OSPF_STR
          "Nbr timer"
          "Enter value\n")
{
    char timebuf[OSPF_TIME_SIZE];
    char val[OSPF_TIME_SIZE];
    struct timeval tv_adj;
    int64_t ms = 0;

    ospf_clock(&tv_adj);
    ms = ((tv_adj.tv_sec + (atoi(argv[0])/1000)) * 1000)    \
        + ((tv_adj.tv_usec + (atoi(argv[0])% 1000) ) / 1000);
    snprintf(val, sizeof(val), "%ld",ms);

    printf("tmr adjust=%s\n",
            ospf_timer_adjust(val, timebuf, sizeof(timebuf)));

    printf("nbr tmr =%s\n",
            ospf_neighbor_time_print(val, timebuf, sizeof(timebuf)));

}

/******************************************************************************/
/*                      DEBUG CODE - END                                                                             */
/******************************************************************************/
#endif

void
ospf_vty_init(void)
{
    /* "ospf enable" commands. */
    install_element(CONFIG_NODE, &cli_ospf_global_enable_cmd);
    install_element(CONFIG_NODE, &cli_no_ospf_global_enable_cmd);

    /* "router ospf" commands. */
    install_element(CONFIG_NODE, &cli_ospf_router_cmd);
    install_element(CONFIG_NODE, &cli_no_ospf_router_cmd);

    /* "ospf router-id" commands. */
    install_element(OSPF_NODE, &cli_ospf_router_id_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_router_id_cmd);

    /* enable router instance commands */
    install_element(OSPF_NODE, &cli_ospf_enable_instance_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_enable_instance_cmd);

    /* network area id */
    install_element(OSPF_NODE, &cli_ospf_router_network_area_id_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_network_area_ip_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_no_network_area_id_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_no_network_area_ip_cmd);

    /* Show commands */
    install_element(ENABLE_NODE, &cli_ip_ospf_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_br_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_interface_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_detail_show_cmd);

    /* show running-config router ospf */
    install_element(ENABLE_NODE, &cli_ip_ospf_running_config_show_cmd);

#ifndef OSPF_DEBUG
    /* Debug Command */
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_add_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_tmr_test_cmd);
#endif
}
