
/* OSPF CLI implementation with OPS vtysh.
 *
 * Copyright (C) 2000 Kunihiro Ishiguro
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
#include "openswitch-dflt.h"
#include "ospf_vty.h"

/* Making formatted timer strings. */
#define MINUTE_IN_SECONDS   60
#define HOUR_IN_SECONDS     (60*MINUTE_IN_SECONDS)
#define DAY_IN_SECONDS      (24*HOUR_IN_SECONDS)
#define WEEK_IN_SECONDS     (7*DAY_IN_SECONDS)


extern struct ovsdb_idl *idl;
extern unsigned char ospf_vlink_count[];

VLOG_DEFINE_THIS_MODULE(ospf_vty);


/* Function to verify if the input string is in IP format. */
static bool
ospf_string_is_an_ip_addr(const char *string)
{
    union sockunion su;
    return (str2sockunion(string, &su) >= 0);
}

/* Utility functions. */
static int
ospf_str_to_area_id (const char *str, struct in_addr *area_id)
{
  char *endptr = NULL;
  unsigned long ret;

  /* match "A.B.C.D". */
  if (strchr (str, '.') != NULL)
    {
      ret = inet_aton (str, area_id);
      if (!ret)
        return -1;
    }
  /* match "<0-4294967295>". */
  else
    {
      if (*str == '-')
        return -1;
      errno = 0;
      ret = strtoul (str, &endptr, 10);
      if (*endptr != '\0' || errno || ret > UINT32_MAX)
        return -1;

      area_id->s_addr = htonl (ret);
    }

  return 0;
}


/* Function to get the statistics from neighbor table. */
int64_t
ospf_get_statistics_from_neighbor(const struct ovsrec_ospf_neighbor *
                             ovs_ospf_neighbor, const char *key)
{
    int i = 0;

    VLOG_DBG("ospf nbr statistics key=%s%s", key, VTY_NEWLINE);
    for (i = 0; i <ovs_ospf_neighbor->n_statistics; i++) {
        if (!strcmp(ovs_ospf_neighbor->key_statistics[i], key))
            return ovs_ospf_neighbor->value_statistics[i];
    }
    return 0;
}

/* Function to adjust the sec and micro sec */
struct timeval
ospf_tv_adjust (struct timeval a)
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

/* Function to subtract two time val and return the adjusted value. */
struct timeval
ospf_tv_sub (struct timeval a, struct timeval b)
{
  struct timeval ret;

  ret.tv_sec = a.tv_sec - b.tv_sec;
  ret.tv_usec = a.tv_usec - b.tv_usec;

  return ospf_tv_adjust (ret);
}

/* Function to get the quagga monotonic time. */
void ospf_clock (struct timeval *tv)
{
    quagga_gettime(QUAGGA_CLK_MONOTONIC, tv);
}


/* Function to convert the given msec into months,week,days,hours,month and sec format. */
const char *
ospf_timeval_convert (struct timeval *t, char *buf, size_t size)
{
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

/* Function to print the neighbor up time. */
const char *
ospf_neighbor_up_time_print (const char *val, char *buf, int size)
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

    tv_due = ospf_tv_sub(tv_adj, tv_val);

    return ospf_timeval_convert(&tv_due, buf, size);

}

/* Function to adjust the time when the msec passed as input is more than the current time. */
const char *
ospf_timer_adjust_post (const char *val, char *buf, int size)
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

    tv_due = ospf_tv_sub(tv_val, tv_adj);

    memset (buf, 0, size);

    return ospf_timeval_convert(&tv_due, buf, size);
}


/* Function to adjust the time when the msec passed as input is less than the current time. */
const char *
ospf_timer_adjust_pre (const char *val, char *buf, int size)
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

    if ((tv_val.tv_sec > tv_adj.tv_sec) ||
        ((tv_adj.tv_sec <= 0) && (tv_val.tv_usec > tv_adj.tv_usec)))

    {
        return "now";
    }

    tv_due = ospf_tv_sub(tv_adj, tv_val);

    memset (buf, 0, size);

    return ospf_timeval_convert(&tv_due, buf, size);
}

/* Function to print the options in neighbor table. */
char *
ospf_nbr_options_print (const struct ovsrec_ospf_neighbor* nbr_row)
{
    static char buf[OSPF_OPTION_STR_MAXLEN];
    int i = 0;
    int options = 0;

    for (i =0; i < nbr_row->n_nbr_options; i++)
    {
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_O) == 0)
            options = options | OSPF_OPTION_O;
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_DC) == 0)
            options = options | OSPF_OPTION_DC;
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_EA) == 0)
            options = options | OSPF_OPTION_EA;
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_NP) == 0)
            options = options | OSPF_OPTION_NP;
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_MC) == 0)
            options = options | OSPF_OPTION_MC;
        if(strcmp(nbr_row->nbr_options[i], OSPF_NBR_OPTION_STRING_E) == 0)
            options = options | OSPF_OPTION_E;
    }

  snprintf (buf, OSPF_OPTION_STR_MAXLEN, "%d  *|%s|%s|%s|%s|%s|%s|*",
            options,
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
ospf_get_vrf_by_name(const char *name)
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
int
ospf_area_remove_from_router(
                             const struct ovsrec_ospf_router *ospf_router_row,
                             int64_t area_id)
{
    int64_t *area;
    struct ovsrec_ospf_area **area_list;
    int i = 0, j;
    bool is_present = false;

    /* Remove OSPF_area table reference in OSPF_Router table. */
    area = xmalloc(sizeof(int64_t) * (ospf_router_row->n_areas));
    area_list = xmalloc(sizeof * ospf_router_row->key_areas *
                        (ospf_router_row->n_areas));

    if (!area || !area_list)
    {
        SAFE_FREE(area);
        SAFE_FREE(area_list);
        VLOG_DBG("Memory alloc failed, could not remove area from router.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0, j = 0; i < ospf_router_row->n_areas; i++) {
        if(ospf_router_row->key_areas[i] !=  area_id) {
            area[j] = ospf_router_row->key_areas[i];
            area_list[j] = ospf_router_row->value_areas[i];
            j++;
        }
        else
        {
            is_present = true;
        }
    }

    if (is_present == true)
        ovsrec_ospf_router_set_areas(ospf_router_row, area, area_list,
                                   (ospf_router_row->n_areas - 1));

    SAFE_FREE(area);
    SAFE_FREE(area_list);

    return CMD_SUCCESS;
}

/*
 * Find the ospf router with matching instance id
 */
static const struct ovsrec_ospf_router *
ospf_router_lookup_by_instance_id(const struct ovsrec_vrf *vrf_row,
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

    if (ospf_nbr_row->nbr_router_id != 0)
        return false;

    if (ospf_nbr_row->n_nbr_if_addr > 0)
        return false;

    if (ospf_nbr_row->nbma_nbr)
        return false;

    return true;
}

int
ospf_neighbor_remove_from_interface(const struct
                            ovsrec_ospf_interface *interface_row,
                            const struct ovsrec_ospf_neighbor *ospf_nbr_row)
{
    struct ovsrec_ospf_neighbor **ospf_nbr_list;
    int i = 0, j = 0;
    bool is_present = false;

    /* Insert OSPF_Router table reference in VRF table. */
    ospf_nbr_list = xmalloc(sizeof * interface_row->neighbors *
                            (interface_row->n_neighbors));

    if (!ospf_nbr_list)
    {
        VLOG_DBG("Memory alloc failed, could not remove nbr from interface.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < interface_row->n_neighbors; i++)
    {
        if (interface_row->neighbors[i] != ospf_nbr_row)
        {
            ospf_nbr_list[j] = interface_row->neighbors[i];
            j++;
        }
        else
        {
            is_present = true;
        }
    }

    if (is_present == true)
        ovsrec_ospf_interface_set_neighbors(interface_row, ospf_nbr_list,
                                            (interface_row->n_neighbors - 1));

    SAFE_FREE(ospf_nbr_list);

    return CMD_SUCCESS;
}


/* Add the router row to the VRF table. */
int
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

    if (!instance_list || !ospf_routers_list)
    {
        SAFE_FREE(instance_list);
        SAFE_FREE(ospf_routers_list);
        VLOG_DBG("Memory alloc failed, could not insert router in VRF.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

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
    SAFE_FREE(instance_list);
    SAFE_FREE(ospf_routers_list);

    return CMD_SUCCESS;
}

/* Set the OSPF_Router table values to default values. */
void
ospf_router_tbl_default(const struct ovsrec_ospf_router *ospf_router_row)
{
    struct smap smap;
    const bool passive_intf_default = false;
    char *keys[OSPF_NUM_SPF_KEYS];
    int64_t values[OSPF_NUM_SPF_KEYS];
    char *lsa_keys[OSPF_NUM_LSA_TIMER_KEYS];
    int64_t lsa_values[OSPF_NUM_LSA_TIMER_KEYS];
    char *distance_keys[OSPF_ROUTER_DISTANCE_MAX];
    int64_t distance_values[OSPF_ROUTER_DISTANCE_MAX];


    if (ospf_router_row == NULL)
    {
        return;
    }

    /* Router Id */
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_ROUTER_ID_STATIC, OSPF_ROUTER_ID_STATIC_DEFAULT);
    ovsrec_ospf_router_set_router_id(ospf_router_row, &smap);
    smap_destroy(&smap);

    /* Distance */
    distance_keys[OSPF_ROUTER_DISTANCE_ALL]          = OSPF_KEY_DISTANCE_ALL;
    distance_values[OSPF_ROUTER_DISTANCE_ALL]        =
                                        OSPF_ROUTER_DISTANCE_DEFAULT;
    distance_keys[OSPF_ROUTER_DISTANCE_EXTERNAL]     =
                                        OSPF_KEY_DISTANCE_EXTERNAL;
    distance_values[OSPF_ROUTER_DISTANCE_EXTERNAL]   =
                                        OSPF_ROUTER_DISTANCE_DEFAULT;
    distance_keys[OSPF_ROUTER_DISTANCE_INTER_AREA]   =
                                        OSPF_KEY_DISTANCE_INTER_AREA;
    distance_values[OSPF_ROUTER_DISTANCE_INTER_AREA] =
                                        OSPF_ROUTER_DISTANCE_DEFAULT;
    distance_keys[OSPF_ROUTER_DISTANCE_INTRA_AREA]   =
                                        OSPF_KEY_DISTANCE_INTRA_AREA;
    distance_values[OSPF_ROUTER_DISTANCE_INTRA_AREA] =
                                        OSPF_ROUTER_DISTANCE_DEFAULT;

    ovsrec_ospf_router_set_distance(ospf_router_row, distance_keys,
                                    distance_values, OSPF_ROUTER_DISTANCE_MAX);

    /* Default_information */
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_DEFAULT_INFO_ORIG, OSPF_DEFAULT_INFO_ORIG_DEFAULT);
    smap_add(&smap, OSPF_KEY_ALWAYS, OSPF_ALWAYS_DEFAULT);
    ovsrec_ospf_router_set_default_information(ospf_router_row, &smap);
    smap_destroy(&smap);

    /* Passive-interface-default */
    ovsrec_ospf_router_set_passive_interface_default(ospf_router_row,
                                                    &passive_intf_default, 1);

    /* SPF config */
    keys[OSPF_SPF_DELAY]        = OSPF_KEY_SPF_DELAY;
    values[OSPF_SPF_DELAY]      = OSPF_SPF_DELAY_DEFAULT;
    keys[OSPF_SPF_HOLD_TIME]    = OSPF_KEY_SPF_HOLD_TIME;
    values[OSPF_SPF_HOLD_TIME]  = OSPF_SPF_HOLDTIME_DEFAULT;
    keys[OSPF_SPF_MAX_WAIT]     = OSPF_KEY_SPF_MAX_WAIT;
    values[OSPF_SPF_MAX_WAIT]   = OSPF_SPF_MAX_HOLDTIME_DEFAULT;
    ovsrec_ospf_router_set_spf_calculation(ospf_router_row, keys, values,
                                      OSPF_NUM_SPF_KEYS);

    /* Other config */
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_AUTO_COST_REF_BW,
                OSPF_AUTO_COST_REF_BW_DEFAULT);
    smap_add(&smap, OSPF_KEY_DEFAULT_METRIC, OSPF_DEFAULT_METRIC_DEFAULT);
    smap_add(&smap, OSPF_KEY_LOG_ADJACENCY_CHGS,
                OSPF_LOG_ADJACENCY_CHGS_DEFAULT);
    smap_add(&smap, OSPF_KEY_LOG_ADJACENCY_DETAIL,
                OSPF_LOG_ADJACENCY_DETAIL_DEFAULT);
    smap_add(&smap, OSPF_KEY_RFC1583_COMPATIBLE,
                OSPF_RFC1583_COMPATIBLE_DEFAULT);
    smap_add(&smap, OSPF_KEY_ENABLE_OPAQUE_LSA,
                OSPF_ENABLE_OPAQUE_LSA_DEFAULT);
    ovsrec_ospf_router_set_other_config(ospf_router_row, &smap);
    smap_destroy(&smap);

    /* Stub router configurations */
    smap_init(&smap);
    smap_add(&smap, OSPF_KEY_ROUTER_STUB_ADMIN, "false");
    ovsrec_ospf_router_set_stub_router_adv(ospf_router_row, &smap);
    smap_destroy(&smap);

    /* LSA timers */
    lsa_keys[OSPF_LSA_ARRIVAL_INTERVAL]    = OSPF_KEY_ARRIVAL_INTERVAL;
    lsa_values[OSPF_LSA_ARRIVAL_INTERVAL]  = OSPF_LSA_ARRIVAL_INTERVAL_DEFAULT;
    lsa_keys[OSPF_LSA_GROUP_PACING]    = OSPF_KEY_LSA_GROUP_PACING;
    lsa_values[OSPF_LSA_GROUP_PACING]  = OSPF_LSA_GROUP_PACING_DEFAULT;
    ovsrec_ospf_router_set_lsa_timers(ospf_router_row, lsa_keys,
                                        lsa_values, OSPF_NUM_LSA_TIMER_KEYS);

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

    vrf_row = ospf_get_vrf_by_name(vrf_name);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
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

    return CMD_SUCCESS;
}

/* "no router ospf" command handler. */
int
ospf_router_remove_from_vrf(const struct ovsrec_vrf *vrf_row,
                           int64_t instance_id)
{
    int64_t *instance_list;
    struct ovsrec_ospf_router **ospf_routers_list;
    int i, j;
    bool is_present = false;

    /* Remove OSPF_Router table reference in VRF table. */
    instance_list = xmalloc(sizeof(int64_t) * (vrf_row->n_ospf_routers));
    ospf_routers_list = xmalloc(sizeof * vrf_row->key_ospf_routers *
                                (vrf_row->n_ospf_routers));

    if (!instance_list || !ospf_routers_list)
    {
        SAFE_FREE(instance_list);
        SAFE_FREE(ospf_routers_list);
        VLOG_DBG("Memory alloc failed, could not remove router from VRF.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0, j = 0; i < vrf_row->n_ospf_routers; i++)
    {
        if (vrf_row->key_ospf_routers[i] != instance_id)
        {
            instance_list[j] = vrf_row->key_ospf_routers[i];
            ospf_routers_list[j] = vrf_row->value_ospf_routers[i];
            j++;
        }
        else
        {
            is_present = true;
        }
    }

    if (is_present == true)
        ovsrec_vrf_set_ospf_routers(vrf_row, instance_list, ospf_routers_list,
                                   (vrf_row->n_ospf_routers - 1));

    SAFE_FREE(instance_list);
    SAFE_FREE(ospf_routers_list);

    return CMD_SUCCESS;
}

/* Remove the interface row matching the interface name and remove the reference from
     area table. */
int
ospf_interface_remove_from_area(const struct ovsrec_ospf_area *area_row,
                               const struct ovsrec_ospf_interface *interface_row)
{
    struct ovsrec_ospf_interface **ospf_interface_list;
    int i, j;
    bool is_present = false;

    /* Remove OSPF_Router table reference in VRF table. */
    ospf_interface_list = xmalloc(sizeof * area_row->ospf_interfaces *
                                  (area_row->n_ospf_interfaces));

    if (!ospf_interface_list)
    {
        VLOG_DBG("Memory alloc failed, could not remove interface from area.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0, j = 0; i < area_row->n_ospf_interfaces; i++)
    {
        if (area_row->ospf_interfaces[i] != interface_row)
        {
            ospf_interface_list[j] = area_row->ospf_interfaces[i];
            j++;
        }
        else
        {
            is_present = true;
        }
    }

    if (is_present == true)
        ovsrec_ospf_area_set_ospf_interfaces(area_row, ospf_interface_list,
                                   (area_row->n_ospf_interfaces - 1));
    SAFE_FREE(ospf_interface_list);

    return CMD_SUCCESS;
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

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(vrf_name);
    if (vrf_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
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
            interface_row = area_row->ospf_interfaces[j];
            for (k = 0; k < interface_row->n_neighbors; k++)
            {
                ospf_nbr_row = interface_row->neighbors[k];
                /* Delete the neighbor */
                /* The return value is not checked here. Have to check if router delete will
                         do all these. */
                ospf_neighbor_remove_from_interface(interface_row,
                                                    ospf_nbr_row);
                ovsrec_ospf_neighbor_delete(ospf_nbr_row);
            }

            /* The return value is not checked here. Have to check if router delete will
                     do all these. */
            ospf_interface_remove_from_area(area_row, interface_row);
            ovsrec_ospf_interface_delete(interface_row);
        }

       /* The return value is not checked here. Have to check if router delete will
                do all these. */
       ospf_area_remove_from_router(ospf_router_row,
                                    ospf_router_row->key_areas[i]);

       ovsrec_ospf_area_delete(area_row);
    }

        /* Delete the ospf row for matching instance id. */
        if (ospf_router_remove_from_vrf(vrf_row, instance_id) != CMD_SUCCESS)
        {
            OSPF_ABORT_DB_TXN(ospf_router_txn,
                              "Deleting router instance failed.");
        }
        ovsrec_ospf_router_delete(ospf_router_row);


    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;
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

        vrf_row = ospf_get_vrf_by_name(vrf_name);
        if (vrf_row == NULL) {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
        }

        /* See if it already exists. */
        ospf_router_row =
        ospf_router_lookup_by_instance_id(vrf_row, (int64_t)vty->index);

        /* If does not exist, nothing to modify. */
        if (ospf_router_row == NULL)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn,
                                  "OSPF router is not present.");
        }
        else
        {
            /* Set the router-id with matching instance id. */
            smap_clone(&smap, &ospf_router_row->router_id);
            smap_replace(&smap, OSPF_KEY_ROUTER_ID_VAL, inet_ntoa(id));
            smap_replace(&smap, OSPF_KEY_ROUTER_ID_STATIC, "true");
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

    return CMD_SUCCESS;
}

/* "no router-id <id> command handler." */
static int
ospf_no_router_id_cmd_execute(char *vrf_name)
{
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(vrf_name);
    if (vrf_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                    (int64_t)vty->index);

    /* If does not exist, nothing to modify. */
    if (ospf_router_row == NULL) {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    } else {
        /* Unset the router-id. */
        smap_clone(&smap, &ospf_router_row->router_id);
        smap_remove(&smap, OSPF_KEY_ROUTER_ID_VAL);
        smap_replace(&smap, OSPF_KEY_ROUTER_ID_STATIC, "false");
        ovsrec_ospf_router_set_router_id(ospf_router_row, &smap);
    }

    smap_destroy(&smap);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;
}


/* Get the area row matching the area id from the OSPF_Router table.*/
static struct ovsrec_ospf_area *
ospf_area_lookup_by_area_id(
                            const struct ovsrec_ospf_router *router_row,
                            int64_t area_id)
{
    int i = 0;

    for (i = 0; i < router_row->n_areas; i++)
    {
        if (router_row->key_areas[i] == area_id)
        {
            return router_row->value_areas[i];
        }
    }

    return NULL;
}

/* Set the default values for the area row. */
void
ospf_area_tbl_default_set (const struct ovsrec_ospf_area *area_row)
{
    char** key_area_statistics = NULL;
    int64_t *area_stat_value = NULL;

    if (area_row == NULL)
    {
        return;
    }
    ovsrec_ospf_area_set_area_type(area_row,
                             OVSREC_OSPF_AREA_AREA_TYPE_DEFAULT);
    ovsrec_ospf_area_set_nssa_translator_role(area_row,
                             OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE);

    key_area_statistics =
        xmalloc(OSPF_STAT_NAME_LEN * (OSPF_AREA_STATISTICS_MAX));
    area_stat_value =
        xmalloc(sizeof *area_row->value_statistics *
                              (OSPF_AREA_STATISTICS_MAX));

    if (!key_area_statistics || !area_stat_value)
    {
        SAFE_FREE(key_area_statistics);
        SAFE_FREE(area_stat_value);
        return;
    }
    key_area_statistics[OSPF_AREA_STATISTICS_SPF_CALC] =
                           OSPF_KEY_AREA_STATS_SPF_EXEC;
    key_area_statistics[OSPF_AREA_STATISTICS_ABR_COUNT] =
                           OSPF_KEY_AREA_STATS_ABR_COUNT;
    key_area_statistics[OSPF_AREA_STATISTICS_ASBR_COUNT] =
                           OSPF_KEY_AREA_STATS_ASBR_COUNT;

    area_stat_value[OSPF_AREA_STATISTICS_SPF_CALC] = 0;
    area_stat_value[OSPF_AREA_STATISTICS_ABR_COUNT] = 0;
    area_stat_value[OSPF_AREA_STATISTICS_ASBR_COUNT] = 0;

    ovsrec_ospf_area_set_statistics(area_row,key_area_statistics,
                        area_stat_value,OSPF_AREA_STATISTICS_MAX);

    SAFE_FREE (key_area_statistics);
    SAFE_FREE(area_stat_value);
}

/* Insert the area row into the OSPF_Router table. */
int
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

    if (!area || !area_list)
    {
        SAFE_FREE(area);
        SAFE_FREE(area_list);
        VLOG_DBG("Memory alloc failed, could not insert area in router.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

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

    SAFE_FREE(area);
    SAFE_FREE(area_list);

    ospf_area_tbl_default_set(area_row);

    return CMD_SUCCESS;
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
        interface_name = area_row->ospf_interfaces[i]->name;

        if (strcmp(interface_name, ifname) == 0)
        {
            interface_row = area_row->ospf_interfaces[i];
            break;
        }
    }

    return interface_row;

}


/* Set the reference to the interface row to the area table. */
int
ospf_area_set_interface(const struct ovsrec_ospf_area *area_row,
                         const struct ovsrec_ospf_interface *interface_row)
{
    struct ovsrec_ospf_interface **ospf_interface_list;
    int i = 0;

    /* Insert OSPF_Interface table reference in OSPF_Area table. */
    ospf_interface_list = xmalloc(sizeof * area_row->ospf_interfaces *
                              (area_row->n_ospf_interfaces + 1));

    if (!ospf_interface_list)
    {
        VLOG_DBG("Memory alloc failed, could not add interface to area.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < area_row->n_ospf_interfaces; i++) {
        ospf_interface_list[i] = area_row->ospf_interfaces[i];
    }

    ospf_interface_list[area_row->n_ospf_interfaces] =
                        CONST_CAST(struct ovsrec_ospf_interface *, interface_row);
    ovsrec_ospf_area_set_ospf_interfaces(area_row, ospf_interface_list,
                               (area_row->n_ospf_interfaces + 1));

    SAFE_FREE(ospf_interface_list);
    return CMD_SUCCESS;
}


int
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

    if (!network_list || !area_list)
    {
        SAFE_FREE(network_list);
        SAFE_FREE(area_list);
        VLOG_DBG("Memory alloc failed, could not add network to router.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

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
    SAFE_FREE(network_list);
    SAFE_FREE(area_list);
    return CMD_SUCCESS;
}


int
ospf_network_remove_from_ospf_router(
                        const struct ovsrec_ospf_router *ospf_router_row,
                        const int64_t area_id,
                        const char *network_range)
{
    int64_t *area_list;
    char **network_list;
    int i, j;
    bool is_present = false;

    network_list =
        xmalloc(OSPF_NETWORK_RANGE_LEN * ospf_router_row->n_networks);
    area_list =
        xmalloc(sizeof * ospf_router_row->value_networks *
                (ospf_router_row->n_networks));

    if (!network_list || !area_list)
    {
        SAFE_FREE(network_list);
        SAFE_FREE(area_list);
        VLOG_DBG("Memory alloc failed, could not remove network from router.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0, j = 0; i < ospf_router_row->n_networks; i++)
    {
        if ((!strcmp(ospf_router_row->key_networks[i], network_range)) &&
            (ospf_router_row->value_networks[i] == area_id))
        {
            is_present = true;
        }
        else
        {
            network_list[j] =
                ospf_router_row->key_networks[i];
            area_list[j] =
                ospf_router_row->value_networks[i];
            j++;
        }
    }

    if (is_present == true)
        ovsrec_ospf_router_set_networks(ospf_router_row,
                                        network_list,
                                        area_list,
                                        (ospf_router_row->n_networks - 1));
    SAFE_FREE(network_list);
    SAFE_FREE(area_list);

    return CMD_SUCCESS;
}


static int
ospf_router_area_id_cmd_execute(bool no_flag, int instance_id,
                                const char *network_range, int64_t area_id)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    int i = 0;
    bool is_present = false;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    OVSREC_OSPF_ROUTER_FOR_EACH(ospf_router_row,idl)
    {
        for (i = 0; i < ospf_router_row->n_networks; i++)
        {
            if ((strcmp(ospf_router_row->key_networks[i], network_range) == 0)
                && (ospf_router_row->value_networks[i] != area_id))
            {
                if (no_flag == false)
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
            else if (!(strcmp(ospf_router_row->key_networks[i], network_range))
                     && (ospf_router_row->value_networks[i] == area_id))
            {
                if (no_flag == false)
                {
                    OSPF_ABORT_DB_TXN(ospf_router_txn,
                                      "Configuration already exists.");
                }
                else
                {
                    is_present = true;
                }
            }
        }
    }

    if ((no_flag == true) && (is_present == false))
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Configuration does not exist.");
    }

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                              instance_id);

    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    if (!no_flag)
    {
        if (ospf_network_insert_ospf_router(ospf_router_row,
                                        area_id, network_range) != CMD_SUCCESS)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Adding network failed.");
        }
    }
    else
    {
        if (ospf_network_remove_from_ospf_router(ospf_router_row, area_id,
                                                 network_range)!= CMD_SUCCESS)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Removing network failed.");
        }
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

/*Function to get the statistics from area table. */
int64_t
ospf_get_area_statistics(const struct ovsrec_ospf_area* ospf_area_row,
                             const char *key)
{
    int i = 0;

    if (!(ospf_area_row->key_statistics))
        return 0;

    for (i = 0; i < ospf_area_row->n_statistics; i++)
    {
        if (!strcmp(ospf_area_row->key_statistics[i], key))
            return ospf_area_row->value_statistics[i];
    }

    return 0;
}

/* Display one row of OSPF_Area. */
void
ospf_one_area_show(struct vty *vty,int64_t area_id,
                             const struct ovsrec_ospf_area *ospf_area_row)
{
    char area_str[OSPF_SHOW_STR_LEN];
    const char *val = NULL;
    int64_t number_lsa = 0;
    char timebuf[OSPF_TIME_SIZE];
    int64_t int_val = 0;

    /* Area id */
    OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
    if (area_id == 0)
    {
        vty_out(vty, "  Area ID:  %s (Backbone)%s", area_str, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Area ID:  %s", area_str);

        if (ospf_area_row->area_type)
        {
            if (!strcmp(ospf_area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_NSSA))
            {
                vty_out(vty, " (NSSA)");
            }
            else if (!strcmp(ospf_area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_NSSA_NO_SUMMARY))
            {
                vty_out(vty, " (NSSA, no summary)");
            }
            else if (!strcmp(ospf_area_row->area_type,
                             OVSREC_OSPF_AREA_AREA_TYPE_STUB))
            {
                vty_out(vty, " (Stub)");
            }
            else if (!strcmp(ospf_area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_STUB_NO_SUMMARY))
            {
                vty_out(vty, " (Stub, no summary)");
            }
            vty_out(vty, " %s", VTY_NEWLINE);
        }
    }

    /* Number of interfaces */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_AREA_ACTIVE_INTERFACE);
    if(val)
    {
        vty_out(vty, "    Number of interfaces in this area: Total: %d,"
                " Active:%s %s", (int)ospf_area_row->n_ospf_interfaces,
                val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of interfaces in this area: Total: %d,"
                " Active:0 %s", (int)ospf_area_row->n_ospf_interfaces,
                VTY_NEWLINE);
    }

    /* Stub-router state for this area */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_STUB_ROUTER_STATE_ACTIVE);
    if(val && (strcmp(val, "true") == 0))
    {
        vty_out(vty, "   Originating stub / maximum-distance Router-LSA "
                     "is active%s", VTY_NEWLINE);
    }

    /* Fully adjacent neighbors */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_AREA_FULL_NEIGHBORS);
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
    if(ospf_area_row->ospf_auth_type)
    {
        vty_out(vty, "    Area has %s authentication %s",
        strcmp(ospf_area_row->ospf_auth_type,
               OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_MD5)== 0 ?
               "message digest":
        strcmp(ospf_area_row->ospf_auth_type,
               OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_TEXT) == 0 ?
               "simple password" : "no ",
        VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Area has no authentication %s", VTY_NEWLINE);
    }
    /* Fully adjacent VL neighbors */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_AREA_FULL_VL_NEIGHBORS);
    if(val)
    {
        vty_out(vty, "    Number of full virtual adjacencies going through this area: %d%s",
                atoi(val), VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of full virtual adjacencies going through this area: 0%s",
                 VTY_NEWLINE);
    }
    /* SPF timestamp */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_AREA_SPF_LAST_RUN);
    if(val)
    {
        vty_out(vty, "    SPF algorithm last executed ago: %s%s",
                ospf_timer_adjust_pre(val, timebuf, sizeof(timebuf)), VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    SPF algorithm last executed : 0ms ago%s",
                 VTY_NEWLINE);
    }

    /* SPF alg executed times */
    vty_out(vty, "    SPF algorithm executed %ld times%s",
            ospf_get_area_statistics(ospf_area_row,
                                     OSPF_KEY_AREA_STATS_SPF_EXEC),
            VTY_NEWLINE);

    /* Stub router support */
    val = smap_get(&ospf_area_row->status, OSPF_KEY_ENABLE_STUB_ROUTER_ACTIVE);
    if (val)
    {
        vty_out(vty, "  Stub router advertisement is %s%s",
             (strcmp(val,"false") == 0)?"not configured":"configured",
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
                + ospf_area_row->n_opaque_link_lsas;

    vty_out(vty, "    Number of LSA %ld %s", number_lsa, VTY_NEWLINE);

    /* Router LSA */
    int_val = smap_get_int(&ospf_area_row->status, OSPF_KEY_AREA_ROUTER_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "    Number of router LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_router_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of router LSA %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_router_lsas, VTY_NEWLINE);
    }

    /* Network LSA */
    int_val = smap_get_int(&ospf_area_row->status,
                           OSPF_KEY_AREA_NETWORK_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "    Number of network LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_network_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of network LSA %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_network_lsas, VTY_NEWLINE);
    }

    /* ABR Summary LSA */
    int_val = smap_get_int(&ospf_area_row->status,
                           OSPF_KEY_AREA_ABR_SUMMARY_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty,
                "    Number of ABR summary LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_abr_summary_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of ABR summary LSA %ld. "
                "Checksum Sum 0x00000000 %s",
                ospf_area_row->n_abr_summary_lsas, VTY_NEWLINE);
    }

    /* ASBR Summary LSA */
    int_val = smap_get_int(&ospf_area_row->status,
                           OSPF_KEY_AREA_ASBR_SUMMARY_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty,
                "    Number of ASBR summary LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_asbr_summary_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty,
                "    Number of ASBR summary LSA %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_asbr_summary_lsas, VTY_NEWLINE);
    }

    /* NSSA LSA */
    int_val = smap_get_int(&ospf_area_row->status, OSPF_KEY_AREA_NSSA_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "    Number of NSSA LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_as_nssa_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of NSSA LSA %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_as_nssa_lsas, VTY_NEWLINE);
    }

    /* Opaque link LSA */
    int_val = smap_get_int(&ospf_area_row->status,
                           OSPF_KEY_AREA_OPAQUE_LINK_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "    Number of opaque link LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_opaque_link_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of opaque link %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_opaque_link_lsas, VTY_NEWLINE);
    }

    /* Opaque area LSA */
    int_val = smap_get_int(&ospf_area_row->status,
                           OSPF_KEY_AREA_OPAQUE_AREA_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "    Number of opaque area LSA %ld. Checksum Sum 0x%08lx %s",
                ospf_area_row->n_opaque_area_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "    Number of opaque area %ld. Checksum Sum 0x00000000 %s",
                ospf_area_row->n_opaque_area_lsas, VTY_NEWLINE);
    }

}

/* Display the area table rows. */
void
ospf_area_show(struct vty *vty,
                        const struct ovsrec_ospf_router *ospf_router_row)
{
    int i = 0;

    /* Loop through the areas and print them one at a time. */
    for (i = 0; i < ospf_router_row->n_areas; i++)
    {
        ospf_one_area_show(vty, ospf_router_row->key_areas[i],
                              ospf_router_row->value_areas[i]);

    }
}


/*Function to get the SPF calculation values from OSPF router table. */
int64_t
ospf_get_SPF_calc_from_router(const struct ovsrec_ospf_router*
                             ospf_router, const char *key)
{
    int i = 0;

    for (i = 0; i <ospf_router->n_spf_calculation; i++)
    {
        if (!strcmp(ospf_router->key_spf_calculation[i], key))
            return ospf_router->value_spf_calculation[i];
    }

    return 0;
}

void
ospf_ip_router_show()
{
    const struct ovsrec_ospf_router *ospf_router_row;
    const struct ovsrec_vrf *vrf_row;
    const char *val = NULL;
    int64_t instance_tag = 1;
    int64_t int_val = 0;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);

    /* See if it already exists. */
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_tag);
    if (ospf_router_row == NULL)
    {
        vty_out (vty, " OSPF Routing Process not enabled%s", VTY_NEWLINE);
        return;
    }

    /* Display all the router information */
    /* Router id */
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && (strcmp(val, OSPF_DEFAULT_STR) != 0))
        vty_out(vty, "  OSPF Routing Process, Router ID:  %s%s",
                val, VTY_NEWLINE);
    else
        vty_out(vty, "  OSPF Routing Process, Router ID:  %s%s",
                OSPF_DEFAULT_STR, VTY_NEWLINE);

    vty_out(vty, "  This implementation conforms to RFC2328%s",
            VTY_NEWLINE);

    /* RFC 1583 compatibility */
    val = smap_get(&ospf_router_row->other_config,
                   OSPF_KEY_RFC1583_COMPATIBLE);
    if (val)
    {
        vty_out(vty, "  RFC1583 Compatibility flag is %s%s",
               (strcmp(val, "false") == 0)?"disabled":"enabled",
               VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  RFC1583 Compatibility flag is %s%s",
                            "disabled", VTY_NEWLINE);
    }

    /* Opaque capability */
    val = smap_get(&ospf_router_row->other_config,
                   OSPF_KEY_ENABLE_OPAQUE_LSA);
    if (val)
    {
        vty_out(vty, "  Opaque Capability flag is %s",
                (strcmp(val, "false") == 0)?"disabled":"enabled");
    }
    else
    {
        vty_out(vty, "  Opaque Capability flag is disabled");
    }

    /* Opaque Orig blocked */
    val = smap_get(&ospf_router_row->status,
                   OSPF_KEY_OPAQUE_ORIGIN_BLOCKED);
    if (val)
    {
        vty_out(vty, "  %s%s",
               (strcmp(val, "true") == 0)? " (origination blocked)" : "",
               VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, " %s", VTY_NEWLINE);
    }

    val = smap_get(&ospf_router_row->stub_router_adv,
                   OSPF_KEY_ROUTER_STUB_ADMIN);
    if (val)
    {
        vty_out (vty, "  Stub router advertisement is configured "
                      "administratively%s", VTY_NEWLINE);
    }

    /* Stub Startup time */
    int_val = smap_get_int(&ospf_router_row->stub_router_adv,
                           OSPF_KEY_ROUTER_STUB_ADV_STARTUP, 0);
    if(int_val)
    {
       vty_out (vty, "  Stub router advertisement is configured%s",
                VTY_NEWLINE);

       vty_out(vty, "      Enabled for %lds after start-up%s", int_val,
               VTY_NEWLINE);
     }

    if (ospf_router_row->n_spf_calculation > 0)
    {
        /* SPF scheduling delay */
        vty_out(vty, "  Initial SPF scheduling delay %ld millisec(s)%s",
                ospf_get_SPF_calc_from_router(ospf_router_row,
                                              OSPF_KEY_SPF_DELAY),
                VTY_NEWLINE);


        /* Minimum hold time */
        vty_out(vty, "  Minimum hold time between consecutive "
                "SPFs %ld millisec(s)%s",
                ospf_get_SPF_calc_from_router(ospf_router_row,
                                              OSPF_KEY_SPF_HOLD_TIME),
                VTY_NEWLINE);

        /* Maximum hold time */
        vty_out(vty, "  Maximum hold time between consecutive "
                "SPFs %ld millisec(s)%s",
                ospf_get_SPF_calc_from_router(ospf_router_row,
                                              OSPF_KEY_SPF_MAX_WAIT),
                VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Initial SPF scheduling delay %d millisec(s)%s",
                             0, VTY_NEWLINE);
        vty_out(vty, "  Minimum hold time between consecutive SPFs %d "
                     "millisec(s)%s",
                             0, VTY_NEWLINE);
        vty_out(vty, "  Maximum hold time between consecutive SPFs %d "
                "millisec(s)%s", 0, VTY_NEWLINE);
    }

    /* SPF hold multiplier */
    val = smap_get(&ospf_router_row->status, OSPF_KEY_SPF_HOLD_MULTIPLIER);
    if (val)
    {
        vty_out(vty, "  Hold time multiplier is currently %s%s",
             val,
             VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Hold time multiplier is currently %d%s",
                OSPF_SPF_HOLD_MULTIPLIER_DEFAULT, VTY_NEWLINE);
    }

    /* ABR type */
    val = smap_get(&ospf_router_row->status, OSPF_KEY_ROUTER_STATUS_ABR);
    if (val)
    {
        vty_out(vty, "  This router is an ABR. %s", VTY_NEWLINE);
    }
    else
    {
        val = smap_get(&ospf_router_row->status, OSPF_KEY_ROUTER_STATUS_ASBR);
        if (val)
        {
            vty_out(vty, "  This router is an ASBR"
                         "(injecting external routing information)%s",
                         VTY_NEWLINE);
        }
    }

    /* Number of external LSA */
    int_val = smap_get_int(&ospf_router_row->status,
                           OSPF_KEY_ROUTER_EXT_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "  Number of external LSA %ld. "
                     "Checksum Sum 0x%08lx%s",
                     ospf_router_row->n_as_ext_lsas, int_val, VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Number of external LSA %ld. "
                     "Checksum Sum 0x00000000%s",
                     ospf_router_row->n_as_ext_lsas, VTY_NEWLINE);
    }

    /* Number of opaque AS LSA */
    int_val = smap_get_int(&ospf_router_row->status,
                           OSPF_KEY_ROUTER_OPAQUE_CHKSUM, 0);
    if (int_val)
    {
        vty_out(vty, "  Number of opaque AS LSA %ld. "
                     "Checksum Sum 0x%08lx%s",
                     ospf_router_row->n_opaque_as_lsas, int_val,
                     VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Number of opaque AS LSA %ld. "
                     "Checksum Sum 0x00000000%s",
                     ospf_router_row->n_opaque_as_lsas, VTY_NEWLINE);
    }

    /* Number of areas */
    vty_out(vty, "  Number of areas attached to this router: %ld%s",
                    ospf_router_row->n_areas, VTY_NEWLINE);

    /* Adjacency logging */
    val = smap_get(&ospf_router_row->other_config,
                   OSPF_KEY_LOG_ADJACENCY_CHGS);
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
ospf_ifsm_print(const char *if_state)
{
    if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_DEPEND_UPON) == 0)
        return "Depend Upon";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_DOWN) == 0)
            return "Down";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_LOOPBACK) == 0)
            return "Loopback";
    else if(strcmp(if_state,
                   OVSREC_OSPF_INTERFACE_IFSM_STATE_POINT_TO_POINT) == 0)
            return "Point-to-Point";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_DR_OTHER) == 0)
            return "DR Other";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_BACKUP_DR) == 0)
            return "DR Backup";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_DR) == 0)
            return "DR";
    else if(strcmp(if_state, OVSREC_OSPF_INTERFACE_IFSM_STATE_WAITING) == 0)
            return "Waiting";
    else
        return "NONE";
}

/*Function to add the OSPF intervals in the port table. */
int
ospf_add_port_intervals(const struct ovsrec_port* port_row,
                            char *key, int64_t interval)
{
    int i = 0;
    char **ospf_key_timers = xmalloc(OSPF_TIMER_KEY_MAX_LENGTH *
                                    (port_row->n_ospf_intervals + 1));
    int64_t *intervals_list = xmalloc(sizeof *port_row->value_ospf_intervals *
                                    (port_row->n_ospf_intervals + 1));

    if (!ospf_key_timers || !intervals_list)
    {
        SAFE_FREE(ospf_key_timers);
        SAFE_FREE(intervals_list);
        VLOG_DBG("Memory alloc failed, could not add port intervals.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < port_row->n_ospf_intervals; i++)
    {
        ospf_key_timers[i] = port_row->key_ospf_intervals[i];
        intervals_list[i] = port_row->value_ospf_intervals[i];
    }

    ospf_key_timers[port_row->n_ospf_intervals] = key;
    intervals_list[port_row->n_ospf_intervals] = interval;

    ovsrec_port_set_ospf_intervals(port_row, ospf_key_timers, intervals_list,
                                  (port_row->n_ospf_intervals + 1));

    SAFE_FREE(ospf_key_timers);
    SAFE_FREE(intervals_list);

    return CMD_SUCCESS;
}

/*Function to set the OSPF intervals in the port table. */
int
ospf_set_port_intervals(const struct ovsrec_port* port_row,
                            const char *key, int64_t interval)
{
    int i = 0;

    if (!port_row || !key)
    {
        VLOG_DBG("Set port interval failed, key or row was not present.%s",
                 VTY_NEWLINE);
        return 0;
    }

    /* If key is already present, then only update the value. */
    for (i = 0; i < port_row->n_ospf_intervals; i++)
    {
        if (!strcmp(port_row->key_ospf_intervals[i], key))
        {
            port_row->value_ospf_intervals[i] = interval;
            ovsrec_port_set_ospf_intervals(port_row,
                                           port_row->key_ospf_intervals,
                                           port_row->value_ospf_intervals,
                                           port_row->n_ospf_intervals);
            return CMD_SUCCESS;
        }
    }

    /* If key is not present, then something is wrong. We should have added
      the default values already. Add the key and value and log that the key was not present. */
    VLOG_DBG("OSPF interval %s was added newly.\n%s", key, VTY_NEWLINE);
    return ospf_add_port_intervals(port_row, (char *)key, interval);

}

/*Function to get the intervals from port table. */
int64_t
ospf_get_port_intervals(const struct ovsrec_port* port_row,
                             const char *key)
{
    int i = 0;

    if (!port_row || !key)
        return 0;

    for (i = 0; i < port_row->n_ospf_intervals; i++)
    {
        if (!strcmp(port_row->key_ospf_intervals[i], key))
            return port_row->value_ospf_intervals[i];
    }

    return 0;
}

static int ospf_interval_cmd_execute(const char* ifname,
                                           const char* key,
                                           int64_t interval)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "Interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Interface is not attached to any port.");
    }

    /* For dead interval, check if dead interval is more than hello interval. */
    if (!strcmp(key, OSPF_KEY_DEAD_INTERVAL))
    {
        int hello_interval = 0;
        hello_interval = ospf_get_port_intervals(port_row,
                                                 OSPF_KEY_HELLO_INTERVAL);
        if(hello_interval > interval)
        {
            OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Dead interval cannot be less than hello interval.");
        }
        else if((interval % hello_interval) != 0)
        {
            OSPF_ABORT_DB_TXN(ospf_router_txn,
                      "Dead interval should be multiple(s) of hello interval.");
        }
    }

    if (ospf_set_port_intervals(port_row, key, interval) != CMD_SUCCESS)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Interval to be configured was not present.");
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}


static int ospf_max_metric_startup_cmd_execute(bool no_flag,
                                                         const char* vrf_name,
                                                         int instance_id,
                                                         const char* startup)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(vrf_name);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                              instance_id);

    if (ospf_router_row == NULL)
    {
        smap_destroy(&smap);
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    smap_clone(&smap, &ospf_router_row->stub_router_adv);

    if (!no_flag)
    {
        /* Set the startup interval and clear the admin flag. */
        smap_replace(&smap, OSPF_KEY_ROUTER_STUB_STARTUP, startup);
        smap_remove(&smap, OSPF_KEY_ROUTER_STUB_ADMIN);
    }
    else
    {
        smap_remove(&smap, OSPF_KEY_ROUTER_STUB_STARTUP);
    }

    ovsrec_ospf_router_set_stub_router_adv(ospf_router_row, &smap);

    smap_destroy(&smap);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

static int ospf_max_metric_admin_cmd_execute(bool no_flag,
                                                        const char* vrf_name,
                                                        int instance_id)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(vrf_name);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                              instance_id);

    if (ospf_router_row == NULL)
    {
        smap_destroy(&smap);
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    smap_clone(&smap, &ospf_router_row->stub_router_adv);

    if (!no_flag)
    {
        /* Set the admin flag. and clear up startup interval. */
        smap_replace(&smap, OSPF_KEY_ROUTER_STUB_ADMIN, "true");
        smap_remove(&smap, OSPF_KEY_ROUTER_STUB_STARTUP);
    }
    else
    {
        smap_replace(&smap, OSPF_KEY_ROUTER_STUB_ADMIN, "false");
    }

    ovsrec_ospf_router_set_stub_router_adv(ospf_router_row, &smap);

    smap_destroy(&smap);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

static struct ovsrec_ospf_neighbor*
find_ospf_nbr_by_if_addr (const struct ovsrec_ospf_interface* ovs_oi, struct in_addr src)
{
    struct ovsrec_ospf_neighbor* nbr_row = NULL;
    int i = 0;

    for (i = 0 ; i < ovs_oi->n_neighbors ; i++)
    {
        nbr_row = ovs_oi->neighbors[i];
        if (nbr_row && (*(nbr_row->nbr_if_addr) == (int64_t)(src.s_addr)))
            return nbr_row;
    }
    return NULL;
}

static void
ospf_vl_interface_one_row_print(struct vty *vty,
                        const struct ovsrec_ospf_interface *ospf_interface_row)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_area *ospf_area_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    int i, j, n_adjacent_nbrs = 0;
    int64_t area_id = 0, router_id = 0, intervals = 0;
    int64_t dr_id = 0, bdr_id = 0, dr_if_addr = 0, bdr_if_addr = 0;
    const char *val = NULL;
    bool is_dr_present = false;
    bool is_bdr_present = false;
    bool is_present = false;
    const struct ovsrec_vrf *vrf_row;
    int instance_id = 1;
    char timebuf[OSPF_TIME_SIZE];
    char area_str[OSPF_SHOW_STR_LEN];
    int ret;
    struct in_addr id;
    struct in_addr peer_id;

    memset (&id, 0, sizeof (struct in_addr));
    memset (&peer_id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_id);

    /* Get the port row. */
    port_row = ospf_interface_row->port;
    if(!port_row ||
       !port_row->ip4_address ||
       !ospf_interface_row->ospf_vlink)
    {
        vty_out(vty, "Interface %s", ospf_interface_row->name);
        vty_out(vty, " MTU 1500 bytes, BW 0 Kbit <DOWN>%s",VTY_NEWLINE);
        return;
    }

    OVSREC_OSPF_AREA_FOR_EACH(ospf_area_row, idl)
    {
        for (j = 0; j < ospf_area_row->n_ospf_interfaces; j++)
        {
            if (strcmp(ospf_area_row->ospf_interfaces[j]->name,ospf_interface_row->name) == 0)
            {
                is_present = true;
                break;
            }
        }
        if (is_present == true)
            break;
    }

    /* Get the area id from the router table */
    if ((ospf_area_row != NULL) &&
        (strcmp(ospf_area_row->ospf_interfaces[j]->name, ospf_interface_row->name) == 0) &&
        (ospf_router_row != NULL))
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
    vty_out(vty, "Interface %s", ospf_interface_row->name);
    vty_out(vty, " MTU 1500 bytes, BW 0 Kbit <UP>%s",VTY_NEWLINE);

    /* IP */
    if (port_row->ip4_address) {
        peer_id.s_addr = ospf_interface_row->ospf_vlink->peer_router_id;
        vty_out(vty, "  Internet address %s, Peer %s,", port_row->ip4_address,
                                inet_ntoa (peer_id));
    }

    if(ospf_area_row != NULL)
    {
        OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
        vty_out(vty, " Area %s", area_str);

        if ((strcmp(ospf_area_row->area_type, OSPF_AREA_TYPE_NSSA) == 0) ||
            (strcmp(ospf_area_row->area_type,
                    OSPF_AREA_TYPE_NSSA_NO_SUMMARY) == 0))
        {
            vty_out(vty, " [NSSA]");
        }
        else if ((strcmp(ospf_area_row->area_type, OSPF_AREA_TYPE_STUB) == 0) ||
            (strcmp(ospf_area_row->area_type,
                    OSPF_AREA_TYPE_STUB_NO_SUMMARY) == 0))

        {
            vty_out(vty, " [Stub]");
        }
    }
    vty_out(vty, "%s", VTY_NEWLINE);

    /* MTU ignore */
    if (port_row->n_ospf_mtu_ignore)
    {
        bool bool_value = *port_row->ospf_mtu_ignore;
        if (bool_value)
        {
            vty_out(vty, "  MTU mismatch detection: enabled%s", VTY_NEWLINE);
        }
        else
            vty_out(vty, "  MTU mismatch detection: not enabled%s", VTY_NEWLINE);
    }
    else
        vty_out(vty, "  MTU mismatch detection: not enabled%s", VTY_NEWLINE);

    /* Router ID */
    if (ospf_router_row != NULL)
    {
        val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
        if(val)
        {
            vty_out(vty, "  Router ID : %s,", val);


            ret = inet_aton (val, &id);
            if (!ret || (id.s_addr == 0))
            {
                VLOG_DBG("Invalid Router id - %s%s", val, VTY_NEWLINE);
            }
            else
            {
                router_id = id.s_addr;
            }
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

    vty_out(vty, " Network Type VIRTUALLINK,");

    /* cost */
    if (port_row->n_ospf_if_out_cost > 0)
        vty_out(vty, " Cost: %ld %s", port_row->ospf_if_out_cost[0], VTY_NEWLINE);
    else
        vty_out(vty, " Cost: %d %s", OSPF_DEFAULT_COST, VTY_NEWLINE);

    /* Transmit delay */
    intervals = smap_get_int(&(ospf_interface_row->ospf_vlink->other_config),
                    OSPF_KEY_TRANSMIT_DELAY,OSPF_TRANSMIT_DELAY_DEFAULT);
    vty_out(vty, "  Transmit Delay is %ld sec,", intervals);

    /* State */
    vty_out(vty, " State <%s>,",ospf_ifsm_print(ospf_interface_row->ifsm_state));

    /* Priority */
    if(port_row->n_ospf_priority)
        vty_out(vty, " Priority %ld %s", *port_row->ospf_priority, VTY_NEWLINE);

    /* Parse through the neighbor table and get information */
    is_dr_present = false;
    is_bdr_present = false;
    n_adjacent_nbrs = 0;

    for (j = 0; j < ospf_interface_row->n_neighbors; j++)
    {
        ospf_nbr_row = ospf_interface_row->neighbors[j];

        /* DR */
        if ((ospf_nbr_row->n_nbr_router_id > 0) &&
            (*ospf_nbr_row->nbr_router_id == router_id))
        {
            struct in_addr if_addr;
            const struct ovsrec_ospf_neighbor* dr_nbr = NULL;
            if((ospf_nbr_row->dr) && (*ospf_nbr_row->dr))
            {
                dr_id = *ospf_nbr_row->dr;
                if (dr_id)
                {
                    if_addr.s_addr = dr_id;
                    dr_nbr = find_ospf_nbr_by_if_addr(ospf_interface_row,if_addr);
                    if (dr_nbr)
                        dr_if_addr = *dr_nbr->nbr_if_addr;
                    is_dr_present = true;
                }
            }

            /* BDR */
            if((ospf_nbr_row->bdr) && (*ospf_nbr_row->bdr))
            {
                bdr_id = *ospf_nbr_row->bdr;
                if (bdr_id)
                {
                    if_addr.s_addr = bdr_id;
                    dr_nbr = find_ospf_nbr_by_if_addr(ospf_interface_row,if_addr);
                    if (dr_nbr)
                        dr_if_addr = *dr_nbr->nbr_if_addr;
                    is_bdr_present = true;
                }
            }

            break;
        }
    }

    for (j = 0; j < ospf_interface_row->n_neighbors; j++)
    {
      ospf_nbr_row = ospf_interface_row->neighbors[j];
      /* Count number of adjacent neighbor */
      if(strcmp(ospf_nbr_row->nfsm_state, OSPF_NEIGHBOR_FSM_FULL) == 0)
      {
          n_adjacent_nbrs++;
      }
    }

    char show_str[OSPF_SHOW_STR_LEN];

    if(is_dr_present)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(dr_id));
        vty_out(vty, "  Designated Router (ID) %s,", show_str);
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(dr_if_addr));
        vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "  No designated router on this network%s",
                 VTY_NEWLINE);
    }

    if(is_bdr_present)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(bdr_id));
        vty_out(vty, "  Backup Designated Router (ID) %s,", show_str);
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(bdr_if_addr));
        vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "  No backup designated router on this network%s",
                 VTY_NEWLINE);
    }
    vty_out(vty, "  Multicast group memberships: <None>%s", VTY_NEWLINE);

    /* Timer intervals */
    /* Hello */
    intervals  = smap_get_int(&(ospf_interface_row->ospf_vlink->other_config),
                    OSPF_KEY_HELLO_INTERVAL,OSPF_HELLO_INTERVAL_DEFAULT);
    vty_out(vty, "  Timer intervals configured, Hello %ld", intervals);

    /* Dead */
    intervals = smap_get_int(&(ospf_interface_row->ospf_vlink->other_config),
                    OSPF_KEY_DEAD_INTERVAL, OSPF_DEAD_INTERVAL_DEFAULT);
    vty_out(vty, " Dead %ld", intervals);

    /* Wait */
    vty_out(vty, " wait %ld", intervals);

    /* Retransmit */
    intervals = smap_get_int(&(ospf_interface_row->ospf_vlink->other_config),
                    OSPF_KEY_RETRANSMIT_INTERVAL, OSPF_RETRANSMIT_INTERVAL_DEFAULT);
    vty_out(vty, " Retransmit %ld%s", intervals,VTY_NEWLINE);

    /* Hello due in */
    val =smap_get(&ospf_interface_row->status, OSPF_KEY_INTERFACE_ACTIVE);
    if (val)
    {
        if (atoi(val) == true)
        {
            val = smap_get(&ospf_interface_row->status, OSPF_KEY_HELLO_DUE);
            if (val)
            {
                vty_out(vty, "    Hello due in  %s %s",
                        ospf_timer_adjust_post(val, timebuf, sizeof(timebuf)),
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

    /* Neighbor count and Adjacent neighbor count */
    if (ospf_interface_row->n_neighbors)
        vty_out(vty, "  Neighbor Count is %ld, Adjacent neighbor count is %d%s",
            (ospf_interface_row->n_neighbors - 1), n_adjacent_nbrs, VTY_NEWLINE);
    else
        vty_out(vty, "  Neighbor Count is 0, Adjacent neighbor count is %d%s",
                n_adjacent_nbrs, VTY_NEWLINE);

}

void
ospf_interface_one_row_print(struct vty *vty,const char* ifname,
                        const struct ovsrec_ospf_interface *ospf_interface_row)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_interface *interface_row = NULL;
    const struct ovsrec_ospf_area *ospf_area_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    int i, j, n_adjacent_nbrs = 0;
    int64_t area_id = 0, router_id = 0, intervals = 0;
    int64_t dr_id = 0, bdr_id = 0, dr_if_addr = 0, bdr_if_addr = 0;
    const char *val = NULL;
    bool is_dr_present = false;
    bool is_bdr_present = false;
    bool is_present = false;
    const struct ovsrec_vrf *vrf_row;
    int instance_id = 1;
    char timebuf[OSPF_TIME_SIZE];
    char area_str[OSPF_SHOW_STR_LEN];
    int ret;
    struct in_addr id;

    memset (&id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_id);

    /* Get the port row. */
    port_row = ospf_interface_row->port;

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
            if (strcmp(ospf_area_row->ospf_interfaces[j]->name, ifname) == 0)
            {
                is_present = true;
                break;
            }
        }
        if (is_present == true)
            break;
    }

    /* Get the area id from the router table */
    if ((ospf_area_row != NULL) &&
        (strcmp(ospf_area_row->ospf_interfaces[j]->name, ifname) == 0) &&
        (ospf_router_row != NULL))
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
    vty_out(vty, "Interface %s", ifname);

    /* MTU, Admin and oper states */
    if(interface_row->n_mtu > 0)
    {
        vty_out(vty, " MTU %ld bytes,", interface_row->mtu[0]);
    }
    if(interface_row->n_link_speed > 0)
    {
        vty_out(vty, " BW %ld Mbps", (interface_row->link_speed[0])/1000000);
    }
    if(interface_row->admin_state)
    {
        vty_out(vty, "  <%s,%s,", interface_row->admin_state, "BROADCAST");

        if(interface_row->link_state)
        {
            vty_out(vty, "%s", interface_row->link_state);
        }
        vty_out(vty, " >");
    }
    vty_out(vty, "%s", VTY_NEWLINE);

    /* IP */
    if (port_row->ip4_address) {
        vty_out(vty, "  Internet address %s", port_row->ip4_address);
    }

    if(ospf_area_row != NULL)
    {
        OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
        vty_out(vty, " Area %s", area_str);

        if ((strcmp(ospf_area_row->area_type, OSPF_AREA_TYPE_NSSA) == 0) ||
            (strcmp(ospf_area_row->area_type,
                    OSPF_AREA_TYPE_NSSA_NO_SUMMARY) == 0))
        {
            vty_out(vty, " [NSSA]");
        }
        else if ((strcmp(ospf_area_row->area_type, OSPF_AREA_TYPE_STUB) == 0) ||
            (strcmp(ospf_area_row->area_type,
                    OSPF_AREA_TYPE_STUB_NO_SUMMARY) == 0))

        {
            vty_out(vty, " [Stub]");
        }
    }
    vty_out(vty, "%s", VTY_NEWLINE);

    /* MTU ignore */
    if (port_row->n_ospf_mtu_ignore)
    {
        bool bool_value = *port_row->ospf_mtu_ignore;
        if (bool_value)
        {
            vty_out(vty, "  MTU mismatch detection: enabled%s", VTY_NEWLINE);
        }
        else
            vty_out(vty, "  MTU mismatch detection: not enabled%s", VTY_NEWLINE);
    }
    else
        vty_out(vty, "  MTU mismatch detection: not enabled%s", VTY_NEWLINE);

    /* Router ID */
    if (ospf_router_row != NULL)
    {
        val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
        if(val)
        {
            vty_out(vty, "  Router ID : %s,", val);


            ret = inet_aton (val, &id);
            if (!ret || (id.s_addr == 0))
            {
                VLOG_DBG("Invalid Router id - %s%s", val, VTY_NEWLINE);
            }
            else
            {
                router_id = id.s_addr;
            }
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
    if (port_row->ospf_if_type)
    {
        vty_out(vty, " Network Type %s,",
                ospf_interface_type_convert(port_row->ospf_if_type));
    }
    else
    {
        vty_out(vty, " Network Type BROADCAST,");
    }

    /* cost */
    if (port_row->n_ospf_if_out_cost > 0)
        vty_out(vty, " Cost: %ld %s", port_row->ospf_if_out_cost[0], VTY_NEWLINE);
    else
        vty_out(vty, " Cost: %d %s", 0, VTY_NEWLINE);

    /* Transmit delay */
    intervals = ospf_get_port_intervals(port_row, OSPF_KEY_TRANSMIT_DELAY);
    vty_out(vty, "  Transmit Delay is %ld sec,",
            (intervals > 0) ? intervals : OSPF_TRANSMIT_DELAY_DEFAULT);

    /* State */
    vty_out(vty, " State <%s>,",ospf_ifsm_print(ospf_interface_row->ifsm_state));

    /* Priority */
    if(port_row->n_ospf_priority)
        vty_out(vty, " Priority %ld %s", *port_row->ospf_priority, VTY_NEWLINE);

    /* Parse through the neighbor table and get information */
    is_dr_present = false;
    is_bdr_present = false;
    n_adjacent_nbrs = 0;

    for (j = 0; j < ospf_interface_row->n_neighbors; j++)
    {
        ospf_nbr_row = ospf_interface_row->neighbors[j];

        /* DR */
        if ((ospf_nbr_row->n_nbr_router_id > 0) &&
            (*ospf_nbr_row->nbr_router_id == router_id))
        {
            if((ospf_nbr_row->dr) && (*ospf_nbr_row->dr))
            {
                dr_id = *ospf_nbr_row->dr;
            }

            /* BDR */
            if((ospf_nbr_row->bdr) && (*ospf_nbr_row->bdr) &&
               (ospf_nbr_row->nbr_if_addr))
            {
                bdr_id = *ospf_nbr_row->bdr;
            }

            /* Should not count self neighbor entry. */
            continue;
        }

        /* Count number of adjacent neighbor */
        if(strcmp(ospf_nbr_row->nfsm_state, OSPF_NEIGHBOR_FSM_FULL) == 0)
        {
            n_adjacent_nbrs++;
        }
    }

    for (j = 0; j < ospf_interface_row->n_neighbors; j++)
    {
        ospf_nbr_row = ospf_interface_row->neighbors[j];

        if ((ospf_nbr_row->nbr_if_addr) &&
            (*ospf_nbr_row->nbr_if_addr == dr_id))
        {
            is_dr_present = true;
            dr_if_addr = *ospf_nbr_row->nbr_if_addr;
            dr_id = *ospf_nbr_row->nbr_router_id;
        }

        if ((ospf_nbr_row->nbr_if_addr) &&
            (*ospf_nbr_row->nbr_if_addr == bdr_id))
        {
            is_bdr_present = true;
            bdr_if_addr = *ospf_nbr_row->nbr_if_addr;
            bdr_id = *ospf_nbr_row->nbr_router_id;
        }

    }

    if(is_dr_present || is_bdr_present)
    {
        char show_str[OSPF_SHOW_STR_LEN];

        if(is_dr_present)
        {
            memset(show_str,'\0', OSPF_SHOW_STR_LEN);
            OSPF_IP_STRING_CONVERT(show_str, ntohl(dr_id));
            vty_out(vty, "  Designated Router (ID) %s,", show_str);

            memset(show_str,'\0', OSPF_SHOW_STR_LEN);
            OSPF_IP_STRING_CONVERT(show_str, ntohl(dr_if_addr));
            vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
        }
        else
        {
            vty_out (vty, "  No designated router on this network%s",
                     VTY_NEWLINE);
        }

        if(is_bdr_present)
        {
            memset(show_str,'\0', OSPF_SHOW_STR_LEN);
            OSPF_IP_STRING_CONVERT(show_str, ntohl(bdr_id));
            vty_out(vty, "  Backup Designated Router (ID) %s,", show_str);

            memset(show_str,'\0', OSPF_SHOW_STR_LEN);
            OSPF_IP_STRING_CONVERT(show_str, ntohl(bdr_if_addr));
            vty_out(vty, "  Interface Address %s%s", show_str, VTY_NEWLINE);
        }
        else
        {
            vty_out (vty, "  No backup designated router on this network%s",
                     VTY_NEWLINE);
        }

        vty_out(vty, "  Multicast group memberships: OSPFAllRouters "   \
            "OSPFDesignatedRouters %s", VTY_NEWLINE);
    }
    else
    {
        vty_out(vty, "  Multicast group memberships: OSPFAllRouters %s",
                    VTY_NEWLINE);
    }

    /* Timer intervals */
    /* Hello */
    intervals = ospf_get_port_intervals(port_row, OSPF_KEY_HELLO_INTERVAL);
    vty_out(vty, "  Timer intervals configured, Hello %ld",
            (intervals > 0) ? intervals : OSPF_HELLO_INTERVAL_DEFAULT);

    /* Dead */
    intervals = ospf_get_port_intervals(port_row, OSPF_KEY_DEAD_INTERVAL);
    vty_out(vty, " Dead %ld",
            (intervals > 0) ? intervals : OSPF_ROUTER_DEAD_INTERVAL_DEFAULT);

    /* Wait */
    intervals = ospf_get_port_intervals(port_row, OSPF_KEY_DEAD_INTERVAL);
    vty_out(vty, " wait %ld",
            (intervals > 0) ? intervals : OSPF_ROUTER_DEAD_INTERVAL_DEFAULT);


    /* Retransmit */
    intervals = ospf_get_port_intervals(port_row, OSPF_KEY_RETRANSMIT_INTERVAL);
    vty_out(vty, " Retransmit %ld%s",
            (intervals > 0) ? intervals : OSPF_RETRANSMIT_INTERVAL_DEFAULT,
            VTY_NEWLINE);

    /* Hello due in */
    val =smap_get(&ospf_interface_row->status, OSPF_KEY_INTERFACE_ACTIVE);
    if (val)
    {
        if (atoi(val) == true)
        {
            val = smap_get(&ospf_interface_row->status, OSPF_KEY_HELLO_DUE);
            if (val)
            {
                vty_out(vty, "    Hello due in  %s %s",
                        ospf_timer_adjust_post(val, timebuf, sizeof(timebuf)),
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

    /* Neighbor count and Adjacent neighbor count */
    if (ospf_interface_row->n_neighbors)
        vty_out(vty, "  Neighbor Count is %ld, Adjacent neighbor count is %d%s",
            (ospf_interface_row->n_neighbors - 1), n_adjacent_nbrs, VTY_NEWLINE);
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
            if (!ospf_interface_row->ospf_vlink)
                ospf_interface_one_row_print(vty, ospf_interface_row->name,
                                             ospf_interface_row);
            else
                ospf_vl_interface_one_row_print (vty, ospf_interface_row);
        }
    }
}

/* Function to convert the neighbor FSM to printable string. */
char *
ospf_nbr_state_print(const char *state)
{
    if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_ATTEMPT) == 0)
        return OSPF_NFSM_STATE_ATTEMPT;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_DELETED) == 0)
        return OSPF_NFSM_STATE_DELETED;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_DEPEND_UPON) == 0)
        return OSPF_NFSM_STATE_DEPEND_UPON;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_DOWN) == 0)
        return OSPF_NFSM_STATE_DOWN;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_EX_START) == 0)
        return OSPF_NFSM_STATE_EXSTART;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_EXCHANGE) == 0)
        return OSPF_NFSM_STATE_EXCHANGE;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_FULL) == 0)
        return OSPF_NFSM_STATE_FULL;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_INIT) == 0)
        return OSPF_NFSM_STATE_INIT;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_LOADING) == 0)
        return OSPF_NFSM_STATE_LOADING;
    else if(strcmp(state, OVSREC_OSPF_NEIGHBOR_NFSM_STATE_TWO_WAY) == 0)
        return OSPF_NFSM_STATE_2_WAY;
    else
        return "NONE";
}

static const char *
ospf_nbr_dr_state_print (struct in_addr router_id,
                            const struct ovsrec_ospf_interface* ovs_oi,
                                   int64_t nbr_ifaddr)
{
    int i = 0;

    for (i = 0 ; i < ovs_oi->n_neighbors ; i++)
    {
        if (ovs_oi->neighbors[i]->nbr_router_id[0] == router_id.s_addr)
        {
            if (ovs_oi->neighbors[i]->dr &&
                ovs_oi->neighbors[i]->dr[0] == nbr_ifaddr)
                return "DR";
            else if (ovs_oi->neighbors[i]->bdr &&
                    ovs_oi->neighbors[i]->bdr[0] == nbr_ifaddr)
                return "Backup";
        }
    }

    return "DROther";
}

void
ospf_neighbor_one_row_print(
                            const struct ovsrec_ospf_interface* ovs_oi,
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

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_id);

    /* Get router id from the OSPF_Router table. */
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && ( strcmp(val, OSPF_DEFAULT_STR) != 0))
    {
        /* Convert the router id to integer. */
        ret = inet_aton (val, &id);
        if (!ret || (id.s_addr == 0))
        {
            VLOG_DBG("Could not display nbr. Router id - %s%s", val,
                     VTY_NEWLINE);
            return;
        }

        /* Check if the neighbor row is self entry. */
        if ((ospf_nbr_row->nbr_router_id)
            && (id.s_addr == *ospf_nbr_row->nbr_router_id))
            return;
    }

    /* Dont print the "down" neighbors if "all" was not entered. */
    if((strcmp(ospf_nbr_row->nfsm_state,
                OVSREC_OSPF_NEIGHBOR_NFSM_STATE_DOWN) == 0) &&
       (all_flag != true))
        return;

    /* Nbr router id */
    if(ospf_nbr_row->n_nbr_router_id)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->nbr_router_id));
    }
    else
        strncpy(show_str, OSPF_DEFAULT_STR, OSPF_SHOW_STR_LEN);

    memset(state_str,'\0', OSPF_SHOW_STR_LEN);
    snprintf(state_str, OSPF_SHOW_STR_LEN, "%s/%s",
                 ospf_nbr_state_print(ospf_nbr_row->nfsm_state),
                 ospf_nbr_dr_state_print (id,ovs_oi,ospf_nbr_row->nbr_if_addr[0]));


    /* Priority and State. */
    if(ospf_nbr_row->n_nbr_priority)
    {
        vty_out(vty, "%-15s %3ld %-15s ", show_str, *ospf_nbr_row->nbr_priority,
                state_str);
    }
    else
    {
        vty_out(vty, "%-15s %3d %-15s ", show_str, 0, state_str);
    }

    val = smap_get(&ospf_nbr_row->status, OSPF_KEY_NEIGHBOR_DEAD_TIMER_DUE);
    vty_out (vty, "%9s ", ospf_timer_adjust_post(val, timebuf, sizeof(timebuf)));

    if (ospf_nbr_row->nbr_if_addr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->nbr_if_addr));
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
               if (!interface_row->ospf_vlink) {
                    port_row = interface_row->port;
                    snprintf(show_str, OSPF_SHOW_STR_LEN, "%s:%s",
                             interface_row->name,
                            port_row->ip4_address);
                }
                else
                    snprintf(show_str, OSPF_SHOW_STR_LEN, "%s",
                             interface_row->name);
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

    vty_out (vty, "%5ld %5ld %5ld%s",
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
    int instance_id = 1, i= 0, j = 0;
    const char *val = NULL;
    struct in_addr id;
    int ret;
    char show_str[OSPF_SHOW_STR_LEN];
    char timebuf[OSPF_TIME_SIZE];
    bool is_present = false;
    char area_str[OSPF_SHOW_STR_LEN];

    memset (&id, 0, sizeof (struct in_addr));

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_id);

    // Get router id from the OSPF_Router table.
    val = smap_get(&ospf_router_row->router_id, OSPF_KEY_ROUTER_ID_VAL);
    if (val && ( strcmp(val, OSPF_DEFAULT_STR) != 0))
    {
        /* Convert the router id to integer. */
        ret = inet_aton (val, &id);
        if (!ret || (id.s_addr == 0))
        {
            VLOG_DBG("Could not display nbr. Router id - %s%s", val,
                     VTY_NEWLINE);
            return;
        }

        /* Check if the neighbor row is self entry. */
        if ((ospf_nbr_row->nbr_router_id)
            && (id.s_addr == *ospf_nbr_row->nbr_router_id))
            return;
    }

    /* Dont print the "down" neighbors if "all" was not entered. */
    if((strcmp(ospf_nbr_row->nfsm_state,
                OVSREC_OSPF_NEIGHBOR_NFSM_STATE_DOWN) == 0) &&
       (all_flag != true))
        return;

    /* Nbr router id */
    if(ospf_nbr_row->nbr_router_id)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->nbr_router_id));
        vty_out (vty, "Neighbor %s", show_str);
    }
    else
    {
        vty_out (vty, "Neighbor %s", OSPF_DEFAULT_STR);
    }

    if (ospf_nbr_row->nbr_if_addr)
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->nbr_if_addr));
        vty_out (vty, ",  interface address %s", show_str);
    }
    else
    {
        vty_out (vty, ",  interface address %s", OSPF_DEFAULT_STR);
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
            if (area_row->ospf_interfaces[i] == interface_row)
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
            OSPF_IP_STRING_CONVERT(area_str, \
                                   ntohl(ospf_router_row->key_areas[j]));
            vty_out (vty, "    In the area %s ", area_str);
            break;
        }
    }

    if (area_row)
    {
        if ((strcmp(area_row->area_type, OSPF_AREA_TYPE_NSSA) == 0) ||
        (strcmp(area_row->area_type, OSPF_AREA_TYPE_NSSA_NO_SUMMARY) == 0))
        {
            vty_out(vty, " [NSSA]");
        }
        else if ((strcmp(area_row->area_type, OSPF_AREA_TYPE_STUB) == 0) ||
        (strcmp(area_row->area_type, OSPF_AREA_TYPE_STUB_NO_SUMMARY) == 0))

        {
            vty_out(vty, " [Stub]");
        }
    }

    if (interface_row)
    {
        vty_out(vty, " via interface %s%s", interface_row->name,
                VTY_NEWLINE);
    }

    if(ospf_nbr_row->n_nbr_priority > 0)
    {
        vty_out (vty, "    Neighbor priority is %ld, "   \
                 "State is %s, %ld state changes%s",
                 *ospf_nbr_row->nbr_priority,
                 ospf_nbr_state_print(ospf_nbr_row->nfsm_state),
                 ospf_get_statistics_from_neighbor(ospf_nbr_row,
                 OSPF_KEY_NEIGHBOR_STATE_CHG_CNT),
                 VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "    Neighbor priority is 0, "   \
                 "State is %s, %ld state changes%s",
                 ospf_nbr_row->nfsm_state,
                 ospf_get_statistics_from_neighbor(ospf_nbr_row,
                 OSPF_KEY_NEIGHBOR_STATE_CHG_CNT),
                 VTY_NEWLINE);
    }

    /* Neighbor uptime */
    val = smap_get(&ospf_nbr_row->status,
                   OSPF_KEY_NEIGHBOR_LAST_UP_TIMESTAMP);
    vty_out (vty, "    Neighbor is up for %s%s",
                     ospf_neighbor_up_time_print(val, timebuf, sizeof(timebuf)),
                     VTY_NEWLINE);

    if ((ospf_nbr_row->dr) && (*ospf_nbr_row->dr))
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->dr));
        vty_out (vty, "    DR is %s ", show_str);
    }

    if ((ospf_nbr_row->bdr) && (*ospf_nbr_row->bdr))
    {
        memset(show_str,'\0', OSPF_SHOW_STR_LEN);
        OSPF_IP_STRING_CONVERT(show_str, ntohl(*ospf_nbr_row->bdr));
        vty_out (vty, ",BDR is %s ", show_str);
    }
    vty_out (vty, "%s", VTY_NEWLINE);

    if (ospf_nbr_row->n_nbr_options > 0)
    {
        vty_out (vty, "    Options   %s%s",
                ospf_nbr_options_print(ospf_nbr_row), VTY_NEWLINE);
    }
    else
    {
        vty_out (vty, "    Options 0  *|-|-|-|-|-|-|*%s", VTY_NEWLINE);
    }

    val = smap_get(&ospf_nbr_row->status, OSPF_KEY_NEIGHBOR_DEAD_TIMER_DUE);
    vty_out (vty, "    Dead timer due in %s %s ",
             ospf_timer_adjust_post(val, timebuf, sizeof(timebuf)), VTY_NEWLINE);

    vty_out (vty, "   Database Summary List %ld%s",
                ospf_get_statistics_from_neighbor(ospf_nbr_row,
                OSPF_KEY_NEIGHBOR_DB_SUMMARY_CNT), VTY_NEWLINE);

    vty_out (vty, "    Link State Request List %ld %s",
            ospf_get_statistics_from_neighbor(ospf_nbr_row,
                            OSPF_KEY_NEIGHBOR_LS_REQUEST_CNT), VTY_NEWLINE);

    vty_out (vty, "    Link State Retransmission List %ld %s",
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
  vty_out(vty, "-----------------------------------------------"
               "-----------------------------------------------------%s",
                VTY_NEWLINE);

}

void
ospf_ip_router_neighbor_show(const char* ifname,
                                     int64_t nbr_id,
                                     bool all_flag)
{
    const struct ovsrec_ospf_neighbor *ospf_nbr_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    int i =0;

    /* Print the header */
    show_ip_ospf_neighbour_header(vty);

    if (ifname != NULL)
    {
        /* Print neighbor details for the interface name passed. */
        OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            if (strcmp(ospf_interface_row->name, ifname) == 0)
            {
                for(i = 0; i < ospf_interface_row->n_neighbors; i++)
                {
                    ospf_nbr_row = ospf_interface_row->neighbors[i];
                    ospf_neighbor_one_row_print(ospf_interface_row,
                                                ospf_nbr_row, all_flag);
                }
            }
        }
    }
    else if (nbr_id != 0)
    {
        /* Print all the neighbor entries matching the nbr id present */
        OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            for(i = 0; i < ospf_interface_row->n_neighbors; i++)
            {
                ospf_nbr_row = ospf_interface_row->neighbors[i];
                if (nbr_id == *ospf_nbr_row->nbr_router_id)
                    ospf_neighbor_one_row_print(ospf_interface_row,
                                                ospf_nbr_row, all_flag);
            }
        }
    }
    else
    {
        /* Print all the rows */
         OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
        {
            for(i = 0; i < ospf_interface_row->n_neighbors; i++)
            {
                ospf_nbr_row = ospf_interface_row->neighbors[i];
                ospf_neighbor_one_row_print(ospf_interface_row,
                                                ospf_nbr_row, all_flag);
            }
        }
    }
}

int64_t
ospf_get_distance(const struct ovsrec_ospf_router *router_row,
                      const char *key)
{
    int i = 0;

    if (!strcmp(router_row->key_distance[i], key))
        return router_row->value_distance[i];

    return 0;
}

/* Function to display the show running of area related commands */
void ospf_area_show_running(const struct ovsrec_ospf_router *router_row)
{
    const struct ovsrec_ospf_area *area_row = NULL;
    int i = 0;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];
    char disp_str[OSPF_SHOW_STR_LEN];
    bool is_present = false;

    for (i = 0; i < router_row->n_areas; i++)
    {
        OSPF_IP_STRING_CONVERT(area_str, ntohl(router_row->key_areas[i]));
        area_row = router_row->value_areas[i];

        /* area authentication */
        if(area_row->ospf_auth_type)
        {
            if (!strcmp(area_row->ospf_auth_type,
                        OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_MD5 ))
            {
                vty_out(vty, "%4sarea %s authentication message-digest%s", "",
                        area_str, VTY_NEWLINE);
            }
            else if (!strcmp(area_row->ospf_auth_type,
                             OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_TEXT))
            {
                vty_out(vty, "%4sarea %s authentication%s", "",
                        area_str, VTY_NEWLINE);
            }
        }

        /* area type */
        if (area_row->nssa_translator_role)
        {
            if (!strcmp(area_row->nssa_translator_role,
                OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_NEVER))
            {
                strncpy(disp_str, "translate-never", OSPF_SHOW_STR_LEN);
                is_present = true;
            }
            else if (!strcmp(area_row->nssa_translator_role,
                OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_ALWAYS))
            {
                strncpy(disp_str, "translate-always", OSPF_SHOW_STR_LEN);
                is_present = true;
            }
        }

        if (area_row->area_type)
        {
            if (!strcmp(area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_NSSA))
            {
                vty_out(vty, "%4sarea %s nssa", "", area_str);

                if (is_present == true)
                {
                    vty_out(vty, " %s", disp_str);
                }
                vty_out(vty, "%s", VTY_NEWLINE);
            }
            else if (!strcmp(area_row->area_type,
                            OVSREC_OSPF_AREA_AREA_TYPE_NSSA_NO_SUMMARY))
            {
                vty_out(vty, "%4sarea %s nssa", "", area_str);

                if (is_present == true)
                {
                    vty_out(vty, " %s", disp_str);
                }

                vty_out(vty, " no-summary%s", VTY_NEWLINE);
            }
            else if (!strcmp(area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_STUB))
            {
                vty_out(vty, "%4sarea %s stub%s", "", area_str, VTY_NEWLINE);
            }
            else if (!strcmp(area_row->area_type,
                            OVSREC_OSPF_AREA_AREA_TYPE_STUB_NO_SUMMARY))
            {
                vty_out(vty, "%4sarea %s stub no-summary%s", "", area_str,
                        VTY_NEWLINE);
            }
        }
    }

}

/* Function to display the configurations made. */
void
ospf_running_config_show()
{
    const struct ovsrec_vrf *ovs_vrf = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    int i = 0, j = 0;
    int64_t distance = 0;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];

    OVSREC_VRF_FOR_EACH(ovs_vrf, idl)
    {
        for (j = 0; j < ovs_vrf->n_ospf_routers; j++)
        {
            /* OPS_TODO: Have to check if router table is not default and then print show
                                          running. */
            vty_out (vty, "%s%s", "router ospf", VTY_NEWLINE);

            ospf_router_row = ovs_vrf->value_ospf_routers[j];

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

            /* max-metric  admin */
            val = smap_get(&ospf_router_row->stub_router_adv,
                            OSPF_KEY_ROUTER_STUB_ADMIN);
            if(val && (strcmp(val, "true") == 0))
            {
                vty_out(vty, "%4s%s%s", "",
                        "max-metric router-lsa", VTY_NEWLINE);
            }

            /* max-metric  startup */
            val = smap_get(&ospf_router_row->stub_router_adv,
                            OSPF_KEY_ROUTER_STUB_STARTUP);
            if(val)
            {
                vty_out(vty, "%4s%s %s%s", "",
                        "max-metric router-lsa on-startup", val, VTY_NEWLINE);
            }

            /* Distance */
            distance = ospf_get_distance(ospf_router_row,
                                         OVSREC_OSPF_ROUTER_DISTANCE_ALL);
            if (distance > 0 && (distance != OSPF_ROUTER_DISTANCE_DEFAULT))
            {
                vty_out(vty, "%4s%s %d%s", "", "distance", distance, VTY_NEWLINE);
            }

            ospf_area_show_running(ospf_router_row);
        }
    }

    return;
}

static void
ospf_route_network_show(const struct ovsrec_ospf_router *router_row)
{
    const struct ovsrec_ospf_route *route_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    int i = 0, j = 0, area_id = 0;
    int64_t cost = OSPF_DEFAULT_COST;
    char area_str[OSPF_SHOW_STR_LEN];

    memset(area_str,'\0', OSPF_SHOW_STR_LEN);

    vty_out (vty, "============ OSPF network routing table ============%s",
         VTY_NEWLINE);

    OVSREC_OSPF_AREA_FOR_EACH(area_row, idl)
    {
        /* Print inter area routes. */
        for (i = 0; i < area_row->n_inter_area_ospf_routes; i++)
        {
            route_row = area_row->inter_area_ospf_routes[i];
            area_id = smap_get_int(&route_row->route_info,
                                   OSPF_KEY_ROUTE_AREA_ID, 0);
            if (area_id != 0)
            {
                OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
            }
            else
            {
                strncpy(area_str, "0.0.0.0", OSPF_SHOW_STR_LEN - 1);
            }

            cost = smap_get_int(&route_row->route_info,
                                OSPF_KEY_ROUTE_COST, OSPF_DEFAULT_COST);
            vty_out (vty, "N IA %-18s    [%lu] area: %s%s", route_row->prefix,
                     cost, area_str, VTY_NEWLINE);
            for(j = 0; j < route_row->n_paths; j++)
                vty_out (vty, "%24s   %s%s", "", route_row->paths[j],
                         VTY_NEWLINE);
        }

        /* Print intra area routes. */
        for (i = 0; i < area_row->n_intra_area_ospf_routes; i++)
        {
            route_row = area_row->intra_area_ospf_routes[i];
            area_id = smap_get_int(&route_row->route_info,
                                   OSPF_KEY_ROUTE_AREA_ID, 0);
            if (area_id != 0)
            {
                OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
            }
            else
            {
                strncpy(area_str, "0.0.0.0", OSPF_SHOW_STR_LEN - 1);
            }

            cost = smap_get_int(&route_row->route_info,
                                OSPF_KEY_ROUTE_COST, OSPF_DEFAULT_COST);
            vty_out (vty, "N    %-18s    [%lu] area: %s%s", route_row->prefix,
                     cost, area_str, VTY_NEWLINE);
            for(j = 0; j < route_row->n_paths; j++)
                vty_out (vty, "%24s   %s%s", "", route_row->paths[j],
                         VTY_NEWLINE);
        }
    }
    vty_out (vty, "%s", VTY_NEWLINE);

}


static void
ospf_route_router_show(const struct ovsrec_ospf_router *router_row)
{
    const struct ovsrec_ospf_route *route_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    int i = 0, j = 0, area_id = 0;
    int64_t cost = OSPF_DEFAULT_COST;
    char area_str[OSPF_SHOW_STR_LEN];
    const char *abr = NULL;
    const char *asbr = NULL;

    memset(area_str,'\0', OSPF_SHOW_STR_LEN);

    vty_out (vty, "============ OSPF router routing table =============%s",
         VTY_NEWLINE);

    OVSREC_OSPF_AREA_FOR_EACH(area_row, idl)
    {
        /* Print inter area routes. */
        for (i = 0; i < area_row->n_router_ospf_routes; i++)
        {
            route_row = area_row->router_ospf_routes[i];
            area_id = smap_get_int(&route_row->route_info,
                                   OSPF_KEY_ROUTE_AREA_ID, 0);
            if (area_id != 0)
            {
                OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
            }
            else
            {
                strncpy(area_str, "0.0.0.0", OSPF_SHOW_STR_LEN - 1);
            }

            cost = smap_get_int(&route_row->route_info,
                                OSPF_KEY_ROUTE_COST, OSPF_DEFAULT_COST);
            abr = smap_get(&route_row->route_info, OSPF_KEY_ROUTE_TYPE_ABR);
            asbr = smap_get(&route_row->route_info, OSPF_KEY_ROUTE_TYPE_ASBR);

            vty_out (vty, "R    %-15s    %s [%lu] area: %s%s%s%s",
                     route_row->prefix,
                     !strcmp(route_row->path_type,
                     OSPF_PATH_TYPE_STRING_INTER_AREA) ? "IA" : "  ",
                     cost, area_str,
                     (abr && !strcmp(abr, "true")) ? ", ABR" : "",
                     (asbr && !strcmp(asbr, "true")) ? ", ASBR" : "",
                     VTY_NEWLINE);

            for(j = 0; j < route_row->n_paths; j++)
                vty_out (vty, "%24s   %s%s", "", route_row->paths[j],
                         VTY_NEWLINE);
        }
    }
    vty_out (vty, "%s", VTY_NEWLINE);

}

static void
ospf_route_external_show(const struct ovsrec_ospf_router *router_row)
{
    const struct ovsrec_ospf_route *route_row = NULL;
    int i = 0, j = 0, area_id = 0;
    int64_t cost = OSPF_DEFAULT_COST;
    char area_str[OSPF_SHOW_STR_LEN];
    const char *val = NULL;

    memset(area_str,'\0', OSPF_SHOW_STR_LEN);

    vty_out (vty, "============ OSPF external routing table ===========%s",
         VTY_NEWLINE);

    /* Print inter area routes. */
    for (i = 0; i < router_row->n_ext_ospf_routes; i++)
    {
        route_row = router_row->ext_ospf_routes[i];
        area_id = smap_get_int(&route_row->route_info,
                               OSPF_KEY_ROUTE_AREA_ID, 0);
        if (area_id != 0)
        {
            OSPF_IP_STRING_CONVERT(area_str, ntohl(area_id));
        }
        else
        {
            strncpy(area_str, "0.0.0.0", OSPF_SHOW_STR_LEN - 1);
        }

        val = smap_get(&route_row->route_info,
                                OSPF_KEY_ROUTE_EXT_TYPE);
        cost = smap_get_int(&route_row->route_info,
                            OSPF_KEY_ROUTE_COST, OSPF_DEFAULT_COST);

        if(!strcmp(val, OSPF_EXT_TYPE_STRING_TYPE1))
        {
            vty_out (vty, "N E1 %-18s    [%lu] tag: %u%s",
                     route_row->prefix,
                     cost,
                     smap_get_int(&route_row->route_info,
                                  OSPF_KEY_ROUTE_EXT_TAG, 0),
                     VTY_NEWLINE);
        }
        else if(!strcmp(val, OSPF_EXT_TYPE_STRING_TYPE2))
        {
            vty_out (vty, "N E2 %-18s    [%lu/%u] tag: %u%s",
                     route_row->prefix, cost,
                     smap_get_int(&route_row->route_info,
                                  OSPF_KEY_ROUTE_TYPE2_COST,
                                  OSPF_ROUTE_TYPE2_COST_DEFAULT),
                     smap_get_int(&route_row->route_info,
                                  OSPF_KEY_ROUTE_EXT_TAG, 0),
                     VTY_NEWLINE);
        }

        for(j = 0; j < route_row->n_paths; j++)
            vty_out (vty, "%24s   %s%s", "", route_row->paths[j],
                     VTY_NEWLINE);
    }

    vty_out (vty, "%s", VTY_NEWLINE);

}


static int
ospf_ip_route_show()
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    int instance_tag = 1;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);

    /* Get the OSPF_Router row. */
    ospf_router_row =
    ospf_router_lookup_by_instance_id(vrf_row, instance_tag);
    if (ospf_router_row == NULL)
    {
        vty_out (vty, " OSPF Routing Process not enabled%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (ovsrec_ospf_route_first(idl) == NULL )
    {
        vty_out (vty, "No OSPF routing information exist%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    vty_out(vty, "%s%s%s%s",
            "Codes: N - Network, R - Router, IA - Inter Area,", VTY_NEWLINE,
            "       E1 - External Type 1, E2 - External Type 2", VTY_NEWLINE);

    /* Show Network routes. */
    ospf_route_network_show (ospf_router_row);

    /* Show Router routes. */
    ospf_route_router_show (ospf_router_row);

    /* Show AS External routes. */
    ospf_route_external_show (ospf_router_row);

    return CMD_SUCCESS;

}

DEFUN(cli_ospf_router,
      cli_ospf_router_cmd,
      "router ospf",
      ROUTER_STR
      OSPF_CONF_STR)
{
    int64_t instance_tag = 1;

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
    int64_t instance_tag = 1;

    /* When VRF and instance is supported proper values will be passed */
    ospf_no_router_cmd_execute(DEFAULT_VRF_NAME, instance_tag);
    return CMD_SUCCESS;
}


/*  router-id. */
DEFUN(cli_ospf_router_id,
      cli_ospf_router_id_cmd,
      "router-id A.B.C.D",
      OSPF_ROUTER_ID_STR
      OSPF_ROUTER_ID_VAL_STR)
{
    return ospf_router_id_cmd_execute(DEFAULT_VRF_NAME,
                                      CONST_CAST(char*, argv[0]));
}

/* no  router-id. */
DEFUN(cli_no_ospf_router_id,
      cli_no_ospf_router_id_cmd,
      "no router-id",
      NO_STR
      OSPF_ROUTER_ID_STR)
{
    return ospf_no_router_id_cmd_execute(DEFAULT_VRF_NAME);
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
    int instance_id = 1;
    int64_t area_id = 0;
    struct prefix_ipv4 p;

    area_id = htonl(atoi(argv[1]));

    /* TO DO: Check the validity of network range.*/
    VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);

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

    /* Check the validity if its not backone. */
    if (!strcmp(argv[1], OSPF_DEFAULT_STR))
    {
        id.s_addr = 0;
    }
    else
    {
        if(ospf_string_is_an_ip_addr(argv[1]))
        {
            ret = inet_aton (argv[1], &id);
            if (!ret || (id.s_addr == 0))
            {
                vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
                return CMD_WARNING;
            }
        }
    }

    return ospf_router_area_id_cmd_execute(false, instance_id, argv[0],
                                           id.s_addr);
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

    if (atoi(argv[1]) != 0)
    {
        OSPF_IP_STRING_CONVERT(ip_str, atoi(argv[1]));

        ret = inet_aton (ip_str, &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }
    else
    {
        id.s_addr = 0;
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

    if (strcmp(argv[1], OSPF_DEFAULT_STR))
    {
        if(ospf_string_is_an_ip_addr(argv[1]))
        {
            ret = inet_aton (argv[1], &id);
            if (!ret || (id.s_addr == 0))
            {
                vty_out (vty, "Malformed area identifier.%s", VTY_NEWLINE);
                return CMD_WARNING;
            }
        }
    }
    else
    {
        id.s_addr = 0;
    }

    return ospf_router_area_id_cmd_execute(true, instance_id, argv[0],
                                           id.s_addr);
}


DEFUN(cli_ospf_router_max_metric,
      cli_ospf_router_max_metric_cmd,
      "max-metric router-lsa {on-startup <5-86400>}",
      OSPF_MAX_METRIC_STR
      OSPF_ROUTER_LSA_STR
      OSPF_ON_STARTUP_STR
      OSPF_STARTUP_TIME_STR)
{

    if(argv[0])
    {
        return ospf_max_metric_startup_cmd_execute(false, DEFAULT_VRF_NAME, 1,
                                                   argv[0]);
    }
    else
        return ospf_max_metric_admin_cmd_execute(false, DEFAULT_VRF_NAME, 1);
}

DEFUN(cli_ospf_router_no_max_metric,
      cli_ospf_router_no_max_metric_cmd,
      "no max-metric router-lsa {on-startup}",
      NO_STR
      OSPF_MAX_METRIC_STR
      OSPF_ROUTER_LSA_STR
      OSPF_ON_STARTUP_STR)
{
    if(argv[0])
    {
        ospf_max_metric_startup_cmd_execute(true, DEFAULT_VRF_NAME, 1, 0);
    }
    else
    {
        return ospf_max_metric_admin_cmd_execute(true, DEFAULT_VRF_NAME, 1);
    }
}


DEFUN(cli_ospf_router_hello_interval,
      cli_ospf_router_hello_interval_cmd,
      "ip ospf hello-interval <1-65535>",
      IP_STR
      OSPF_INTERFACE_OSPF
      OSPF_HELLO_INTERVAL_STR
      OSPF_HELLO_INTERVAL_VAL_STR)
{

    return ospf_interval_cmd_execute((char*)vty->index, OSPF_KEY_HELLO_INTERVAL,
                                     atoi(argv[0]));
}

DEFUN(cli_ospf_router_no_hello_interval,
      cli_ospf_router_no_hello_interval_cmd,
      "no ip ospf hello-interval",
      NO_STR
      IP_STR
      OSPF_INTERFACE_OSPF
      OSPF_HELLO_INTERVAL_STR)
{

    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_HELLO_INTERVAL,
                                     OSPF_HELLO_INTERVAL_DEFAULT);
}

DEFUN(cli_ospf_router_dead_interval,
      cli_ospf_router_dead_interval_cmd,
      "ip ospf dead-interval <1-65535>",
      IP_STR
      OSPF_INTERFACE_OSPF
      OSPF_DEAD_INTERVAL_STR
      OSPF_DEAD_INTERVAL_VAL_STR)
{

    return ospf_interval_cmd_execute((char*)vty->index, OSPF_KEY_DEAD_INTERVAL,
                                     atoi(argv[0]));
}

DEFUN(cli_ospf_router_no_dead_interval,
      cli_ospf_router_no_dead_interval_cmd,
      "no ip ospf dead-interval",
      NO_STR
      IP_STR
      OSPF_INTERFACE_OSPF
      OSPF_DEAD_INTERVAL_STR)
{

    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_DEAD_INTERVAL,
                                     OSPF_DEAD_INTERVAL_DEFAULT);
}

DEFUN (cli_ip_ospf_retransmit_interval,
       cli_ip_ospf_retransmit_interval_cmd,
       "ip ospf retransmit-interval <3-65535>",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Time between retransmitting lost link state advertisements\n"
       "Interval in seconds (Default: 5)\n")
{
    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_RETRANSMIT_INTERVAL,
                                     atoi(argv[0]));
}

DEFUN (cli_ip_ospf_no_retransmit_interval,
       cli_ip_ospf_no_retransmit_interval_cmd,
       "no ip ospf retransmit-interval",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Time between retransmitting lost link state advertisements\n")
{
    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_RETRANSMIT_INTERVAL,
                                     OSPF_RETRANSMIT_INTERVAL_DEFAULT);
}

DEFUN (cli_ip_ospf_transmit_delay,
       cli_ip_ospf_transmit_delay_cmd,
       "ip ospf transmit-delay <1-65535>",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Link state transmit delay\n"
       "Interval in seconds (Default: 1)\n")
{
    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_TRANSMIT_DELAY,
                                     atoi(argv[0]));
}

DEFUN (cli_ip_ospf_no_transmit_delay,
       cli_ip_ospf_no_transmit_delay_cmd,
       "no ip ospf transmit-delay",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Link state transmit delay\n")
{
    return ospf_interval_cmd_execute((char*)vty->index,
                                     OSPF_KEY_TRANSMIT_DELAY,
                                     OSPF_TRANSMIT_DELAY_DEFAULT);
}

/* Function to handle "[no] ip ospf priority <value>" command. */
static int
ospf_interface_priority_cmd_execute(const char* ifname,
                                    const int64_t priority)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 interface is present.");
    }

    ovsrec_port_set_ospf_priority(port_row, &priority, 1);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

DEFUN (cli_ip_ospf_priority,
       cli_ip_ospf_priority_cmd,
       "ip ospf priority <0-255>",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Router priority\n"
       "Priority value (Default: 1)\n")
{
    int64_t priority = 0;

    priority = atoi(argv[0]);

    return ospf_interface_priority_cmd_execute((char*)vty->index, priority);
}

DEFUN (cli_ip_ospf_no_priority,
       cli_ip_ospf_no_priority_cmd,
       "no ip ospf priority",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Router priority\n")
{
    return ospf_interface_priority_cmd_execute((char*)vty->index,
                                               OSPF_ROUTER_PRIORITY_DEFAULT);
}

/* Function to handle "[no] ip ospf mtu-ignore " command. */
static int
ospf_interface_mtu_ignore_cmd_execute(const char* ifname,
                                      bool mtu_ignore)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF Interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 Interface is present.");
    }

    ovsrec_port_set_ospf_mtu_ignore(port_row, &mtu_ignore, 1);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

DEFUN (cli_ip_ospf_mtu_ignore,
       cli_ip_ospf_mtu_ignore_cmd,
       "ip ospf mtu-ignore",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Disable MTU mismatch detection\n")
{
    return ospf_interface_mtu_ignore_cmd_execute((char*)vty->index, true);
}

DEFUN (cli_ip_ospf_no_mtu_ignore,
       cli_ip_ospf_no_mtu_ignore_cmd,
       "no ip ospf mtu-ignore",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Disable MTU mismatch detection\n")
{
    return ospf_interface_mtu_ignore_cmd_execute((char*)vty->index, false);
}

/* Function to handle "[no] ip ospf cost <cost>" command. */
static int
ospf_interface_cost_cmd_execute(const char* ifname,
                                      const int64_t cost)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 Interface is present.");
    }

    ovsrec_port_set_ospf_if_out_cost(port_row, &cost, 1);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

DEFUN (cli_ip_ospf_cost,
       cli_ip_ospf_cost_cmd,
       "ip ospf cost <1-65535>",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Interface cost\n"
       "Cost\n")
{
    int64_t cost = strtol (argv[0], NULL, 10);

    return ospf_interface_cost_cmd_execute((char*)vty->index, cost);
}

DEFUN (cli_ip_ospf_no_cost,
       cli_ip_ospf_no_cost_cmd,
       "no ip ospf cost",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Interface cost\n")
{
    return ospf_interface_cost_cmd_execute((char*)vty->index, OSPF_DEFAULT_COST);
}

/* Function to handle "[no] ip ospf network <type>" command. */
static int
ospf_interface_network_cmd_execute(const char* ifname,
                                   const char* network_type)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 Interface is present.");
    }

    ovsrec_port_set_ospf_if_type(port_row, network_type);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

DEFUN (cli_ip_ospf_network,
       cli_ip_ospf_network_cmd,
       "ip ospf network (broadcast|point-to-point)",
       IP_STR
       OSPF_INTERFACE_OSPF
       "Network type\n"
       "Specify OSPF broadcast multi-access network\n"
       "Specify OSPF point-to-point network\n")
{
    if (!strcmp(argv[0], "broadcast"))
        return ospf_interface_network_cmd_execute((char*)vty->index,
                               OVSREC_PORT_OSPF_IF_TYPE_OSPF_IFTYPE_BROADCAST);
    else if (!strcmp(argv[0], "point-to-point"))
        return ospf_interface_network_cmd_execute((char*)vty->index,
                              OVSREC_PORT_OSPF_IF_TYPE_OSPF_IFTYPE_POINTOPOINT);
    else
        return CMD_OVSDB_FAILURE;
}

DEFUN (cli_ip_ospf_no_network,
       cli_ip_ospf_no_network_cmd,
       "no ip ospf network",
       NO_STR
       IP_STR
       OSPF_INTERFACE_OSPF
       "Network type\n")
{
    return ospf_interface_network_cmd_execute((char*)vty->index,
                               OVSREC_PORT_OSPF_IF_TYPE_OSPF_IFTYPE_BROADCAST);
}

static int ospf_area_auth_cmd_execute(bool no_flag, int instance_id,
                                              int64_t area_id, bool md5_auth)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    int i = 0;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                              instance_id);

    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id);

    if (!no_flag)
    {
        /* Insert the new row and link the area row to the router table. */
        if (area_row == NULL)
        {
            area_row = ovsrec_ospf_area_insert(ospf_router_txn);
            if (ospf_area_insert_to_router(ospf_router_row, area_row, area_id)
                != CMD_SUCCESS)
            {
                OSPF_ERRONEOUS_DB_TXN(ospf_router_txn,
                                      "Could not update configuration.");
            }
        }

        if (md5_auth == true)
        {
            ovsrec_ospf_area_set_ospf_auth_type(area_row,
                                        OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_MD5);
        }
        else
        {
            ovsrec_ospf_area_set_ospf_auth_type(area_row,
                                        OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_TEXT);
        }
    }
    else
    {
        if (area_row == NULL)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn,
                                  "Configuration is not present.");
        }

        ovsrec_ospf_area_set_ospf_auth_type(area_row, NULL);
    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

DEFUN (cli_ospf_area_auth,
          cli_ospf_area_auth_cmd,
          "area (A.B.C.D|<0-4294967295>) authentication",
          OSPF_AREA_STR
          OSPF_AREA_IP_STR
          OSPF_AREA_RANGE
          OSPF_AUTH_ENABLE)
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID (area_id, argv[0]);

    return ospf_area_auth_cmd_execute(false, 1, area_id.s_addr, false);
}

DEFUN (cli_ospf_area_auth_message_digest,
       cli_ospf_area_auth_message_digest_cmd,
       "area (A.B.C.D|<0-4294967295>) authentication message-digest",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       OSPF_AUTH_ENABLE
       OSPF_AUTH_MD5)
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID (area_id, argv[0]);

    return ospf_area_auth_cmd_execute(false, 1, area_id.s_addr, true);
}


DEFUN (cli_no_ospf_area_authentication,
       cli_no_ospf_area_authentication_cmd,
       "no area (A.B.C.D|<0-4294967295>) authentication",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       OSPF_AUTH_ENABLE)
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID (area_id, argv[0]);

    return ospf_area_auth_cmd_execute(true, 1, area_id.s_addr, false);
}

static int
ospf_interface_auth_cmd_execute(const char* ifname,
                                        const char* auth_type)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 Interface is present.");
    }

        ovsrec_port_set_ospf_auth_type(port_row, auth_type);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

/* `ip ospf authentication {message-digest}`*/
DEFUN (cli_ospf_interface_auth,
       cli_ospf_interface_auth_cmd,
       "ip ospf authentication",
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_ENABLE)
{
    return ospf_interface_auth_cmd_execute(vty->index,
                                           OVSREC_PORT_OSPF_AUTH_TYPE_TEXT);
}

DEFUN (cli_ospf_interface_auth_message_digest,
       cli_ospf_interface_auth_message_digest_cmd,
       "ip ospf authentication message-digest",
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_ENABLE
       OSPF_AUTH_MD5)
{
    return ospf_interface_auth_cmd_execute(vty->index,
                                           OVSREC_PORT_OSPF_AUTH_TYPE_MD5);
}

DEFUN (cli_ospf_interface_auth_null,
       cli_ospf_interface_auth_null_cmd,
       "ip ospf authentication null",
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_ENABLE
       OSPF_AUTH_NULL_STR)
{
    return ospf_interface_auth_cmd_execute(vty->index,
                                           OVSREC_PORT_OSPF_AUTH_TYPE_NULL);
}

/* `no ip ospf authentication`*/
DEFUN (cli_no_ospf_interface_auth,
       cli_no_ospf_interface_auth_cmd,
       "no ip ospf authentication",
       NO_STR
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_ENABLE)
{
         return ospf_interface_auth_cmd_execute(vty->index, NULL);
}


/* Update the key if key id is already present.
    Insert md5 key in the Port table if not present */
int
ospf_md5_key_update_to_port(const struct ovsrec_port *port_row,
                           int64_t key_id,const char *key)
{
    int64_t *key_id_list;
    char **key_list;
    int i = 0;

    key_id_list = xmalloc(sizeof(int64_t) * (port_row->n_ospf_auth_md5_keys + 1));
    key_list = xmalloc(sizeof * port_row->key_ospf_auth_md5_keys*
                              (port_row->n_ospf_auth_md5_keys + 1));

    if (!key_id_list || !key_list)
    {
        SAFE_FREE(key_id_list);
        SAFE_FREE(key_list);
        VLOG_DBG("Memory alloc failed, could not update md5 key in port.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < port_row->n_ospf_auth_md5_keys; i++)
    {
        if (port_row->key_ospf_auth_md5_keys[i] == key_id)
        {
            SAFE_FREE(key_id_list);
            SAFE_FREE(key_list);
            return CMD_ERR_AMBIGUOUS;
        }
        else
        {
            key_id_list[i] = port_row->key_ospf_auth_md5_keys[i];
            key_list[i] = port_row->value_ospf_auth_md5_keys[i];
        }
    }

    key_id_list[port_row->n_ospf_auth_md5_keys] = key_id;
    key_list[port_row->n_ospf_auth_md5_keys] = CONST_CAST(char *, key);
    ovsrec_port_set_ospf_auth_md5_keys(port_row, key_id_list, key_list,
                                      (port_row->n_ospf_auth_md5_keys + 1));

    SAFE_FREE(key_id_list);
    SAFE_FREE(key_list);

    return CMD_SUCCESS;
}

/* Update the key if key id is already present.
    Insert md5 key in the Port table if not present */
int
ospf_md5_key_update_to_vl(const struct ovsrec_ospf_vlink *vl_row,
                           int64_t key_id,const char *key)
{
    int64_t *key_id_list;
    char **key_list;
    int i = 0;

    key_id_list = xmalloc(sizeof(int64_t) * (vl_row->n_ospf_auth_md5_keys + 1));
    key_list = xmalloc(sizeof * vl_row->key_ospf_auth_md5_keys*
                              (vl_row->n_ospf_auth_md5_keys + 1));

    if (!key_id_list || !key_list)
    {
        SAFE_FREE(key_id_list);
        SAFE_FREE(key_list);
        VLOG_DBG("Memory alloc failed, could not update md5 key in VL.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < vl_row->n_ospf_auth_md5_keys; i++)
    {
        if (vl_row->key_ospf_auth_md5_keys[i] == key_id)
        {
            SAFE_FREE(key_id_list);
            SAFE_FREE(key_list);
            return CMD_ERR_AMBIGUOUS;
        }
        else
        {
            key_id_list[i] = vl_row->key_ospf_auth_md5_keys[i];
            key_list[i] = vl_row->value_ospf_auth_md5_keys[i];
        }
    }

    key_id_list[vl_row->n_ospf_auth_md5_keys] = key_id;
    key_list[vl_row->n_ospf_auth_md5_keys] = CONST_CAST(char *, key);
    ovsrec_ospf_vlink_set_ospf_auth_md5_keys(vl_row, key_id_list, key_list,
                                      (vl_row->n_ospf_auth_md5_keys + 1));

    SAFE_FREE(key_id_list);
    SAFE_FREE(key_list);

    return CMD_SUCCESS;
}

/* Delete the key if key id is already present.*/
int
ospf_md5_key_remove_from_vl(const struct ovsrec_ospf_vlink *vl_row,
                              int64_t key_id)
{
    int64_t *key_id_list;
    char **key_list;
    int i = 0, j =0;
    bool is_present = false;
    int res = CMD_ERR_NO_MATCH;

    key_id_list = xmalloc(sizeof(int64_t) * (vl_row->n_ospf_auth_md5_keys));
    key_list = xmalloc(sizeof * vl_row->key_ospf_auth_md5_keys*
                              (vl_row->n_ospf_auth_md5_keys));

    if (!key_id_list || !key_list)
    {
        SAFE_FREE(key_id_list);
        SAFE_FREE(key_list);
        VLOG_DBG("Memory alloc failed, could not update md5 key in port.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < vl_row->n_ospf_auth_md5_keys; i++)
    {
        if (vl_row->key_ospf_auth_md5_keys[i] == key_id)
        {
            is_present = true;
            res = CMD_SUCCESS;
        }
        else
        {
            key_id_list[j] = vl_row->key_ospf_auth_md5_keys[i];
            key_list[j] = vl_row->value_ospf_auth_md5_keys[i];
            j++;
        }
    }

    if(is_present == true)
    {
        ovsrec_ospf_vlink_set_ospf_auth_md5_keys(vl_row, key_id_list, key_list,
                                   (vl_row->n_ospf_auth_md5_keys - 1));
    }

    SAFE_FREE(key_id_list);
    SAFE_FREE(key_list);

    return res;
}

/* Delete the key if key id is already present.*/
int
ospf_md5_key_remove_from_port(const struct ovsrec_port *port_row,
                              int64_t key_id)
{
    int64_t *key_id_list;
    char **key_list;
    int i = 0, j =0;
    bool is_present = false;
    int res = CMD_ERR_NO_MATCH;

    key_id_list = xmalloc(sizeof(int64_t) * (port_row->n_ospf_auth_md5_keys));
    key_list = xmalloc(sizeof * port_row->key_ospf_auth_md5_keys*
                              (port_row->n_ospf_auth_md5_keys));

    if (!key_id_list || !key_list)
    {
        SAFE_FREE(key_id_list);
        SAFE_FREE(key_list);
        VLOG_DBG("Memory alloc failed, could not update md5 key in port.%s",
                 VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }

    for (i = 0; i < port_row->n_ospf_auth_md5_keys; i++)
    {
        if (port_row->key_ospf_auth_md5_keys[i] == key_id)
        {
            is_present = true;
            res = CMD_SUCCESS;
        }
        else
        {
            key_id_list[j] = port_row->key_ospf_auth_md5_keys[i];
            key_list[j] = port_row->value_ospf_auth_md5_keys[i];
            j++;
        }
    }

    if(is_present == true)
    {
        ovsrec_port_set_ospf_auth_md5_keys(port_row, key_id_list, key_list,
                                   (port_row->n_ospf_auth_md5_keys - 1));
    }

    SAFE_FREE(key_id_list);
    SAFE_FREE(key_list);

    return res;
}


static int
ospf_interface_auth_key_cmd_execute(const char* ifname, bool no_flag,
                                             int64_t key_id, const char* key)
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_ospf_interface *ospf_interface_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    /* Get the interface row for the interface name passed. */
    OVSREC_OSPF_INTERFACE_FOR_EACH(ospf_interface_row, idl)
    {
        if (strcmp(ospf_interface_row->name, ifname) == 0)
            break;
    }

    if (ospf_interface_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn, "OSPF interface is not present.");
    }
    else
    {
        port_row = ospf_interface_row->port;
    }

    if (port_row == NULL)
    {
        OSPF_ABORT_DB_TXN(ospf_router_txn,
                          "Unattached OSPFv2 Interface is present.");
    }

    if(!no_flag)
    {
        if (key_id == 0)
            ovsrec_port_set_ospf_auth_text_key(port_row, key);
        else
        {
            int result = ospf_md5_key_update_to_port(port_row, key_id, key);
            if ( result == CMD_ERR_AMBIGUOUS)
            {
                OSPF_ABORT_DB_TXN(ospf_router_txn, "MD5 key is already present.");
            }
            else if ( result != CMD_SUCCESS)
            {
                OSPF_ABORT_DB_TXN(ospf_router_txn, "MD5 key updation failed.");
            }
        }
    }
    else
    {
        if (key_id == 0)
            ovsrec_port_set_ospf_auth_text_key(port_row, NULL);
        else
        {
            int res = CMD_OVSDB_FAILURE;
            res = ospf_md5_key_remove_from_port(port_row, key_id);
            if (res == CMD_ERR_NO_MATCH)
            {
                OSPF_ABORT_DB_TXN(ospf_router_txn,
                                  "MD5 key id is not present.");
            }
            else if(res != CMD_SUCCESS)
            {
                OSPF_ABORT_DB_TXN(ospf_router_txn,
                                  "MD5 key id deletion failed.");
            }
        }

    }

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}

/* `ip ospf authentication-key <key>`*/
DEFUN (cli_ospf_interface_auth_key,
       cli_ospf_interface_auth_key_cmd,
       "ip ospf authentication-key AUTH_KEY",
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_KEY
       OSPF_AUTH_KEY_VAL)
{
    return ospf_interface_auth_key_cmd_execute(vty->index, false, 0, argv[0]);
}

/* `no ip ospf authentication-key`*/
DEFUN (cli_no_ospf_interface_auth_key,
       cli_no_ospf_interface_auth_key_cmd,
       "no ip ospf authentication-key",
       NO_STR
       IP_STR
       OSPF_CONF_STR
       OSPF_AUTH_KEY)
{
        return ospf_interface_auth_key_cmd_execute(vty->index, true,
                                                   0, NULL);
}


/* `ip ospf message-digest-key <key_id> md5 <message_digest_key>`*/
DEFUN (cli_ip_ospf_message_digest_key,
       cli_ip_ospf_message_digest_key_cmd,
       "ip ospf message-digest-key <1-255> md5 KEY",
       IP_STR
       OSPF_CONF_STR
       OSPF_MD5_KEY
       OSPF_MD5_KEY_ID
       OSPF_MD5
       OSPF_MD5_PASSWORD)
{
    return ospf_interface_auth_key_cmd_execute(vty->index, false,
                                               atol(argv[0]), argv[1]);
}

/* `no ip ospf message-digest-key <key_id>`*/
DEFUN (cli_no_ip_ospf_message_digest_key,
       cli_no_ip_ospf_message_digest_key_cmd,
       "no ip ospf message-digest-key <1-255>",
       NO_STR
       IP_STR
       OSPF_CONF_STR
       OSPF_MD5_KEY
       OSPF_MD5_KEY_ID)
{
    return ospf_interface_auth_key_cmd_execute(vty->index, true,
                                               atol(argv[0]), NULL);
}

static int ospf_area_type_cmd_execute(bool no_flag, int instance_id,
                                      int64_t area_id, const char *area_type,
                                      const char *nssa_role)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row,
                                                        instance_id);

    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id);

    if (!no_flag)
    {
        /* Insert the new row and link the area row to the router table. */
        if (area_row == NULL)
        {
            area_row = ovsrec_ospf_area_insert(ospf_router_txn);
            if (ospf_area_insert_to_router(ospf_router_row, area_row, area_id)
                != CMD_SUCCESS)
            {
                OSPF_ERRONEOUS_DB_TXN(ospf_router_txn,
                                      "Could not update configuration.");
            }
        }
    }
    else
    {
        if (area_row == NULL)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn,
                                  "Configuration is not present.");
        }
    }

    ovsrec_ospf_area_set_area_type(area_row, area_type);

    if(nssa_role)
        ovsrec_ospf_area_set_nssa_translator_role(area_row, nssa_role);

    /* End of transaction. */
    OSPF_END_DB_TXN(ospf_router_txn);

    return CMD_SUCCESS;

}



DEFUN (cli_ospf_area_nssa_translate,
       cli_ospf_area_nssa_translate_cmd,
       "area (A.B.C.D|<0-4294967295>) nssa (translate-candidate|"
       "translate-never|translate-always)",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n"
       "Configure NSSA-ABR for translate election (default)\n"
       "Configure NSSA-ABR to never translate\n"
       "Configure NSSA-ABR to always translate\n")
{
    struct in_addr area_id;
    char nssa_role[OSPF_SHOW_STR_LEN];

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    if (!strcmp(argv[1], "translate-candidate"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE,
                OSPF_SHOW_STR_LEN);
    else if (!strcmp(argv[1], "translate-never"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_NEVER,
                OSPF_SHOW_STR_LEN);
    else if (!strcmp(argv[1], "translate-always"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_ALWAYS,
                OSPF_SHOW_STR_LEN);
    else
    {
        vty_out(vty, "NSSA type passed is incorrect.%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                                      OVSREC_OSPF_AREA_AREA_TYPE_NSSA,
                                      nssa_role);
}

DEFUN (cli_ospf_area_nssa_translate_no_summary,
       cli_ospf_area_nssa_translate_no_summary_cmd,
       "area (A.B.C.D|<0-4294967295>) nssa (translate-candidate|"
       "translate-never|translate-always) no-summary",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n"
       "Configure NSSA-ABR for translate election (default)\n"
       "Configure NSSA-ABR to never translate\n"
       "Configure NSSA-ABR to always translate\n"
       OSPF_NO_SUMMARY_STR)
{
    struct in_addr area_id;
    char nssa_role[OSPF_SHOW_STR_LEN];

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    if (!strcmp(argv[1], "translate-candidate"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE,
                OSPF_SHOW_STR_LEN);
    else if (!strcmp(argv[1], "translate-never"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_NEVER,
                OSPF_SHOW_STR_LEN);
    else if (!strcmp(argv[1], "translate-always"))
        strncpy(nssa_role, OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_ALWAYS,
                OSPF_SHOW_STR_LEN);
    else
    {
        vty_out(vty, "NSSA type passed is incorrect.%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                                    OVSREC_OSPF_AREA_AREA_TYPE_NSSA_NO_SUMMARY,
                                    nssa_role);
}

/* `area (<area_ip>|<area_id>) nssa [no_summary]` */
DEFUN (cli_ospf_area_nssa,
       cli_ospf_area_nssa_cmd,
       "area (A.B.C.D|<0-4294967295>) nssa",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                               OVSREC_OSPF_AREA_AREA_TYPE_NSSA,
                               OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE);
}


DEFUN (cli_ospf_area_nssa_no_summary,
       cli_ospf_area_nssa_no_summary_cmd,
       "area (A.B.C.D|<0-4294967295>) nssa no-summary",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n"
       OSPF_NO_SUMMARY_STR)
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                               OVSREC_OSPF_AREA_AREA_TYPE_NSSA_NO_SUMMARY,
                               OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE);
}


/* `no area (<area_ip>|<area_id>) nssa [no_summary]` */
DEFUN (cli_ospf_area_no_nssa_translate,
       cli_ospf_area_no_nssa_translate_cmd,
       "no area (A.B.C.D|<0-4294967295>) nssa",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    return ospf_area_type_cmd_execute(true, 1, area_id.s_addr,
                               OVSREC_OSPF_AREA_AREA_TYPE_DEFAULT,
                               OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_CANDIDATE);
}


DEFUN (cli_ospf_area_no_nssa_no_summary,
       cli_ospf_area_no_nssa_no_summary_cmd,
       "no area (A.B.C.D|<0-4294967295>) nssa no-summary",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as nssa\n"
       OSPF_NO_SUMMARY_STR)
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("NSSA", area_id, argv[0]);

    return ospf_area_type_cmd_execute(true, 1, area_id.s_addr,
                                      OVSREC_OSPF_AREA_AREA_TYPE_NSSA, NULL);
}


/* `area (<area_ip>|<area_id>) stub ` */
DEFUN (cli_ospf_area_stub,
       cli_ospf_area_stub_cmd,
       "area (A.B.C.D|<0-4294967295>) stub",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as stub\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("stub", area_id, argv[0]);

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                                      OVSREC_OSPF_AREA_AREA_TYPE_STUB,
                                      NULL);
}

/* `area (<area_ip>|<area_id>) stub [no_summary]` */
DEFUN (cli_ospf_area_stub_no_summary,
       cli_ospf_area_stub_no_summary_cmd,
       "area (A.B.C.D|<0-4294967295>) stub no-summary",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as stub\n"
       "Do not inject inter-area routes into area\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("stub", area_id, argv[0]);

    return ospf_area_type_cmd_execute(false, 1, area_id.s_addr,
                                     OVSREC_OSPF_AREA_AREA_TYPE_STUB_NO_SUMMARY,
                                     NULL);
}

/* `no area (<area_ip>|<area_id>) stub ` */
DEFUN (cli_no_ospf_area_stub,
       cli_no_ospf_area_stub_cmd,
       "no area (A.B.C.D|<0-4294967295>) stub",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as stub\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("stub", area_id, argv[0]);

    return ospf_area_type_cmd_execute(true, 1, area_id.s_addr,
                                      OVSREC_OSPF_AREA_AREA_TYPE_DEFAULT,
                                      NULL);
}

/* `no area (<area_ip>|<area_id>) stub [no_summary]` */
DEFUN (cli_no_ospf_area_stub_no_summary,
       cli_no_ospf_area_stub_no_summary_cmd,
       "no area (A.B.C.D|<0-4294967295>) stub no-summary",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure OSPF area as stub\n"
       "Do not inject inter-area routes into area\n")
{
    struct in_addr area_id;

    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID_NO_BB ("stub", area_id, argv[0]);

    return ospf_area_type_cmd_execute(true, 1, area_id.s_addr,
                                     OVSREC_OSPF_AREA_AREA_TYPE_DEFAULT,
                                     NULL);
}



DEFUN (cli_ip_ospf_show,
       cli_ip_ospf_show_cmd,
       "show ip ospf",
       SHOW_STR
       IP_STR
       OSPF_STR)
{
    ospf_ip_router_show();
    return CMD_SUCCESS;
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
    return CMD_SUCCESS;

}


DEFUN (cli_ip_ospf_neighbor_show,
       cli_ip_ospf_neighbor_show_cmd,
       "show ip ospf neighbor {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       ALL_STR)
{
    if (argv[0])
        ospf_ip_router_neighbor_show(NULL, 0, true);
    else
        ospf_ip_router_neighbor_show(NULL, 0, false);
    return CMD_SUCCESS;
}

DEFUN (cli_ip_ospf_neighbor_detail_show,
       cli_ip_ospf_neighbor_detail_show_cmd,
       "show ip ospf neighbor detail {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       DETAIL_STR
       ALL_STR)
{
    if (argv[0])
        ospf_ip_router_neighbor_detail_show(NULL, 0, true);
    else
        ospf_ip_router_neighbor_detail_show(NULL, 0, false);
    return CMD_SUCCESS;
}

DEFUN (cli_ip_ospf_neighbor_ifname_show,
       cli_ip_ospf_neighbor_ifname_show_cmd,
       "show ip ospf neighbor IFNAME {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       IFNAME_STR
       ALL_STR)
{
    if (argv[1])
        ospf_ip_router_neighbor_show(argv[0], 0, true);
    else
        ospf_ip_router_neighbor_show(argv[0], 0, false);
    return CMD_SUCCESS;
}

DEFUN (cli_ip_ospf_nbr_ifname_detail_show,
       cli_ip_ospf_nbr_ifname_detail_show_cmd,
       "show ip ospf neighbor IFNAME detail {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       IFNAME_STR
       DETAIL_STR
       ALL_STR)
{
    if (argv[1])
        ospf_ip_router_neighbor_detail_show(argv[0], 0, true);
    else
        ospf_ip_router_neighbor_detail_show(argv[0], 0, false);
    return CMD_SUCCESS;
}

DEFUN (cli_ip_ospf_neighbor_nbrid_show,
       cli_ip_ospf_neighbor_nbrid_show_cmd,
       "show ip ospf neighbor A.B.C.D {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       OSPF_NEIGHBOR_ID_STR
       ALL_STR)
{
    struct in_addr id;
    int ret;

    memset (&id, 0, sizeof (struct in_addr));
    if(ospf_string_is_an_ip_addr(argv[0]))
    {
        ret = inet_aton (argv[0], &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed neighbor identifier.%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    if (argv[1])
        ospf_ip_router_neighbor_show(NULL, id.s_addr, true);
    else
        ospf_ip_router_neighbor_show(NULL, id.s_addr, false);
    return CMD_SUCCESS;
}

DEFUN (cli_ip_ospf_nbr_nbrid_detail_show,
       cli_ip_ospf_nbr_nbrid_detail_show_cmd,
       "show ip ospf neighbor A.B.C.D detail {all}",
       SHOW_STR
       IP_STR
       OSPF_STR
       OSPF_NEIGHBOR_SHOW_STR
       OSPF_NEIGHBOR_ID_STR
       DETAIL_STR
       ALL_STR)
{
    struct in_addr id;
    int ret;

    memset (&id, 0, sizeof (struct in_addr));
    if(ospf_string_is_an_ip_addr(argv[0]))
    {
        ret = inet_aton (argv[0], &id);
        if (!ret || (id.s_addr == 0))
        {
            vty_out (vty, "Malformed neighbor identifier.%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    if (argv[1])
        ospf_ip_router_neighbor_detail_show(NULL, id.s_addr, true);
    else
        ospf_ip_router_neighbor_detail_show(NULL, id.s_addr, false);
    return CMD_SUCCESS;
}


DEFUN (cli_ip_ospf_route_show,
       cli_ip_ospf_route_show_cmd,
       "show ip ospf route",
       SHOW_STR
       IP_STR
       OSPF_STR
       ROUTE_STR)
{
    return ospf_ip_route_show();
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
    return CMD_SUCCESS;
}

DEFUN (cli_ospf_admin_distance,
       cli_ospf_admin_distance_cmd,
       "distance <1-255>",
       "Define an administrative distance\n"
       "OSPF Administrative distance\n")
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** key_distance = NULL;
    int64_t* value_distance = NULL;
    unsigned char distance = 0;
    int instance_id = 1;
    int i = 0;
    bool key_found = false;

    do {
        vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
        if (vrf_row == NULL)
        {
            VLOG_DBG ("VRF is not present.");
            break;
        }
        /* See if it already exists. */
        ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                                                  instance_id);
        if (ospf_router_row == NULL)
        {
            VLOG_DBG ("OSPF Router instance not present");
            break;
        }
        distance = (unsigned char)atoi (argv[0]);
        if (distance < 1 && distance > 255)
        {
            VLOG_DBG ("Invalid  OSPF administrative distance");
            break;
        }
        /* OPS_TODO : Handle if the keys are not populated in "router ospf" command*/
        key_distance = xmalloc(sizeof *ospf_router_row->key_distance *
                                    ospf_router_row->n_distance);
        value_distance = xmalloc(sizeof *ospf_router_row->value_distance *
                                     ospf_router_row->n_distance);
        if (!key_distance || !value_distance)
        {
            SAFE_FREE(key_distance);
            SAFE_FREE(value_distance);
            VLOG_DBG ("Memory allocation failed");
            break;
        }
        OSPF_START_DB_TXN(ospf_router_txn);
        for (i = 0 ; i < ospf_router_row->n_distance;i++)
        {
            key_distance[i] = ospf_router_row->key_distance[i];
            if(!strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_ALL))
                value_distance[i] = (int64_t)distance;
            else
                value_distance[i] = ospf_router_row->value_distance[i];
        }
        ovsrec_ospf_router_set_distance(ospf_router_row,key_distance,value_distance,
            ospf_router_row->n_distance);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE (key_distance);
        SAFE_FREE (value_distance);
    }while (0);

  return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_admin_distance,
       cli_no_ospf_admin_distance_cmd,
       "no distance <1-255>",
       NO_STR
       "Define an administrative distance\n"
       "OSPF Administrative distance\n")
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** key_distance = NULL;
    int64_t* value_distance = NULL;
    unsigned char distance = 0;
    int instance_id = 1;
    int i = 0;
    bool key_found = false;

    do {
        vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
        if (vrf_row == NULL)
        {
            VLOG_DBG ("VRF is not present.");
            break;
        }
        /* See if it already exists. */
        ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                                                  instance_id);
        if (ospf_router_row == NULL)
        {
            VLOG_DBG ("OSPF Router instance not present");
            break;
        }
        distance = OSPF_ROUTER_DISTANCE_DEFAULT;
        /* OPS_TODO : Handle if the keys are not populated in "router ospf" command*/
        key_distance = xmalloc(sizeof *ospf_router_row->key_distance *
                                    ospf_router_row->n_distance);
        value_distance = xmalloc(sizeof *ospf_router_row->value_distance *
                                     ospf_router_row->n_distance);
        if (!key_distance || !value_distance)
        {
            SAFE_FREE(key_distance);
            SAFE_FREE(value_distance);
            VLOG_DBG ("Memory allocation failed");
            break;
        }
        OSPF_START_DB_TXN(ospf_router_txn);
        for (i = 0 ; i < ospf_router_row->n_distance;i++)
        {
            key_distance[i] = ospf_router_row->key_distance[i];
            if(!strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_ALL))
            {
                if (value_distance[i] != atoi(argv[0]))
                {
                    OSPF_ABORT_DB_TXN(ospf_router_txn,
                                      "Value was not configured earlier.");
                    return CMD_SUCCESS;
                }
                value_distance[i] = (int64_t)distance;
            }
            else
                value_distance[i] = ospf_router_row->value_distance[i];
        }
        ovsrec_ospf_router_set_distance(ospf_router_row,key_distance,value_distance,
            ospf_router_row->n_distance);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE (key_distance);
        SAFE_FREE (value_distance);
    }while (0);
  return CMD_SUCCESS;
}

DEFUN (cli_ospf_distance_ospf,
       cli_ospf_distance_ospf_cmd,
       "distance ospf "
         "{intra-area <1-255>|inter-area <1-255>|external <1-255>}",
       "Define an administrative distance\n"
       "OSPF Administrative distance\n"
       "Intra-area routes\n"
       "Distance for intra-area routes\n"
       "Inter-area routes\n"
       "Distance for inter-area routes\n"
       "External routes\n"
       "Distance for external routes\n")
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** key_distance = NULL;
    int64_t* value_distance = NULL;
    unsigned char distance_intra = 0,distance_inter = 0,distance_external = 0;
    int instance_id = 1;
    int i = 0;
    bool key_found = false;

  if (argc < 3) /* should not happen */
    return CMD_WARNING;

  /* As just distance ospf will also be allowed*/
  if (!argv[0] && !argv[1] && !argv[2])
    {
      vty_out(vty, "%% Command incomplete. (Arguments required)%s",
              VTY_NEWLINE);
      return CMD_WARNING;
    }
  if (argv[0])
    distance_intra = atoi (argv[0]);
  if (argv[1])
    distance_inter = atoi (argv[1]);
  if (argv[2])
    distance_external = atoi (argv[2]);

    do {
        vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
        if (vrf_row == NULL)
        {
            VLOG_DBG ("VRF is not present.");
            break;
        }
        /* See if it already exists. */
        ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                                                  instance_id);
        if (ospf_router_row == NULL)
        {
            VLOG_DBG ("OSPF Router instance not present");
            break;
        }
        /* OPS_TODO : Handle if the keys are not populated in "router ospf" command*/
        key_distance = xmalloc(sizeof *ospf_router_row->key_distance *
                                    ospf_router_row->n_distance);
        value_distance = xmalloc(sizeof *ospf_router_row->value_distance *
                                     ospf_router_row->n_distance);
        if (!key_distance || !value_distance)
        {
            SAFE_FREE(key_distance);
            SAFE_FREE(value_distance);
            VLOG_DBG ("Memory allocation failed");
            break;
        }
        OSPF_START_DB_TXN(ospf_router_txn);
        for (i = 0 ; i < ospf_router_row->n_distance;i++)
        {
            key_distance[i] = ospf_router_row->key_distance[i];
            if(argv[0] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_INTRA_AREA))
                value_distance[i] = (int64_t)distance_intra;
            else if (argv[1] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_INTER_AREA))
                value_distance[i] = (int64_t)distance_inter;
            else if (argv[2] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_EXTERNAL))
                value_distance[i] = (int64_t)distance_external;
            else
                value_distance[i] = ospf_router_row->value_distance[i];
        }
        ovsrec_ospf_router_set_distance(ospf_router_row,key_distance,value_distance,
            ospf_router_row->n_distance);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE (key_distance);
        SAFE_FREE (value_distance);
    }while (0);

  return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_distance_ospf,
       cli_no_ospf_distance_ospf_cmd,
       "no distance ospf {intra-area|inter-area|external}",
       NO_STR
       "Define an administrative distance\n"
       "OSPF Administrative distance\n"
       "OSPF Distance\n"
       "Intra-area routes\n"
       "Inter-area routes\n"
       "External routes\n")
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** key_distance = NULL;
    int64_t* value_distance = NULL;
    unsigned char distance = 0;
    int instance_id = 1;
    int i = 0;
    bool key_found = false;

  if (argc < 3) /* should not happen */
    return CMD_WARNING;

  /* As just distance ospf will also be allowed*/
  if (!argv[0] && !argv[1] && !argv[2])
    {
      vty_out(vty, "%% Command incomplete. (Arguments required)%s",
              VTY_NEWLINE);
      return CMD_WARNING;
    }

    do {
        vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
        if (vrf_row == NULL)
        {
            VLOG_DBG ("VRF is not present.");
            break;
        }
        /* See if it already exists. */
        ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                                                  instance_id);
        if (ospf_router_row == NULL)
        {
            VLOG_DBG ("OSPF Router instance not present");
            break;
        }
        /* OPS_TODO : Handle if the keys are not populated in "router ospf" command*/
        key_distance = xmalloc(sizeof *ospf_router_row->key_distance *
                                    ospf_router_row->n_distance);
        value_distance = xmalloc(sizeof *ospf_router_row->value_distance *
                                     ospf_router_row->n_distance);
        if (!key_distance || !value_distance)
        {
            SAFE_FREE(key_distance);
            SAFE_FREE(value_distance);
            VLOG_DBG ("Memory allocation failed");
            break;
        }
        OSPF_START_DB_TXN(ospf_router_txn);
        distance = OSPF_ROUTER_DISTANCE_DEFAULT;
        for (i = 0 ; i < ospf_router_row->n_distance;i++)
        {
            key_distance[i] = ospf_router_row->key_distance[i];
            if(argv[0] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_INTRA_AREA))
                value_distance[i] = (int64_t)distance;
            else if (argv[1] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_INTER_AREA))
                value_distance[i] = (int64_t)distance;
            else if (argv[2] &&
                !strcmp(ospf_router_row->key_distance[i],OVSREC_OSPF_ROUTER_DISTANCE_EXTERNAL))
                value_distance[i] = (int64_t)distance;
            else
                value_distance[i] = ospf_router_row->value_distance[i];
        }
        ovsrec_ospf_router_set_distance(ospf_router_row,key_distance,value_distance,
            ospf_router_row->n_distance);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE (key_distance);
        SAFE_FREE (value_distance);
    }while (0);

  return CMD_SUCCESS;
}

DEFUN (cli_ospf_redistribute_source,
       cli_ospf_redistribute_source_cmd,
       "redistribute (connected|static|bgp)",
       REDIST_STR
       CLI_REDIST_HELP_STR_OSPFD)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** redistribute = NULL;
    int instance_id = OSPF_DEFAULT_INSTANCE_ID;
    int redist_found = false;
    int i = 0;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
       VLOG_DBG ("VRF is not present.");
       return CMD_SUCCESS;
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                               instance_id);
    if (ospf_router_row == NULL)
    {
       VLOG_DBG ("OSPF Router instance not present");
       return CMD_SUCCESS;
    }
    for (i = 0 ; i < ospf_router_row->n_redistribute ; i++)
        if (ospf_router_row->redistribute[i] &&
            !strcmp(argv[0],ospf_router_row->redistribute[i]))
            redist_found = true;
    if (redist_found)
    {
        VLOG_DBG ("%s Already redistribute to OSPF %d",argv[0],instance_id);
        return CMD_SUCCESS;
    }
    else
    {
        redistribute = xmalloc(sizeof *ospf_router_row->redistribute *
                                    ospf_router_row->n_redistribute+1);
        if (!redistribute)
            return CMD_SUCCESS;
        OSPF_START_DB_TXN(ospf_router_txn);
        for (i = 0 ; i < ospf_router_row->n_redistribute ; i++)
            redistribute[i] = ospf_router_row->redistribute[i];
        redistribute[ospf_router_row->n_redistribute] = argv[0];
        ovsrec_ospf_router_set_redistribute(ospf_router_row,redistribute,ospf_router_row->n_redistribute+1);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE(redistribute);
    }

  return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_redistribute_source,
       cli_no_ospf_redistribute_source_cmd,
       "no redistribute (connected|static|bgp)",
       NO_STR
       REDIST_STR
       CLI_REDIST_HELP_STR_OSPFD)
{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    char** redistribute = NULL;
    int instance_id = OSPF_DEFAULT_INSTANCE_ID;
    int redist_found = false;
    int i = 0, j = 0;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
       VLOG_DBG ("VRF is not present.");
       return CMD_SUCCESS;
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                               instance_id);
    if (ospf_router_row == NULL)
    {
       VLOG_DBG ("OSPF Router instance not present");
       return CMD_SUCCESS;
    }
    for (i = 0 ; i < ospf_router_row->n_redistribute ; i++)
        if (ospf_router_row->redistribute[i] &&
            !strcmp(argv[0],ospf_router_row->redistribute[i]))
            redist_found = true;
    if (redist_found)
    {
        redistribute = xmalloc(sizeof *ospf_router_row->redistribute *
                                    ospf_router_row->n_redistribute-1);
        if (!redistribute)
            return CMD_SUCCESS;
        OSPF_START_DB_TXN(ospf_router_txn);
        for (i = 0,j = 0 ; i < ospf_router_row->n_redistribute ; i++)
            if (ospf_router_row->redistribute[i] &&
            strcmp(argv[0],ospf_router_row->redistribute[i]))
            redistribute[j++] = ospf_router_row->redistribute[i];

        ovsrec_ospf_router_set_redistribute(ospf_router_row,redistribute,ospf_router_row->n_redistribute-1);
        OSPF_END_DB_TXN(ospf_router_txn);
        SAFE_FREE(redistribute);
    }
    else
    {
        VLOG_DBG ("%s Not redistributed to OSPF %d",argv[0],instance_id);
        return CMD_SUCCESS;
    }

  return CMD_SUCCESS;
}

/* Default information originate. */
DEFUN (cli_ospf_default_information_originate,
       cli_ospf_default_information_originate_cmd,
       "default-information originate "
       "{always}",
       DEFAULT_REDIST_STR
       DEFAULT_REDIST_ORIGINATE_STR
       DEFAULT_REDIST_ALWAYS_STR)

{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap def_redist_smap;
    char** redistribute = NULL;
    int instance_id = OSPF_DEFAULT_INSTANCE_ID;
    bool redist_default = false;
    bool redist_default_always = false;
    int i = 0, j = 0;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
       VLOG_DBG ("VRF is not present.");
       return CMD_SUCCESS;
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                               instance_id);
    if (ospf_router_row == NULL)
    {
       VLOG_DBG ("OSPF Router instance not present");
       return CMD_SUCCESS;
    }
    smap_clone(&def_redist_smap,&(ospf_router_row->default_information));
    redist_default = smap_get_bool(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE,false);
    if (!redist_default)
        smap_replace(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE,
                                        OSPF_DEFAULT_INFO_ORIGINATE_SET);
    if (NULL != argv[0])
    {
        redist_default_always = smap_get_bool(&def_redist_smap,
                                    OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS,false);
        if (!redist_default_always)
            smap_replace(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS,
                                        OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS_SET);
    }
    else
        smap_replace(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS,
                                        OSPF_ALWAYS_DEFAULT);

    OSPF_START_DB_TXN(ospf_router_txn);
    ovsrec_ospf_router_set_default_information(ospf_router_row,&def_redist_smap);
    OSPF_END_DB_TXN(ospf_router_txn);
    smap_destroy(&def_redist_smap);

    return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_default_information_originate,
       cli_no_ospf_default_information_originate_cmd,
       "no default-information originate ",
       DEFAULT_REDIST_STR
       DEFAULT_REDIST_ORIGINATE_STR)

{
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *ospf_router_txn=NULL;
    struct smap def_redist_smap;
    int instance_id = OSPF_DEFAULT_INSTANCE_ID;
    int i = 0, j = 0;

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
       VLOG_DBG ("VRF is not present.");
       return CMD_SUCCESS;
    }
    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id (vrf_row,
                                               instance_id);
    if (ospf_router_row == NULL)
    {
       VLOG_DBG ("OSPF Router instance not present");
       return CMD_SUCCESS;
    }
    smap_clone(&def_redist_smap,&(ospf_router_row->default_information));
    smap_replace(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE,OSPF_DEFAULT_INFO_ORIG_DEFAULT);
    smap_replace(&def_redist_smap,OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS,OSPF_DEFAULT_INFO_ORIG_DEFAULT);

    OSPF_START_DB_TXN(ospf_router_txn);
    ovsrec_ospf_router_set_default_information(ospf_router_row,&def_redist_smap);
    OSPF_END_DB_TXN(ospf_router_txn);
    smap_destroy(&def_redist_smap);

    return CMD_SUCCESS;
}

static int
ospf_vlink_set_other_config(const struct ovsrec_ospf_vlink *vlink_row,
                                int argc, const char **argv, int found)
{
    struct smap smap;

	const char temp[10];

	if (found == 1)
    {
        smap_clone(&smap, &(vlink_row->other_config));
        switch(argv[2][0])
        {
            case 'h':
            /* Hello interval */
            if (argc == 3)
            {
                snprintf(temp, 10, "%d", OSPF_HELLO_INTERVAL_DEFAULT);
                smap_replace(&smap, OSPF_KEY_HELLO_INTERVAL, temp);
            }
            else
                smap_replace(&smap, OSPF_KEY_HELLO_INTERVAL, argv[3]);
            break;

            case 'r':
            /* Retransmit Interval */
            if (argc == 3)
            {
                snprintf(temp, 10, "%d", OSPF_RETRANSMIT_INTERVAL_DEFAULT);
                smap_replace(&smap, OSPF_KEY_RETRANSMIT_INTERVAL, temp);
            }
            else
                smap_replace(&smap, OSPF_KEY_RETRANSMIT_INTERVAL, argv[3]);
            break;

            case 't':
            /* Transmit Delay */
            if (argc == 3)
            {
                snprintf(temp, 10, "%d", OSPF_TRANSMIT_DELAY_DEFAULT);
                smap_replace(&smap, OSPF_KEY_TRANSMIT_DELAY, temp);
            }
            else
                smap_replace(&smap, OSPF_KEY_TRANSMIT_DELAY, argv[3]);
            break;

            case 'd':
            /* Dead Interval */
            if (argc == 3)
            {
                snprintf(temp, 10, "%d", OSPF_DEAD_INTERVAL_DEFAULT);
                smap_replace(&smap, OSPF_KEY_DEAD_INTERVAL, temp);
            }
            else
            {
                int hello_interval = 0, interval = 0;
                hello_interval = smap_get_int(&smap, OSPF_KEY_HELLO_INTERVAL,
                                    OSPF_HELLO_INTERVAL_DEFAULT);

                interval = atoi(argv[3]);
                if (hello_interval > interval)
                {
                    vty_out (vty, "Dead interval cannot be less than hello interval.");
                    smap_destroy(&smap);
                    return 1;
                }
                else if((interval % hello_interval) != 0)
                {
                    vty_out (vty, "Dead interval should be multiple(s) of hello interval.");
                    smap_destroy(&smap);
                    return 1;
                }
                smap_replace(&smap, OSPF_KEY_DEAD_INTERVAL, argv[3]);
            }
            break;
        }
    }
	else if (found == 0)
    {
        smap_init(&smap);

        if (argc > 2 && argv[2][0] == 'h')
            smap_add(&smap, OSPF_KEY_HELLO_INTERVAL, argv[3]);        
		else
        {
            snprintf(temp, 10, "%d", OSPF_HELLO_INTERVAL_DEFAULT);
            smap_add(&smap, OSPF_KEY_HELLO_INTERVAL, temp);
        }

        if (argc > 2 && argv[2][0] == 'd')
        {
            int hello_interval = 0, interval = 0;
            hello_interval = smap_get_int(&smap, OSPF_KEY_HELLO_INTERVAL,
                                 OSPF_HELLO_INTERVAL_DEFAULT);

            interval = atoi(argv[3]);
            if (hello_interval > interval)
            {
                vty_out (vty, "Dead interval cannot be less than hello interval.");
                smap_destroy(&smap);
                return 1;
            }
            else if((interval % hello_interval) != 0)
            {
                vty_out (vty, "Dead interval should be multiple(s) of hello interval.");
                smap_destroy(&smap);
                return 1;
            }
            smap_add(&smap, OSPF_KEY_DEAD_INTERVAL, argv[3]);
        }
        else
        {
            snprintf(temp, 10, "%d", OSPF_DEAD_INTERVAL_DEFAULT);
            smap_add(&smap, OSPF_KEY_DEAD_INTERVAL, temp);
        }

        if (argc > 2 && argv[2][0] == 't')
            smap_add(&smap, OSPF_KEY_TRANSMIT_DELAY, argv[3]);	
        else
        {
            snprintf(temp, 10, "%d", OSPF_TRANSMIT_DELAY_DEFAULT);
			smap_add(&smap, OSPF_KEY_TRANSMIT_DELAY, temp);
        }

        if (argc > 2 && argv[2][0] == 'r')
            smap_add(&smap, OSPF_KEY_RETRANSMIT_INTERVAL, temp);
        else
        {
            snprintf(temp, 10, "%d", OSPF_RETRANSMIT_INTERVAL_DEFAULT);
            smap_add(&smap, OSPF_KEY_RETRANSMIT_INTERVAL, temp);
        }
    }
    else
    {
        vty_out(vty, "Could not update vlink other configuration.%s", VTY_NEWLINE);
        return 1;
    }
    ovsrec_ospf_vlink_set_other_config(vlink_row, &smap);
    smap_destroy(&smap);
	return 0;
}

void
ospf_vlink_set_authentication(const struct ovsrec_ospf_vlink *vlink_row,
                                int argc, const char **argv, bool no_flag)
{
    char* auth_type = NULL;
    if (no_flag)
    {
        ovsrec_ospf_vlink_set_ospf_auth_type(vlink_row,NULL);
        return;
    }

    if (argc == 2) /* Simple authentication */
    {
        auth_type = OVSREC_OSPF_VLINK_OSPF_AUTH_TYPE_TEXT;
        ovsrec_ospf_vlink_set_ospf_auth_type(vlink_row,auth_type);
        return;
    }
    if (argc >2 &&
        !strcmp(argv[2],"message-digest")) /* message-digest */
    {
        auth_type = OVSREC_OSPF_VLINK_OSPF_AUTH_TYPE_MD5;
        ovsrec_ospf_vlink_set_ospf_auth_type(vlink_row,auth_type);
        return;
    }
    else if (argc >2 &&
        !strcmp(argv[2],"null")) /* NULL */
    {
        auth_type = OVSREC_OSPF_VLINK_OSPF_AUTH_TYPE_NULL;
        ovsrec_ospf_vlink_set_ospf_auth_type(vlink_row,auth_type);
        return;
    }
}

void
ospf_vlink_set_authentication_key(const struct ovsrec_ospf_vlink *vlink_row,
                                         int argc, const char **argv)
{
    char* auth_text = NULL;
    int64_t key_id = 0;
    int result;
    int i = 0;

    if (3 == argc) /* Simple auth key */
    {
        auth_text = (argv[2])?argv[2]:NULL;
        ovsrec_ospf_vlink_set_ospf_auth_text_key(vlink_row,auth_text);
        return;
    }

    if (argc > 3) /* message-digest-key */
    {
        key_id = atoi(argv[2]);
        auth_text = (argv[3])?argv[3]:NULL;
        result = ospf_md5_key_update_to_vl(vlink_row, key_id, auth_text);
        if ( result == CMD_ERR_AMBIGUOUS)
        {
            vty_out(vty,"MD5 key is already present%s",VTY_NEWLINE);
        }
        else if ( result != CMD_SUCCESS)
        {
            vty_out(vty,"MD5 key updation failed.%",VTY_NEWLINE);
        }
    }
}

void
ospf_vlink_unset_authentication_key (const struct ovsrec_ospf_vlink *vlink_row,
                                                     int argc, const char **argv)
{
    char* auth_text = NULL;
    int64_t key_id = 0;
    int result;
    int i = 0;

    if (2 == argc) /* Simple auth key */
    {
        ovsrec_ospf_vlink_set_ospf_auth_text_key(vlink_row,NULL);
        return;
    }

    if (argc > 2) /* message-digest-key */
    {
        key_id = atoi(argv[2]);
        result = ospf_md5_key_remove_from_vl(vlink_row, key_id);
        if ( result == CMD_ERR_NO_MATCH)
        {
            vty_out(vty,"MD5 key not present.",VTY_NEWLINE);
        }
        else if ( result != CMD_SUCCESS)
        {
            vty_out(vty,"MD5 key deletion failed.",VTY_NEWLINE);
        }
    }
}

DEFUN (cli_ospf_area_vlink,
       cli_ospf_area_vlink_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0;

    char name[10];
    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);

    /* Insert the new row and link the area row to the router table. */
    if (area_row == NULL)
    {
        area_row = ovsrec_ospf_area_insert(ospf_router_txn);
        if (ospf_area_insert_to_router(ospf_router_row, area_row,
                                       area_id.s_addr) != CMD_SUCCESS)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Could not update configuration.");
        }
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
        {
            found = 1;

            if (argc > 2)
            {
                if (ospf_vlink_set_other_config(vlink_row, argc, argv, found))
                {
                    SAFE_FREE(ospf_vlink);
                    SAFE_FREE(key_ospf_vlink);
                    OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "");
                }
                else
                    OSPF_END_DB_TXN(ospf_router_txn);
            }
            else
            {
                vty_out(vty, "Area ID and Peer Router ID is already configured\n");
                break;
            }
        }
    }

    /* Insert the new row and link the vlink row to the area table. */
    if (found == 0)
    {
        ospf_vlink = xmalloc (sizeof * area_row->value_ospf_vlinks *
                                       (area_row->n_ospf_vlinks + 1));
        key_ospf_vlink = xmalloc (sizeof * area_row->key_ospf_vlinks *
                                           (area_row->n_ospf_vlinks + 1));
        if (!ospf_vlink || !key_ospf_vlink)
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Memory allocation failed");
            return CMD_WARNING;
        }

        for (i = 0 ; i < area_row->n_ospf_vlinks ; i++)
        {
             ospf_vlink[i] = area_row->value_ospf_vlinks[i];
             key_ospf_vlink[i] = area_row->key_ospf_vlinks[i];
        }

        vlink_row = ovsrec_ospf_vlink_insert(ospf_router_txn);
        for (i = 0; i < 256; i++)
        {
            if (!(ospf_vlink_count[i]))
                break;
        }

        if (i < 256)
        {
            snprintf(name, 10, "VLINK%d", i);
        }
        else
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Maximum vlink count reached.");
            return CMD_WARNING;
        }

        ovsrec_ospf_vlink_set_name(vlink_row, name);
        ovsrec_ospf_vlink_set_area_id(vlink_row, area_id.s_addr);
        ovsrec_ospf_vlink_set_peer_router_id(vlink_row, peer_router_id.s_addr);

        if (ospf_vlink_set_other_config(vlink_row, argc, argv, found))
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "");
        }

        ospf_vlink[area_row->n_ospf_vlinks] = vlink_row;
        key_ospf_vlink[area_row->n_ospf_vlinks] = vlink_row->peer_router_id;
        ovsrec_ospf_area_set_ospf_vlinks(area_row, key_ospf_vlink, ospf_vlink,
                                         area_row->n_ospf_vlinks + 1);
    }

    /* End of transaction. */
    OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED)) {
        ospf_vlink_count[i] = 0;
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[i] = 1;

    return CMD_SUCCESS;
}

DEFUN (cli_ospf_area_vlink_auth,
       cli_ospf_area_vlink_auth_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D authentication ",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Enable authentication on this virtual link\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0;

    char name[10];
    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);

    /* Insert the new row and link the area row to the router table. */
    if (area_row == NULL)
    {
        area_row = ovsrec_ospf_area_insert(ospf_router_txn);
        if (ospf_area_insert_to_router(ospf_router_row, area_row,
                                       area_id.s_addr) != CMD_SUCCESS)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Could not update configuration.");
        }
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
            found = 1;
            break;
    }

    /* Insert the new row and link the vlink row to the area table. */
    if (found == 0)
    {
        ospf_vlink = xmalloc (sizeof * area_row->value_ospf_vlinks *
                                       (area_row->n_ospf_vlinks + 1));
        key_ospf_vlink = xmalloc (sizeof * area_row->key_ospf_vlinks *
                                           (area_row->n_ospf_vlinks + 1));
        if (!ospf_vlink || !key_ospf_vlink)
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Memory allocation failed");
            return CMD_WARNING;
        }

        for (i = 0 ; i < area_row->n_ospf_vlinks ; i++)
        {
             ospf_vlink[i] = area_row->value_ospf_vlinks[i];
             key_ospf_vlink[i] = area_row->key_ospf_vlinks[i];
        }

        vlink_row = ovsrec_ospf_vlink_insert(ospf_router_txn);
        for (i = 0; i < 256; i++)
        {
            if (!(ospf_vlink_count[i]))
                break;
        }

        if (i < 256)
        {
            snprintf(name, 10, "VLINK%d", i);
        }
        else
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Maximum vlink count reached.");
            return CMD_WARNING;
        }

        ovsrec_ospf_vlink_set_area_id(vlink_row, area_id.s_addr);
        ovsrec_ospf_vlink_set_name(vlink_row, name);
        ovsrec_ospf_vlink_set_peer_router_id(vlink_row, peer_router_id.s_addr);

        ospf_vlink_set_authentication (vlink_row, argc, argv,false);

        ospf_vlink[area_row->n_ospf_vlinks] = vlink_row;
        key_ospf_vlink[area_row->n_ospf_vlinks] = vlink_row->peer_router_id;
        ovsrec_ospf_area_set_ospf_vlinks(area_row, key_ospf_vlink, ospf_vlink,
                                         area_row->n_ospf_vlinks + 1);
    }
    else
        ospf_vlink_set_authentication (vlink_row, argc, argv,false);

    /* End of transaction. */
    OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED)) {
        ospf_vlink_count[i] = 0;
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[i] = 1;

    return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_area_vlink,
       cli_no_ospf_area_vlink_cmd,
       "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0, n = 0, j = 0;

    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);
    if (area_row == NULL)
    {
		OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Area configuration is not present.");
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
        {
            found = 1;
            if (argc > 2)
            {
                if (ospf_vlink_set_other_config(vlink_row, argc, argv, found))
                    OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "");
                else
                    OSPF_END_DB_TXN(ospf_router_txn);
            }
            break;
        }
    }

    if (found == 1)
    {
        ospf_vlink = xmalloc (sizeof * area_row->value_ospf_vlinks *
                                          (area_row->n_ospf_vlinks - 1));
        key_ospf_vlink = xmalloc (sizeof * area_row->key_ospf_vlinks *
                                             (area_row->n_ospf_vlinks - 1));
        if (!ospf_vlink || !key_ospf_vlink)
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Memory allocation failed");
            return CMD_WARNING;
        }

        sscanf(vlink_row->name, "VLINK%d", &n);
        for (i = j = 0 ; i < area_row->n_ospf_vlinks && j < (area_row->n_ospf_vlinks - 1); i++)
        {
            if (vlink_row->peer_router_id != (area_row->value_ospf_vlinks[i]->peer_router_id))
            {
                ospf_vlink[j] = area_row->value_ospf_vlinks[i];
                key_ospf_vlink[j++] = area_row->key_ospf_vlinks[i];
            }
        }

        ovsrec_ospf_area_set_ospf_vlinks(area_row, key_ospf_vlink, ospf_vlink,
                                         area_row->n_ospf_vlinks - 1);
    }
    else
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Virtual link configuration is not present.");
    }

    /* End of transaction. */
       OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED))
    {
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[n] = 0;

    return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_area_vlink_auth,
       cli_no_ospf_area_vlink_auth_cmd,
       "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D authentication",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Enable authentication on this virtual link\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0, n = 0;

    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);
    if (area_row == NULL)
    {
		OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Area configuration is not present.");
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
        {
            found = 1;
            break;
        }
    }

    if (found == 1)
    {
        ospf_vlink_set_authentication (vlink_row, argc, argv,true);
    }
    else
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Virtual link configuration is not present.");
    }

    /* End of transaction. */
    OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED))
    {
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[n] = 0;

    return CMD_SUCCESS;
}

DEFUN (cli_ospf_area_vlink_auth_key,
       cli_ospf_area_vlink_auth_key_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D authentication-key "
       "AUTH_KEY",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Authentication password (key)\n"
       "The OSPF password (key)\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0;

    char name[10];
    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);

    /* Insert the new row and link the area row to the router table. */
    if (area_row == NULL)
    {
        area_row = ovsrec_ospf_area_insert(ospf_router_txn);
        if (ospf_area_insert_to_router(ospf_router_row, area_row,
                                       area_id.s_addr) != CMD_SUCCESS)
        {
            OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Could not update configuration.");
        }
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
            found = 1;
            break;
    }

    /* Insert the new row and link the vlink row to the area table. */
    if (found == 0)
    {
        ospf_vlink = xmalloc (sizeof * area_row->value_ospf_vlinks *
                                       (area_row->n_ospf_vlinks + 1));
        key_ospf_vlink = xmalloc (sizeof * area_row->key_ospf_vlinks *
                                           (area_row->n_ospf_vlinks + 1));
        if (!ospf_vlink || !key_ospf_vlink)
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Memory allocation failed");
            return CMD_WARNING;
        }

        for (i = 0 ; i < area_row->n_ospf_vlinks ; i++)
        {
             ospf_vlink[i] = area_row->value_ospf_vlinks[i];
             key_ospf_vlink[i] = area_row->key_ospf_vlinks[i];
        }

        vlink_row = ovsrec_ospf_vlink_insert(ospf_router_txn);
        for (i = 0; i < 256; i++)
        {
            if (!(ospf_vlink_count[i]))
                break;
        }

        if (i < 256)
        {
            snprintf(name, 10, "VLINK%d", i);
        }
        else
        {
            SAFE_FREE(ospf_vlink);
            SAFE_FREE(key_ospf_vlink);
            VLOG_DBG ("Maximum vlink count reached.");
            return CMD_WARNING;
        }

        ovsrec_ospf_vlink_set_area_id(vlink_row, area_id.s_addr);
        ovsrec_ospf_vlink_set_name(vlink_row, name);
        ovsrec_ospf_vlink_set_peer_router_id(vlink_row, peer_router_id.s_addr);

        ospf_vlink_set_authentication_key (vlink_row, argc, argv);

        ospf_vlink[area_row->n_ospf_vlinks] = vlink_row;
        key_ospf_vlink[area_row->n_ospf_vlinks] = vlink_row->peer_router_id;
        ovsrec_ospf_area_set_ospf_vlinks(area_row, key_ospf_vlink, ospf_vlink,
                                         area_row->n_ospf_vlinks + 1);
    }
    else
        ospf_vlink_set_authentication_key (vlink_row, argc, argv);

    /* End of transaction. */
    OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED)) {
        ospf_vlink_count[i] = 0;
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[i] = 1;

    return CMD_SUCCESS;
}

DEFUN (cli_no_ospf_area_vlink_auth_key,
       cli_no_ospf_area_vlink_auth_key_cmd,
       "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
       "authentication-key",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Authentication password (key)\n")
{
    struct smap smap;
    struct in_addr area_id;
    struct in_addr peer_router_id;

    struct ovsdb_idl_txn *ospf_router_txn = NULL;
    struct ovsrec_ospf_vlink** ospf_vlink = NULL;
    const struct ovsrec_ospf_vlink *vlink_row = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    const struct ovsrec_ospf_area *area_row = NULL;

    int64_t* key_ospf_vlink = NULL;
    int instance_id = 1;
    int i = 0, n = 0;

    bool found = 0;

    memset (&peer_router_id, 0, sizeof (struct in_addr));
    memset (&area_id, 0, sizeof (struct in_addr));

    OSPF_GET_AREA_ID(area_id, argv[0]);
    OSPF_GET_AREA_ID(peer_router_id, argv[1]);

    /* Start of transaction. */
    OSPF_START_DB_TXN(ospf_router_txn);

    vrf_row = ospf_get_vrf_by_name(DEFAULT_VRF_NAME);
    if (vrf_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "VRF is not present.");
    }

    /* See if it already exists. */
    ospf_router_row = ospf_router_lookup_by_instance_id(vrf_row, instance_id);
    if (ospf_router_row == NULL)
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "OSPF router is not present.");
    }

    area_row = ospf_area_lookup_by_area_id(ospf_router_row, area_id.s_addr);
    if (area_row == NULL)
    {
		OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Area configuration is not present.");
    }

    OVSREC_OSPF_VLINK_FOR_EACH(vlink_row, idl)
    {
        if (vlink_row->area_id == area_id.s_addr &&
            vlink_row->peer_router_id == peer_router_id.s_addr)
        {
            found = 1;
            break;
        }
    }

    if (found == 1)
    {
        ospf_vlink_unset_authentication_key (vlink_row, argc, argv);
    }
    else
    {
        OSPF_ERRONEOUS_DB_TXN(ospf_router_txn, "Virtual link configuration is not present.");
    }

    /* End of transaction. */
    OSPF_END_DB_TXN_STATUS(ospf_router_txn);
    if ((status != TXN_SUCCESS) && (status != TXN_UNCHANGED))
    {
        return CMD_WARNING;
    }
    else
        ospf_vlink_count[n] = 0;

    return CMD_SUCCESS;
}

ALIAS (cli_ospf_area_vlink_auth,
       cli_ospf_area_vlink_auth_param1_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D authentication "
       "(message-digest|null)",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Enable authentication on this virtual link\n"
       "Use message-digest authentication\n"
       "Use null authentication\n")

ALIAS (cli_ospf_area_vlink_auth_key,
       cli_ospf_area_vlink_auth_key_md5_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
       "message-digest-key <1-255> md5 MD5_KEY",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Message digest authentication password (key)\n"
       "Key ID\n"
       "Use MD5 algorithm\n"
       "The OSPF password (key)")

ALIAS (cli_no_ospf_area_vlink_auth_key,
       cli_no_ospf_area_vlink_auth_key_md5_cmd,
       "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
       "message-digest-key <1-255>",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Message digest authentication password (key)\n"
       "Key ID\n")

ALIAS (cli_ospf_area_vlink,
       cli_ospf_area_vlink_param1_cmd,
       "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
       "(hello-interval|retransmit-interval|transmit-delay|dead-interval) <1-65535>",
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Time between HELLO packets\n"
       "Time between retransmitting lost link state advertisements\n"
       "Link state transmit delay\n"
       "Interval after which a neighbor is declared dead\n"
       "Seconds\n")

ALIAS (cli_no_ospf_area_vlink,
       cli_no_ospf_area_vlink_param1_cmd,
       "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D "
       "(hello-interval|retransmit-interval|transmit-delay|dead-interval)",
       NO_STR
       OSPF_AREA_STR
       OSPF_AREA_IP_STR
       OSPF_AREA_RANGE
       "Configure a virtual link\n"
       "Router ID of the remote ABR\n"
       "Time between HELLO packets\n"
       "Time between retransmitting lost link state advertisements\n"
       "Link state transmit delay\n"
       "Interval after which a neighbor is declared dead\n")

void
ospf_vty_init(void)
{

    /* "router ospf" commands. */
    install_element(CONFIG_NODE, &cli_ospf_router_cmd);
    install_element(CONFIG_NODE, &cli_no_ospf_router_cmd);

    /* "ospf router-id" commands. */
    install_element(OSPF_NODE, &cli_ospf_router_id_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_router_id_cmd);

    /* network area id */
    install_element(OSPF_NODE, &cli_ospf_router_network_area_id_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_network_area_ip_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_no_network_area_id_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_no_network_area_ip_cmd);
    install_element(OSPF_NODE, &cli_ospf_admin_distance_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_admin_distance_cmd);
    install_element(OSPF_NODE, &cli_ospf_distance_ospf_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_distance_ospf_cmd);
    install_element(OSPF_NODE, &cli_ospf_redistribute_source_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_redistribute_source_cmd);
    install_element(OSPF_NODE, &cli_ospf_default_information_originate_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_default_information_originate_cmd);

    /* max-metric command */
    install_element(OSPF_NODE, &cli_ospf_router_max_metric_cmd);
    install_element(OSPF_NODE, &cli_ospf_router_no_max_metric_cmd);

    /* Area Authentication */
    install_element(OSPF_NODE, &cli_ospf_area_auth_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_auth_message_digest_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_area_authentication_cmd);

    /* Area Type */
    install_element(OSPF_NODE, &cli_ospf_area_nssa_translate_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_no_nssa_no_summary_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_nssa_translate_no_summary_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_no_nssa_translate_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_nssa_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_nssa_no_summary_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_stub_cmd);
    install_element(OSPF_NODE, &cli_ospf_area_stub_no_summary_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_area_stub_cmd);
    install_element(OSPF_NODE, &cli_no_ospf_area_stub_no_summary_cmd);

    /* "area virtual-link" commands. */
    install_element (OSPF_NODE, &cli_ospf_area_vlink_cmd);
    install_element (OSPF_NODE, &cli_ospf_area_vlink_param1_cmd);
    install_element (OSPF_NODE, &cli_no_ospf_area_vlink_cmd);
    install_element (OSPF_NODE, &cli_no_ospf_area_vlink_param1_cmd);
    install_element (OSPF_NODE, &cli_ospf_area_vlink_auth_cmd);
    install_element (OSPF_NODE, &cli_ospf_area_vlink_auth_param1_cmd);
    install_element (OSPF_NODE, &cli_no_ospf_area_vlink_auth_cmd);
    install_element (OSPF_NODE, &cli_ospf_area_vlink_auth_key_cmd);
    install_element (OSPF_NODE, &cli_no_ospf_area_vlink_auth_key_cmd);
    install_element (OSPF_NODE, &cli_ospf_area_vlink_auth_key_md5_cmd);
    install_element (OSPF_NODE, &cli_no_ospf_area_vlink_auth_key_md5_cmd);

    /* OSPF Intervals */
    install_element(INTERFACE_NODE, &cli_ospf_router_hello_interval_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_router_no_hello_interval_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_router_dead_interval_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_router_no_dead_interval_cmd);

    install_element(INTERFACE_NODE, &cli_ip_ospf_retransmit_interval_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_retransmit_interval_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_transmit_delay_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_transmit_delay_cmd);

    install_element(INTERFACE_NODE, &cli_ip_ospf_priority_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_priority_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_mtu_ignore_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_mtu_ignore_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_cost_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_cost_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_network_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_no_network_cmd);

    /* Interface authentication */
    install_element(INTERFACE_NODE, &cli_ospf_interface_auth_message_digest_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_interface_auth_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_interface_auth_null_cmd);
    install_element(INTERFACE_NODE, &cli_no_ospf_interface_auth_cmd);
    install_element(INTERFACE_NODE, &cli_ospf_interface_auth_key_cmd);
    install_element(INTERFACE_NODE, &cli_no_ospf_interface_auth_key_cmd);
    install_element(INTERFACE_NODE, &cli_ip_ospf_message_digest_key_cmd);
    install_element(INTERFACE_NODE, &cli_no_ip_ospf_message_digest_key_cmd);

    /* Show commands */
    install_element(ENABLE_NODE, &cli_ip_ospf_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_interface_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_detail_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_ifname_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_nbr_ifname_detail_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_neighbor_nbrid_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_nbr_nbrid_detail_show_cmd);
    install_element(ENABLE_NODE, &cli_ip_ospf_route_show_cmd);

    /* show running-config router ospf */
    install_element(ENABLE_NODE, &cli_ip_ospf_running_config_show_cmd);

    vtysh_init_intf_ospf_context_clients();
}
