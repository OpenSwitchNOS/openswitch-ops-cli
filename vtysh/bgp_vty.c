
/* BGP CLI implementation with Halon vtysh.
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
 * File: bgp_vty.c
 *
 * Purpose: This file contains implementation of all BGP configuration
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
#include "bgp_vty.h"
#include "vswitch-idl.h"
#include "openvswitch/vlog.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "util.h"
#include "prefix.h"
#include "sockunion.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_if.h"

#include <lib/version.h>
#include "getopt.h"
#include "memory.h"
#include "vtysh/vtysh_user.h"
#include "ovsdb-idl.h"
#include "lib/prefix.h"
#include "lib/routemap.h"
#include "lib/plist.h"

/*
** enable this if exra debugging is required
*/
//#define EXTRA_DEBUG

extern struct ovsdb_idl *idl;

static struct rt_map_context {
    char name[80];
    char action[80];
    int pref;
};
struct rt_map_context rmp_context;

#define NET_BUFSZ    18
#define BGP_ATTR_DEFAULT_WEIGHT 32768
/* BGP Information flags taken from bgp_route.h
 * TODO: Remove this duplicate declaration. Need to separate
 * these flags from bgp_route.h
 */
#define BGP_INFO_IGP_CHANGED    (1 << 0)
#define BGP_INFO_DAMPED         (1 << 1)
#define BGP_INFO_HISTORY        (1 << 2)
#define BGP_INFO_SELECTED       (1 << 3)
#define BGP_INFO_VALID          (1 << 4)
#define BGP_INFO_ATTR_CHANGED   (1 << 5)
#define BGP_INFO_DMED_CHECK     (1 << 6)
#define BGP_INFO_DMED_SELECTED  (1 << 7)
#define BGP_INFO_STALE          (1 << 8)
#define BGP_INFO_REMOVED        (1 << 9)
#define BGP_INFO_COUNTED        (1 << 10)
#define BGP_INFO_MULTIPATH      (1 << 11)
#define BGP_INFO_MULTIPATH_CHG  (1 << 12)

#define BGP_SHOW_SCODE_HEADER \
    "Status codes: s suppressed, d damped, " \
    "h history, * valid, > best, = multipath,%s" \
    "              i internal, S Stale, R Removed%s"

#define BGP_SHOW_OCODE_HEADER \
    "Origin codes: i - IGP, e - EGP, ? - incomplete%s%s"

#define BGP_SHOW_HEADER \
    "   Network          Next Hop            Metric LocPrf Weight Path%s"

VLOG_DEFINE_THIS_MODULE(bgp_vty);

/* Structure definition for path attributes column in the
 * OVSDB BGP_Route table. These fields are owned by bgpd and shared
 * with CLI daemon.
 */
typedef struct route_psd_bgp_s {
    int flags;
    const char *aspath;
    const char *origin;
    int local_pref;
    bool internal;
    bool ibgp;
    const char *uptime;
} route_psd_bgp_t;


/********************** simple error handling ***********************/

static void
report_unimplemented_command (struct vty *vty, int argc, const char *argv[])
{
    int i;

    vty_out(vty, "This command is not yet implemented "
                 "but here are the parameters:\n");
    vty_out(vty, "argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
	vty_out(vty, "   arg %d: %s\n", i, argv[i]);
    }
}

/*
** depending on the outcome of the db transaction, returns
** the appropriate value for the cli command execution.
*/

static const char *_undefined = "undefined";
static char itoa_buffer [64];

static char *
safe_print_string (size_t count, char *string)
{
    if ((count > 0) && string)
	return string;
    return _undefined;
}

static char *
safe_print_integer (size_t count, int64_t *iptr)
{
    if ((count > 0) && iptr) {
	sprintf(itoa_buffer, "%"PRId64, *iptr);
	return itoa_buffer;
    }
    return _undefined;
}

static char *
safe_print_bool (size_t count, bool *bvalue)
{
    if ((count > 0) && bvalue) {
	return
	    *bvalue ? "yes" : "no";
    }
    return _undefined;
}

static char *
safe_print_smap_value (const struct smap *smap, char *key)
{
    const char *value = smap_get(smap, key);
    return value ? (char*)value : _undefined;
}

static bool
string_is_an_ip_address (const char *string)
{
    union sockunion su;
    return
	(str2sockunion(string, &su) >= 0);
}
#ifndef ENABLE_OVSDB
static bool
string_is_a_name (const char *string)
{
    union sockunion su;
    return
	(str2sockunion(string, &su) < 0);
}
#endif
/*
** find the vrf with matching name
*/
static const struct ovsrec_vrf *
get_ovsrec_vrf_with_name (char *name)
{
    /* HALON TODO change this later when multi vrf's are supported */
    return ovsrec_vrf_first(idl);
}

/*
** find the bgp router with matching asn
*/
static const struct ovsrec_bgp_router *
get_ovsrec_bgp_router_with_asn (const struct ovsrec_vrf *vrf_row, int asn)
{
    int i = 0;
    for (i = 0; i < vrf_row->n_bgp_routers; i++) {
        if (vrf_row->key_bgp_routers[i] == asn) {
	        return vrf_row->value_bgp_routers[i];
        }
    }
    return NULL;
}

/*
** makes a bgp neighbor database object into a bgp peer/neighbor
*/
static void
define_object_as_a_bgp_peer (const struct ovsrec_bgp_neighbor *bgpn)
{
    bool is_peer_group = false;
    ovsrec_bgp_neighbor_set_is_peer_group(bgpn, &is_peer_group, 1);
}

/*
** makes a bgp neighbor database object into a bgp peer group
*/
static void
define_object_as_a_bgp_peer_group (const struct ovsrec_bgp_neighbor *bgpn)
{
    bool is_peer_group = true;
    ovsrec_bgp_neighbor_set_is_peer_group(bgpn, &is_peer_group, 1);
}

/*
** This function determines if an object represented
** by the bgp_neighbor is actually a peer object or
** a peer group object.  If is_peer_group is not
** specified, then object *IS* a peer/neighbor.
** If specified, its bool value determines the object
** type.
*/
static bool
object_is_bgp_peer_group (const struct ovsrec_bgp_neighbor *bgpn)
{
    if (0 == bgpn->n_is_peer_group) return false;
    return
	*(bgpn->is_peer_group) ? true : false;
}
#define object_is_peer_group	object_is_bgp_peer_group

static bool
object_is_bgp_neighbor (const struct ovsrec_bgp_neighbor *bgpn)
{
    if (0 == bgpn->n_is_peer_group) return true;
    return
	*(bgpn->is_peer_group) ? false : true;
}
#define object_is_neighbor	object_is_bgp_neighbor
#define object_is_bgp_peer	object_is_bgp_neighbor
#define object_is_peer		object_is_bgp_neighbor

/*
** generic bgp neighbor/peer group object find function
** for matching bgp router and matching name.  for a bgp
** neighbor object, name is an ip address and for a peer
** group object, it is just a user defined name.
*/
static const struct ovsrec_bgp_neighbor *
find_matching_neighbor_or_peer_group_object (bool is_peer_group,
    const struct ovsrec_bgp_router *ovs_bgpr,const char *name)
{
    int i = 0;
	/* correct type, now match its parent bgp router and name */
        for (i = 0; i < ovs_bgpr->n_bgp_neighbors; i++) {
            if (ovs_bgpr &&
               (0 == strcmp(ovs_bgpr->key_bgp_neighbors[i], name)))
	        return ovs_bgpr->value_bgp_neighbors[i];
        }
    return NULL;
}

/*
** Find the bgp neighbor with matching bgp_router and ip address.
*/
static const struct ovsrec_bgp_neighbor *
get_bgp_neighbor_with_bgp_router_and_ipaddr (const struct ovsrec_bgp_router *ovs_bgpr,
    const char *ipaddr)
{
    return
	find_matching_neighbor_or_peer_group_object(false, ovs_bgpr, ipaddr);
}

/*
** Find the bgp peer group with matching bgp_router and name
*/
static const struct ovsrec_bgp_neighbor *
get_bgp_peer_group_with_bgp_router_and_name (const struct ovsrec_bgp_router *ovs_bgpr,
    const char *name)
{
    return
	find_matching_neighbor_or_peer_group_object(true, ovs_bgpr, name);
}

/********************************************************************************/

static void
print_route_status(struct vty *vty, route_psd_bgp_t *ppsd)
{
    int64_t flags = ppsd->flags;
  /* Route status display. */
    if (flags & BGP_INFO_REMOVED)
        vty_out (vty, "R");
    else if (flags & BGP_INFO_STALE)
        vty_out (vty, "S");
    /*else if (binfo->extra && binfo->extra->suppress)
      vty_out (vty, "s");*/
    else if (!(flags & BGP_INFO_HISTORY))
        vty_out (vty, "*");
    else
        vty_out (vty, " ");
    /* Selected */
    if (flags & BGP_INFO_HISTORY)
        vty_out (vty, "h");
    else if (flags & BGP_INFO_DAMPED)
        vty_out (vty, "d");
    else if (flags & BGP_INFO_SELECTED)
        vty_out (vty, ">");
    else if (flags & BGP_INFO_MULTIPATH)
        vty_out (vty, "=");
    else
        vty_out (vty, " ");
    /* Internal route. */
    if (ppsd->ibgp)
        vty_out (vty, "i");
    else
        vty_out (vty, " ");
}

static void
bgp_get_rib_path_attributes(const struct ovsrec_bgp_route *rib_row,
                            route_psd_bgp_t *data)
{
    assert(data);
    memset(data, 0, sizeof(*data));

    data->flags = smap_get_int(&rib_row->path_attributes,
                               OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_FLAGS, 0);
    data->aspath = smap_get(&rib_row->path_attributes,
                            OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_AS_PATH);
    data->origin = smap_get(&rib_row->path_attributes,
                            OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_ORIGIN);
    data->local_pref = smap_get_int(&rib_row->path_attributes,
                                    OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_LOC_PREF, 0);
    const char *value;
    value = smap_get(&rib_row->path_attributes,
                     OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_INTERNAL);
    if (!strcmp(value, "true")) {
        data->internal = 1;
    } else {
        data->internal = 0;
    }
    value = smap_get(&rib_row->path_attributes,
                     OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_IBGP);
    if (!strcmp(value, "true")) {
        data->ibgp = 1;
    } else {
        data->ibgp = 0;
    }
    data->uptime = smap_get(&rib_row->path_attributes,
                            OVSDB_BGP_ROUTE_PATH_ATTRIBUTES_UPTIME);
    return;
}

/*
 * This function returns BGP neighbor structure given
 * BGP neighbor IP address.
 */
static const struct ovsrec_bgp_neighbor*
bgp_peer_lookup (const struct ovsrec_bgp_router *bgp_row, const char *peer_id)
{
    int i = 0;
    assert(peer_id);

    for (i = 0; i < bgp_row->n_bgp_neighbors; i++) {
        if (!(bgp_row->value_bgp_neighbors[i]) &&
           (0 == strcmp(bgp_row->key_bgp_neighbors[i], peer_id)))
            return (bgp_row->value_bgp_neighbors[i]);
    }
    return NULL;
}


static const char*
bgp_get_origin_long_str(const char *c)
{
    if (*c == 'i')
        return "IGP";
    else if (*c == 'e')
        return "EGP";
    else
        return "incomplete";
}

/* Function to get neighbor name from BGP Router */
char *
get_bgp_neighbor_name_from_bgp_router (const struct ovsrec_bgp_router *ovs_bgpr, const char *name)
{
    int i = 0;
    /* correct type, now match its parent bgp router and name */
    for (i = 0; i < ovs_bgpr->n_bgp_neighbors; i++) {
        if (ovs_bgpr &&
           (0 == strcmp(ovs_bgpr->key_bgp_neighbors[i], name)))
            return ovs_bgpr->key_bgp_neighbors[i];
        }
    return NULL;
}

static int
bgp_get_peer_weight(const struct ovsrec_bgp_router *bgp_row,
                    const struct ovsrec_bgp_route *rib_row,
                    const char *peer)
{
    const struct ovsrec_bgp_neighbor *bgp_peer = NULL;
    assert(rib_row);
    assert(peer);
    if (strncmp(peer, "Static", 6) == 0) {
        return BGP_ATTR_DEFAULT_WEIGHT;
    }
    bgp_peer = bgp_peer_lookup(bgp_row, peer);
    if (!bgp_peer) {
        VLOG_ERR("BGP peer info not found for route %s\n",
                 rib_row->prefix);
        return BGP_ATTR_DEFAULT_WEIGHT;
    } else {
        if (bgp_peer->n_weight) {
            return *bgp_peer->weight;
        } else {
            VLOG_DBG("BGP peer %s weight not configured\n",
                      get_bgp_neighbor_name_from_bgp_router(bgp_row, peer));
            return 0;
        }
    }
}

static int
bgp_get_rib_count(void)
{
    int count = 0;
    const struct ovsrec_bgp_route *rib_row = NULL;
    OVSREC_BGP_ROUTE_FOR_EACH(rib_row, idl) {
        count++;
    }
    return count;
}

static int
bgp_rib_cmp(void *a, void *b)
{
    int res;
    struct ovsrec_bgp_route *rt1 = (struct ovsrec_bgp_route *)a;
    struct ovsrec_bgp_route *rt2 = (struct ovsrec_bgp_route *)b;
    res = strcmp(rt1->prefix, rt2->prefix);
    if (res == 0) {
        // compare nexthops
        if (rt1->bgp_nexthops[0] && rt2->bgp_nexthops[0])
            return (strcmp(rt1->bgp_nexthops[0]->ip_address,
                           rt2->bgp_nexthops[0]->ip_address));
    } else {
        return res;
    }
}

static void
bgp_rib_sort_init(struct ovsrec_bgp_route **rib_sorted, int count)
{
    int kk = 0;
    const struct ovsrec_bgp_route *rib_row = NULL;
    assert(*rib_sorted == NULL);
    *rib_sorted = (struct ovsrec_bgp_route *)calloc(count,
                                                    sizeof(struct ovsrec_bgp_route));
    OVSREC_BGP_ROUTE_FOR_EACH(rib_row, idl) {
        memcpy((*rib_sorted)+kk, rib_row, sizeof(struct ovsrec_bgp_route));
        kk++;
    }
    qsort((void *)*rib_sorted,
          count,
          sizeof(struct ovsrec_bgp_route),
          bgp_rib_cmp);
}

static void
bgp_rib_sort_fin(struct ovsrec_bgp_route **rib_sorted)
{
    if (*rib_sorted) {
        free(*rib_sorted);
        *rib_sorted = NULL;
    }
}

/* Function to print route status code */
static void show_routes (struct vty *vty,
                         const struct ovsrec_bgp_router *bgp_row)
{
    const struct ovsrec_bgp_route *rib_row = NULL;
    int ii = 0, def_metric = 0, kk = 0;
    const struct ovsrec_bgp_nexthop *nexthop_row = NULL;
    const struct ovsrec_bgp_neighbor *bgp_peer = NULL;
    route_psd_bgp_t psd, *ppsd = NULL;
    struct ovsrec_bgp_route *rib_sorted = NULL;
    int count = bgp_get_rib_count();

    ppsd = &psd;
    bgp_rib_sort_init(&rib_sorted, count);

    // Read BGP routes from BGP local RIB
    for (kk = 0; kk < count; kk++) {
        rib_row = &rib_sorted[kk];
        bgp_get_rib_path_attributes(rib_row, ppsd);
        print_route_status(vty, ppsd);
        if (rib_row->prefix) {
            int len = 0;
            len = strlen(rib_row->prefix);
            vty_out(vty, "%s", rib_row->prefix);
            if (len < NET_BUFSZ)
                vty_out (vty, "%*s", NET_BUFSZ-len-1, " ");
            // nexthop
            if (!strcmp(rib_row->address_family, OVSREC_ROUTE_ADDRESS_FAMILY_IPV4)) {
                // Get the nexthop list
                VLOG_DBG("No. of next hops : %d", rib_row->n_bgp_nexthops);
                for (ii = 0; ii < rib_row->n_bgp_nexthops; ii++) {
                    if (ii != 0) {
                        vty_out (vty, VTY_NEWLINE);
                        vty_out (vty, "%*s", NET_BUFSZ, " ");
                    }
                    nexthop_row = rib_row->bgp_nexthops[ii];
                    vty_out (vty, "%-19s", nexthop_row->ip_address);
                }
                if (!rib_row->n_bgp_nexthops)
                    vty_out (vty, "%-19s", "0.0.0.0");
                if (rib_row->n_metric)
                    vty_out (vty, "%7d", *rib_row->metric);
                else
                    vty_out (vty, "%7d", def_metric);
                // Print local preference
                vty_out (vty, "%7d", ppsd->local_pref);
                // Print weight for non-static routes
                vty_out (vty, "%7d ", bgp_get_peer_weight(bgp_row,
                                                          rib_row,
                                                          rib_row->peer));
                // Print AS path
                if (ppsd->aspath) {
                    vty_out(vty, "%s", ppsd->aspath);
                    vty_out(vty, " ");
                }
                // print origin
                if (ppsd->origin)
                    vty_out(vty, "%s", ppsd->origin);
            } else {
                // HALON_TODO: Add ipv6 later
                VLOG_INFO("Address family not supported yet\n");
            }
            vty_out (vty, VTY_NEWLINE);
        }
    }
    vty_out(vty, "Total number of entries %d\n", count);
    bgp_rib_sort_fin(&rib_sorted);
}

DEFUN(vtysh_show_ip_bgp,
      vtysh_show_ip_bgp_cmd,
      "show ip bgp",
      SHOW_STR
      IP_STR
      BGP_STR)
{
    const struct ovsrec_bgp_router *bgp_row = NULL;

    vty_out (vty, BGP_SHOW_SCODE_HEADER, VTY_NEWLINE, VTY_NEWLINE);
    vty_out (vty, BGP_SHOW_OCODE_HEADER, VTY_NEWLINE, VTY_NEWLINE);

    bgp_row = ovsrec_bgp_router_first(idl);
    if (!bgp_row) {
        vty_out(vty, "%% No bgp router configured\n");
        return CMD_SUCCESS;
    }

    // OPS_TODO: Need to update this when multiple BGP routers are supported.
    char *id = bgp_row->router_id;
    if (id) {
        vty_out (vty, "Local router-id %s\n", id);
    } else {
        vty_out (vty, "Router-id not configured\n");
    }
    vty_out (vty, BGP_SHOW_HEADER, VTY_NEWLINE);
    show_routes(vty, bgp_row);
    return CMD_SUCCESS;
}

static void
bgp_get_paths_count_for_prefix(const char *ip, int *count, int *best)
{
    const struct ovsrec_bgp_route *rib_row;
    route_psd_bgp_t psd, *ppsd = NULL;

    assert(ip);
    assert(count);
    assert(best);
    ppsd = &psd;
    *count = *best = 0;
    // Get all routes matching this prefix
    OVSREC_BGP_ROUTE_FOR_EACH(rib_row, idl) {
        bgp_get_rib_path_attributes(rib_row, ppsd);
        if (rib_row->prefix
            && strcmp(rib_row->prefix, ip) == 0) {
            (*count)++;
            if (ppsd->flags & BGP_INFO_SELECTED)
                (*best)++;
        }
    }
}

static int
show_route_detail(struct vty *vty,
                  const struct ovsrec_bgp_router *bgp_row,
                  const struct ovsrec_bgp_route *rib_row,
                  boolean print_header)
{
    int ret;
    int count, best;
    route_psd_bgp_t psd, *ppsd = NULL;
    struct prefix p;
    boolean static_route = 0;
    const char *str;
    ppsd = &psd;
    count = best = 0;

    ret = str2prefix(rib_row->prefix, &p);
    if (!ret) {
        vty_out (vty, "address is malformed%s", VTY_NEWLINE);
        return CMD_WARNING;
    }
    bgp_get_rib_path_attributes(rib_row, ppsd);

    if (print_header) {
        vty_out (vty, "BGP routing table entry for %s%s",
                 rib_row->prefix, VTY_NEWLINE);
        bgp_get_paths_count_for_prefix(rib_row->prefix, &count, &best);
        vty_out (vty, "Paths: (%d available", count);
        if (best) {
            vty_out (vty, ", best #%d", best);
            //vty_out (vty, ", table Routing Table)");
        }
        else {
            vty_out (vty, ", no best path");
        }
        vty_out (vty, ")%s", VTY_NEWLINE);
    }
    // Print protocol specific info
    /* Line1 display AS-path, Aggregator */
    str = (*ppsd->aspath) ? ppsd->aspath : "Local";
    vty_out (vty, "AS: %s", str);
    if (ppsd->flags & BGP_INFO_REMOVED)
        vty_out (vty, ", (removed)");
    if (ppsd->flags & BGP_INFO_STALE)
        vty_out (vty, ", (stale)");
    if (ppsd->flags & BGP_INFO_HISTORY)
        vty_out (vty, ", (history entry)");
    if (ppsd->flags & BGP_INFO_DAMPED)
        vty_out (vty, ", (suppressed due to dampening)");
    vty_out (vty, "%s", VTY_NEWLINE);

    /* Line2 display Next-hop, Neighbor, Router-id */
    if (!rib_row->n_bgp_nexthops)
        vty_out (vty, "    %s", "0.0.0.0");
    else
        vty_out (vty, "    %s", rib_row->bgp_nexthops[0]->ip_address);
    if (strncmp(rib_row->peer, "Static", 6) == 0) {
        vty_out (vty, " from %s ",
                 p.family == AF_INET ? "0.0.0.0" : "::");
        vty_out (vty, "(%s)", bgp_row->router_id);
        static_route = 1;
    } else {
        if (!(ppsd->flags & BGP_INFO_VALID))
            vty_out (vty, " (inaccessible)");
        vty_out (vty, " from %s", rib_row->peer);
        // HALON_TODO: display peer router_id when it is saved in table.
    }
    vty_out (vty, "%s", VTY_NEWLINE);
    /* Line 3 display Origin, Med, Locpref, Weight, valid,
       Int/Ext/Local, Atomic, best */
    vty_out (vty, "      Origin %s", bgp_get_origin_long_str(ppsd->origin));
    int metric = (rib_row->n_metric) ? *rib_row->metric : 0;
    vty_out (vty, ", metric %d", metric);
    vty_out (vty, ", localpref %d", ppsd->local_pref);
    vty_out (vty, ", weight %d", bgp_get_peer_weight(bgp_row,
                                                     rib_row,
                                                     rib_row->peer));
    if (! (ppsd->flags & BGP_INFO_HISTORY))
        vty_out (vty, ", valid");
    if (!static_route) {
        if (ppsd->internal)
            vty_out (vty, ", internal");
        else
            vty_out (vty, ", external");
    } else if (static_route) {
        vty_out (vty, ", sourced, local");
    } else {
        vty_out (vty, ", sourced");
    }
    if (ppsd->flags & BGP_INFO_SELECTED)
        vty_out (vty, ", best");
    vty_out (vty, "%s", VTY_NEWLINE);
    /* Line 4 display Uptime */
    vty_out (vty, "      Last update: %s", ppsd->uptime);
    vty_out (vty, "%s", VTY_NEWLINE);
    return 0;
}

static int
bgp_show_route(char *vrf_name, struct vty *vty, const char *view_name, const char *ip_str,
               afi_t afi, safi_t safi)
{
    const struct ovsrec_bgp_router *bgp_row = NULL;
    const struct ovsrec_bgp_route *rib_row = NULL;
    struct ovsrec_bgp_route *rib_sorted = NULL;
    struct prefix match;
    int cmpLen = 0, found = 0, ret, ii = 0;
    boolean print_header = 0;
    int count = bgp_get_rib_count();

    bgp_row = ovsrec_bgp_router_first(idl);
    if (!bgp_row) {
        vty_out(vty, "%% No bgp router configured\n");
        return CMD_SUCCESS;
    }
    if (!ip_str)
        return CMD_WARNING;
    cmpLen = strlen(ip_str);
    ret = str2prefix (ip_str, &match);
    if (!ret) {
        vty_out (vty, "%% Address is malformed%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    bgp_rib_sort_init(&rib_sorted, count);
    for (ii = 0; ii < count; ii++) {
        rib_row = &rib_sorted[ii];
        if (rib_row->prefix
            && strncmp(rib_row->prefix, ip_str, cmpLen) == 0) {
            if (!found) {
                found = 1;
                print_header = 1;
            }
            show_route_detail(vty, bgp_row, rib_row, print_header);
            print_header = 0;
        }
    }
    bgp_rib_sort_fin(&rib_sorted);
    if(!found) {
        vty_out (vty, "%% Network not in table%s", VTY_NEWLINE);
        return CMD_WARNING;
    }
    return CMD_SUCCESS;
}

DEFUN (vtysh_show_ip_bgp_route,
       vtysh_show_ip_bgp_route_cmd,
       "show ip bgp A.B.C.D",
       SHOW_STR
       IP_STR
       BGP_STR
       "Network in the BGP routing table to display\n")
{
    return bgp_show_route (NULL, vty, NULL, argv[0], AFI_IP, SAFI_UNICAST);
}

DEFUN (vtysh_show_ip_bgp_prefix,
       vtysh_show_ip_bgp_prefix_cmd,
       "show ip bgp A.B.C.D/M",
       SHOW_STR
       IP_STR
       BGP_STR
       "IP prefix <network>/<length>, e.g., 35.0.0.0/8\n")
{
    return bgp_show_route (NULL, vty, NULL, argv[0], AFI_IP, SAFI_UNICAST);
}


/* BGP global configuration.  */

DEFUN (bgp_multiple_instance_func,
       bgp_multiple_instance_cmd,
       "bgp multiple-instance",
       BGP_STR
       "Enable bgp multiple instance\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_multiple_instance,
       no_bgp_multiple_instance_cmd,
       "no bgp multiple-instance",
       NO_STR
       BGP_STR
       "BGP multiple instance\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_config_type,
       bgp_config_type_cmd,
       "bgp config-type (cisco|zebra)",
       BGP_STR
       "Configuration type\n"
       "cisco\n"
       "zebra\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_config_type,
       no_bgp_config_type_cmd,
       "no bgp config-type",
       NO_STR
       BGP_STR
       "Display configuration type\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_synchronization,
       no_synchronization_cmd,
       "no synchronization",
       NO_STR
       "Perform IGP synchronization\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_auto_summary,
       no_auto_summary_cmd,
       "no auto-summary",
       NO_STR
       "Enable automatic network number summarization\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN_DEPRECATED (neighbor_version,
		  neighbor_version_cmd,
		  NEIGHBOR_CMD "version (4|4-)",
		  NEIGHBOR_STR
		  NEIGHBOR_ADDR_STR
		  "Set the BGP version to match a neighbor\n"
		  "Neighbor's BGP version\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

void
bgp_router_insert_to_vrf(struct ovsrec_vrf *vrf_row,
                         struct ovsrec_bgp_router *bgp_router_row,
                         int64_t asn)
{
    int64_t *asn_list;
    struct ovsrec_bgp_router **bgp_routers_list;
    int i = 0;

    /* Insert BGP_Router table reference in VRF table */
    asn_list = xmalloc(sizeof(int64_t) * (vrf_row->n_bgp_routers + 1));
    bgp_routers_list = xmalloc(sizeof * vrf_row->key_bgp_routers *
                              (vrf_row->n_bgp_routers + 1));
    for (i = 0; i < vrf_row->n_bgp_routers; i++) {
        asn_list[i] = vrf_row->key_bgp_routers[i];
        bgp_routers_list[i] = vrf_row->value_bgp_routers[i];
    }
    asn_list[vrf_row->n_bgp_routers] = asn;
    bgp_routers_list[vrf_row->n_bgp_routers] = bgp_router_row;
    ovsrec_vrf_set_bgp_routers(vrf_row, asn_list, bgp_routers_list,
                              (vrf_row->n_bgp_routers + 1));
    free(asn_list);
    free(bgp_routers_list);
}

static int
cli_router_bgp_cmd_execute (char *vrf_name, int64_t asn)
{
    const struct ovsrec_bgp_router *bgp_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *bgp_router_txn;

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
	ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }
    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, asn);

    /* If does not exist, create a new one */
    if (bgp_router_row == NULL) {
        bgp_router_row = ovsrec_bgp_router_insert(bgp_router_txn);
        bgp_router_insert_to_vrf(vrf_row, bgp_router_row, asn);
#ifdef EXTRA_DEBUG
	    vty_out(vty, "new bgp router created with asn : %d\n", asn);
#endif // EXTRA_DEBUG
    }
    else {
        VLOG_DBG("bgp router already exists!");
    }

    /* Get the context from previous command for sub-commands */
    vty->node = BGP_NODE;
    vty->index = (void*) asn;

    /* End of transaction */
    END_DB_TXN(bgp_router_txn);
}

/* "router bgp" commands. */
DEFUN (router_bgp,
       router_bgp_cmd,
       "router bgp " CMD_AS_RANGE,
       ROUTER_STR
       BGP_STR
       AS_STR)
{
    return
	cli_router_bgp_cmd_execute(NULL, atoi(argv[0]));
}

ALIAS (router_bgp,
       router_bgp_view_cmd,
       "router bgp " CMD_AS_RANGE " view WORD",
       ROUTER_STR
       BGP_STR
       AS_STR
       "BGP view\n"
       "view name\n")

void
bgp_router_remove_from_vrf(struct ovsrec_vrf *vrf_row,
                           struct ovsrec_bgp_router *bgp_router_row,
                           int64_t asn)
{
    int64_t *asn_list;
    struct ovsrec_bgp_router **bgp_routers_list;
    int i = 0;

    /* Insert BGP_Router table reference in VRF table */
    asn_list = xmalloc(sizeof(int64_t) * (vrf_row->n_bgp_routers - 1));
    bgp_routers_list = xmalloc(sizeof * vrf_row->key_bgp_routers *
                              (vrf_row->n_bgp_routers - 1));
    for (i = 0; i < vrf_row->n_bgp_routers; i++) {
        if(asn_list[i] != asn) {
            asn_list[i] = vrf_row->key_bgp_routers[i];
            bgp_routers_list[i] = vrf_row->value_bgp_routers[i];
        }
    }
    ovsrec_vrf_set_bgp_routers(vrf_row, asn_list, bgp_routers_list,
                              (vrf_row->n_bgp_routers - 1));
    free(asn_list);
    free(bgp_routers_list);
}

static int
cli_no_router_bgp_cmd_execute (char *vrf_name, int64_t asn)
{
    const struct ovsrec_bgp_router *bgp_router_row = NULL;
    const struct ovsrec_vrf *vrf_row = NULL;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, asn);

    /* If does not exist, nothing to delete */
    if (bgp_router_row == NULL) {
	ABORT_DB_TXN(bgp_router_txn, "No such bgp router found to delete");
    } else {
        /* Delete the bgp row for matching asn */
        ovsrec_bgp_router_delete(bgp_router_row);
        bgp_router_remove_from_vrf(vrf_row, bgp_router_row, asn);
    }

    /* End of transaction*/
    END_DB_TXN(bgp_router_txn);
}

/* "no router bgp" commands. */
DEFUN (no_router_bgp,
       no_router_bgp_cmd,
       "no router bgp " CMD_AS_RANGE,
       NO_STR
       ROUTER_STR
       BGP_STR
       AS_STR)
{
    return cli_no_router_bgp_cmd_execute(NULL, atoi(argv[0]));
}

ALIAS (no_router_bgp,
       no_router_bgp_view_cmd,
       "no router bgp " CMD_AS_RANGE " view WORD",
       NO_STR
       ROUTER_STR
       BGP_STR
       AS_STR
       "BGP view\n"
       "view name\n")

static int
cli_bgp_router_id_cmd_execute (char *vrf_name, char *router_ip_addr)
{
    int ret;
    struct in_addr id;
    const struct ovsrec_bgp_router *bgp_router_row;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;

    if(string_is_an_ip_address(router_ip_addr)) {
        ret = inet_aton (router_ip_addr, &id);
        if (!ret || (id.s_addr == 0)) {
            vty_out (vty, "%% Malformed bgp router identifier%s", VTY_NEWLINE);
            return CMD_WARNING;
        }

        /* Start of transaction */
        START_DB_TXN(bgp_router_txn);

        VLOG_DBG("vty_index for router_id: %d\n",(int64_t)vty->index);

        vrf_row = get_ovsrec_vrf_with_name(vrf_name);
        if (vrf_row == NULL) {
            ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
        }
        /* See if it already exists */
        bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

        /* If does not exist, nothing to modify */
        if (bgp_router_row == NULL) {
            ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
        } else {
            /* Set the router-id with matching asn */
            ovsrec_bgp_router_set_router_id(bgp_router_row,inet_ntoa(id));
        }

        /* End of transaction*/
        END_DB_TXN(bgp_router_txn);
    }
    else {
        vty_out (vty, "%% Malformed bgp router identifier%s", VTY_NEWLINE);
        return CMD_WARNING;
    }
}

/* BGP router-id.  */

DEFUN (bgp_router_id,
       bgp_router_id_cmd,
       "bgp router-id A.B.C.D",
       BGP_STR
       "Override configured router identifier\n"
       "Manually configured router identifier\n")
{
    return
        cli_bgp_router_id_cmd_execute (NULL, CONST_CAST(char*,argv[0]));
}

DEFUN (no_bgp_router_id,
       no_bgp_router_id_cmd,
       "no bgp router-id",
       NO_STR
       BGP_STR
       "Override configured router identifier\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_router_id,
       no_bgp_router_id_val_cmd,
       "no bgp router-id A.B.C.D",
       NO_STR
       BGP_STR
       "Override configured router identifier\n"
       "Manually configured router identifier\n")

/* BGP Cluster ID.  */

DEFUN (bgp_cluster_id,
       bgp_cluster_id_cmd,
       "bgp cluster-id A.B.C.D",
       BGP_STR
       "Configure Route-Reflector Cluster-id\n"
       "Route-Reflector Cluster-id in IP address format\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (bgp_cluster_id,
       bgp_cluster_id32_cmd,
       "bgp cluster-id <1-4294967295>",
       BGP_STR
       "Configure Route-Reflector Cluster-id\n"
       "Route-Reflector Cluster-id as 32 bit quantity\n")

DEFUN (no_bgp_cluster_id,
       no_bgp_cluster_id_cmd,
       "no bgp cluster-id",
       NO_STR
       BGP_STR
       "Configure Route-Reflector Cluster-id\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_cluster_id,
       no_bgp_cluster_id_arg_cmd,
       "no bgp cluster-id A.B.C.D",
       NO_STR
       BGP_STR
       "Configure Route-Reflector Cluster-id\n"
       "Route-Reflector Cluster-id in IP address format\n")

DEFUN (bgp_confederation_identifier,
       bgp_confederation_identifier_cmd,
       "bgp confederation identifier " CMD_AS_RANGE,
       "BGP specific commands\n"
       "AS confederation parameters\n"
       "AS number\n"
       "Set routing domain confederation AS\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_confederation_identifier,
       no_bgp_confederation_identifier_cmd,
       "no bgp confederation identifier",
       NO_STR
       "BGP specific commands\n"
       "AS confederation parameters\n"
       "AS number\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_confederation_identifier,
       no_bgp_confederation_identifier_arg_cmd,
       "no bgp confederation identifier " CMD_AS_RANGE,
       NO_STR
       "BGP specific commands\n"
       "AS confederation parameters\n"
       "AS number\n"
       "Set routing domain confederation AS\n")

DEFUN (bgp_confederation_peers,
       bgp_confederation_peers_cmd,
       "bgp confederation peers ." CMD_AS_RANGE,
       "BGP specific commands\n"
       "AS confederation parameters\n"
       "Peer ASs in BGP confederation\n"
       AS_STR)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_confederation_peers,
       no_bgp_confederation_peers_cmd,
       "no bgp confederation peers ." CMD_AS_RANGE,
       NO_STR
       "BGP specific commands\n"
       "AS confederation parameters\n"
       "Peer ASs in BGP confederation\n"
       AS_STR)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

static int
cli_bgp_maxpaths_cmd_execute(char *vrf_name, int64_t max_paths)
{
    struct ovsrec_bgp_router *bgp_router_row;
    struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;
    int64_t *pmax_paths = NULL;
    int size = 0;

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    VLOG_DBG("vty_index for maxpaths : %d\n",(int64_t)vty->index);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    /* If does not exist, nothing to modify */
    if (bgp_router_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
    } else {
        // CLI does not allow a value less than 1 to be set, so we use it to
        // identify the set and "no" case.
        if (max_paths) {
            pmax_paths = &max_paths;
            size = 1;
        }

        /* Set the maximum-paths with matching asn */
        ovsrec_bgp_router_set_maximum_paths(bgp_router_row, pmax_paths, size);
    }

    /* End of transaction*/
    END_DB_TXN(bgp_router_txn);
}

/* Maximum-paths configuration */
DEFUN (bgp_maxpaths,
       bgp_maxpaths_cmd,
       "maximum-paths <1-255>",
       "Forward packets over multiple paths\n"
       "Number of paths\n")
{
    return cli_bgp_maxpaths_cmd_execute(NULL, atoi(argv[0]));
}

DEFUN (bgp_maxpaths_ibgp,
       bgp_maxpaths_ibgp_cmd,
       "maximum-paths ibgp <1-255>",
       "Forward packets over multiple paths\n"
       "iBGP-multipath\n"
       "Number of paths\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_maxpaths,
       no_bgp_maxpaths_cmd,
       "no maximum-paths",
       NO_STR
       "Forward packets over multiple paths\n"
       "Number of paths\n")
{
    return cli_bgp_maxpaths_cmd_execute(NULL, 0);
}

ALIAS (no_bgp_maxpaths,
       no_bgp_maxpaths_arg_cmd,
       "no maximum-paths <1-255>",
       NO_STR
       "Forward packets over multiple paths\n"
       "Number of paths\n")

DEFUN (no_bgp_maxpaths_ibgp,
       no_bgp_maxpaths_ibgp_cmd,
       "no maximum-paths ibgp",
       NO_STR
       "Forward packets over multiple paths\n"
       "iBGP-multipath\n"
       "Number of paths\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_maxpaths_ibgp,
       no_bgp_maxpaths_ibgp_arg_cmd,
       "no maximum-paths ibgp <1-255>",
       NO_STR
       "Forward packets over multiple paths\n"
       "iBGP-multipath\n"
       "Number of paths\n")

/* BGP timers.  */

static int
cli_bgp_timers_cmd_execute(char *vrf_name, int64_t keepalive, int64_t holdtime)
{
    struct ovsrec_bgp_router *bgp_router_row;
    struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;
    char **bgp_key_timers = NULL;
    int64_t bgp_value_timers[2];

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    VLOG_DBG("vty_index for timers : %d\n",(int64_t)vty->index);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    /* If does not exist, nothing to modify */
    if (bgp_router_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
    }
    else {
        if (keepalive >= 0 && keepalive <= 65535 && holdtime >= 0 && holdtime <= 65535) {
           /* Holdtime value check. */
           if (holdtime < 3 && holdtime != 0)
           {
               vty_out (vty, "%% hold time value must be either 0 or greater than 3%s",
               VTY_NEWLINE);
           }
           else
           {
               bgp_key_timers =  xmalloc(TIMER_KEY_MAX_LENGTH * BGP_MAX_TIMERS);
               bgp_key_timers[0] = OVSDB_BGP_TIMER_KEEPALIVE;
               bgp_key_timers[1] = OVSDB_BGP_TIMER_HOLDTIME;

               bgp_value_timers[0] = keepalive;
               bgp_value_timers[1] = holdtime;

               /* Set the timers with matching asn */
               ovsrec_bgp_router_set_timers(bgp_router_row, bgp_key_timers,
                                            bgp_value_timers, 2);
           }
        }
        else {
            VLOG_INFO("The timer values are not in the range.\n"
                      "Please refer to following range values: "
                      "keepalive <0-65535> holdtime <0-65535>");
        }
    }

    if(bgp_key_timers != NULL)
        free(bgp_key_timers);

    /* End of transaction*/
    END_DB_TXN(bgp_router_txn);
}

static int
cli_no_bgp_timers_cmd_execute(char *vrf_name)
{
    struct ovsrec_bgp_router *bgp_router_row;
    struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *bgp_router_txn = NULL;

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    VLOG_DBG("vty_index for timers : %d\n",(int)(vty->index));

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int)(vty->index));

    /* If does not exist, nothing to modify */
    if (bgp_router_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
    }

    /* Set the timers with matching asn */
    ovsrec_bgp_router_set_timers(bgp_router_row, NULL, NULL, 0);

    /* End of transaction*/
    END_DB_TXN(bgp_router_txn);
}

DEFUN (bgp_timers,
       bgp_timers_cmd,
       "timers bgp <0-65535> <0-65535>",
       "Adjust routing timers\n"
       "BGP timers\n"
       "Keepalive interval\n"
       "Holdtime\n")
{
    return ((argc==2) ?
        cli_bgp_timers_cmd_execute(NULL, atoi(argv[0]), atoi(argv[1])) : CMD_ERR_AMBIGUOUS);
}

DEFUN (no_bgp_timers,
       no_bgp_timers_cmd,
       "no timers bgp",
       NO_STR
       "Adjust routing timers\n"
       "BGP timers\n")
{
    return cli_no_bgp_timers_cmd_execute(NULL);
}

ALIAS (no_bgp_timers,
       no_bgp_timers_arg_cmd,
       "no timers bgp <0-65535> <0-65535>",
       NO_STR
       "Adjust routing timers\n"
       "BGP timers\n"
       "Keepalive interval\n"
       "Holdtime\n")

DEFUN (bgp_client_to_client_reflection,
       bgp_client_to_client_reflection_cmd,
       "bgp client-to-client reflection",
       "BGP specific commands\n"
       "Configure client to client route reflection\n"
       "reflection of routes allowed\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_client_to_client_reflection,
       no_bgp_client_to_client_reflection_cmd,
       "no bgp client-to-client reflection",
       NO_STR
       "BGP specific commands\n"
       "Configure client to client route reflection\n"
       "reflection of routes allowed\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp always-compare-med" configuration. */
DEFUN (bgp_always_compare_med,
       bgp_always_compare_med_cmd,
       "bgp always-compare-med",
       "BGP specific commands\n"
       "Allow comparing MED from different neighbors\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_always_compare_med,
       no_bgp_always_compare_med_cmd,
       "no bgp always-compare-med",
       NO_STR
       "BGP specific commands\n"
       "Allow comparing MED from different neighbors\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp deterministic-med" configuration. */
DEFUN (bgp_deterministic_med,
       bgp_deterministic_med_cmd,
       "bgp deterministic-med",
       "BGP specific commands\n"
       "Pick the best-MED path among paths advertised from the neighboring AS\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_deterministic_med,
       no_bgp_deterministic_med_cmd,
       "no bgp deterministic-med",
       NO_STR
       "BGP specific commands\n"
       "Pick the best-MED path among paths advertised from the neighboring AS\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp graceful-restart" configuration. */
DEFUN (bgp_graceful_restart,
       bgp_graceful_restart_cmd,
       "bgp graceful-restart",
       "BGP specific commands\n"
       "Graceful restart capability parameters\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_graceful_restart,
       no_bgp_graceful_restart_cmd,
       "no bgp graceful-restart",
       NO_STR
       "BGP specific commands\n"
       "Graceful restart capability parameters\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_graceful_restart_stalepath_time,
       bgp_graceful_restart_stalepath_time_cmd,
       "bgp graceful-restart stalepath-time <1-3600>",
       "BGP specific commands\n"
       "Graceful restart capability parameters\n"
       "Set the max time to hold onto restarting peer's stale paths\n"
       "Delay value (seconds)\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_graceful_restart_stalepath_time,
       no_bgp_graceful_restart_stalepath_time_cmd,
       "no bgp graceful-restart stalepath-time",
       NO_STR
       "BGP specific commands\n"
       "Graceful restart capability parameters\n"
       "Set the max time to hold onto restarting peer's stale paths\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_graceful_restart_stalepath_time,
       no_bgp_graceful_restart_stalepath_time_val_cmd,
       "no bgp graceful-restart stalepath-time <1-3600>",
       NO_STR
       "BGP specific commands\n"
       "Graceful restart capability parameters\n"
       "Set the max time to hold onto restarting peer's stale paths\n"
       "Delay value (seconds)\n")

/* "bgp fast-external-failover" configuration. */
DEFUN (bgp_fast_external_failover,
       bgp_fast_external_failover_cmd,
       "bgp fast-external-failover",
       BGP_STR
       "Immediately reset session if a link to a directly connected external peer goes down\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_fast_external_failover,
       no_bgp_fast_external_failover_cmd,
       "no bgp fast-external-failover",
       NO_STR
       BGP_STR
       "Immediately reset session if a link to a directly connected external peer goes down\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp enforce-first-as" configuration. */
DEFUN (bgp_enforce_first_as,
       bgp_enforce_first_as_cmd,
       "bgp enforce-first-as",
       BGP_STR
       "Enforce the first AS for EBGP routes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_enforce_first_as,
       no_bgp_enforce_first_as_cmd,
       "no bgp enforce-first-as",
       NO_STR
       BGP_STR
       "Enforce the first AS for EBGP routes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp bestpath compare-routerid" configuration.  */
DEFUN (bgp_bestpath_compare_router_id,
       bgp_bestpath_compare_router_id_cmd,
       "bgp bestpath compare-routerid",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "Compare router-id for identical EBGP paths\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_bestpath_compare_router_id,
       no_bgp_bestpath_compare_router_id_cmd,
       "no bgp bestpath compare-routerid",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "Compare router-id for identical EBGP paths\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp bestpath as-path ignore" configuration.  */
DEFUN (bgp_bestpath_aspath_ignore,
       bgp_bestpath_aspath_ignore_cmd,
       "bgp bestpath as-path ignore",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Ignore as-path length in selecting a route\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_bestpath_aspath_ignore,
       no_bgp_bestpath_aspath_ignore_cmd,
       "no bgp bestpath as-path ignore",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Ignore as-path length in selecting a route\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp bestpath as-path confed" configuration.  */
DEFUN (bgp_bestpath_aspath_confed,
       bgp_bestpath_aspath_confed_cmd,
       "bgp bestpath as-path confed",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Compare path lengths including confederation sets & sequences in selecting a route\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_bestpath_aspath_confed,
       no_bgp_bestpath_aspath_confed_cmd,
       "no bgp bestpath as-path confed",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Compare path lengths including confederation sets & sequences in selecting a route\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp bestpath as-path multipath-relax" configuration.  */
DEFUN (bgp_bestpath_aspath_multipath_relax,
       bgp_bestpath_aspath_multipath_relax_cmd,
       "bgp bestpath as-path multipath-relax",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Allow load sharing across routes that have different AS paths (but same length)\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_bestpath_aspath_multipath_relax,
       no_bgp_bestpath_aspath_multipath_relax_cmd,
       "no bgp bestpath as-path multipath-relax",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "AS-path attribute\n"
       "Allow load sharing across routes that have different AS paths (but same length)\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp log-neighbor-changes" configuration.  */
DEFUN (bgp_log_neighbor_changes,
       bgp_log_neighbor_changes_cmd,
       "bgp log-neighbor-changes",
       "BGP specific commands\n"
       "Log neighbor up/down and reset reason\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_log_neighbor_changes,
       no_bgp_log_neighbor_changes_cmd,
       "no bgp log-neighbor-changes",
       NO_STR
       "BGP specific commands\n"
       "Log neighbor up/down and reset reason\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* "bgp bestpath med" configuration. */
DEFUN (bgp_bestpath_med,
       bgp_bestpath_med_cmd,
       "bgp bestpath med (confed|missing-as-worst)",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Compare MED among confederation paths\n"
       "Treat missing MED as the least preferred one\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_bestpath_med2,
       bgp_bestpath_med2_cmd,
       "bgp bestpath med confed missing-as-worst",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Compare MED among confederation paths\n"
       "Treat missing MED as the least preferred one\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (bgp_bestpath_med2,
       bgp_bestpath_med3_cmd,
       "bgp bestpath med missing-as-worst confed",
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Treat missing MED as the least preferred one\n"
       "Compare MED among confederation paths\n")

DEFUN (no_bgp_bestpath_med,
       no_bgp_bestpath_med_cmd,
       "no bgp bestpath med (confed|missing-as-worst)",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Compare MED among confederation paths\n"
       "Treat missing MED as the least preferred one\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_bestpath_med2,
       no_bgp_bestpath_med2_cmd,
       "no bgp bestpath med confed missing-as-worst",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Compare MED among confederation paths\n"
       "Treat missing MED as the least preferred one\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_bestpath_med2,
       no_bgp_bestpath_med3_cmd,
       "no bgp bestpath med missing-as-worst confed",
       NO_STR
       "BGP specific commands\n"
       "Change the default bestpath selection\n"
       "MED attribute\n"
       "Treat missing MED as the least preferred one\n"
       "Compare MED among confederation paths\n")

/* "no bgp default ipv4-unicast". */
DEFUN (no_bgp_default_ipv4_unicast,
       no_bgp_default_ipv4_unicast_cmd,
       "no bgp default ipv4-unicast",
       NO_STR
       "BGP specific commands\n"
       "Configure BGP defaults\n"
       "Activate ipv4-unicast for a peer by default\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_default_ipv4_unicast,
       bgp_default_ipv4_unicast_cmd,
       "bgp default ipv4-unicast",
       "BGP specific commands\n"
       "Configure BGP defaults\n"
       "Activate ipv4-unicast for a peer by default\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* Configure static BGP network */
static int
cli_bgp_network_cmd_execute (char *vrf_name, char *network)
{
   int ret = 0, i = 0;
    struct prefix p;
    struct ovsrec_bgp_router *bgp_router_row;
    struct ovsrec_vrf *vrf_row;
    char **network_list;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;

    /* Convert IP prefix string to struct prefix. */
    ret = str2prefix (network, &p);
    if (! ret ) {
        vty_out (vty, "%% Malformed prefix%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (bgp_router_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
    }
    else {
        VLOG_DBG("vty_index for network : %ld\n",(int64_t)vty->index);
        /* Insert networks in BGP_Router table */
        network_list = xmalloc((NETWORK_MAX_LEN*sizeof(char)) *
                               (bgp_router_row->n_networks + 1));
        for (i = 0; i < bgp_router_row->n_networks; i++) {
            network_list[i] = bgp_router_row->networks[i];
        }
        network_list[bgp_router_row->n_networks] = network;
        ovsrec_bgp_router_set_networks(bgp_router_row, network_list,
                                      (bgp_router_row->n_networks + 1));
        free(network_list);
    }

    /* End of transaction */
    END_DB_TXN(bgp_router_txn);
}

/* Installing command for "network <network>/<length>" */
DEFUN (bgp_network,
       bgp_network_cmd,
       "network A.B.C.D/M",
       "Specify a network to announce via BGP\n"
       "IP prefix <network>/<length>, e.g., 35.0.0.0/8\n")
{
    return
        cli_bgp_network_cmd_execute(NULL, CONST_CAST(char*,argv[0]));
}

/* Unconfigure static BGP network */
static int
cli_no_bgp_network_cmd_execute (char *vrf_name, char *network)
{
    int ret = 0, i = 0, j = 0;
    struct prefix p;
    struct in_addr *id;
    char buf[SU_ADDRSTRLEN];
    const struct ovsrec_bgp_router *bgp_router_row;
    struct ovsrec_vrf *vrf_row;
    char **network_list;
    struct ovsdb_idl_txn *bgp_router_txn=NULL;

    /* Convert IP prefix string to struct prefix. */
    ret = str2prefix (network, &p);
    if (! ret) {
        vty_out (vty, "%% Malformed prefix%s", VTY_NEWLINE);
        return CMD_WARNING;
    }

    /* Start of transaction */
    START_DB_TXN(bgp_router_txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no vrf found");
    }

    /* See if it already exists */
    bgp_router_row = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (bgp_router_row == NULL) {
        ERRONEOUS_DB_TXN(bgp_router_txn, "no bgp router found");
    }
    else {
        VLOG_DBG("vty_index for no network : %ld\n",(int64_t)vty->index);
        /* Insert networks in BGP_Router table */
        network_list = xmalloc((NETWORK_MAX_LEN*sizeof(char)) *
                               (bgp_router_row->n_networks - 1));
        for (i = 0,j = 0; i < bgp_router_row->n_networks; i++) {
            if(!strcmp(bgp_router_row->networks[i], network)) {
                continue;
            }
            else {
                network_list[j++] = bgp_router_row->networks[i];
            }
        }
        ovsrec_bgp_router_set_networks(bgp_router_row, network_list,
                                      (bgp_router_row->n_networks - 1));
        free(network_list);
    }

    /* End of transaction */
    END_DB_TXN(bgp_router_txn);
}

DEFUN (no_bgp_network,
       no_bgp_network_cmd,
       "no network A.B.C.D/M",
       NO_STR
       "Specify a network to announce via BGP\n"
       "IP prefix <network>/<length>, e.g., 35.0.0.0/8\n")
{
    return cli_no_bgp_network_cmd_execute(NULL, argv[0]);
}

/* "bgp import-check" configuration.  */
DEFUN (bgp_network_import_check,
       bgp_network_import_check_cmd,
       "bgp network import-check",
       "BGP specific commands\n"
       "BGP network command\n"
       "Check BGP network route exists in IGP\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_network_import_check,
       no_bgp_network_import_check_cmd,
       "no bgp network import-check",
       NO_STR
       "BGP specific commands\n"
       "BGP network command\n"
       "Check BGP network route exists in IGP\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_default_local_preference,
       bgp_default_local_preference_cmd,
       "bgp default local-preference <0-4294967295>",
       "BGP specific commands\n"
       "Configure BGP defaults\n"
       "local preference (higher=more preferred)\n"
       "Configure default local preference value\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_default_local_preference,
       no_bgp_default_local_preference_cmd,
       "no bgp default local-preference",
       NO_STR
       "BGP specific commands\n"
       "Configure BGP defaults\n"
       "local preference (higher=more preferred)\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_default_local_preference,
       no_bgp_default_local_preference_val_cmd,
       "no bgp default local-preference <0-4294967295>",
       NO_STR
       "BGP specific commands\n"
       "Configure BGP defaults\n"
       "local preference (higher=more preferred)\n"
       "Configure default local preference value\n")

/*
** Assigns a remote-as to an *EXISTING* peer group OR
** creates a NEW peer with remote-as if none exists or
** changes the remote-as of an already existing peer.
** Note that if the peer already exists and is bound
** to a peer group, its remote-as cannot be changed.
*/
static int
cli_neighbor_remote_as_cmd_execute (char *vrf_name, struct vty *vty,
                                    int argc, char *argv[])
{
    const char *peer_str = argv[0];
    int64_t remote_as = (int64_t) atoi(argv[1]);
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor, *ovs_peer_grp;
    struct ovsdb_idl_txn *txn;
    bool update_all_peers = false;
    char *name;
    int i = 0;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }
    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "% Bgp router context not available");
    }

    /* an ipv4 or v6 address, must be a neighbor/peer */
    if (string_is_an_ip_address(peer_str)) {
	ovs_bgp_neighbor =
	    get_bgp_neighbor_with_bgp_router_and_ipaddr
		(bgp_router_context, peer_str);
	if (ovs_bgp_neighbor) {
	    if (ovs_bgp_neighbor->bgp_peer_group) {
		char error_message[128];
		for (i = 0; i < bgp_router_context->n_bgp_neighbors + 1; i++){
			if(0 == strcmp(bgp_router_context->value_bgp_neighbors[i], ovs_bgp_neighbor)){
				name = bgp_router_context->key_bgp_neighbors[i];
			}
		}
		sprintf(error_message,
		    "%% Bound to peer group %s already, cannot change remote-as\n",name);
		ABORT_DB_TXN(txn, error_message);
	    }
	} else {
	    ovs_bgp_neighbor = ovsrec_bgp_neighbor_insert(txn);
	    if (!ovs_bgp_neighbor) {
	       ERRONEOUS_DB_TXN(txn, "%% Bgp neighbor object creation failed\n");
	    }
	    define_object_as_a_bgp_peer(ovs_bgp_neighbor);

	    /* Add peer group reference to the BGP Router table */
	    bgp_neighbor_peer_group_insert_to_bgp_router(bgp_router_context, ovs_bgp_neighbor, peer_str);
	}
    /* a name, must be a peer group */
    } else {
	ovs_bgp_neighbor =
	    get_bgp_peer_group_with_bgp_router_and_name
		(bgp_router_context, peer_str);
	if (!ovs_bgp_neighbor) {
	    ABORT_DB_TXN(txn, "%% Create the peer-group first\n");
	}
	update_all_peers = true;
	ovs_peer_grp = ovs_bgp_neighbor;
    }
    ovsrec_bgp_neighbor_set_remote_as(ovs_bgp_neighbor, &remote_as, 1);

    /*
    ** if we are a peer group whose remote-as has just been set or changed,
    ** update the remote-as of all the peers bound to this peer group.
    */
    if (update_all_peers) {
	OVSREC_BGP_NEIGHBOR_FOR_EACH(ovs_bgp_neighbor, idl) {
	    if (object_is_bgp_neighbor(ovs_bgp_neighbor)) {
		if (ovs_bgp_neighbor->bgp_peer_group == ovs_peer_grp) {
		    ovsrec_bgp_neighbor_set_remote_as
			(ovs_bgp_neighbor, &remote_as, 1);
		}
	    }
	}
    }

    /* done */
    END_DB_TXN(txn);
}
#ifndef ENABLE_OVSDB
static int
cli_no_neighbor_remote_as_cmd_execute (struct vty *vty,
    int argc, const char *argv[])
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}
#endif
DEFUN (neighbor_remote_as,
       neighbor_remote_as_cmd,
       NEIGHBOR_CMD2 "remote-as " CMD_AS_RANGE,
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a BGP neighbor\n"
       AS_STR)
{
    if (argc != 2) {
	vty_out(vty, "\nargc should be 2, it is %d; %s: %d\n",
	    argc, __FILE__, __LINE__);
	return CMD_WARNING;
    }
    return
	cli_neighbor_remote_as_cmd_execute(NULL, vty, argc, argv);
}

void
bgp_neighbor_remove_for_matching_peer_group_from_bgp_router(
                         struct ovsrec_bgp_router *bgp_router_context,
                         struct ovsrec_bgp_neighbor *ovs_bgp_neighbor)
{
    struct ovsrec_bgp_neighbor **bgp_neighbor_peer_group_list;
    char **bgp_neighbor_peer_name_list;
    int i = 0;

    bgp_neighbor_peer_name_list = xmalloc(80 * (bgp_router_context->n_bgp_neighbors - 1));
    bgp_neighbor_peer_group_list = xmalloc(sizeof * bgp_router_context->value_bgp_neighbors *
                              (bgp_router_context->n_bgp_neighbors - 1));

    for (i = 0; i < bgp_router_context->n_bgp_neighbors; i++) {
        if (0 != strcmp(bgp_router_context->value_bgp_neighbors[i],
                        ovs_bgp_neighbor)) {
            bgp_neighbor_peer_name_list[i] = bgp_router_context->key_bgp_neighbors[i];
            bgp_neighbor_peer_group_list[i] = bgp_router_context->value_bgp_neighbors[i];
        }
    }
    ovsrec_bgp_router_set_bgp_neighbors(bgp_router_context,
                                   bgp_neighbor_peer_name_list,
                                   bgp_neighbor_peer_group_list,
                                   (bgp_router_context->n_bgp_neighbors - 1));
    free(bgp_neighbor_peer_name_list);
    free(bgp_neighbor_peer_group_list);
}

static int
delete_neighbor_peer_group (const struct ovsrec_bgp_router *bgp_router_context,
    const char *name)
{
    const struct ovsrec_bgp_neighbor *peer_group;
    const struct ovsrec_bgp_neighbor *bgpn, *bgpn_next;

    peer_group =
	get_bgp_peer_group_with_bgp_router_and_name
	    (bgp_router_context, name);
    if (!peer_group) {
        return CMD_ERR_NO_MATCH;
    }

    /* Delete all neighbors bound to this peer group */
    OVSREC_BGP_NEIGHBOR_FOR_EACH_SAFE(bgpn, bgpn_next, idl) {
	if (object_is_neighbor(bgpn) && (bgpn->bgp_peer_group == peer_group)) {
            /* Remove the neighbor reference from BGP Router */
            bgp_neighbor_remove_for_matching_peer_group_from_bgp_router(
                                                           bgp_router_context,
                                                           bgpn);
	    ovsrec_bgp_neighbor_delete(bgpn);
	}
    }

    /* Delete the peer-group */
    bgp_neighbor_peer_group_remove_from_bgp_router(bgp_router_context,
                                                  peer_group,
                                                  name);
    ovsrec_bgp_neighbor_delete(peer_group);
    return CMD_SUCCESS;
}

static int
cli_no_neighbor_cmd_execute (char *vrf_name, const char *peer_str)
{
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    /* peer/neighbor */
    if (string_is_an_ip_address(peer_str)) {
	ovs_bgp_neighbor =
	    get_bgp_neighbor_with_bgp_router_and_ipaddr
		(bgp_router_context, peer_str);
	if (ovs_bgp_neighbor) {
	    ovsrec_bgp_neighbor_delete(ovs_bgp_neighbor);
	}
    /* peer group */
    } else {
        int res = delete_neighbor_peer_group(bgp_router_context, peer_str);
        if (res == CMD_ERR_NO_MATCH) {
            ERRONEOUS_DB_TXN(txn, "peer group does not exist.");
        }
    }
        /* Delete the neighbor/peer-group reference from BGP Router */
        bgp_neighbor_peer_group_remove_from_bgp_router(bgp_router_context,
                                                       ovs_bgp_neighbor,
                                                       peer_str);

    /* done */
    END_DB_TXN(txn);
}

DEFUN (no_neighbor,
       no_neighbor_cmd,
       NO_NEIGHBOR_CMD2,
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2)
{
    return cli_no_neighbor_cmd_execute(NULL, argv[0]);
}

ALIAS (no_neighbor,
       no_neighbor_remote_as_cmd,
       NO_NEIGHBOR_CMD "remote-as " CMD_AS_RANGE,
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Specify a BGP neighbor\n"
       AS_STR)

static int
cli_no_neighbor_peer_group_cmd_execute (char *vrf_name, const char *name)
{
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *peer_group;
    struct ovsdb_idl_txn *txn;
    int res;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    /* Delete the peer-group and also neighbors belonging to this peer-group */
    res = delete_neighbor_peer_group(bgp_router_context, name);
    if (res == CMD_ERR_NO_MATCH) {
        ERRONEOUS_DB_TXN(txn, "peer-group does not exist.");
    }

    /* done */
    END_DB_TXN(txn);
}

void
bgp_neighbor_peer_group_insert_to_bgp_router(struct ovsrec_bgp_router *bgp_router_context,
                                    struct ovsrec_bgp_neighbor *ovs_bgp_neighbor_peer_group,
                                    char *name)
{
    struct ovsrec_bgp_neighbor **bgp_neighbor_peer_group_list;
    char **bgp_neighbor_peer_name_list;
    int i = 0;

    bgp_neighbor_peer_name_list = xmalloc(80 * (bgp_router_context->n_bgp_neighbors + 1));
    bgp_neighbor_peer_group_list = xmalloc(sizeof * bgp_router_context->value_bgp_neighbors *
                              (bgp_router_context->n_bgp_neighbors + 1));
    for (i = 0; i < bgp_router_context->n_bgp_neighbors; i++) {
        bgp_neighbor_peer_name_list[i] = bgp_router_context->key_bgp_neighbors[i];
        bgp_neighbor_peer_group_list[i] = bgp_router_context->value_bgp_neighbors[i];
    }
    bgp_neighbor_peer_name_list[bgp_router_context->n_bgp_neighbors] = name;
    bgp_neighbor_peer_group_list[bgp_router_context->n_bgp_neighbors] =
                                                     ovs_bgp_neighbor_peer_group;
    ovsrec_bgp_router_set_bgp_neighbors(bgp_router_context, bgp_neighbor_peer_name_list,
                                        bgp_neighbor_peer_group_list,
                                        (bgp_router_context->n_bgp_neighbors + 1));
    free(bgp_neighbor_peer_name_list);
    free(bgp_neighbor_peer_group_list);
}

static int
cli_neighbor_peer_group_cmd_execute (char *vrf_name, const char *groupName)
{
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_peer_group;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_peer_group =
	get_bgp_peer_group_with_bgp_router_and_name
	    (bgp_router_context, groupName);
    if (ovs_bgp_peer_group) {
        ABORT_DB_TXN(txn, "peer group already exists");
    }

    ovs_bgp_peer_group = ovsrec_bgp_neighbor_insert(txn);
    if (!ovs_bgp_peer_group) {
        ERRONEOUS_DB_TXN(txn,
	    "bgp neighbor (peer group) object creation failed");
    }

    define_object_as_a_bgp_peer_group(ovs_bgp_peer_group);

    /* Add peer group reference to the BGP Router table */
    bgp_neighbor_peer_group_insert_to_bgp_router(bgp_router_context, ovs_bgp_peer_group, groupName);

    /* done */
    END_DB_TXN(txn);
}

DEFUN (neighbor_peer_group,
       neighbor_peer_group_cmd,
       "neighbor WORD peer-group",
       NEIGHBOR_STR
       "Neighbor tag\n"
       "Configure peer-group\n")
{
    return cli_neighbor_peer_group_cmd_execute(NULL, argv[0]);
}

DEFUN (no_neighbor_peer_group,
       no_neighbor_peer_group_cmd,
       "no neighbor WORD peer-group",
       NO_STR
       NEIGHBOR_STR
       "Neighbor tag\n"
       "Configure peer-group\n")
{
    return cli_no_neighbor_peer_group_cmd_execute(NULL, argv[0]);
}

DEFUN (no_neighbor_peer_group_remote_as,
       no_neighbor_peer_group_remote_as_cmd,
       "no neighbor WORD remote-as " CMD_AS_RANGE,
       NO_STR
       NEIGHBOR_STR
       "Neighbor tag\n"
       "Specify a BGP neighbor\n"
       AS_STR)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_local_as,
       neighbor_local_as_cmd,
       NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE,
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_local_as_no_prepend,
       neighbor_local_as_no_prepend_cmd,
       NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE " no-prepend",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n"
       "Do not prepend local-as to updates from ebgp peers\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}
DEFUN (neighbor_local_as_no_prepend_replace_as,
       neighbor_local_as_no_prepend_replace_as_cmd,
       NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE " no-prepend replace-as",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n"
       "Do not prepend local-as to updates from ebgp peers\n"
       "Do not prepend local-as to updates from ibgp peers\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_local_as,
       no_neighbor_local_as_cmd,
       NO_NEIGHBOR_CMD2 "local-as",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_local_as,
       no_neighbor_local_as_val_cmd,
       NO_NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE,
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n")

ALIAS (no_neighbor_local_as,
       no_neighbor_local_as_val2_cmd,
       NO_NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE " no-prepend",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n"
       "Do not prepend local-as to updates from ebgp peers\n")

ALIAS (no_neighbor_local_as,
       no_neighbor_local_as_val3_cmd,
       NO_NEIGHBOR_CMD2 "local-as " CMD_AS_RANGE " no-prepend replace-as",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify a local-as number\n"
       "AS number used as local AS\n"
       "Do not prepend local-as to updates from ebgp peers\n"
       "Do not prepend local-as to updates from ibgp peers\n")

static int
cli_neighbor_password_execute(char *vrf_name, int argc, const char *argv[])
{
    const char *ip_addr = argv[0];
    char *str;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (ovs_bgp_neighbor) {
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_password(ovs_bgp_neighbor, argv[1]);
    }
    END_DB_TXN(txn);
}

DEFUN (neighbor_password,
       neighbor_password_cmd,
       NEIGHBOR_CMD2 "password LINE",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Set a password\n"
       "The password\n")
{
    if (argc != 2) {
       vty_out(vty, "\n%%Insufficient parameters, neighbor <ipaddr> password <pwd>\n");
       return CMD_WARNING;
    }

    return cli_neighbor_password_execute(NULL, argc, argv);
}

DEFUN (no_neighbor_password,
       no_neighbor_password_cmd,
       NO_NEIGHBOR_CMD2 "password",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Set a password\n")
{
    const char *ip_addr = argv[0];
    char *str;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;
    char * vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (ovs_bgp_neighbor) {
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_password(ovs_bgp_neighbor, NULL);
    }
    END_DB_TXN(txn);
}

DEFUN (neighbor_activate,
       neighbor_activate_cmd,
       NEIGHBOR_CMD2 "activate",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Enable the Address Family for this Neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_activate,
       no_neighbor_activate_cmd,
       NO_NEIGHBOR_CMD2 "activate",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Enable the Address Family for this Neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/*
 * If the peer does not exist, create it first and then bind it
 * to the peer group.  If it already exists, it must not already
 * be bound to another peer group.  Also, the peer group MUST
 * already have a remote-as configured before any peers can be created
 * and bound to it.  All these are checked below.
 */
static int
cli_neighbor_set_peer_group_cmd_execute(char *vrf_name, const char *ip_addr,
                                        const char *peer_group)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_neighbor *ovs_bgp_peer_group;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    /* This *MUST* be already available */
    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row,
                                                        (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    /* This *MUST* be already available */
    ovs_bgp_peer_group = get_bgp_peer_group_with_bgp_router_and_name(
                                bgp_router_context, peer_group);
    if (!ovs_bgp_peer_group) {
        ABORT_DB_TXN(txn, "Configure the peer-group first.");
    }

    /* this may or may not be present */
    ovs_bgp_neighbor = get_bgp_neighbor_with_bgp_router_and_ipaddr(
                            bgp_router_context, ip_addr);

    /*
     * Create a peer first and assign values from peer-group.
     * However, to be able to do that, the peer group MUST
     * have its remote-as defined.  If not, a new peer cannot
     * be created & assigned to the peer group.  If a peer
     * already exists however, it CAN be bound to a peer group
     * which may not have a remote-as.
     */
    if (!ovs_bgp_neighbor) {
        ovs_bgp_neighbor = ovsrec_bgp_neighbor_insert(txn);
        if (!ovs_bgp_neighbor) {
           ERRONEOUS_DB_TXN(txn, "bgp neighbor object creation failed");
        }
        define_object_as_a_bgp_peer(ovs_bgp_neighbor);

        /* Add peer reference to the BGP Router table */
        bgp_neighbor_peer_group_insert_to_bgp_router(bgp_router_context,
                                                     ovs_bgp_neighbor, ip_addr);
    } else {
        if (ovs_bgp_neighbor->bgp_peer_group) {
            if (ovs_bgp_neighbor->bgp_peer_group == ovs_bgp_peer_group) {
                ABORT_DB_TXN(txn, "Configuration already exists.");
            } else {
                ERRONEOUS_DB_TXN(txn, "Cannot change the peer-group. "
                                      "Deconfigure first");
            }
        }
    }

    /* if peer group has a remote-as, it becomes primary */
    if (ovs_bgp_peer_group->n_remote_as > 0) {
        ovsrec_bgp_neighbor_set_remote_as(ovs_bgp_neighbor,
                                          ovs_bgp_peer_group->remote_as, 1);
    } else if (!ovs_bgp_neighbor->n_remote_as) {
        /* no remote-as in peer group or peer, unacceptable */
        ERRONEOUS_DB_TXN(txn, "Specify peer remote AS or peer-group "
                              "remote AS first");
    }

    /* make this peer bound to the peer group */
    ovsrec_bgp_neighbor_set_bgp_peer_group(ovs_bgp_neighbor,
                                           ovs_bgp_peer_group);

    /* done */
    END_DB_TXN(txn);
}

DEFUN (neighbor_set_peer_group,
       neighbor_set_peer_group_cmd,
       NEIGHBOR_CMD "peer-group WORD",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Member of the peer-group\n"
       "peer-group name\n")
{
    return cli_neighbor_set_peer_group_cmd_execute(NULL, argv[0], argv[1]);
}

void
bgp_neighbor_peer_group_remove_from_bgp_router(struct ovsrec_bgp_router *bgp_router_context,
                                    struct ovsrec_bgp_neighbor *ovs_bgp_neighbor_peer,
                                    char *name)
{
    struct ovsrec_bgp_neighbor **bgp_neighbor_peer_group_list;
    char **bgp_neighbor_peer_name_list;
    int i, j;

    bgp_neighbor_peer_name_list = xmalloc(80 * (bgp_router_context->n_bgp_neighbors - 1));
    bgp_neighbor_peer_group_list = xmalloc(sizeof * bgp_router_context->value_bgp_neighbors *
                              (bgp_router_context->n_bgp_neighbors - 1));
    for (i = 0, j = 0; i < bgp_router_context->n_bgp_neighbors; i++) {
        if (0 != strcmp(bgp_router_context->key_bgp_neighbors[i], name)) {
            bgp_neighbor_peer_name_list[j] = bgp_router_context->key_bgp_neighbors[i];
            bgp_neighbor_peer_group_list[j] = bgp_router_context->value_bgp_neighbors[i];
            j++;
        }
    }
    ovsrec_bgp_router_set_bgp_neighbors(bgp_router_context, bgp_neighbor_peer_name_list,
                                        bgp_neighbor_peer_group_list,
                                        (bgp_router_context->n_bgp_neighbors - 1));
    free(bgp_neighbor_peer_name_list);
    free(bgp_neighbor_peer_group_list);
}

static int
cli_no_neighbor_set_peer_group_cmd_execute (char *vrf_name, const char *ip_addr,
    const char *peer_group)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbors_peer_group;
    struct ovsdb_idl_txn *txn;
    char *name;
    int i = 0;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
	get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (!ovs_bgp_neighbor) {
        ERRONEOUS_DB_TXN(txn, "neighbor does not exist");
    }

    /* Check if the existing peer is already configured with a peer group */
    ovs_bgp_neighbors_peer_group = ovs_bgp_neighbor->bgp_peer_group;
    if (ovs_bgp_neighbors_peer_group) {
	/*
	** if peer group had a remote-as, peer gets deleted,
	** else gets simply disassociated from the peer group.
	*/
        if (ovs_bgp_neighbors_peer_group->n_remote_as) {
            /* Remove the neighbor reference from BGP Router */
            bgp_neighbor_peer_group_remove_from_bgp_router(bgp_router_context,
                                                           ovs_bgp_neighbor,
                                                           ip_addr);
            ovsrec_bgp_neighbor_delete(ovs_bgp_neighbor);
        } else {
            ovsrec_bgp_neighbor_set_bgp_peer_group(ovs_bgp_neighbor, NULL);
        }

    } else {
        ABORT_DB_TXN(txn, "Neighbor is not in a peer group\n");
    }

    /* done */
    END_DB_TXN(txn);
}

DEFUN (no_neighbor_set_peer_group,
       no_neighbor_set_peer_group_cmd,
       NO_NEIGHBOR_CMD "peer-group WORD",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Member of the peer-group\n"
       "peer-group name\n")
{
    return cli_no_neighbor_set_peer_group_cmd_execute(NULL, argv[0], argv[1]);
}

/* neighbor passive. */
DEFUN (neighbor_passive,
       neighbor_passive_cmd,
       NEIGHBOR_CMD2 "passive",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Don't send open messages to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_passive,
       no_neighbor_passive_cmd,
       NO_NEIGHBOR_CMD2 "passive",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Don't send open messages to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor shutdown. */
DEFUN (neighbor_shutdown,
       neighbor_shutdown_cmd,
       NEIGHBOR_CMD2 "shutdown",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Administratively shut down this neighbor\n")
{
    const char *ip_addr = argv[0];
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;
    const bool shutdown = true;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
    ovs_bgp_neighbor =
	get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (!ovs_bgp_neighbor) {
        ABORT_DB_TXN(txn, "no neighbor");
    }
    if (ovs_bgp_neighbor->shutdown) {
	    ABORT_DB_TXN(txn, "no op command");
    }
    ovsrec_bgp_neighbor_set_shutdown(ovs_bgp_neighbor, &shutdown, 1);

    /* done */
    END_DB_TXN(txn);
}

DEFUN (no_neighbor_shutdown,
       no_neighbor_shutdown_cmd,
       NO_NEIGHBOR_CMD2 "shutdown",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Administratively shut down this neighbor\n")
{
    const char *ip_addr = argv[0];
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
    ovs_bgp_neighbor =
	get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (!ovs_bgp_neighbor) {
        ABORT_DB_TXN(txn, "no neighbor");
    }
    if (!ovs_bgp_neighbor->shutdown) {
	    ABORT_DB_TXN(txn, "no op command");
    }
    ovsrec_bgp_neighbor_set_shutdown(ovs_bgp_neighbor, NULL, 0);

    /* done */
    END_DB_TXN(txn);
}

/* Deprecated neighbor capability route-refresh. */
DEFUN_DEPRECATED (neighbor_capability_route_refresh,
		  neighbor_capability_route_refresh_cmd,
		  NEIGHBOR_CMD2 "capability route-refresh",
		  NEIGHBOR_STR
		  NEIGHBOR_ADDR_STR2
		  "Advertise capability to the peer\n"
		  "Advertise route-refresh capability to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN_DEPRECATED (no_neighbor_capability_route_refresh,
		  no_neighbor_capability_route_refresh_cmd,
		  NO_NEIGHBOR_CMD2 "capability route-refresh",
		  NO_STR
		  NEIGHBOR_STR
		  NEIGHBOR_ADDR_STR2
		  "Advertise capability to the peer\n"
		  "Advertise route-refresh capability to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor capability dynamic. */
DEFUN (neighbor_capability_dynamic,
       neighbor_capability_dynamic_cmd,
       NEIGHBOR_CMD2 "capability dynamic",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Advertise capability to the peer\n"
       "Advertise dynamic capability to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_capability_dynamic,
       no_neighbor_capability_dynamic_cmd,
       NO_NEIGHBOR_CMD2 "capability dynamic",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Advertise capability to the peer\n"
       "Advertise dynamic capability to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor dont-capability-negotiate */
DEFUN (neighbor_dont_capability_negotiate,
       neighbor_dont_capability_negotiate_cmd,
       NEIGHBOR_CMD2 "dont-capability-negotiate",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Do not perform capability negotiation\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_dont_capability_negotiate,
       no_neighbor_dont_capability_negotiate_cmd,
       NO_NEIGHBOR_CMD2 "dont-capability-negotiate",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Do not perform capability negotiation\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor capability orf prefix-list. */
DEFUN (neighbor_capability_orf_prefix,
       neighbor_capability_orf_prefix_cmd,
       NEIGHBOR_CMD2 "capability orf prefix-list (both|send|receive)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Advertise capability to the peer\n"
       "Advertise ORF capability to the peer\n"
       "Advertise prefixlist ORF capability to this neighbor\n"
       "Capability to SEND and RECEIVE the ORF to/from this neighbor\n"
       "Capability to RECEIVE the ORF from this neighbor\n"
       "Capability to SEND the ORF to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_capability_orf_prefix,
       no_neighbor_capability_orf_prefix_cmd,
       NO_NEIGHBOR_CMD2 "capability orf prefix-list (both|send|receive)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Advertise capability to the peer\n"
       "Advertise ORF capability to the peer\n"
       "Advertise prefixlist ORF capability to this neighbor\n"
       "Capability to SEND and RECEIVE the ORF to/from this neighbor\n"
       "Capability to RECEIVE the ORF from this neighbor\n"
       "Capability to SEND the ORF to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor next-hop-self. */
DEFUN (neighbor_nexthop_self,
       neighbor_nexthop_self_cmd,
       NEIGHBOR_CMD2 "next-hop-self {all}",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Disable the next hop calculation for this neighbor\n"
       "Apply also to ibgp-learned routes when acting as a route reflector\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_nexthop_self,
       no_neighbor_nexthop_self_cmd,
       NO_NEIGHBOR_CMD2 "next-hop-self {all}",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Disable the next hop calculation for this neighbor\n"
       "Apply also to ibgp-learned routes when acting as a route reflector\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

static int
cli_neighbor_remove_private_as_cmd_execute (struct vty *vty,
    int argc,const char *argv[])
{
    const char *ip_addr = argv[0];
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;
    const bool remove_private_as = true;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router not configured");
    }
    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (!ovs_bgp_neighbor) {
        ABORT_DB_TXN(txn, "no neighbor configured");
    }
    if (ovs_bgp_neighbor->remove_private_as) {
        ABORT_DB_TXN(txn, "command exists");
    }
    ovsrec_bgp_neighbor_set_remove_private_as(ovs_bgp_neighbor, &remove_private_as, 1);
    END_DB_TXN(txn);
}

/* neighbor remove-private-AS. */
DEFUN (neighbor_remove_private_as,
       neighbor_remove_private_as_cmd,
       NEIGHBOR_CMD2 "remove-private-AS",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Remove private AS number from outbound updates\n")
{
    if (argc != 1) {
        VLOG_ERR("\nargc should be 1, it is %d; %s: %d\n",
            argc, __FILE__, __LINE__);
        return CMD_WARNING;
    }
    return cli_neighbor_remove_private_as_cmd_execute(vty, argc, argv);
}

static int
cli_no_neighbor_remove_private_as_cmd_execute (struct vty *vty,
    int argc,const char *argv[])
{
    const char *ip_addr = argv[0];
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router not configured");
    }
    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (!ovs_bgp_neighbor) {
        ABORT_DB_TXN(txn, "no neighbor configured");
    }
    if (!ovs_bgp_neighbor->remove_private_as) {
        ABORT_DB_TXN(txn, "command exists");
    }
    ovsrec_bgp_neighbor_set_remove_private_as(ovs_bgp_neighbor,NULL, 0);
    END_DB_TXN(txn);
}

DEFUN (no_neighbor_remove_private_as,
       no_neighbor_remove_private_as_cmd,
       NO_NEIGHBOR_CMD2 "remove-private-AS",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Remove private AS number from outbound updates\n")
{
     if (argc != 1) {
        VLOG_ERR("\nargc should be 1, it is %d; %s: %d\n",
            argc, __FILE__, __LINE__);
        return CMD_WARNING;
    }
    return cli_no_neighbor_remove_private_as_cmd_execute(vty, argc, argv);
}

/* neighbor send-community. */
DEFUN (neighbor_send_community,
       neighbor_send_community_cmd,
       NEIGHBOR_CMD2 "send-community",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Send Community attribute to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_send_community,
       no_neighbor_send_community_cmd,
       NO_NEIGHBOR_CMD2 "send-community",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Send Community attribute to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor send-community extended. */
DEFUN (neighbor_send_community_type,
       neighbor_send_community_type_cmd,
       NEIGHBOR_CMD2 "send-community (both|extended|standard)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Send Community attribute to this neighbor\n"
       "Send Standard and Extended Community attributes\n"
       "Send Extended Community attributes\n"
       "Send Standard Community attributes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_send_community_type,
       no_neighbor_send_community_type_cmd,
       NO_NEIGHBOR_CMD2 "send-community (both|extended|standard)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Send Community attribute to this neighbor\n"
       "Send Standard and Extended Community attributes\n"
       "Send Extended Community attributes\n"
       "Send Standard Community attributes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

static int
cli_neighbor_soft_reconfiguration_inbound_cmd_execute (char *vrf_name, const char *ip_addr)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (bgp_router_context) {
        ovs_bgp_neighbor =
	    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
        if (ovs_bgp_neighbor) {
            if (ovs_bgp_neighbor->inbound_soft_reconfiguration) {
                ABORT_DB_TXN(txn, "inbound_soft_reconfiguration already set");
            } else {
                const bool inb_soft_rcfg = true;

                ovsrec_bgp_neighbor_set_inbound_soft_reconfiguration
		    (ovs_bgp_neighbor, &inb_soft_rcfg, 1);
            }
        } else {
            ABORT_DB_TXN(txn, "no neighbor");
        }
    } else {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    /* done */
    END_DB_TXN(txn);
}

/* neighbor soft-reconfig. */
DEFUN (neighbor_soft_reconfiguration,
       neighbor_soft_reconfiguration_cmd,
       NEIGHBOR_CMD2 "soft-reconfiguration inbound",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Per neighbor soft reconfiguration\n"
       "Allow inbound soft reconfiguration for this neighbor\n")
{
    return cli_neighbor_soft_reconfiguration_inbound_cmd_execute(NULL, argv[0]);
}

static int
cli_no_neighbor_soft_reconfiguration_inbound_cmd_execute(char *vrf_name, const char *ip_addr)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (bgp_router_context) {
        ovs_bgp_neighbor =
	    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
        if (ovs_bgp_neighbor) {
            if (!ovs_bgp_neighbor->inbound_soft_reconfiguration) {
                ABORT_DB_TXN(txn, "inbound_soft_reconfiguration doesn't exist");
            } else {
                ovsrec_bgp_neighbor_set_inbound_soft_reconfiguration
		    (ovs_bgp_neighbor, NULL, 0);
            }
        } else {
            ABORT_DB_TXN(txn, "no neighbor");
        }
    } else {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    /* done */
    END_DB_TXN(txn);
}

DEFUN (no_neighbor_soft_reconfiguration,
       no_neighbor_soft_reconfiguration_cmd,
       NO_NEIGHBOR_CMD2 "soft-reconfiguration inbound",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Per neighbor soft reconfiguration\n"
       "Allow inbound soft reconfiguration for this neighbor\n")
{
    return cli_no_neighbor_soft_reconfiguration_inbound_cmd_execute(NULL, argv[0]);
}

DEFUN (neighbor_route_reflector_client,
       neighbor_route_reflector_client_cmd,
       NEIGHBOR_CMD2 "route-reflector-client",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure a neighbor as Route Reflector client\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_route_reflector_client,
       no_neighbor_route_reflector_client_cmd,
       NO_NEIGHBOR_CMD2 "route-reflector-client",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure a neighbor as Route Reflector client\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor route-server-client. */
DEFUN (neighbor_route_server_client,
       neighbor_route_server_client_cmd,
       NEIGHBOR_CMD2 "route-server-client",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure a neighbor as Route Server client\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_route_server_client,
       no_neighbor_route_server_client_cmd,
       NO_NEIGHBOR_CMD2 "route-server-client",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure a neighbor as Route Server client\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_nexthop_local_unchanged,
       neighbor_nexthop_local_unchanged_cmd,
       NEIGHBOR_CMD2 "nexthop-local unchanged",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure treatment of outgoing link-local nexthop attribute\n"
       "Leave link-local nexthop unchanged for this peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_nexthop_local_unchanged,
       no_neighbor_nexthop_local_unchanged_cmd,
       NO_NEIGHBOR_CMD2 "nexthop-local unchanged",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Configure treatment of outgoing link-local-nexthop attribute\n"
       "Leave link-local nexthop unchanged for this peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_attr_unchanged,
       neighbor_attr_unchanged_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_attr_unchanged1,
       neighbor_attr_unchanged1_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged (as-path|next-hop|med)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_attr_unchanged2,
       neighbor_attr_unchanged2_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged as-path (next-hop|med)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_attr_unchanged3,
       neighbor_attr_unchanged3_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged next-hop (as-path|med)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "As-path attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_attr_unchanged4,
       neighbor_attr_unchanged4_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged med (as-path|next-hop)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "As-path attribute\n"
       "Nexthop attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged5_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged as-path next-hop med",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged6_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged as-path med next-hop",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Med attribute\n"
       "Nexthop attribute\n")

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged7_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged next-hop med as-path",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "Med attribute\n"
       "As-path attribute\n")

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged8_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged next-hop as-path med",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "As-path attribute\n"
       "Med attribute\n")

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged9_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged med next-hop as-path",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "Nexthop attribute\n"
       "As-path attribute\n")

ALIAS (neighbor_attr_unchanged,
       neighbor_attr_unchanged10_cmd,
       NEIGHBOR_CMD2 "attribute-unchanged med as-path next-hop",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "As-path attribute\n"
       "Nexthop attribute\n")

DEFUN (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_attr_unchanged1,
       no_neighbor_attr_unchanged1_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged (as-path|next-hop|med)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_attr_unchanged2,
       no_neighbor_attr_unchanged2_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged as-path (next-hop|med)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_attr_unchanged3,
       no_neighbor_attr_unchanged3_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged next-hop (as-path|med)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "As-path attribute\n"
       "Med attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_attr_unchanged4,
       no_neighbor_attr_unchanged4_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged med (as-path|next-hop)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "As-path attribute\n"
       "Nexthop attribute\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged5_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged as-path next-hop med",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Nexthop attribute\n"
       "Med attribute\n")

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged6_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged as-path med next-hop",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "As-path attribute\n"
       "Med attribute\n"
       "Nexthop attribute\n")

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged7_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged next-hop med as-path",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "Med attribute\n"
       "As-path attribute\n")

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged8_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged next-hop as-path med",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Nexthop attribute\n"
       "As-path attribute\n"
       "Med attribute\n")

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged9_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged med next-hop as-path",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "Nexthop attribute\n"
       "As-path attribute\n")

ALIAS (no_neighbor_attr_unchanged,
       no_neighbor_attr_unchanged10_cmd,
       NO_NEIGHBOR_CMD2 "attribute-unchanged med as-path next-hop",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP attribute is propagated unchanged to this neighbor\n"
       "Med attribute\n"
       "As-path attribute\n"
       "Nexthop attribute\n")

/* For old version Zebra compatibility.  */
DEFUN_DEPRECATED (neighbor_transparent_as,
		  neighbor_transparent_as_cmd,
		  NEIGHBOR_CMD "transparent-as",
		  NEIGHBOR_STR
		  NEIGHBOR_ADDR_STR
		  "Do not append my AS number even peer is EBGP peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN_DEPRECATED (neighbor_transparent_nexthop,
		  neighbor_transparent_nexthop_cmd,
		  NEIGHBOR_CMD "transparent-nexthop",
		  NEIGHBOR_STR
		  NEIGHBOR_ADDR_STR
		  "Do not change nexthop even peer is EBGP peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor ebgp-multihop. */
DEFUN (neighbor_ebgp_multihop,
       neighbor_ebgp_multihop_cmd,
       NEIGHBOR_CMD2 "ebgp-multihop",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Allow EBGP neighbors not on directly connected networks\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_ebgp_multihop_ttl,
       neighbor_ebgp_multihop_ttl_cmd,
       NEIGHBOR_CMD2 "ebgp-multihop <1-255>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Allow EBGP neighbors not on directly connected networks\n"
       "maximum hop count\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_ebgp_multihop,
       no_neighbor_ebgp_multihop_cmd,
       NO_NEIGHBOR_CMD2 "ebgp-multihop",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Allow EBGP neighbors not on directly connected networks\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_ebgp_multihop,
       no_neighbor_ebgp_multihop_ttl_cmd,
       NO_NEIGHBOR_CMD2 "ebgp-multihop <1-255>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Allow EBGP neighbors not on directly connected networks\n"
       "maximum hop count\n")

/* disable-connected-check */
DEFUN (neighbor_disable_connected_check,
       neighbor_disable_connected_check_cmd,
       NEIGHBOR_CMD2 "disable-connected-check",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "one-hop away EBGP peer using loopback address\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_disable_connected_check,
       no_neighbor_disable_connected_check_cmd,
       NO_NEIGHBOR_CMD2 "disable-connected-check",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "one-hop away EBGP peer using loopback address\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* Enforce multihop.  */
ALIAS (neighbor_disable_connected_check,
       neighbor_enforce_multihop_cmd,
       NEIGHBOR_CMD2 "enforce-multihop",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Enforce EBGP neighbors perform multihop\n")

/* Enforce multihop.  */
ALIAS (no_neighbor_disable_connected_check,
       no_neighbor_enforce_multihop_cmd,
       NO_NEIGHBOR_CMD2 "enforce-multihop",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Enforce EBGP neighbors perform multihop\n")

static int
cli_neighbor_description_execute(int argc,const char *argv[])
{
    const char *ip_addr = argv[0];
    char *str;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_vrf *vrf_row;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (ovs_bgp_neighbor) {
        str = argv_concat(argv, argc, 1);
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_description(ovs_bgp_neighbor, str);
        XFREE (MTYPE_TMP, str);
    }
    END_DB_TXN(txn);
}

DEFUN (neighbor_description,
       neighbor_description_cmd,
       NEIGHBOR_CMD2 "description .LINE",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Neighbor specific description\n"
       "Up to 80 characters describing this neighbor\n")
{
    if (argc == 1) {
        vty_out(vty, "\n%%Insufficient parameters: neighbor <ipaddr> description <desc>\n");
        return CMD_WARNING;
    }

    return cli_neighbor_description_execute(argc, argv);
}

DEFUN (no_neighbor_description,
       no_neighbor_description_cmd,
       NO_NEIGHBOR_CMD2 "description",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Neighbor specific description\n")
{
    const char *ip_addr = argv[0];
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);
    if (ovs_bgp_neighbor) {
        ovsrec_bgp_neighbor_set_description(ovs_bgp_neighbor, NULL);
    }
    END_DB_TXN(txn);
}

ALIAS (no_neighbor_description,
       no_neighbor_description_val_cmd,
       NO_NEIGHBOR_CMD2 "description .LINE",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Neighbor specific description\n"
       "Up to 80 characters describing this neighbor\n")

#if 0
DEFUN (neighbor_update_source,
       neighbor_update_source_cmd,
       NEIGHBOR_CMD2 "update-source " BGP_UPDATE_SOURCE_STR,
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Source of routing updates\n"
       BGP_UPDATE_SOURCE_HELP_STR)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}
#endif

DEFUN (no_neighbor_update_source,
       no_neighbor_update_source_cmd,
       NO_NEIGHBOR_CMD2 "update-source",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Source of routing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* neighbor default-originate. */
DEFUN (neighbor_default_originate,
       neighbor_default_originate_cmd,
       NEIGHBOR_CMD2 "default-originate",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Originate default route to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_default_originate_rmap,
       neighbor_default_originate_rmap_cmd,
       NEIGHBOR_CMD2 "default-originate route-map WORD",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Originate default route to this neighbor\n"
       "Route-map to specify criteria to originate default\n"
       "route-map name\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_default_originate,
       no_neighbor_default_originate_cmd,
       NO_NEIGHBOR_CMD2 "default-originate",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Originate default route to this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_default_originate,
       no_neighbor_default_originate_rmap_cmd,
       NO_NEIGHBOR_CMD2 "default-originate route-map WORD",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Originate default route to this neighbor\n"
       "Route-map to specify criteria to originate default\n"
       "route-map name\n")

/* Set specified peer's BGP port.  */
DEFUN (neighbor_port,
       neighbor_port_cmd,
       NEIGHBOR_CMD "port <0-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Neighbor's BGP port\n"
       "TCP port number\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_port,
       no_neighbor_port_cmd,
       NO_NEIGHBOR_CMD "port",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Neighbor's BGP port\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_port,
       no_neighbor_port_val_cmd,
       NO_NEIGHBOR_CMD "port <0-65535>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Neighbor's BGP port\n"
       "TCP port number\n")

DEFUN (neighbor_weight,
       neighbor_weight_cmd,
       NEIGHBOR_CMD2 "weight <0-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Set default weight for routes from this neighbor\n"
       "default weight\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_weight,
       no_neighbor_weight_cmd,
       NO_NEIGHBOR_CMD2 "weight",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Set default weight for routes from this neighbor\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_weight,
       no_neighbor_weight_val_cmd,
       NO_NEIGHBOR_CMD2 "weight <0-65535>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Set default weight for routes from this neighbor\n"
       "default weight\n")

/* Override capability negotiation. */
DEFUN (neighbor_override_capability,
       neighbor_override_capability_cmd,
       NEIGHBOR_CMD2 "override-capability",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Override capability negotiation result\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_override_capability,
       no_neighbor_override_capability_cmd,
       NO_NEIGHBOR_CMD2 "override-capability",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Override capability negotiation result\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_strict_capability,
       neighbor_strict_capability_cmd,
       NEIGHBOR_CMD "strict-capability-match",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Strict capability negotiation match\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_strict_capability,
       no_neighbor_strict_capability_cmd,
       NO_NEIGHBOR_CMD "strict-capability-match",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Strict capability negotiation match\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

typedef struct timer_val {
    int64_t keepalive;
    int64_t holdtime;
} timer_val_t;

static int
cli_neighbor_timers_execute (char *vrf_name, int argc, const char *argv[])
{
    const char *ip_addr = argv[0];
    char *key_timers[2];
    timer_val_t tim_val;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;

    VTY_GET_INTEGER_RANGE ("Keepalive", tim_val.keepalive, argv[1], 0, 65535);
    VTY_GET_INTEGER_RANGE ("Holdtime", tim_val.holdtime, argv[2], 0, 65535);

     if (tim_val.holdtime < 3 && tim_val.holdtime != 0) {
        vty_out(vty, "\n%d%s%dHold time cannot be 1 or 2\n",
        argc, __FILE__, __LINE__);
        return CMD_WARNING;
    }
    tim_val.keepalive = (tim_val.keepalive < (tim_val.holdtime / 3) ? tim_val.keepalive : (tim_val.holdtime / 3));

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
        ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);

    if (ovs_bgp_neighbor) {
        key_timers[0] = OVSDB_BGP_TIMER_KEEPALIVE;
        key_timers[1] = OVSDB_BGP_TIMER_HOLDTIME;
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_timers(ovs_bgp_neighbor, key_timers, (int64_t *)&tim_val, 2);
    }
    END_DB_TXN(txn);

}

DEFUN (neighbor_timers,
       neighbor_timers_cmd,
       NEIGHBOR_CMD2 "timers <0-65535> <0-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP per neighbor timers\n"
       "Keepalive interval\n"
       "Holdtime\n")
{
    if (argc != 3) {
        vty_out(vty, "\n%%Insufficient parameters, neighbor <ipaddr> timers <keepalive><holdtime>");
        return CMD_WARNING;
    }
    return cli_neighbor_timers_execute(NULL, argc, argv);
}

DEFUN (no_neighbor_timers,
       no_neighbor_timers_cmd,
       NO_NEIGHBOR_CMD2 "timers",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "BGP per neighbor timers\n")
{
    const char *ip_addr = argv[0];
    char *key_timers[2];
    timer_val_t tim_val;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }
        ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);

    if (ovs_bgp_neighbor) {
        key_timers[0] = "Keepalive";
        key_timers[1] = "Holdtimer";
        tim_val.keepalive = 0;
        tim_val.holdtime = 0;
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_timers(ovs_bgp_neighbor, key_timers, (int64_t *)&tim_val,0);
    }
    END_DB_TXN(txn);

}

DEFUN (neighbor_timers_connect,
       neighbor_timers_connect_cmd,
       NEIGHBOR_CMD "timers connect <0-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "BGP per neighbor timers\n"
       "BGP connect timer\n"
       "Connect timer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_timers_connect,
       no_neighbor_timers_connect_cmd,
       NO_NEIGHBOR_CMD "timers connect",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "BGP per neighbor timers\n"
       "BGP connect timer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_timers_connect,
       no_neighbor_timers_connect_val_cmd,
       NO_NEIGHBOR_CMD "timers connect <0-65535>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "BGP per neighbor timers\n"
       "BGP connect timer\n"
       "Connect timer\n")

DEFUN (neighbor_advertise_interval,
       neighbor_advertise_interval_cmd,
       NEIGHBOR_CMD "advertisement-interval <0-600>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Minimum interval between sending BGP routing updates\n"
       "time in seconds\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_advertise_interval,
       no_neighbor_advertise_interval_cmd,
       NO_NEIGHBOR_CMD "advertisement-interval",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Minimum interval between sending BGP routing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_advertise_interval,
       no_neighbor_advertise_interval_val_cmd,
       NO_NEIGHBOR_CMD "advertisement-interval <0-600>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Minimum interval between sending BGP routing updates\n"
       "time in seconds\n")

DEFUN (neighbor_interface,
       neighbor_interface_cmd,
       NEIGHBOR_CMD "interface WORD",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Interface\n"
       "Interface name\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_interface,
       no_neighbor_interface_cmd,
       NO_NEIGHBOR_CMD "interface WORD",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR
       "Interface\n"
       "Interface name\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_distribute_list,
       neighbor_distribute_list_cmd,
       NEIGHBOR_CMD2 "distribute-list (<1-199>|<1300-2699>|WORD) (in|out)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Filter updates to/from this neighbor\n"
       "IP access-list number\n"
       "IP access-list number (expanded range)\n"
       "IP Access-list name\n"
       "Filter incoming updates\n"
       "Filter outgoing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_distribute_list,
       no_neighbor_distribute_list_cmd,
       NO_NEIGHBOR_CMD2 "distribute-list (<1-199>|<1300-2699>|WORD) (in|out)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Filter updates to/from this neighbor\n"
       "IP access-list number\n"
       "IP access-list number (expanded range)\n"
       "IP Access-list name\n"
       "Filter incoming updates\n"
       "Filter outgoing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_prefix_list,
       neighbor_prefix_list_cmd,
       NEIGHBOR_CMD2 "prefix-list WORD (in|out)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Filter updates to/from this neighbor\n"
       "Name of a prefix list\n"
       "Filter incoming updates\n"
       "Filter outgoing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_prefix_list,
       no_neighbor_prefix_list_cmd,
       NO_NEIGHBOR_CMD2 "prefix-list WORD (in|out)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Filter updates to/from this neighbor\n"
       "Name of a prefix list\n"
       "Filter incoming updates\n"
       "Filter outgoing updates\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_filter_list,
       neighbor_filter_list_cmd,
       NEIGHBOR_CMD2 "filter-list WORD (in|out)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Establish BGP filters\n"
       "AS path access-list name\n"
       "Filter incoming routes\n"
       "Filter outgoing routes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_filter_list,
       no_neighbor_filter_list_cmd,
       NO_NEIGHBOR_CMD2 "filter-list WORD (in|out)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Establish BGP filters\n"
       "AS path access-list name\n"
       "Filter incoming routes\n"
       "Filter outgoing routes\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

struct ovsrec_route_map *
get_neighbor_route_map (const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor,
                        char *name, char *direction)
{
    struct ovsrec_route_map *rt_map = NULL;
    char *direct, *rm_name;

    int i;
    for (i = 0; i < ovs_bgp_neighbor->n_route_maps; i++)
    {
        direct = ovs_bgp_neighbor->key_route_maps[i];
        rm_name = ovs_bgp_neighbor->value_route_maps[i]->name;

        if (!strcmp(name, rm_name) && !strcmp(direction, direct))
        {
            rt_map = ovs_bgp_neighbor->value_route_maps[i];
            break;
        }
    }

    return rt_map;
}

static int
cli_neighbor_route_map_cmd_execute (char *vrf_name, char *ipAddr, char *name,
                                    char *direction)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;
    const struct ovsrec_route_map *rt_map_row;
    bool rm_found = false;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
	get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ipAddr);
    if (!ovs_bgp_neighbor) {
        ERRONEOUS_DB_TXN(txn, "no existing neighbor found");
    }

    // Since neighbor exists, we need to check the route-map name and
    // direction to identify if it's a duplicate.
    if (get_neighbor_route_map(ovs_bgp_neighbor, name, direction)) {
        ABORT_DB_TXN(txn, "configuration exists");
    }

    // Check if the specified route-map exists.
    OVSREC_ROUTE_MAP_FOR_EACH(rt_map_row, idl) {
        if (!strcmp(rt_map_row->name, name)) {
            rm_found = true;
            break;
        }
    }

    if (!rm_found) {
        ERRONEOUS_DB_TXN(txn, "route-map doesn't exist");
    }

    int num_elems = ovs_bgp_neighbor->n_route_maps;
    char **directions = xmalloc(sizeof(*directions) * (num_elems+1));
    struct ovsrec_route_map **rt_maps =
	xmalloc(sizeof(*rt_maps) * (num_elems+1));

    int i;
    bool dir_found = false;
    for (i = 0; i < num_elems; i++) {
        directions[i] = ovs_bgp_neighbor->key_route_maps[i];
        rt_maps[i] = ovs_bgp_neighbor->value_route_maps[i];
        if (!strcmp(direction, directions[i])) {
            rt_maps[i] = CONST_CAST(struct ovsrec_route_map*,rt_map_row);
            dir_found = true;
        }
    }

    if (!dir_found) {
        directions[num_elems] = direction;
        rt_maps[num_elems] = CONST_CAST(struct ovsrec_route_map*,rt_map_row);
        num_elems++;
    }

    ovsrec_bgp_neighbor_set_route_maps(ovs_bgp_neighbor, directions,
	rt_maps, num_elems);

    free(directions);
    free(rt_maps);

    /* done */
    END_DB_TXN(txn);
}

// Handle
DEFUN (neighbor_route_map,
       neighbor_route_map_cmd,
       NEIGHBOR_CMD2 "route-map WORD (in|out|import|export)",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Apply route map to neighbor\n"
       "Name of route map\n"
       "Apply map to incoming routes\n"
       "Apply map to outbound routes\n"
       "Apply map to routes going into a Route-Server client's table\n"
       "Apply map to routes coming from a Route-Server client")
{
    return cli_neighbor_route_map_cmd_execute(NULL,
                                              CONST_CAST(char*,argv[0]),
                                              CONST_CAST(char*, argv[1]),
                                              CONST_CAST(char*, argv[2]));
}

static int
cli_no_neighbor_route_map_cmd_execute (char *vrf_name, char *ipAddr, char *direction)
{
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *bgp_router_context;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    struct ovsdb_idl_txn *txn;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
	get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ipAddr);
    if (!ovs_bgp_neighbor) {
        ERRONEOUS_DB_TXN(txn, "no existing neighbor found");
    }

    if (!ovs_bgp_neighbor->n_route_maps) {
        ABORT_DB_TXN(txn, "no existing neighbor route-map to unset");
    }

    // Check to see if a route-map is configured for the direction
    int num_elems = ovs_bgp_neighbor->n_route_maps;
    char **directions = xmalloc(sizeof(*directions) * num_elems);
    struct ovsrec_route_map **rt_maps =
	xmalloc(sizeof(*rt_maps) * num_elems);
    char *direct;

    int i, j;
    bool dir_found = false;
    for (i = 0, j = 0; i < num_elems; i++) {
        direct = ovs_bgp_neighbor->key_route_maps[i];

        if (!strcmp(direction, direct)) {
            // If found, then we skip adding this route-map configuration.
            dir_found = true;
            num_elems--;
            continue;
        } else {
            // This is not the entry we are deleting, so make sure it remains
            // in the ovsdb.
            directions[j] = direct;
            rt_maps[j++] = ovs_bgp_neighbor->value_route_maps[i];
        }
    }

    if (!dir_found) {
        free(directions);
        free(rt_maps);
        ABORT_DB_TXN(txn, "neighbor route-map for the direction doesn't exist");
    }

    ovsrec_bgp_neighbor_set_route_maps
	(ovs_bgp_neighbor, directions, rt_maps, num_elems);

    free(directions);
    free(rt_maps);

    /* done */
    END_DB_TXN(txn);
}

DEFUN (no_neighbor_route_map,
       no_neighbor_route_map_cmd,
       NO_NEIGHBOR_CMD2 "route-map WORD (in|out|import|export)",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Apply route map to neighbor\n"
       "Name of route map\n"
       "Apply map to incoming routes\n"
       "Apply map to outbound routes\n"
       "Apply map to routes going into a Route-Server client's table\n"
       "Apply map to routes coming from a Route-Server client")
{
    return cli_no_neighbor_route_map_cmd_execute(NULL,
                                                 CONST_CAST(char*,argv[0]),
                                                 CONST_CAST(char*,argv[2]));
}

DEFUN (neighbor_unsuppress_map,
       neighbor_unsuppress_map_cmd,
       NEIGHBOR_CMD2 "unsuppress-map WORD",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Route-map to selectively unsuppress suppressed routes\n"
       "Name of route map\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_unsuppress_map,
       no_neighbor_unsuppress_map_cmd,
       NO_NEIGHBOR_CMD2 "unsuppress-map WORD",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Route-map to selectively unsuppress suppressed routes\n"
       "Name of route map\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* Maximum number of prefix configuration.  prefix count is different
   for each peer configuration.  So this configuration can be set for
   each peer configuration. */
DEFUN (neighbor_maximum_prefix,
       neighbor_maximum_prefix_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_maximum_prefix_threshold,
       neighbor_maximum_prefix_threshold_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> <1-100>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_maximum_prefix_warning,
       neighbor_maximum_prefix_warning_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> warning-only",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Only give warning message when limit is exceeded\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_maximum_prefix_threshold_warning,
       neighbor_maximum_prefix_threshold_warning_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> <1-100> warning-only",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n"
       "Only give warning message when limit is exceeded\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_maximum_prefix_restart,
       neighbor_maximum_prefix_restart_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> restart <1-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Restart bgp connection after limit is exceeded\n"
       "Restart interval in minutes")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (neighbor_maximum_prefix_threshold_restart,
       neighbor_maximum_prefix_threshold_restart_cmd,
       NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> <1-100> restart <1-65535>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n"
       "Restart bgp connection after limit is exceeded\n"
       "Restart interval in minutes")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_val_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n")

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_threshold_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> warning-only",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n")

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_warning_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> warning-only",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Only give warning message when limit is exceeded\n")

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_threshold_warning_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> <1-100> warning-only",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n"
       "Only give warning message when limit is exceeded\n")

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_restart_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> restart <1-65535>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Restart bgp connection after limit is exceeded\n"
       "Restart interval in minutes")

ALIAS (no_neighbor_maximum_prefix,
       no_neighbor_maximum_prefix_threshold_restart_cmd,
       NO_NEIGHBOR_CMD2 "maximum-prefix <1-4294967295> <1-100> restart <1-65535>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Maximum number of prefix accept from this peer\n"
       "maximum no. of prefix limit\n"
       "Threshold value (%) at which to generate a warning msg\n"
       "Restart bgp connection after limit is exceeded\n"
       "Restart interval in minutes")

static int
cli_allow_as_in_execute(char *vrf_name, int argc, const char *argv[])
{
    const char *ip_addr = argv[0];
    char *str;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;
    int64_t allow_num;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);
    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);

    if (ovs_bgp_neighbor) {
        VTY_GET_INTEGER_RANGE ("AS number", allow_num, argv[1], 1, 10);
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_allow_as_in(ovs_bgp_neighbor, &allow_num, 1);
    }
    END_DB_TXN(txn);
}
/* "neighbor allowas-in" */
DEFUN (neighbor_allowas_in,
       neighbor_allowas_in_cmd,
       NEIGHBOR_CMD2 "allowas-in",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Accept as-path with my AS present in it\n")
{
    if (argc != 2) {
    vty_out(vty, "\n%%Insufficient parameters, neighbor <ipaddr> allowas-in <val>\n");
    return CMD_WARNING;
    }
    return cli_allow_as_in_execute(NULL, argc, argv);
}

ALIAS (neighbor_allowas_in,
       neighbor_allowas_in_arg_cmd,
       NEIGHBOR_CMD2 "allowas-in <1-10>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Accept as-path with my AS present in it\n"
       "Number of occurances of AS number\n")

DEFUN (no_neighbor_allowas_in,
       no_neighbor_allowas_in_cmd,
       NO_NEIGHBOR_CMD2 "allowas-in",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "allow local ASN appears in aspath attribute\n")
{
    const char *ip_addr = argv[0];
    int64_t allow_num;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    const struct ovsrec_bgp_router *bgp_router_context;
    struct ovsdb_idl_txn *txn;
    char *vrf_name = NULL;

    START_DB_TXN(txn);

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        ERRONEOUS_DB_TXN(txn, "no vrf found");
    }

    bgp_router_context = get_ovsrec_bgp_router_with_asn(vrf_row, (int64_t)vty->index);

    if (!bgp_router_context) {
        ERRONEOUS_DB_TXN(txn, "bgp router context not available");
    }

    ovs_bgp_neighbor =
    get_bgp_neighbor_with_bgp_router_and_ipaddr(bgp_router_context, ip_addr);

    if (ovs_bgp_neighbor) {
        // to write to ovsdb nbr table
        ovsrec_bgp_neighbor_set_allow_as_in(ovs_bgp_neighbor, &allow_num, 0);
    }
    END_DB_TXN(txn);
}

DEFUN (neighbor_ttl_security,
       neighbor_ttl_security_cmd,
       NEIGHBOR_CMD2 "ttl-security hops <1-254>",
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify the maximum number of hops to the BGP peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_neighbor_ttl_security,
       no_neighbor_ttl_security_cmd,
       NO_NEIGHBOR_CMD2 "ttl-security hops <1-254>",
       NO_STR
       NEIGHBOR_STR
       NEIGHBOR_ADDR_STR2
       "Specify the maximum number of hops to the BGP peer\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* Address family configuration.  */
DEFUN (address_family_ipv4,
       address_family_ipv4_cmd,
       "address-family ipv4",
       "Enter Address Family command mode\n"
       "Address family\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (address_family_ipv4_safi,
       address_family_ipv4_safi_cmd,
       "address-family ipv4 (unicast|multicast)",
       "Enter Address Family command mode\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (address_family_ipv6_safi,
       address_family_ipv6_safi_cmd,
       "address-family ipv6 (unicast|multicast)",
       "Enter Address Family command mode\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_all,
       clear_ip_bgp_all_cmd,
       "clear ip bgp *",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all,
       clear_bgp_all_cmd,
       "clear bgp *",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n")

ALIAS (clear_ip_bgp_all,
       clear_bgp_ipv6_all_cmd,
       "clear bgp ipv6 *",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n")

ALIAS (clear_ip_bgp_all,
       clear_ip_bgp_instance_all_cmd,
       "clear ip bgp view WORD *",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n")

ALIAS (clear_ip_bgp_all,
       clear_bgp_instance_all_cmd,
       "clear bgp view WORD *",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n")

DEFUN (clear_ip_bgp_peer,
       clear_ip_bgp_peer_cmd,
       "clear ip bgp (A.B.C.D|X:X::X:X)",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer,
       clear_bgp_peer_cmd,
       "clear bgp (A.B.C.D|X:X::X:X)",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n")

ALIAS (clear_ip_bgp_peer,
       clear_bgp_ipv6_peer_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X)",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n")

DEFUN (clear_ip_bgp_peer_group,
       clear_ip_bgp_peer_group_cmd,
       "clear ip bgp peer-group WORD",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_group,
       clear_bgp_peer_group_cmd,
       "clear bgp peer-group WORD",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n")

ALIAS (clear_ip_bgp_peer_group,
       clear_bgp_ipv6_peer_group_cmd,
       "clear bgp ipv6 peer-group WORD",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n")

DEFUN (clear_ip_bgp_external,
       clear_ip_bgp_external_cmd,
       "clear ip bgp external",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_external,
       clear_bgp_external_cmd,
       "clear bgp external",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n")

ALIAS (clear_ip_bgp_external,
       clear_bgp_ipv6_external_cmd,
       "clear bgp ipv6 external",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n")

DEFUN (clear_ip_bgp_as,
       clear_ip_bgp_as_cmd,
       "clear ip bgp " CMD_AS_RANGE,
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as,
       clear_bgp_as_cmd,
       "clear bgp " CMD_AS_RANGE,
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n")

ALIAS (clear_ip_bgp_as,
       clear_bgp_ipv6_as_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE,
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n")

/* Outbound soft-reconfiguration */
DEFUN (clear_ip_bgp_all_soft_out,
       clear_ip_bgp_all_soft_out_cmd,
       "clear ip bgp * soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_soft_out,
       clear_ip_bgp_all_out_cmd,
       "clear ip bgp * out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_ip_bgp_all_soft_out,
       clear_ip_bgp_instance_all_soft_out_cmd,
       "clear ip bgp view WORD * soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_all_ipv4_soft_out,
       clear_ip_bgp_all_ipv4_soft_out_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_ipv4_soft_out,
       clear_ip_bgp_all_ipv4_out_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_instance_all_ipv4_soft_out,
       clear_ip_bgp_instance_all_ipv4_soft_out_cmd,
       "clear ip bgp view WORD * ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_all_vpnv4_soft_out,
       clear_ip_bgp_all_vpnv4_soft_out_cmd,
       "clear ip bgp * vpnv4 unicast soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_vpnv4_soft_out,
       clear_ip_bgp_all_vpnv4_out_cmd,
       "clear ip bgp * vpnv4 unicast out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_bgp_all_soft_out,
       clear_bgp_all_soft_out_cmd,
       "clear bgp * soft out",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_all_soft_out,
       clear_bgp_instance_all_soft_out_cmd,
       "clear bgp view WORD * soft out",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_all_soft_out,
       clear_bgp_all_out_cmd,
       "clear bgp * out",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_all_soft_out,
       clear_bgp_ipv6_all_soft_out_cmd,
       "clear bgp ipv6 * soft out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_all_soft_out,
       clear_bgp_ipv6_all_out_cmd,
       "clear bgp ipv6 * out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_peer_soft_out,
       clear_ip_bgp_peer_soft_out_cmd,
       "clear ip bgp A.B.C.D soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_soft_out,
       clear_ip_bgp_peer_out_cmd,
       "clear ip bgp A.B.C.D out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_peer_ipv4_soft_out,
       clear_ip_bgp_peer_ipv4_soft_out_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_ipv4_soft_out,
       clear_ip_bgp_peer_ipv4_out_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_peer_vpnv4_soft_out,
       clear_ip_bgp_peer_vpnv4_soft_out_cmd,
       "clear ip bgp A.B.C.D vpnv4 unicast soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_vpnv4_soft_out,
       clear_ip_bgp_peer_vpnv4_out_cmd,
       "clear ip bgp A.B.C.D vpnv4 unicast out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_bgp_peer_soft_out,
       clear_bgp_peer_soft_out_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) soft out",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_soft_out,
       clear_bgp_ipv6_peer_soft_out_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) soft out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_peer_soft_out,
       clear_bgp_peer_out_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) out",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_peer_soft_out,
       clear_bgp_ipv6_peer_out_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_peer_group_soft_out,
       clear_ip_bgp_peer_group_soft_out_cmd,
       "clear ip bgp peer-group WORD soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_group_soft_out,
       clear_ip_bgp_peer_group_out_cmd,
       "clear ip bgp peer-group WORD out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_peer_group_ipv4_soft_out,
       clear_ip_bgp_peer_group_ipv4_soft_out_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_group_ipv4_soft_out,
       clear_ip_bgp_peer_group_ipv4_out_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_bgp_peer_group_soft_out,
       clear_bgp_peer_group_soft_out_cmd,
       "clear bgp peer-group WORD soft out",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_group_soft_out,
       clear_bgp_ipv6_peer_group_soft_out_cmd,
       "clear bgp ipv6 peer-group WORD soft out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_peer_group_soft_out,
       clear_bgp_peer_group_out_cmd,
       "clear bgp peer-group WORD out",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_peer_group_soft_out,
       clear_bgp_ipv6_peer_group_out_cmd,
       "clear bgp ipv6 peer-group WORD out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_external_soft_out,
       clear_ip_bgp_external_soft_out_cmd,
       "clear ip bgp external soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_external_soft_out,
       clear_ip_bgp_external_out_cmd,
       "clear ip bgp external out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_external_ipv4_soft_out,
       clear_ip_bgp_external_ipv4_soft_out_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_external_ipv4_soft_out,
       clear_ip_bgp_external_ipv4_out_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_bgp_external_soft_out,
       clear_bgp_external_soft_out_cmd,
       "clear bgp external soft out",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_external_soft_out,
       clear_bgp_ipv6_external_soft_out_cmd,
       "clear bgp ipv6 external soft out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_external_soft_out,
       clear_bgp_external_out_cmd,
       "clear bgp external out",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_external_soft_out,
       clear_bgp_ipv6_external_out_cmd,
       "clear bgp ipv6 external WORD out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_as_soft_out,
       clear_ip_bgp_as_soft_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_soft_out,
       clear_ip_bgp_as_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_as_ipv4_soft_out,
       clear_ip_bgp_as_ipv4_soft_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_ipv4_soft_out,
       clear_ip_bgp_as_ipv4_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_ip_bgp_as_vpnv4_soft_out,
       clear_ip_bgp_as_vpnv4_soft_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " vpnv4 unicast soft out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_vpnv4_soft_out,
       clear_ip_bgp_as_vpnv4_out_cmd,
       "clear ip bgp " CMD_AS_RANGE " vpnv4 unicast out",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Soft reconfig outbound update\n")

DEFUN (clear_bgp_as_soft_out,
       clear_bgp_as_soft_out_cmd,
       "clear bgp " CMD_AS_RANGE " soft out",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_as_soft_out,
       clear_bgp_ipv6_as_soft_out_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " soft out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_as_soft_out,
       clear_bgp_as_out_cmd,
       "clear bgp " CMD_AS_RANGE " out",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig outbound update\n")

ALIAS (clear_bgp_as_soft_out,
       clear_bgp_ipv6_as_out_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " out",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig outbound update\n")

/* Inbound soft-reconfiguration */
DEFUN (clear_ip_bgp_all_soft_in,
       clear_ip_bgp_all_soft_in_cmd,
       "clear ip bgp * soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_soft_in,
       clear_ip_bgp_instance_all_soft_in_cmd,
       "clear ip bgp view WORD * soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_ip_bgp_all_soft_in,
       clear_ip_bgp_all_in_cmd,
       "clear ip bgp * in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_all_in_prefix_filter,
       clear_ip_bgp_all_in_prefix_filter_cmd,
       "clear ip bgp * in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_in_prefix_filter,
       clear_ip_bgp_instance_all_in_prefix_filter_cmd,
       "clear ip bgp view WORD * in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")


DEFUN (clear_ip_bgp_all_ipv4_soft_in,
       clear_ip_bgp_all_ipv4_soft_in_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_ipv4_soft_in,
       clear_ip_bgp_all_ipv4_in_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_instance_all_ipv4_soft_in,
       clear_ip_bgp_instance_all_ipv4_soft_in_cmd,
       "clear ip bgp view WORD * ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_all_ipv4_in_prefix_filter,
       clear_ip_bgp_all_ipv4_in_prefix_filter_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_instance_all_ipv4_in_prefix_filter,
       clear_ip_bgp_instance_all_ipv4_in_prefix_filter_cmd,
       "clear ip bgp view WORD * ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_all_vpnv4_soft_in,
       clear_ip_bgp_all_vpnv4_soft_in_cmd,
       "clear ip bgp * vpnv4 unicast soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_vpnv4_soft_in,
       clear_ip_bgp_all_vpnv4_in_cmd,
       "clear ip bgp * vpnv4 unicast in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_all_soft_in,
       clear_bgp_all_soft_in_cmd,
       "clear bgp * soft in",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_all_soft_in,
       clear_bgp_instance_all_soft_in_cmd,
       "clear bgp view WORD * soft in",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_all_soft_in,
       clear_bgp_ipv6_all_soft_in_cmd,
       "clear bgp ipv6 * soft in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_all_soft_in,
       clear_bgp_all_in_cmd,
       "clear bgp * in",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_all_soft_in,
       clear_bgp_ipv6_all_in_cmd,
       "clear bgp ipv6 * in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_all_in_prefix_filter,
       clear_bgp_all_in_prefix_filter_cmd,
       "clear bgp * in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_all_in_prefix_filter,
       clear_bgp_ipv6_all_in_prefix_filter_cmd,
       "clear bgp ipv6 * in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")

DEFUN (clear_ip_bgp_peer_soft_in,
       clear_ip_bgp_peer_soft_in_cmd,
       "clear ip bgp A.B.C.D soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_soft_in,
       clear_ip_bgp_peer_in_cmd,
       "clear ip bgp A.B.C.D in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_peer_in_prefix_filter,
       clear_ip_bgp_peer_in_prefix_filter_cmd,
       "clear ip bgp A.B.C.D in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig inbound update\n"
       "Push out the existing ORF prefix-list\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_ipv4_soft_in,
       clear_ip_bgp_peer_ipv4_soft_in_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_ipv4_soft_in,
       clear_ip_bgp_peer_ipv4_in_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_peer_ipv4_in_prefix_filter,
       clear_ip_bgp_peer_ipv4_in_prefix_filter_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out the existing ORF prefix-list\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_vpnv4_soft_in,
       clear_ip_bgp_peer_vpnv4_soft_in_cmd,
       "clear ip bgp A.B.C.D vpnv4 unicast soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_vpnv4_soft_in,
       clear_ip_bgp_peer_vpnv4_in_cmd,
       "clear ip bgp A.B.C.D vpnv4 unicast in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_peer_soft_in,
       clear_bgp_peer_soft_in_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) soft in",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_soft_in,
       clear_bgp_ipv6_peer_soft_in_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) soft in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_peer_soft_in,
       clear_bgp_peer_in_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) in",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_peer_soft_in,
       clear_bgp_ipv6_peer_in_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_peer_in_prefix_filter,
       clear_bgp_peer_in_prefix_filter_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) in prefix-filter",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig inbound update\n"
       "Push out the existing ORF prefix-list\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_in_prefix_filter,
       clear_bgp_ipv6_peer_in_prefix_filter_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig inbound update\n"
       "Push out the existing ORF prefix-list\n")

DEFUN (clear_ip_bgp_peer_group_soft_in,
       clear_ip_bgp_peer_group_soft_in_cmd,
       "clear ip bgp peer-group WORD soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_group_soft_in,
       clear_ip_bgp_peer_group_in_cmd,
       "clear ip bgp peer-group WORD in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_peer_group_in_prefix_filter,
       clear_ip_bgp_peer_group_in_prefix_filter_cmd,
       "clear ip bgp peer-group WORD in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_group_ipv4_soft_in,
       clear_ip_bgp_peer_group_ipv4_soft_in_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_group_ipv4_soft_in,
       clear_ip_bgp_peer_group_ipv4_in_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_peer_group_ipv4_in_prefix_filter,
       clear_ip_bgp_peer_group_ipv4_in_prefix_filter_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_peer_group_soft_in,
       clear_bgp_peer_group_soft_in_cmd,
       "clear bgp peer-group WORD soft in",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_group_soft_in,
       clear_bgp_ipv6_peer_group_soft_in_cmd,
       "clear bgp ipv6 peer-group WORD soft in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_peer_group_soft_in,
       clear_bgp_peer_group_in_cmd,
       "clear bgp peer-group WORD in",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_peer_group_soft_in,
       clear_bgp_ipv6_peer_group_in_cmd,
       "clear bgp ipv6 peer-group WORD in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_peer_group_in_prefix_filter,
       clear_bgp_peer_group_in_prefix_filter_cmd,
       "clear bgp peer-group WORD in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_group_in_prefix_filter,
       clear_bgp_ipv6_peer_group_in_prefix_filter_cmd,
       "clear bgp ipv6 peer-group WORD in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")

DEFUN (clear_ip_bgp_external_soft_in,
       clear_ip_bgp_external_soft_in_cmd,
       "clear ip bgp external soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_external_soft_in,
       clear_ip_bgp_external_in_cmd,
       "clear ip bgp external in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_external_in_prefix_filter,
       clear_ip_bgp_external_in_prefix_filter_cmd,
       "clear ip bgp external in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_external_ipv4_soft_in,
       clear_ip_bgp_external_ipv4_soft_in_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_external_ipv4_soft_in,
       clear_ip_bgp_external_ipv4_in_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_external_ipv4_in_prefix_filter,
       clear_ip_bgp_external_ipv4_in_prefix_filter_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_external_soft_in,
       clear_bgp_external_soft_in_cmd,
       "clear bgp external soft in",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_external_soft_in,
       clear_bgp_ipv6_external_soft_in_cmd,
       "clear bgp ipv6 external soft in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_external_soft_in,
       clear_bgp_external_in_cmd,
       "clear bgp external in",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_external_soft_in,
       clear_bgp_ipv6_external_in_cmd,
       "clear bgp ipv6 external WORD in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_external_in_prefix_filter,
       clear_bgp_external_in_prefix_filter_cmd,
       "clear bgp external in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_external_in_prefix_filter,
       clear_bgp_ipv6_external_in_prefix_filter_cmd,
       "clear bgp ipv6 external in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")

DEFUN (clear_ip_bgp_as_soft_in,
       clear_ip_bgp_as_soft_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_soft_in,
       clear_ip_bgp_as_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_as_in_prefix_filter,
       clear_ip_bgp_as_in_prefix_filter_cmd,
       "clear ip bgp " CMD_AS_RANGE " in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_as_ipv4_soft_in,
       clear_ip_bgp_as_ipv4_soft_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_ipv4_soft_in,
       clear_ip_bgp_as_ipv4_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_ip_bgp_as_ipv4_in_prefix_filter,
       clear_ip_bgp_as_ipv4_in_prefix_filter_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) in prefix-filter",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_as_vpnv4_soft_in,
       clear_ip_bgp_as_vpnv4_soft_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " vpnv4 unicast soft in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_as_vpnv4_soft_in,
       clear_ip_bgp_as_vpnv4_in_cmd,
       "clear ip bgp " CMD_AS_RANGE " vpnv4 unicast in",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family modifier\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_as_soft_in,
       clear_bgp_as_soft_in_cmd,
       "clear bgp " CMD_AS_RANGE " soft in",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_as_soft_in,
       clear_bgp_ipv6_as_soft_in_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " soft in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_as_soft_in,
       clear_bgp_as_in_cmd,
       "clear bgp " CMD_AS_RANGE " in",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n")

ALIAS (clear_bgp_as_soft_in,
       clear_bgp_ipv6_as_in_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " in",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n")

DEFUN (clear_bgp_as_in_prefix_filter,
       clear_bgp_as_in_prefix_filter_cmd,
       "clear bgp " CMD_AS_RANGE " in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_as_in_prefix_filter,
       clear_bgp_ipv6_as_in_prefix_filter_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " in prefix-filter",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig inbound update\n"
       "Push out prefix-list ORF and do inbound soft reconfig\n")

/* Both soft-reconfiguration */
DEFUN (clear_ip_bgp_all_soft,
       clear_ip_bgp_all_soft_cmd,
       "clear ip bgp * soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_soft,
       clear_ip_bgp_instance_all_soft_cmd,
       "clear ip bgp view WORD * soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n")


DEFUN (clear_ip_bgp_all_ipv4_soft,
       clear_ip_bgp_all_ipv4_soft_cmd,
       "clear ip bgp * ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_instance_all_ipv4_soft,
       clear_ip_bgp_instance_all_ipv4_soft_cmd,
       "clear ip bgp view WORD * ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_all_vpnv4_soft,
       clear_ip_bgp_all_vpnv4_soft_cmd,
       "clear ip bgp * vpnv4 unicast soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_all_soft,
       clear_bgp_all_soft_cmd,
       "clear bgp * soft",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_all_soft,
       clear_bgp_instance_all_soft_cmd,
       "clear bgp view WORD * soft",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig\n")

ALIAS (clear_bgp_all_soft,
       clear_bgp_ipv6_all_soft_cmd,
       "clear bgp ipv6 * soft",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig\n")

DEFUN (clear_ip_bgp_peer_soft,
       clear_ip_bgp_peer_soft_cmd,
       "clear ip bgp A.B.C.D soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_ipv4_soft,
       clear_ip_bgp_peer_ipv4_soft_cmd,
       "clear ip bgp A.B.C.D ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_vpnv4_soft,
       clear_ip_bgp_peer_vpnv4_soft_cmd,
       "clear ip bgp A.B.C.D vpnv4 unicast soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_peer_soft,
       clear_bgp_peer_soft_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) soft",
       CLEAR_STR
       BGP_STR
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_soft,
       clear_bgp_ipv6_peer_soft_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) soft",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig\n")

DEFUN (clear_ip_bgp_peer_group_soft,
       clear_ip_bgp_peer_group_soft_cmd,
       "clear ip bgp peer-group WORD soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_peer_group_ipv4_soft,
       clear_ip_bgp_peer_group_ipv4_soft_cmd,
       "clear ip bgp peer-group WORD ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_peer_group_soft,
       clear_bgp_peer_group_soft_cmd,
       "clear bgp peer-group WORD soft",
       CLEAR_STR
       BGP_STR
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_group_soft,
       clear_bgp_ipv6_peer_group_soft_cmd,
       "clear bgp ipv6 peer-group WORD soft",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all members of peer-group\n"
       "BGP peer-group name\n"
       "Soft reconfig\n")

DEFUN (clear_ip_bgp_external_soft,
       clear_ip_bgp_external_soft_cmd,
       "clear ip bgp external soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_external_ipv4_soft,
       clear_ip_bgp_external_ipv4_soft_cmd,
       "clear ip bgp external ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all external peers\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_external_soft,
       clear_bgp_external_soft_cmd,
       "clear bgp external soft",
       CLEAR_STR
       BGP_STR
       "Clear all external peers\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_external_soft,
       clear_bgp_ipv6_external_soft_cmd,
       "clear bgp ipv6 external soft",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all external peers\n"
       "Soft reconfig\n")

DEFUN (clear_ip_bgp_as_soft,
       clear_ip_bgp_as_soft_cmd,
       "clear ip bgp " CMD_AS_RANGE " soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_as_ipv4_soft,
       clear_ip_bgp_as_ipv4_soft_cmd,
       "clear ip bgp " CMD_AS_RANGE " ipv4 (unicast|multicast) soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_ip_bgp_as_vpnv4_soft,
       clear_ip_bgp_as_vpnv4_soft_cmd,
       "clear ip bgp " CMD_AS_RANGE " vpnv4 unicast soft",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Address family\n"
       "Address Family Modifier\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (clear_bgp_as_soft,
       clear_bgp_as_soft_cmd,
       "clear bgp " CMD_AS_RANGE " soft",
       CLEAR_STR
       BGP_STR
       "Clear peers with the AS number\n"
       "Soft reconfig\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_as_soft,
       clear_bgp_ipv6_as_soft_cmd,
       "clear bgp ipv6 " CMD_AS_RANGE " soft",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear peers with the AS number\n"
       "Soft reconfig\n")

/* RS-client soft reconfiguration. */
#ifdef HAVE_IPV6
DEFUN (clear_bgp_all_rsclient,
       clear_bgp_all_rsclient_cmd,
       "clear bgp * rsclient",
       CLEAR_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_all_rsclient,
       clear_bgp_ipv6_all_rsclient_cmd,
       "clear bgp ipv6 * rsclient",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")

ALIAS (clear_bgp_all_rsclient,
       clear_bgp_instance_all_rsclient_cmd,
       "clear bgp view WORD * rsclient",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")

ALIAS (clear_bgp_all_rsclient,
       clear_bgp_ipv6_instance_all_rsclient_cmd,
       "clear bgp ipv6 view WORD * rsclient",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")
#endif /* HAVE_IPV6 */

DEFUN (clear_ip_bgp_all_rsclient,
       clear_ip_bgp_all_rsclient_cmd,
       "clear ip bgp * rsclient",
       CLEAR_STR
       IP_STR
       BGP_STR
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_all_rsclient,
       clear_ip_bgp_instance_all_rsclient_cmd,
       "clear ip bgp view WORD * rsclient",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "Clear all peers\n"
       "Soft reconfig for rsclient RIB\n")

#ifdef HAVE_IPV6
DEFUN (clear_bgp_peer_rsclient,
       clear_bgp_peer_rsclient_cmd,
       "clear bgp (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       BGP_STR
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_bgp_peer_rsclient,
       clear_bgp_ipv6_peer_rsclient_cmd,
       "clear bgp ipv6 (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")

ALIAS (clear_bgp_peer_rsclient,
       clear_bgp_instance_peer_rsclient_cmd,
       "clear bgp view WORD (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")

ALIAS (clear_bgp_peer_rsclient,
       clear_bgp_ipv6_instance_peer_rsclient_cmd,
       "clear bgp ipv6 view WORD (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       BGP_STR
       "Address family\n"
       "BGP view\n"
       "view name\n"
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")
#endif /* HAVE_IPV6 */

DEFUN (clear_ip_bgp_peer_rsclient,
       clear_ip_bgp_peer_rsclient_cmd,
       "clear ip bgp (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (clear_ip_bgp_peer_rsclient,
       clear_ip_bgp_instance_peer_rsclient_cmd,
       "clear ip bgp view WORD (A.B.C.D|X:X::X:X) rsclient",
       CLEAR_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "view name\n"
       "BGP neighbor IP address to clear\n"
       "BGP IPv6 neighbor to clear\n"
       "Soft reconfig for rsclient RIB\n")

DEFUN (show_bgp_views,
       show_bgp_views_cmd,
       "show bgp views",
       SHOW_STR
       BGP_STR
       "Show the defined BGP views\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_bgp_memory,
       show_bgp_memory_cmd,
       "show bgp memory",
       SHOW_STR
       BGP_STR
       "Global BGP memory statistics\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* `show ip bgp summary' commands. */

static int
cli_bgp_show_summary_vty_execute(struct vty *vty, int afi, int safi)
{
    struct ovsrec_vrf *ovs_vrf=NULL;
    struct ovsrec_bgp_router *bgp_router_context=NULL;
    struct ovsrec_bgp_neighbor *ovs_bgp_neighbor=NULL;
    struct ovsrec_route *rib_row = NULL;
    struct ovsdb_idl_txn *txn;
    int peer_count=0,rib_count=0,i=0,len=0,j=0;
    static char header[] = "Neighbor            AS MsgRcvd MsgSent Up/Down  State/PfxRcd\n";

    /* Start of transaction */
    START_DB_TXN(txn);

    bgp_router_context = ovsrec_bgp_router_first(idl);
    ovs_vrf = ovsrec_vrf_first(idl);

    if(ovs_vrf->value_bgp_routers[0])
       vty_out(vty,"BGP router identifier %s, local AS number %ld\n",
               ovs_vrf->value_bgp_routers[0]->router_id,
               ovs_vrf->key_bgp_routers[0]);

    //count the number of RIB entries
    OVSREC_ROUTE_FOR_EACH(rib_row, idl)
    {
        if (strcmp(rib_row->from, OVSREC_ROUTE_FROM_BGP))
            continue;

        rib_count++;
    }
    vty_out(vty,"RIB entries %d\n",rib_count);

    vty_out(vty,"Peers %d\n\n",bgp_router_context->n_bgp_neighbors);

    vty_out(vty,header);

    for(j=0;j<bgp_router_context->n_bgp_neighbors;j++)
    {
        vty_out(vty,"%s",bgp_router_context->key_bgp_neighbors[j]);
        len=strlen(bgp_router_context->key_bgp_neighbors[j]);

        for(i=0;i<(16-len);i++)
          vty_out(vty," ");

        vty_out(vty, "%7d", *bgp_router_context->value_bgp_neighbors[j]->remote_as);
        vty_out(vty, "%7d", bgp_router_context->value_bgp_neighbors[j]->value_statistics[8]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[14]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[4]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[6]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[11]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[1]);

        vty_out(vty, "%7d", bgp_router_context->value_bgp_neighbors[j]->value_statistics[9]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[15]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[5]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[7]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[12]+
                            bgp_router_context->value_bgp_neighbors[j]->value_statistics[2]);


        vty_out(vty, "%8d",
               bgp_router_context->value_bgp_neighbors[j]->value_statistics[16]);

        vty_out(vty, "%8s\n",smap_get(&bgp_router_context->value_bgp_neighbors[j]->status,
                "bgp-peer-state"));
    }

    END_DB_TXN(txn);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_summary,
       show_ip_bgp_summary_cmd,
       "show ip bgp summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Summary of BGP neighbor status\n")
{
    return cli_bgp_show_summary_vty_execute(vty,AFI_IP, SAFI_UNICAST);
}

DEFUN (show_ip_bgp_instance_summary,
       show_ip_bgp_instance_summary_cmd,
       "show ip bgp view WORD summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_ipv4_summary,
       show_ip_bgp_ipv4_summary_cmd,
       "show ip bgp ipv4 (unicast|multicast) summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_ipv4_summary,
       show_bgp_ipv4_safi_summary_cmd,
       "show bgp ipv4 (unicast|multicast) summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")

DEFUN (show_ip_bgp_instance_ipv4_summary,
       show_ip_bgp_instance_ipv4_summary_cmd,
       "show ip bgp view WORD ipv4 (unicast|multicast) summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_instance_ipv4_summary,
       show_bgp_instance_ipv4_safi_summary_cmd,
       "show bgp view WORD ipv4 (unicast|multicast) summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")

DEFUN (show_ip_bgp_vpnv4_all_summary,
       show_ip_bgp_vpnv4_all_summary_cmd,
       "show ip bgp vpnv4 all summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information about all VPNv4 NLRIs\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_vpnv4_rd_summary,
       show_ip_bgp_vpnv4_rd_summary_cmd,
       "show ip bgp vpnv4 rd ASN:nn_or_IP-address:nn summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information for a route distinguisher\n"
       "VPN Route Distinguisher\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

#ifdef HAVE_IPV6
DEFUN (show_bgp_summary,
       show_bgp_summary_cmd,
       "show bgp summary",
       SHOW_STR
       BGP_STR
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_bgp_instance_summary,
       show_bgp_instance_summary_cmd,
       "show bgp view WORD summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_bgp_summary,
       show_bgp_ipv6_summary_cmd,
       "show bgp ipv6 summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Summary of BGP neighbor status\n")

ALIAS (show_bgp_instance_summary,
       show_bgp_instance_ipv6_summary_cmd,
       "show bgp view WORD ipv6 summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Summary of BGP neighbor status\n")

DEFUN (show_bgp_ipv6_safi_summary,
       show_bgp_ipv6_safi_summary_cmd,
       "show bgp ipv6 (unicast|multicast) summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_bgp_instance_ipv6_safi_summary,
       show_bgp_instance_ipv6_safi_summary_cmd,
       "show bgp view WORD ipv6 (unicast|multicast) summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* old command */
DEFUN (show_ipv6_bgp_summary,
       show_ipv6_bgp_summary_cmd,
       "show ipv6 bgp summary",
       SHOW_STR
       IPV6_STR
       BGP_STR
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* old command */
DEFUN (show_ipv6_mbgp_summary,
       show_ipv6_mbgp_summary_cmd,
       "show ipv6 mbgp summary",
       SHOW_STR
       IPV6_STR
       MBGP_STR
       "Summary of BGP neighbor status\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}
#endif /* HAVE_IPV6 */

static void
show_one_bgp_neighbor (struct vty *vty, char **name,
                       const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor)
{
    int i = 0;
    vty_out(vty, "  name: %s, remote-as: %s\n",
	safe_print_string(1, name),
	safe_print_integer(ovs_bgp_neighbor->n_remote_as,
	    ovs_bgp_neighbor->remote_as));
    vty_out(vty, "    state: %s\n",
	safe_print_smap_value(&ovs_bgp_neighbor->status, BGP_PEER_STATE));
    vty_out(vty, "    shutdown: %s\n",
	safe_print_bool(ovs_bgp_neighbor->n_shutdown,
	    ovs_bgp_neighbor->shutdown));
    vty_out(vty, "    description: %s\n",
	safe_print_string(1, ovs_bgp_neighbor->description));
    vty_out(vty, "    capability: %s\n",
	safe_print_string(1, ovs_bgp_neighbor->capability));
    vty_out(vty, "    local_as: %s\n",
	safe_print_integer(ovs_bgp_neighbor->n_local_as,
	    ovs_bgp_neighbor->local_as));
    vty_out(vty, "    local_interface: %s\n",
	ovs_bgp_neighbor->local_interface ?
	    safe_print_string(1, ovs_bgp_neighbor->local_interface->name) :
	    _undefined);
    vty_out(vty, "    inbound_soft_reconfiguration: %s\n",
	safe_print_bool(ovs_bgp_neighbor->n_inbound_soft_reconfiguration,
	    ovs_bgp_neighbor->inbound_soft_reconfiguration));
    vty_out(vty, "    maximum_prefix_limit: %s\n",
	safe_print_integer(ovs_bgp_neighbor->n_maximum_prefix_limit,
	    ovs_bgp_neighbor->maximum_prefix_limit));
    vty_out(vty, "    tcp_port_number: %s\n",
	safe_print_integer(ovs_bgp_neighbor->n_tcp_port_number,
	    ovs_bgp_neighbor->tcp_port_number));
    vty_out(vty, "    statistics:\n");
    for (i = 0; i < ovs_bgp_neighbor->n_statistics; i++) {
	vty_out(vty, "       %s: %ld\n",
	    ovs_bgp_neighbor->key_statistics[i],
	    ovs_bgp_neighbor->value_statistics[i]);
    }
}

/*
** show neighbors in one specific bgp router.
** If "peer" is defined, match only that one,
** otherwise print all.
*/
static void
show_bgp_router_neighbors (struct vty *vty,
                           const struct ovsrec_bgp_router *ovs_bgp_router, const char *peer)
{
    const struct ovsrec_bgp_neighbor *ovs_bgp_neighbor;
    int i = 0;

    /*
    ** if entry IS a neighbor (not a peer group) and
    ** belongs to the specified router, display it provided
    ** neighbor filter (peer) is taken into account.
    */
    for (i = 0; i < ovs_bgp_router->n_bgp_neighbors; i++) {
        if((NULL == peer) ||
            (peer && (0 == strcmp(ovs_bgp_router->key_bgp_neighbors[i], peer)))) {
             show_one_bgp_neighbor(vty, ovs_bgp_router->key_bgp_neighbors[i],
                                   ovs_bgp_router->value_bgp_neighbors[i]);
        }
    }
}

/*
** show all bgp neighbors of all bgp routers.
** If "peer" is defined, show only matching ones.
*/
static void
cli_show_ip_bgp_neighbors_cmd_execute (char *vrf_name, struct vty *vty,
    int argc, const char *argv[])
{
    const char *peer = NULL;
    const struct ovsrec_vrf *vrf_row;
    const struct ovsrec_bgp_router *ovs_bgp_router;
    int i = 0;

    /* is a neighbor defined */
    if (argc == 1) {
	peer = argv[0];
    }

    vrf_row = get_ovsrec_vrf_with_name(vrf_name);
    if (vrf_row == NULL) {
        VLOG_DBG("No VRF found!");
    }
    else {
        for (i = 0; i < vrf_row->n_bgp_routers; i++) {
            show_bgp_router_neighbors(vty, vrf_row->value_bgp_routers[i], peer);
        }
    }
}

/* "show ip bgp neighbors" commands.  */

DEFUN (show_ip_bgp_neighbors,
       show_ip_bgp_neighbors_cmd,
       "show ip bgp neighbors",
       SHOW_STR
       IP_STR
       BGP_STR
       "Detailed information on TCP and BGP neighbor connections\n")
{
    cli_show_ip_bgp_neighbors_cmd_execute(NULL, vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_neighbors,
       show_ip_bgp_ipv4_neighbors_cmd,
       "show ip bgp ipv4 (unicast|multicast) neighbors",
       SHOW_STR
       IP_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Detailed information on TCP and BGP neighbor connections\n")

ALIAS (show_ip_bgp_neighbors,
       show_ip_bgp_vpnv4_all_neighbors_cmd,
       "show ip bgp vpnv4 all neighbors",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information about all VPNv4 NLRIs\n"
       "Detailed information on TCP and BGP neighbor connections\n")

ALIAS (show_ip_bgp_neighbors,
       show_ip_bgp_vpnv4_rd_neighbors_cmd,
       "show ip bgp vpnv4 rd ASN:nn_or_IP-address:nn neighbors",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information for a route distinguisher\n"
       "VPN Route Distinguisher\n"
       "Detailed information on TCP and BGP neighbor connections\n")

ALIAS (show_ip_bgp_neighbors,
       show_bgp_neighbors_cmd,
       "show bgp neighbors",
       SHOW_STR
       BGP_STR
       "Detailed information on TCP and BGP neighbor connections\n")

ALIAS (show_ip_bgp_neighbors,
       show_bgp_ipv6_neighbors_cmd,
       "show bgp ipv6 neighbors",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Detailed information on TCP and BGP neighbor connections\n")

DEFUN (show_ip_bgp_neighbors_peer,
       show_ip_bgp_neighbors_peer_cmd,
       "show ip bgp neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       IP_STR
       BGP_STR
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")
{
    cli_show_ip_bgp_neighbors_cmd_execute(NULL, vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_neighbors_peer,
       show_ip_bgp_ipv4_neighbors_peer_cmd,
       "show ip bgp ipv4 (unicast|multicast) neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       IP_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")

ALIAS (show_ip_bgp_neighbors_peer,
       show_ip_bgp_vpnv4_all_neighbors_peer_cmd,
       "show ip bgp vpnv4 all neighbors A.B.C.D",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information about all VPNv4 NLRIs\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n")

ALIAS (show_ip_bgp_neighbors_peer,
       show_ip_bgp_vpnv4_rd_neighbors_peer_cmd,
       "show ip bgp vpnv4 rd ASN:nn_or_IP-address:nn neighbors A.B.C.D",
       SHOW_STR
       IP_STR
       BGP_STR
       "Display VPNv4 NLRI specific information\n"
       "Display information about all VPNv4 NLRIs\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n")

ALIAS (show_ip_bgp_neighbors_peer,
       show_bgp_neighbors_peer_cmd,
       "show bgp neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       BGP_STR
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")

ALIAS (show_ip_bgp_neighbors_peer,
       show_bgp_ipv6_neighbors_peer_cmd,
       "show bgp ipv6 neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")

DEFUN (show_ip_bgp_instance_neighbors,
       show_ip_bgp_instance_neighbors_cmd,
       "show ip bgp view WORD neighbors",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Detailed information on TCP and BGP neighbor connections\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_instance_neighbors,
       show_bgp_instance_neighbors_cmd,
       "show bgp view WORD neighbors",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Detailed information on TCP and BGP neighbor connections\n")

ALIAS (show_ip_bgp_instance_neighbors,
       show_bgp_instance_ipv6_neighbors_cmd,
       "show bgp view WORD ipv6 neighbors",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Detailed information on TCP and BGP neighbor connections\n")

DEFUN (show_ip_bgp_instance_neighbors_peer,
       show_ip_bgp_instance_neighbors_peer_cmd,
       "show ip bgp view WORD neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_ip_bgp_instance_neighbors_peer,
       show_bgp_instance_neighbors_peer_cmd,
       "show bgp view WORD neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")

ALIAS (show_ip_bgp_instance_neighbors_peer,
       show_bgp_instance_ipv6_neighbors_peer_cmd,
       "show bgp view WORD ipv6 neighbors (A.B.C.D|X:X::X:X)",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Detailed information on TCP and BGP neighbor connections\n"
       "Neighbor to display information about\n"
       "Neighbor to display information about\n")

/* Show BGP's AS paths internal data.  There are both `show ip bgp
   paths' and `show ip mbgp paths'.  Those functions results are the
   same.*/
DEFUN (show_ip_bgp_paths,
       show_ip_bgp_paths_cmd,
       "show ip bgp paths",
       SHOW_STR
       IP_STR
       BGP_STR
       "Path information\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_ipv4_paths,
       show_ip_bgp_ipv4_paths_cmd,
       "show ip bgp ipv4 (unicast|multicast) paths",
       SHOW_STR
       IP_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Path information\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* Show BGP's community internal data. */
DEFUN (show_ip_bgp_community_info,
       show_ip_bgp_community_info_cmd,
       "show ip bgp community-info",
       SHOW_STR
       IP_STR
       BGP_STR
       "List all bgp community information\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_attr_info,
       show_ip_bgp_attr_info_cmd,
       "show ip bgp attribute-info",
       SHOW_STR
       IP_STR
       BGP_STR
       "List all bgp attribute information\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

/* 'show bgp rsclient' commands. */
DEFUN (show_ip_bgp_rsclient_summary,
       show_ip_bgp_rsclient_summary_cmd,
       "show ip bgp rsclient summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_instance_rsclient_summary,
       show_ip_bgp_instance_rsclient_summary_cmd,
       "show ip bgp view WORD rsclient summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_ipv4_rsclient_summary,
      show_ip_bgp_ipv4_rsclient_summary_cmd,
      "show ip bgp ipv4 (unicast|multicast) rsclient summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_ip_bgp_instance_ipv4_rsclient_summary,
      show_ip_bgp_instance_ipv4_rsclient_summary_cmd,
      "show ip bgp view WORD ipv4 (unicast|multicast) rsclient summary",
       SHOW_STR
       IP_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_bgp_instance_ipv4_safi_rsclient_summary,
       show_bgp_instance_ipv4_safi_rsclient_summary_cmd,
       "show bgp view WORD ipv4 (unicast|multicast) rsclient summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_bgp_instance_ipv4_safi_rsclient_summary,
       show_bgp_ipv4_safi_rsclient_summary_cmd,
       "show bgp ipv4 (unicast|multicast) rsclient summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")

#ifdef HAVE_IPV6
DEFUN (show_bgp_rsclient_summary,
       show_bgp_rsclient_summary_cmd,
       "show bgp rsclient summary",
       SHOW_STR
       BGP_STR
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (show_bgp_instance_rsclient_summary,
       show_bgp_instance_rsclient_summary_cmd,
       "show bgp view WORD rsclient summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_bgp_rsclient_summary,
      show_bgp_ipv6_rsclient_summary_cmd,
      "show bgp ipv6 rsclient summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")

ALIAS (show_bgp_instance_rsclient_summary,
      show_bgp_instance_ipv6_rsclient_summary_cmd,
       "show bgp view WORD ipv6 rsclient summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")

DEFUN (show_bgp_instance_ipv6_safi_rsclient_summary,
       show_bgp_instance_ipv6_safi_rsclient_summary_cmd,
       "show bgp view WORD ipv6 (unicast|multicast) rsclient summary",
       SHOW_STR
       BGP_STR
       "BGP view\n"
       "View name\n"
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (show_bgp_instance_ipv6_safi_rsclient_summary,
       show_bgp_ipv6_safi_rsclient_summary_cmd,
       "show bgp ipv6 (unicast|multicast) rsclient summary",
       SHOW_STR
       BGP_STR
       "Address family\n"
       "Address Family modifier\n"
       "Address Family modifier\n"
       "Information about Route Server Clients\n"
       "Summary of all Route Server Clients\n")

#endif /* HAVE IPV6 */

/* Redistribute VTY commands.  */

DEFUN (bgp_redistribute_ipv4,
       bgp_redistribute_ipv4_cmd,
       "redistribute " QUAGGA_IP_REDIST_STR_BGPD,
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv4_rmap,
       bgp_redistribute_ipv4_rmap_cmd,
       "redistribute " QUAGGA_IP_REDIST_STR_BGPD " route-map WORD",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv4_metric,
       bgp_redistribute_ipv4_metric_cmd,
       "redistribute " QUAGGA_IP_REDIST_STR_BGPD " metric <0-4294967295>",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv4_rmap_metric,
       bgp_redistribute_ipv4_rmap_metric_cmd,
       "redistribute " QUAGGA_IP_REDIST_STR_BGPD " route-map WORD metric <0-4294967295>",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n"
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv4_metric_rmap,
       bgp_redistribute_ipv4_metric_rmap_cmd,
       "redistribute " QUAGGA_IP_REDIST_STR_BGPD " metric <0-4294967295> route-map WORD",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n"
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv4,
       no_bgp_redistribute_ipv4_cmd,
       "no redistribute " QUAGGA_IP_REDIST_STR_BGPD,
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv4_rmap,
       no_bgp_redistribute_ipv4_rmap_cmd,
       "no redistribute " QUAGGA_IP_REDIST_STR_BGPD " route-map WORD",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv4_metric,
       no_bgp_redistribute_ipv4_metric_cmd,
       "no redistribute " QUAGGA_IP_REDIST_STR_BGPD " metric <0-4294967295>",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv4_rmap_metric,
       no_bgp_redistribute_ipv4_rmap_metric_cmd,
       "no redistribute " QUAGGA_IP_REDIST_STR_BGPD " route-map WORD metric <0-4294967295>",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n"
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_redistribute_ipv4_rmap_metric,
       no_bgp_redistribute_ipv4_metric_rmap_cmd,
       "no redistribute " QUAGGA_IP_REDIST_STR_BGPD " metric <0-4294967295> route-map WORD",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n"
       "Route map reference\n"
       "Pointer to route-map entries\n")

#ifdef HAVE_IPV6
DEFUN (bgp_redistribute_ipv6,
       bgp_redistribute_ipv6_cmd,
       "redistribute " QUAGGA_IP6_REDIST_STR_BGPD,
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv6_rmap,
       bgp_redistribute_ipv6_rmap_cmd,
       "redistribute " QUAGGA_IP6_REDIST_STR_BGPD " route-map WORD",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv6_metric,
       bgp_redistribute_ipv6_metric_cmd,
       "redistribute " QUAGGA_IP6_REDIST_STR_BGPD " metric <0-4294967295>",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv6_rmap_metric,
       bgp_redistribute_ipv6_rmap_metric_cmd,
       "redistribute " QUAGGA_IP6_REDIST_STR_BGPD " route-map WORD metric <0-4294967295>",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n"
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (bgp_redistribute_ipv6_metric_rmap,
       bgp_redistribute_ipv6_metric_rmap_cmd,
       "redistribute " QUAGGA_IP6_REDIST_STR_BGPD " metric <0-4294967295> route-map WORD",
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n"
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv6,
       no_bgp_redistribute_ipv6_cmd,
       "no redistribute " QUAGGA_IP6_REDIST_STR_BGPD,
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD)
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv6_rmap,
       no_bgp_redistribute_ipv6_rmap_cmd,
       "no redistribute " QUAGGA_IP6_REDIST_STR_BGPD " route-map WORD",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv6_metric,
       no_bgp_redistribute_ipv6_metric_cmd,
       "no redistribute " QUAGGA_IP6_REDIST_STR_BGPD " metric <0-4294967295>",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

DEFUN (no_bgp_redistribute_ipv6_rmap_metric,
       no_bgp_redistribute_ipv6_rmap_metric_cmd,
       "no redistribute " QUAGGA_IP6_REDIST_STR_BGPD " route-map WORD metric <0-4294967295>",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Route map reference\n"
       "Pointer to route-map entries\n"
       "Metric for redistributed routes\n"
       "Default metric\n")
{
    report_unimplemented_command(vty, argc, argv);
    return CMD_SUCCESS;
}

ALIAS (no_bgp_redistribute_ipv6_rmap_metric,
       no_bgp_redistribute_ipv6_metric_rmap_cmd,
       "no redistribute " QUAGGA_IP6_REDIST_STR_BGPD " metric <0-4294967295> route-map WORD",
       NO_STR
       "Redistribute information from another routing protocol\n"
       QUAGGA_IP6_REDIST_HELP_STR_BGPD
       "Metric for redistributed routes\n"
       "Default metric\n"
       "Route map reference\n"
       "Pointer to route-map entries\n")
#endif /* HAVE_IPV6 */
#ifndef ENABLE_OVSDB
/* BGP node structure. */
static struct cmd_node bgp_node =
{
  BGP_NODE,
  "%s(config-router)# ",
  1,
};
#endif
static struct cmd_node bgp_ipv4_unicast_node =
{
  BGP_IPV4_NODE,
  "%s(config-router-af)# ",
  1,
};

static struct cmd_node bgp_ipv4_multicast_node =
{
  BGP_IPV4M_NODE,
  "%s(config-router-af)# ",
  1,
};

static struct cmd_node bgp_ipv6_unicast_node =
{
  BGP_IPV6_NODE,
  "%s(config-router-af)# ",
  1,
};

static struct cmd_node bgp_ipv6_multicast_node =
{
  BGP_IPV6M_NODE,
  "%s(config-router-af)# ",
  1,
};

static struct cmd_node bgp_vpnv4_node =
{
  BGP_VPNV4_NODE,
  "%s(config-router-af)# ",
  1
};

void
bgp_vty_init (void)
{
    /* show bgp command */
    install_element (ENABLE_NODE, &vtysh_show_ip_bgp_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ip_bgp_route_cmd);
    install_element (ENABLE_NODE, &vtysh_show_ip_bgp_prefix_cmd);

    /* Install bgp top node. */
    // install_node(&bgp_node, bgp_config_write);
    install_node(&bgp_ipv4_unicast_node, NULL);
    install_node(&bgp_ipv4_multicast_node, NULL);
    install_node(&bgp_ipv6_unicast_node, NULL);
    install_node(&bgp_ipv6_multicast_node, NULL);
    install_node(&bgp_vpnv4_node, NULL);

    /* "bgp multiple-instance" commands. */
    install_element(CONFIG_NODE, &bgp_multiple_instance_cmd);
    install_element(CONFIG_NODE, &no_bgp_multiple_instance_cmd);

    /* "bgp config-type" commands. */
    install_element(CONFIG_NODE, &bgp_config_type_cmd);
    install_element(CONFIG_NODE, &no_bgp_config_type_cmd);

    /* Dummy commands (Currently not supported) */
    install_element(BGP_NODE, &no_synchronization_cmd);
    install_element(BGP_NODE, &no_auto_summary_cmd);

    /* "router bgp" commands. */
    install_element(CONFIG_NODE, &router_bgp_cmd);
    install_element(CONFIG_NODE, &router_bgp_view_cmd);

    /* "no router bgp" commands. */
    install_element(CONFIG_NODE, &no_router_bgp_cmd);
    install_element(CONFIG_NODE, &no_router_bgp_view_cmd);

    /* "bgp router-id" commands. */
    install_element(BGP_NODE, &bgp_router_id_cmd);
    install_element(BGP_NODE, &no_bgp_router_id_cmd);
    install_element(BGP_NODE, &no_bgp_router_id_val_cmd);

    /* "bgp cluster-id" commands. */
    install_element(BGP_NODE, &bgp_cluster_id_cmd);
    install_element(BGP_NODE, &bgp_cluster_id32_cmd);
    install_element(BGP_NODE, &no_bgp_cluster_id_cmd);
    install_element(BGP_NODE, &no_bgp_cluster_id_arg_cmd);

    /* "bgp confederation" commands. */
    install_element(BGP_NODE, &bgp_confederation_identifier_cmd);
    install_element(BGP_NODE, &no_bgp_confederation_identifier_cmd);
    install_element(BGP_NODE, &no_bgp_confederation_identifier_arg_cmd);

    /* "bgp confederation peers" commands. */
    install_element(BGP_NODE, &bgp_confederation_peers_cmd);
    install_element(BGP_NODE, &no_bgp_confederation_peers_cmd);

    /* "maximum-paths" commands. */
    install_element(BGP_NODE, &bgp_maxpaths_cmd);
    install_element(BGP_NODE, &no_bgp_maxpaths_cmd);
    install_element(BGP_NODE, &no_bgp_maxpaths_arg_cmd);
    install_element(BGP_IPV4_NODE, &bgp_maxpaths_cmd);
    install_element(BGP_IPV4_NODE, &no_bgp_maxpaths_cmd);
    install_element(BGP_IPV4_NODE, &no_bgp_maxpaths_arg_cmd);
    install_element(BGP_NODE, &bgp_maxpaths_ibgp_cmd);
    install_element(BGP_NODE, &no_bgp_maxpaths_ibgp_cmd);
    install_element(BGP_NODE, &no_bgp_maxpaths_ibgp_arg_cmd);
    install_element(BGP_IPV4_NODE, &bgp_maxpaths_ibgp_cmd);
    install_element(BGP_IPV4_NODE, &no_bgp_maxpaths_ibgp_cmd);
    install_element(BGP_IPV4_NODE, &no_bgp_maxpaths_ibgp_arg_cmd);

    /* "timers bgp" commands. */
    install_element(BGP_NODE, &bgp_timers_cmd);
    install_element(BGP_NODE, &no_bgp_timers_cmd);
    install_element(BGP_NODE, &no_bgp_timers_arg_cmd);

    /* "bgp client-to-client reflection" commands */
    install_element(BGP_NODE, &no_bgp_client_to_client_reflection_cmd);
    install_element(BGP_NODE, &bgp_client_to_client_reflection_cmd);

    /* "bgp always-compare-med" commands */
    install_element(BGP_NODE, &bgp_always_compare_med_cmd);
    install_element(BGP_NODE, &no_bgp_always_compare_med_cmd);

    /* "bgp deterministic-med" commands */
    install_element(BGP_NODE, &bgp_deterministic_med_cmd);
    install_element(BGP_NODE, &no_bgp_deterministic_med_cmd);

    /* "bgp graceful-restart" commands */
    install_element(BGP_NODE, &bgp_graceful_restart_cmd);
    install_element(BGP_NODE, &no_bgp_graceful_restart_cmd);
    install_element(BGP_NODE, &bgp_graceful_restart_stalepath_time_cmd);
    install_element(BGP_NODE, &no_bgp_graceful_restart_stalepath_time_cmd);
    install_element(BGP_NODE, &no_bgp_graceful_restart_stalepath_time_val_cmd);

    /* "bgp fast-external-failover" commands */
    install_element(BGP_NODE, &bgp_fast_external_failover_cmd);
    install_element(BGP_NODE, &no_bgp_fast_external_failover_cmd);

    /* "bgp enforce-first-as" commands */
    install_element(BGP_NODE, &bgp_enforce_first_as_cmd);
    install_element(BGP_NODE, &no_bgp_enforce_first_as_cmd);

    /* "bgp bestpath compare-routerid" commands */
    install_element(BGP_NODE, &bgp_bestpath_compare_router_id_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_compare_router_id_cmd);

    /* "bgp bestpath as-path ignore" commands */
    install_element(BGP_NODE, &bgp_bestpath_aspath_ignore_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_aspath_ignore_cmd);

    /* "bgp bestpath as-path confed" commands */
    install_element(BGP_NODE, &bgp_bestpath_aspath_confed_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_aspath_confed_cmd);

    /* "bgp bestpath as-path multipath-relax" commands */
    install_element(BGP_NODE, &bgp_bestpath_aspath_multipath_relax_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_aspath_multipath_relax_cmd);

    /* "bgp log-neighbor-changes" commands */
    install_element(BGP_NODE, &bgp_log_neighbor_changes_cmd);
    install_element(BGP_NODE, &no_bgp_log_neighbor_changes_cmd);

    /* "bgp bestpath med" commands */
    install_element(BGP_NODE, &bgp_bestpath_med_cmd);
    install_element(BGP_NODE, &bgp_bestpath_med2_cmd);
    install_element(BGP_NODE, &bgp_bestpath_med3_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_med_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_med2_cmd);
    install_element(BGP_NODE, &no_bgp_bestpath_med3_cmd);

    /* "no bgp default ipv4-unicast" commands. */
    install_element(BGP_NODE, &no_bgp_default_ipv4_unicast_cmd);
    install_element(BGP_NODE, &bgp_default_ipv4_unicast_cmd);

    /* "bgp network" commands. */
    install_element(BGP_NODE, &bgp_network_cmd);
    install_element(BGP_NODE, &no_bgp_network_cmd);

    /* "bgp network import-check" commands. */
    install_element(BGP_NODE, &bgp_network_import_check_cmd);
    install_element(BGP_NODE, &no_bgp_network_import_check_cmd);

    /* "bgp default local-preference" commands. */
    install_element(BGP_NODE, &bgp_default_local_preference_cmd);
    install_element(BGP_NODE, &no_bgp_default_local_preference_cmd);
    install_element(BGP_NODE, &no_bgp_default_local_preference_val_cmd);

    /* "neighbor remote-as" commands. */
    install_element(BGP_NODE, &neighbor_remote_as_cmd);
    install_element(BGP_NODE, &no_neighbor_cmd);

    /* "neighbor peer-group" commands. */
    install_element(BGP_NODE, &neighbor_peer_group_cmd);
    install_element(BGP_NODE, &no_neighbor_peer_group_cmd);
    install_element(BGP_NODE, &no_neighbor_peer_group_remote_as_cmd);

    /* "neighbor local-as" commands. */
    install_element(BGP_NODE, &neighbor_local_as_cmd);
    install_element(BGP_NODE, &neighbor_local_as_no_prepend_cmd);
    install_element(BGP_NODE, &neighbor_local_as_no_prepend_replace_as_cmd);
    install_element(BGP_NODE, &no_neighbor_local_as_cmd);
    install_element(BGP_NODE, &no_neighbor_local_as_val_cmd);
    install_element(BGP_NODE, &no_neighbor_local_as_val2_cmd);
    install_element(BGP_NODE, &no_neighbor_local_as_val3_cmd);

    /* "neighbor password" commands. */
    install_element(BGP_NODE, &neighbor_password_cmd);
    install_element(BGP_NODE, &no_neighbor_password_cmd);

    /* "neighbor activate" commands. */
    install_element(BGP_NODE, &neighbor_activate_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_activate_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_activate_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_activate_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_activate_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_activate_cmd);

    /* "no neighbor activate" commands. */
    install_element(BGP_NODE, &no_neighbor_activate_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_activate_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_activate_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_activate_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_activate_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_activate_cmd);

    /* "neighbor peer-group set" commands. */
    install_element(BGP_NODE, &neighbor_set_peer_group_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_set_peer_group_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_set_peer_group_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_set_peer_group_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_set_peer_group_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_set_peer_group_cmd);

    /* "no neighbor peer-group unset" commands. */
    install_element(BGP_NODE, &no_neighbor_set_peer_group_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_set_peer_group_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_set_peer_group_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_set_peer_group_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_set_peer_group_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_set_peer_group_cmd);

    /* "neighbor softreconfiguration inbound" commands.*/
    install_element(BGP_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_NODE, &no_neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_soft_reconfiguration_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_soft_reconfiguration_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_soft_reconfiguration_cmd);

    /* "neighbor attribute-unchanged" commands.  */
    install_element(BGP_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_NODE, &no_neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_attr_unchanged10_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged1_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged2_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged3_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged4_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged5_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged6_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged7_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged8_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged9_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_attr_unchanged10_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged1_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged2_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged3_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged4_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged5_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged6_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged7_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged8_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged9_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_attr_unchanged10_cmd);

    /* "nexthop-local unchanged" commands */
    install_element(BGP_IPV6_NODE, &neighbor_nexthop_local_unchanged_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_nexthop_local_unchanged_cmd);

    /* "transparent-as" and "transparent-nexthop" for old version
    compatibility.  */
    install_element(BGP_NODE, &neighbor_transparent_as_cmd);
    install_element(BGP_NODE, &neighbor_transparent_nexthop_cmd);

    /* "neighbor next-hop-self" commands. */
    install_element(BGP_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_NODE, &no_neighbor_nexthop_self_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_nexthop_self_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_nexthop_self_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_nexthop_self_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_nexthop_self_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_nexthop_self_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_nexthop_self_cmd);

    /* "neighbor remove-private-AS" commands. */
    install_element(BGP_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_NODE, &no_neighbor_remove_private_as_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_remove_private_as_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_remove_private_as_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_remove_private_as_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_remove_private_as_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_remove_private_as_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_remove_private_as_cmd);

    /* "neighbor send-community" commands.*/
    install_element(BGP_NODE, &neighbor_send_community_cmd);
    install_element(BGP_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_NODE, &no_neighbor_send_community_type_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_send_community_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_send_community_type_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_send_community_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_send_community_type_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_send_community_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_send_community_type_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_send_community_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_send_community_type_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_send_community_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_send_community_type_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_send_community_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_send_community_type_cmd);

    /* "neighbor route-reflector" commands.*/
    install_element(BGP_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_NODE, &no_neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_route_reflector_client_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_route_reflector_client_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_route_reflector_client_cmd);

    /* "neighbor route-server" commands.*/
    install_element(BGP_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_NODE, &no_neighbor_route_server_client_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_route_server_client_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_route_server_client_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_route_server_client_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_route_server_client_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_route_server_client_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_route_server_client_cmd);

    /* "neighbor passive" commands. */
    install_element(BGP_NODE, &neighbor_passive_cmd);
    install_element(BGP_NODE, &no_neighbor_passive_cmd);

    /* "neighbor shutdown" commands. */
    install_element(BGP_NODE, &neighbor_shutdown_cmd);
    install_element(BGP_NODE, &no_neighbor_shutdown_cmd);

    /* Deprecated "neighbor capability route-refresh" commands.*/
    install_element(BGP_NODE, &neighbor_capability_route_refresh_cmd);
    install_element(BGP_NODE, &no_neighbor_capability_route_refresh_cmd);

    /* "neighbor capability orf prefix-list" commands.*/
    install_element(BGP_NODE, &neighbor_capability_orf_prefix_cmd);
    install_element(BGP_NODE, &no_neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_capability_orf_prefix_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_capability_orf_prefix_cmd);

    /* "neighbor capability dynamic" commands.*/
    install_element(BGP_NODE, &neighbor_capability_dynamic_cmd);
    install_element(BGP_NODE, &no_neighbor_capability_dynamic_cmd);

    /* "neighbor dont-capability-negotiate" commands. */
    install_element(BGP_NODE, &neighbor_dont_capability_negotiate_cmd);
    install_element(BGP_NODE, &no_neighbor_dont_capability_negotiate_cmd);

    /* "neighbor ebgp-multihop" commands. */
    install_element(BGP_NODE, &neighbor_ebgp_multihop_cmd);
    install_element(BGP_NODE, &neighbor_ebgp_multihop_ttl_cmd);
    install_element(BGP_NODE, &no_neighbor_ebgp_multihop_cmd);
    install_element(BGP_NODE, &no_neighbor_ebgp_multihop_ttl_cmd);

    /* "neighbor disable-connected-check" commands.  */
    install_element(BGP_NODE, &neighbor_disable_connected_check_cmd);
    install_element(BGP_NODE, &no_neighbor_disable_connected_check_cmd);
    install_element(BGP_NODE, &neighbor_enforce_multihop_cmd);
    install_element(BGP_NODE, &no_neighbor_enforce_multihop_cmd);

    /* "neighbor description" commands. */
    install_element(BGP_NODE, &neighbor_description_cmd);
    install_element(BGP_NODE, &no_neighbor_description_cmd);
    install_element(BGP_NODE, &no_neighbor_description_val_cmd);

    /* "neighbor update-source" commands. "*/
    // install_element(BGP_NODE, &neighbor_update_source_cmd);
    install_element(BGP_NODE, &no_neighbor_update_source_cmd);

    /* "neighbor default-originate" commands. */
    install_element(BGP_NODE, &neighbor_default_originate_cmd);
    install_element(BGP_NODE, &neighbor_default_originate_rmap_cmd);
    install_element(BGP_NODE, &no_neighbor_default_originate_cmd);
    install_element(BGP_NODE, &no_neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_default_originate_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_default_originate_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_default_originate_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_default_originate_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_default_originate_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_default_originate_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_default_originate_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_default_originate_rmap_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_default_originate_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_default_originate_rmap_cmd);

    /* "neighbor port" commands. */
    install_element(BGP_NODE, &neighbor_port_cmd);
    install_element(BGP_NODE, &no_neighbor_port_cmd);
    install_element(BGP_NODE, &no_neighbor_port_val_cmd);

    /* "neighbor weight" commands. */
    install_element(BGP_NODE, &neighbor_weight_cmd);
    install_element(BGP_NODE, &no_neighbor_weight_cmd);
    install_element(BGP_NODE, &no_neighbor_weight_val_cmd);

    /* "neighbor override-capability" commands. */
    install_element(BGP_NODE, &neighbor_override_capability_cmd);
    install_element(BGP_NODE, &no_neighbor_override_capability_cmd);

    /* "neighbor strict-capability-match" commands. */
    install_element(BGP_NODE, &neighbor_strict_capability_cmd);
    install_element(BGP_NODE, &no_neighbor_strict_capability_cmd);

    /* "neighbor timers" commands. */
    install_element(BGP_NODE, &neighbor_timers_cmd);
    install_element(BGP_NODE, &no_neighbor_timers_cmd);

    /* "neighbor timers connect" commands. */
    install_element(BGP_NODE, &neighbor_timers_connect_cmd);
    install_element(BGP_NODE, &no_neighbor_timers_connect_cmd);
    install_element(BGP_NODE, &no_neighbor_timers_connect_val_cmd);

    /* "neighbor advertisement-interval" commands. */
    install_element(BGP_NODE, &neighbor_advertise_interval_cmd);
    install_element(BGP_NODE, &no_neighbor_advertise_interval_cmd);
    install_element(BGP_NODE, &no_neighbor_advertise_interval_val_cmd);

    /* "neighbor version" commands. */
    install_element(BGP_NODE, &neighbor_version_cmd);

    /* "neighbor interface" commands. */
    install_element(BGP_NODE, &neighbor_interface_cmd);
    install_element(BGP_NODE, &no_neighbor_interface_cmd);

    /* "neighbor distribute" commands. */
    install_element(BGP_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_NODE, &no_neighbor_distribute_list_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_distribute_list_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_distribute_list_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_distribute_list_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_distribute_list_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_distribute_list_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_distribute_list_cmd);

    /* "neighbor prefix-list" commands. */
    install_element(BGP_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_NODE, &no_neighbor_prefix_list_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_prefix_list_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_prefix_list_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_prefix_list_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_prefix_list_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_prefix_list_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_prefix_list_cmd);

    /* "neighbor filter-list" commands. */
    install_element(BGP_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_NODE, &no_neighbor_filter_list_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_filter_list_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_filter_list_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_filter_list_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_filter_list_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_filter_list_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_filter_list_cmd);

    /* "neighbor route-map" commands. */
    install_element(BGP_NODE, &neighbor_route_map_cmd);
    install_element(BGP_NODE, &no_neighbor_route_map_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_route_map_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_route_map_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_route_map_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_route_map_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_route_map_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_route_map_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_route_map_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_route_map_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_route_map_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_route_map_cmd);

    /* "neighbor unsuppress-map" commands. */
    install_element(BGP_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_NODE, &no_neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_unsuppress_map_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_unsuppress_map_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_unsuppress_map_cmd);

    /* "neighbor maximum-prefix" commands. */
    install_element(BGP_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_maximum_prefix_threshold_restart_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_val_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_threshold_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_warning_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_threshold_warning_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_restart_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_maximum_prefix_threshold_restart_cmd);

    /* "neighbor allowas-in" */
    install_element(BGP_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_NODE, &no_neighbor_allowas_in_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_IPV4_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_IPV4_NODE, &no_neighbor_allowas_in_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_IPV4M_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_IPV4M_NODE, &no_neighbor_allowas_in_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_IPV6_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_IPV6_NODE, &no_neighbor_allowas_in_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_IPV6M_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_IPV6M_NODE, &no_neighbor_allowas_in_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_allowas_in_cmd);
    install_element(BGP_VPNV4_NODE, &neighbor_allowas_in_arg_cmd);
    install_element(BGP_VPNV4_NODE, &no_neighbor_allowas_in_cmd);

    /* address-family commands. */
    install_element(BGP_NODE, &address_family_ipv4_cmd);
    install_element(BGP_NODE, &address_family_ipv4_safi_cmd);
#ifdef HAVE_IPV6
    install_element(BGP_NODE, &address_family_ipv6_safi_cmd);
#endif /* HAVE_IPV6 */

    /* "exit-address-family" command. */
    // install_element(BGP_IPV4_NODE, &exit_address_family_cmd);
    // install_element(BGP_IPV4M_NODE, &exit_address_family_cmd);
    // install_element(BGP_IPV6_NODE, &exit_address_family_cmd);
    // install_element(BGP_IPV6M_NODE, &exit_address_family_cmd);
    // install_element(BGP_VPNV4_NODE, &exit_address_family_cmd);

    /* "clear ip bgp commands" */
    install_element(ENABLE_NODE, &clear_ip_bgp_all_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &clear_bgp_all_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_all_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_cmd);
#endif /* HAVE_IPV6 */

    /* "clear ip bgp neighbor soft in" */
    install_element(ENABLE_NODE, &clear_ip_bgp_all_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_vpnv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_vpnv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_vpnv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_vpnv4_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_vpnv4_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_vpnv4_in_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &clear_bgp_all_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_all_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_all_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_all_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_in_prefix_filter_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_soft_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_in_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_in_prefix_filter_cmd);
#endif /* HAVE_IPV6 */

    /* "clear ip bgp neighbor soft out" */
    install_element(ENABLE_NODE, &clear_ip_bgp_all_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_vpnv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_vpnv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_vpnv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_vpnv4_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_vpnv4_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_vpnv4_out_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &clear_bgp_all_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_all_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_all_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_soft_out_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_out_cmd);
#endif /* HAVE_IPV6 */

    /* "clear ip bgp neighbor soft" */
    install_element(ENABLE_NODE, &clear_ip_bgp_all_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_group_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_external_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_ipv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_all_vpnv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_vpnv4_soft_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_as_vpnv4_soft_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &clear_bgp_all_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_all_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_group_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_external_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_as_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_group_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_external_soft_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_as_soft_cmd);
#endif /* HAVE_IPV6 */

    /* "clear ip bgp neighbor rsclient" */
    install_element(ENABLE_NODE, &clear_ip_bgp_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_peer_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_ip_bgp_instance_peer_rsclient_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &clear_bgp_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_instance_all_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_peer_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_instance_peer_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_peer_rsclient_cmd);
    install_element(ENABLE_NODE, &clear_bgp_ipv6_instance_peer_rsclient_cmd);
#endif /* HAVE_IPV6 */

    /* "show ip bgp summary" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_ipv4_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv4_safi_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_ipv4_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv4_safi_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_all_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_rd_summary_cmd);
#ifdef HAVE_IPV6
    install_element(VIEW_NODE, &show_bgp_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_safi_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_safi_summary_cmd);
#endif /* HAVE_IPV6 */
    install_element(RESTRICTED_NODE, &show_ip_bgp_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_instance_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_ipv4_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv4_safi_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_instance_ipv4_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv4_safi_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_vpnv4_all_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_vpnv4_rd_summary_cmd);
#ifdef HAVE_IPV6
    install_element(RESTRICTED_NODE, &show_bgp_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv6_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv6_safi_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv6_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv6_safi_summary_cmd);
#endif /* HAVE_IPV6 */
    install_element(ENABLE_NODE, &show_ip_bgp_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_ipv4_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv4_safi_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_ipv4_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv4_safi_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_all_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_rd_summary_cmd);
#ifdef HAVE_IPV6
    install_element(ENABLE_NODE, &show_bgp_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_safi_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_safi_summary_cmd);
#endif /* HAVE_IPV6 */

    /* "show ip bgp neighbors" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_neighbors_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_ipv4_neighbors_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_ipv4_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_all_neighbors_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_rd_neighbors_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_all_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_vpnv4_rd_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_neighbors_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_ipv4_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_vpnv4_all_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_vpnv4_rd_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_instance_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_neighbors_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_ipv4_neighbors_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_ipv4_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_all_neighbors_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_rd_neighbors_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_all_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_vpnv4_rd_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_neighbors_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_neighbors_peer_cmd);

#ifdef HAVE_IPV6
    install_element(VIEW_NODE, &show_bgp_neighbors_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_neighbors_cmd);
    install_element(VIEW_NODE, &show_bgp_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_neighbors_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_neighbors_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_neighbors_peer_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv6_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_neighbors_peer_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv6_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_bgp_neighbors_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_neighbors_cmd);
    install_element(ENABLE_NODE, &show_bgp_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_neighbors_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_neighbors_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_neighbors_peer_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_neighbors_peer_cmd);

    /* Old commands.  */
    install_element(VIEW_NODE, &show_ipv6_bgp_summary_cmd);
    install_element(VIEW_NODE, &show_ipv6_mbgp_summary_cmd);
    install_element(ENABLE_NODE, &show_ipv6_bgp_summary_cmd);
    install_element(ENABLE_NODE, &show_ipv6_mbgp_summary_cmd);
#endif /* HAVE_IPV6 */

    /* "show ip bgp rsclient" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_ipv4_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_instance_ipv4_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv4_safi_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv4_safi_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_instance_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_ipv4_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_ip_bgp_instance_ipv4_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv4_safi_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv4_safi_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_ipv4_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_instance_ipv4_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv4_safi_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv4_safi_rsclient_summary_cmd);

#ifdef HAVE_IPV6
    install_element(VIEW_NODE, &show_bgp_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_instance_ipv6_safi_rsclient_summary_cmd);
    install_element(VIEW_NODE, &show_bgp_ipv6_safi_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv6_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv6_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_instance_ipv6_safi_rsclient_summary_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_ipv6_safi_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_instance_ipv6_safi_rsclient_summary_cmd);
    install_element(ENABLE_NODE, &show_bgp_ipv6_safi_rsclient_summary_cmd);
#endif /* HAVE_IPV6 */

    /* "show ip bgp paths" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_paths_cmd);
    install_element(VIEW_NODE, &show_ip_bgp_ipv4_paths_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_paths_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_ipv4_paths_cmd);

    /* "show ip bgp community" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_community_info_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_community_info_cmd);

    /* "show ip bgp attribute-info" commands. */
    install_element(VIEW_NODE, &show_ip_bgp_attr_info_cmd);
    install_element(ENABLE_NODE, &show_ip_bgp_attr_info_cmd);

    /* "redistribute" commands.  */
    install_element(BGP_NODE, &bgp_redistribute_ipv4_cmd);
    install_element(BGP_NODE, &no_bgp_redistribute_ipv4_cmd);
    install_element(BGP_NODE, &bgp_redistribute_ipv4_rmap_cmd);
    install_element(BGP_NODE, &no_bgp_redistribute_ipv4_rmap_cmd);
    install_element(BGP_NODE, &bgp_redistribute_ipv4_metric_cmd);
    install_element(BGP_NODE, &no_bgp_redistribute_ipv4_metric_cmd);
    install_element(BGP_NODE, &bgp_redistribute_ipv4_rmap_metric_cmd);
    install_element(BGP_NODE, &bgp_redistribute_ipv4_metric_rmap_cmd);
    install_element(BGP_NODE, &no_bgp_redistribute_ipv4_rmap_metric_cmd);
    install_element(BGP_NODE, &no_bgp_redistribute_ipv4_metric_rmap_cmd);
#ifdef HAVE_IPV6
    install_element(BGP_IPV6_NODE, &bgp_redistribute_ipv6_cmd);
    install_element(BGP_IPV6_NODE, &no_bgp_redistribute_ipv6_cmd);
    install_element(BGP_IPV6_NODE, &bgp_redistribute_ipv6_rmap_cmd);
    install_element(BGP_IPV6_NODE, &no_bgp_redistribute_ipv6_rmap_cmd);
    install_element(BGP_IPV6_NODE, &bgp_redistribute_ipv6_metric_cmd);
    install_element(BGP_IPV6_NODE, &no_bgp_redistribute_ipv6_metric_cmd);
    install_element(BGP_IPV6_NODE, &bgp_redistribute_ipv6_rmap_metric_cmd);
    install_element(BGP_IPV6_NODE, &bgp_redistribute_ipv6_metric_rmap_cmd);
    install_element(BGP_IPV6_NODE, &no_bgp_redistribute_ipv6_rmap_metric_cmd);
    install_element(BGP_IPV6_NODE, &no_bgp_redistribute_ipv6_metric_rmap_cmd);
#endif /* HAVE_IPV6 */

    /* ttl_security commands */
    install_element(BGP_NODE, &neighbor_ttl_security_cmd);
    install_element(BGP_NODE, &no_neighbor_ttl_security_cmd);

    /* "show bgp memory" commands. */
    install_element(VIEW_NODE, &show_bgp_memory_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_memory_cmd);
    install_element(ENABLE_NODE, &show_bgp_memory_cmd);

    /* "show bgp views" commands. */
    install_element(VIEW_NODE, &show_bgp_views_cmd);
    install_element(RESTRICTED_NODE, &show_bgp_views_cmd);
    install_element(ENABLE_NODE, &show_bgp_views_cmd);
}

/*
 * Prefix List
 */

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

const struct ovsrec_prefix_list *
policy_get_prefix_list_in_ovsdb (const char *name)
{
    const struct ovsrec_prefix_list *policy_row;

    OVSREC_PREFIX_LIST_FOR_EACH(policy_row, idl) {
        if (strcmp(policy_row->name, name) == 0) {
            return policy_row;
        }
    }
    return NULL;
}


struct ovsrec_prefix_list_entry *
policy_get_prefix_list_entry_in_ovsdb(int seqnum, char *name, const char *action)
{
    const struct ovsrec_prefix_list *policy_row;
    struct ovsrec_prefix_list_entry *policy_entry_row;
    int i = 0;

    OVSREC_PREFIX_LIST_FOR_EACH(policy_row, idl) {
        if (strcmp(name, policy_row->name) == 0) {
            for(i = 0; i < policy_row->n_prefix_list_entries; i++) {
                if (policy_row->key_prefix_list_entries[i] == seqnum) {
                    policy_entry_row = policy_row->value_prefix_list_entries[i];

                    if (!strcmp(policy_entry_row->action, action)) {
                        return policy_entry_row;
                    }
                }
            }
        }
    }
    return NULL;
}

void
bgp_prefix_list_entry_insert_to_prefix_list(struct ovsrec_prefix_list *policy_row,
                              struct ovsrec_prefix_list_entry  *policy_entry_row,
                              int64_t seq)
{
    int64_t *pref_list;
    struct ovsrec_prefix_list_entry  **policy_entry_list;
    int i, new_size;

    new_size = policy_row->n_prefix_list_entries + 1;
    pref_list = xmalloc(sizeof(int64_t) * new_size);
    policy_entry_list = xmalloc(sizeof * policy_row->value_prefix_list_entries *
                                new_size);

    for (i = 0; i < policy_row->n_prefix_list_entries; i++) {
        pref_list[i] = policy_row->key_prefix_list_entries[i];
        policy_entry_list[i] = policy_row->value_prefix_list_entries[i];
    }
    pref_list[policy_row->n_prefix_list_entries] = seq;
    policy_entry_list[policy_row->n_prefix_list_entries] = policy_entry_row;
    ovsrec_prefix_list_set_prefix_list_entries(policy_row, pref_list,
                                               policy_entry_list, new_size);

    free(pref_list);
    free(policy_entry_list);
}

void
bgp_prefix_list_entry_remove_from_prefix_list(
    struct ovsrec_prefix_list *policy_row,
    int64_t seq)
{
    int64_t *pref_list;
    struct ovsrec_prefix_list_entry  **policy_entry_list;
    int i, j, new_size;

    new_size = policy_row->n_prefix_list_entries - 1;
    pref_list = xmalloc(sizeof(int64_t) * new_size);
    policy_entry_list = xmalloc(sizeof * policy_row->value_prefix_list_entries *
                                new_size);

    for (i = 0, j = 0; i < policy_row->n_prefix_list_entries; i++) {
        if(policy_row->key_prefix_list_entries[i] != seq) {
            pref_list[j] = policy_row->key_prefix_list_entries[i];
            policy_entry_list[j] = policy_row->value_prefix_list_entries[i];
            j++;
        }
    }
    ovsrec_prefix_list_set_prefix_list_entries(policy_row, pref_list,
                                               policy_entry_list, new_size);

    free(pref_list);
    free(policy_entry_list);
}

/*
 * IP Address Prefix List
 */
static int
policy_set_prefix_list_in_ovsdb (struct vty *vty, afi_t afi, const char *name,
                         const char *seq, const char *typestr,
                         const char *prefix, const char *ge, const char *le)

{
    struct ovsdb_idl_txn *policy_txn;
    const struct ovsrec_prefix_list *policy_row;
    const struct ovsrec_prefix_list_entry  *policy_entry_row;
    enum ovsdb_idl_txn_status status;
    int ret;
    struct prefix p;
    int seqnum = -1;

    /* Sequential number. */
    if (seq)
        seqnum = atoi (seq);

    /* Check filter type. */
    if (strncmp ("permit", typestr, 1) == 0){}
    else if (strncmp ("deny", typestr, 1) == 0){}
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
        }
        else
            ret = str2prefix_ipv4 (prefix, (struct prefix_ipv4 *) &p);

        if (ret <= 0)
        {
            vty_out (vty, "%% Malformed IPv4 prefix%s", VTY_NEWLINE);
            return CMD_WARNING;
        }
    }

    START_DB_TXN(policy_txn);

    /*
     * If 'name' row already exists get a row structure pointer
     */
    policy_row = policy_get_prefix_list_in_ovsdb (name);
    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!policy_row) {
        policy_row = ovsrec_prefix_list_insert(policy_txn);
        ovsrec_prefix_list_set_name(policy_row, name);
    }

    policy_entry_row = policy_get_prefix_list_entry_in_ovsdb(seqnum, name,
                                                             typestr);

    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!policy_entry_row) {
        policy_entry_row = ovsrec_prefix_list_entry_insert(policy_txn);
        bgp_prefix_list_entry_insert_to_prefix_list(policy_row, policy_entry_row,
                                                   (int64_t)seqnum);
    }

    ovsrec_prefix_list_entry_set_action(policy_entry_row, typestr);
    ovsrec_prefix_list_entry_set_prefix(policy_entry_row, prefix);

    END_DB_TXN(policy_txn);
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

static int
cli_no_ip_prefix_list_cmd_execute(const char *name)
{
    struct ovsrec_prefix_list *plist_row;
    struct ovsdb_idl_txn *policy_txn;

    /* Start of transaction */
    START_DB_TXN(policy_txn);

    plist_row = policy_get_prefix_list_in_ovsdb(name);
    if (!plist_row) {
        ERRONEOUS_DB_TXN(policy_txn, "Prefix List not found");
    }

    ovsrec_prefix_list_delete(plist_row);

    /* End of transaction */
    END_DB_TXN(policy_txn);
}

DEFUN (no_ip_prefix_list,
       no_ip_prefix_list_cmd,
       "no ip prefix-list WORD",
       NO_STR
       IP_STR
       PREFIX_LIST_STR
       "Name of a prefix list\n")
{
  return cli_no_ip_prefix_list_cmd_execute(argv[0]);
}

static int
cli_no_ip_prefix_list_seq_cmd_execute(const char *name, const char *seq,
                                      const char *action, const char *prefix)
{
    VLOG_DBG("Deleting any prefix list entries...");
    struct ovsrec_prefix_list *plist_row;
    struct ovsrec_prefix_list_entry *plist_entry;
    struct ovsdb_idl_txn *policy_txn;
    bool deleted = false;
    int seqnum = -1;

    /* Start of transaction */
    START_DB_TXN(policy_txn);

    /* Sequential number. */
    if (seq)
        seqnum = atoi(seq);
    else
        ERRONEOUS_DB_TXN(policy_txn, "Invalid seq number");

    plist_row = policy_get_prefix_list_in_ovsdb(name);
    if (!plist_row) {
        ERRONEOUS_DB_TXN(policy_txn, "Prefix List not found");
    }

    plist_entry = policy_get_prefix_list_entry_in_ovsdb(seqnum, name, action);
    if (!plist_entry) {
        ERRONEOUS_DB_TXN(policy_txn, "Prefix List entry not found");
    }

    /* Need to remove the reference to the prefix-list entry from
     * the prefix-list table first.
     */
    bgp_prefix_list_entry_remove_from_prefix_list(plist_row, seqnum);
    ovsrec_prefix_list_entry_delete(plist_entry);

    /* End of transaction */
    END_DB_TXN(policy_txn);
}

DEFUN (no_ip_prefix_list_seq,
       no_ip_prefix_list_seq_cmd,
       "no ip prefix-list WORD seq <1-4294967295> (deny|permit) (A.B.C.D/M|any)",
       NO_STR
       IP_STR
       PREFIX_LIST_STR
       "Name of a prefix list\n"
       "sequence number of an entry\n"
       "Sequence number\n"
       "Specify packets to reject\n"
       "Specify packets to forward\n"
       "IP prefix <network>/<length>, e.g., 35.0.0.0/8\n"
       "Any prefix match.  Same as \"0.0.0.0/0 le 32\"\n")
{
  return cli_no_ip_prefix_list_seq_cmd_execute(argv[0], argv[1], argv[2],
                                               argv[3]);
}

/*
 * Route Map Start Below
 */
const struct ovsrec_route_map *
policy_get_route_map_in_ovsdb (const char * name)
{
  const struct ovsrec_route_map *rt_map_row;

  OVSREC_ROUTE_MAP_FOR_EACH(rt_map_row, idl) {
    if (strcmp(rt_map_row->name, name) == 0) {
       return rt_map_row;
    }
  }
  return NULL;
}

const struct ovsrec_route_map_entry  *
policy_get_route_map_entry_in_ovsdb (unsigned long pref, const char *name,
                                     char *action)
{
    const struct ovsrec_route_map *rt_map_row;
    struct ovsrec_route_map_entry *rt_map_entry_row;
    bool match = false;
    int i = 0;

    OVSREC_ROUTE_MAP_FOR_EACH(rt_map_row, idl) {
        if(strcmp(name, rt_map_row->name) == 0) {
            for ( i = 0 ; i < rt_map_row->n_route_map_entries; i++) {
                if (rt_map_row->key_route_map_entries[i] == pref) {
                    rt_map_entry_row = rt_map_row->value_route_map_entries[i];

                    if (!strcmp(rt_map_entry_row->action, action)) {
                        return rt_map_entry_row;
                    }
                }
            }
        }
    }
    return NULL;
}

void
bgp_route_map_entry_insert_to_route_map(struct ovsrec_route_map *rt_map_row,
                              struct ovsrec_route_map_entry *rt_map_entry_row,
                              int64_t seq)
{
    int64_t *pref_list;
    struct ovsrec_route_map_entry  **rt_map_entry_list;
    int i, new_size;

    new_size = rt_map_row->n_route_map_entries + 1;
    pref_list = xmalloc(sizeof(int64_t) * new_size);
    rt_map_entry_list = xmalloc(sizeof * rt_map_row->value_route_map_entries *
                                new_size);

    for (i = 0; i < rt_map_row->n_route_map_entries; i++) {
        pref_list[i] = rt_map_row->key_route_map_entries[i];
        rt_map_entry_list[i] = rt_map_row->value_route_map_entries[i];
    }
    pref_list[rt_map_row->n_route_map_entries] = seq;
    rt_map_entry_list[rt_map_row->n_route_map_entries] = rt_map_entry_row;
    ovsrec_route_map_set_route_map_entries(rt_map_row, pref_list,
                                           rt_map_entry_list, new_size);

    free(pref_list);
    free(rt_map_entry_list);
}

void
bgp_route_map_entry_remove_from_route_map(
    struct ovsrec_route_map *rt_map_row,
    int64_t seq)
{
    int64_t *pref_list;
    struct ovsrec_route_map_entry  **rt_map_entry_list;
    int i, j, new_size;

    new_size = rt_map_row->n_route_map_entries - 1;
    pref_list = xmalloc(sizeof(int64_t) * new_size);
    rt_map_entry_list = xmalloc(sizeof * rt_map_row->value_route_map_entries *
                                new_size);

    for (i = 0, j = 0; i < rt_map_row->n_route_map_entries; i++) {
        if(rt_map_row->key_route_map_entries[i] != seq) {
            pref_list[j] = rt_map_row->key_route_map_entries[i];
            rt_map_entry_list[j] = rt_map_row->value_route_map_entries[i];
            j++;
        }
    }
    ovsrec_route_map_set_route_map_entries(rt_map_row, pref_list,
                                           rt_map_entry_list, new_size);

    free(pref_list);
    free(rt_map_entry_list);
}

static int
policy_route_map_get_seq(const char *seqstr, unsigned long *pseq)
{
    char *endptr = NULL;

    if (!seqstr) {
        VLOG_ERR("Invalid sequence");
        return CMD_WARNING;
    }

    /* Preference check. */
    *pseq = strtoul(seqstr, &endptr, 10);

    if ((*pseq == ULONG_MAX) || (*endptr != '\0') || (*pseq == 0) ||
        (*pseq > 65535)) {
        VLOG_ERR("Invalid seq number");
        return CMD_WARNING;
    }

    return CMD_SUCCESS;
}

/*
 * Route Map
 */
static int
policy_set_route_map_in_ovsdb(struct vty *vty, const char *name,
                              const char *typestr, const char *seq)
{
    struct ovsdb_idl_txn *policy_txn;
    const struct ovsrec_route_map *rt_map_row;
    const struct ovsrec_route_map_entry  *rt_map_entry_row;
    enum ovsdb_idl_txn_status status;
    unsigned long pref;

    /* Permit check. */
    if ((strncmp(typestr, "permit", strlen (typestr)) != 0) &&
        (strncmp(typestr, "deny", strlen (typestr)) != 0)) {
        VLOG_ERR("The third field must be [permit|deny]");
        return CMD_WARNING;
    }

    if (policy_route_map_get_seq(seq, &pref) != CMD_SUCCESS) {
        return CMD_SUCCESS;
    }

    START_DB_TXN(policy_txn);

    /*
     * If 'name' row already exists get a row structure pointer
     */
    rt_map_row = policy_get_route_map_in_ovsdb (name);

    /*
     * If row not found, create an empty row and set name field
     * The row will be used as uuid, refered to from another table
     */
    if (!rt_map_row) {
        rt_map_row = ovsrec_route_map_insert(policy_txn);
        ovsrec_route_map_set_name(rt_map_row, name);
    }

    /*
     * create a empty row, it will be used as uuid, refer to from another table
     */
    rt_map_entry_row = policy_get_route_map_entry_in_ovsdb(pref, name,
                                                           typestr);

    if (!rt_map_entry_row) {
        rt_map_entry_row = ovsrec_route_map_entry_insert(policy_txn);
        bgp_route_map_entry_insert_to_route_map(rt_map_row, rt_map_entry_row,
                                                (int64_t)pref);

        /* Row was not found, which means it is a new entry. Set default vals */
        ovsrec_route_map_entry_set_action(rt_map_entry_row, typestr);
        ovsrec_route_map_entry_set_match(rt_map_entry_row, NULL);
        ovsrec_route_map_entry_set_set(rt_map_entry_row, NULL);
    }

    rmp_context.pref = pref;
    strncpy(rmp_context.name, name, sizeof(rmp_context.name));
    strncpy(rmp_context.action, typestr, sizeof(rmp_context.action));
    vty->index = &rmp_context;
    vty->node = RMAP_NODE;

    END_DB_TXN(policy_txn);
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
cli_no_route_map_all_cmd_execute(const char *name)
{
    struct ovsrec_route_map *rt_map_row;
    struct ovsdb_idl_txn *policy_txn;

    /* Start of transaction */
    START_DB_TXN(policy_txn);

    rt_map_row = policy_get_route_map_in_ovsdb(name);
    if (!rt_map_row) {
        ERRONEOUS_DB_TXN(policy_txn, "Route Map not found");
    }

    ovsrec_route_map_delete(rt_map_row);

    /* End of transaction */
    END_DB_TXN(policy_txn);
}

DEFUN (no_route_map_all,
       no_rt_map_all_cmd,
       "no route-map WORD",
       NO_STR
       "Create route-map or enter route-map command mode\n"
       "Route map tag\n")
{
    return cli_no_route_map_all_cmd_execute(argv[0]);
}

static int
cli_no_route_map_cmd_execute(const char *name, const char *action,
                             const char *seqstr)
{
    VLOG_DBG("Deleting any route map entries...");
    struct ovsrec_route_map *rt_map_row;
    struct ovsrec_route_map_entry *rt_map_entry_row;
    struct ovsdb_idl_txn *policy_txn;
    unsigned long pref;
    bool deleted = false;

    if (policy_route_map_get_seq(seqstr, &pref) != CMD_SUCCESS) {
        return CMD_SUCCESS;
    }

    /* Start of transaction */
    START_DB_TXN(policy_txn);

    rt_map_row = policy_get_route_map_in_ovsdb(name);
    if (!rt_map_row) {
        ERRONEOUS_DB_TXN(policy_txn, "Route Map not found");
    }

    rt_map_entry_row = policy_get_route_map_entry_in_ovsdb(pref, name, action);
    if (!rt_map_entry_row) {
        ERRONEOUS_DB_TXN(policy_txn, "Route Map entry not found");
    }

    /* Need to remove the reference to the route-map entry from route-map table
     * first.
     */
    bgp_route_map_entry_remove_from_route_map(rt_map_row, pref);

    ovsrec_route_map_entry_delete(rt_map_entry_row);

    /* End of transaction */
    END_DB_TXN(policy_txn);
}

DEFUN (no_route_map,
       no_rt_map_cmd,
       "no route-map WORD (deny|permit) <1-65535>",
       NO_STR
       "Create route-map or enter route-map command mode\n"
       "Route map tag\n"
       "Route map denies set operations\n"
       "Route map permits set operations\n"
       "Sequence to insert to/delete from existing route-map entry\n")
{
    return cli_no_route_map_cmd_execute(argv[0], argv[1], argv[2]);
}

static int
policy_set_route_map_description_in_ovsdb(struct vty *vty,
                                          const char *description)
{
    struct ovsdb_idl_txn *policy_txn;
    const struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    START_DB_TXN(policy_txn);

    ovsrec_route_map_entry_set_description(rt_map_entry_row, description);

    END_DB_TXN(policy_txn);
}

DEFUN (rmap_description,
       rmap_description_cmd,
       "description .LINE",
       "Route-map comment\n"
       "Comment describing this route-map rule\n")
{
    return policy_set_route_map_description_in_ovsdb(vty,
                                                    argv_concat(argv, argc, 0));
}

DEFUN (no_rmap_description,
       no_rmap_description_cmd,
       "no description",
       NO_STR
       "Route-map comment\n")
{
    return policy_set_route_map_description_in_ovsdb(vty, NULL);
}

static int
policy_set_route_map_match_in_ovsdb(
    struct vty *vty,
    const struct ovsrec_route_map_entry  *rt_map_entry_row,
    const char *command, const char *arg)
{
    struct ovsdb_idl_txn *policy_txn;
    struct smap smap_match;
    char *table_key;

    table_key = policy_cmd_to_key_lookup(command, match_table);
    if (table_key == NULL) {
         VLOG_ERR("Route map match wrong key - %s", command);
            return TXN_ERROR;
    }

    START_DB_TXN(policy_txn);

    smap_clone(&smap_match, &rt_map_entry_row->match);

    if (arg) {
        /* Non-empty key, so the value will be set. */
        smap_replace(&smap_match, table_key, arg);
    } else {
        /* Empty key indicates an unset. */
        smap_remove(&smap_match, table_key);
    }

    ovsrec_route_map_entry_set_match(rt_map_entry_row, &smap_match);
    smap_destroy(&smap_match);

    END_DB_TXN(policy_txn);
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
    const struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    return policy_set_route_map_match_in_ovsdb(vty, rt_map_entry_row,
                                               "ip address prefix-list",
                                               argv[0]);
}

static int
policy_set_route_map_set_in_ovsdb (struct vty *vty,
                   const struct ovsrec_route_map_entry  *rt_map_entry_row,
                   const char *command, const char *arg)

{
    struct ovsdb_idl_txn *policy_txn;
    struct smap smap_set;
    char *table_key;
    struct smap *psmap;

    table_key = policy_cmd_to_key_lookup(command, set_table);
    if (table_key == NULL) {
         VLOG_ERR("Route map set wrong key - %s", command);
            return TXN_ERROR;
    }

    START_DB_TXN(policy_txn);

    psmap = &rt_map_entry_row->set;
    smap_clone(&smap_set, psmap);

    if (arg) {
        /* Non-empty key, so the value will be set. */
        smap_replace(&smap_set, table_key, arg);
    } else {
        /* Empty key indicates an unset. */
        smap_remove(&smap_set, table_key);
    }

    ovsrec_route_map_entry_set_set(rt_map_entry_row, &smap_set);
    smap_destroy(&smap_set);
    END_DB_TXN(policy_txn);
}

DEFUN (no_match_ip_address_prefix_list,
       no_match_ip_address_prefix_list_cmd,
       "no match ip address prefix-list",
       NO_STR
       MATCH_STR
       IP_STR
       "Match address of route\n"
       "Match entries of prefix-lists\n")
{
    struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    return policy_set_route_map_match_in_ovsdb(vty, rt_map_entry_row,
                                               "ip address prefix-list",
                                               NULL);
}

ALIAS (no_match_ip_address_prefix_list,
       no_match_ip_address_prefix_list_val_cmd,
       "no match ip address prefix-list WORD",
       NO_STR
       MATCH_STR
       IP_STR
       "Match address of route\n"
       "Match entries of prefix-lists\n"
       "IP prefix-list name\n")

DEFUN (set_metric,
       set_metric_cmd,
       "set metric <0-4294967295>",
       SET_STR
       "Metric value for destination routing protocol\n"
       "Metric value\n")
{
    const struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    return policy_set_route_map_set_in_ovsdb(vty, rt_map_entry_row,
                                             "metric", argv[0]);
}

DEFUN (no_set_metric,
       no_set_metric_cmd,
       "no set metric",
       NO_STR
       SET_STR
       "Metric value for destination routing protocol\n")
{
    const struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    return policy_set_route_map_set_in_ovsdb(vty, rt_map_entry_row,
                                             "metric", NULL);
}

ALIAS (no_set_metric,
       no_set_metric_val_cmd,
       "no set metric <0-4294967295>",
       NO_STR
       SET_STR
       "Metric value for destination routing protocol\n"
       "Metric value\n")

static int
policy_set_route_map_set_community_str_in_ovsdb(struct vty *vty,
                                                const int argc,
                                                const char **argv)
{
    int i;
    int additive = 0;
    char *argstr;
    int ret = 0;
    int n = 0;

    const struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    argstr = xmalloc(1024);

    if (!argstr)
        return 0;

    for (i = 0; i < argc; i++)
    {
        if (strncmp (argv[i], "additive", strlen (argv[i])) == 0)
        {
            additive = 1;
            continue;
        }

        n += sprintf(&argstr[n], "%s", argv[i]);
    }

    if (additive)
    {
        n += sprintf(&argstr[n], " %s", "additive");
    }

    policy_set_route_map_set_in_ovsdb(vty, rt_map_entry_row,
                                      "community", argstr);

    free (argstr);
    return ret;
}

DEFUN (set_community,
       set_community_cmd,
       "set community .AA:NN",
       SET_STR
       "BGP community attribute\n"
       "Community number in aa:nn format or local-AS|no-advertise|no-export|internet or additive\n")
{
    return policy_set_route_map_set_community_str_in_ovsdb(vty, argc, argv);
}

DEFUN (no_set_community,
       no_set_community_cmd,
       "no set community",
       NO_STR
       SET_STR
       "BGP community attribute\n")
{
    struct ovsrec_route_map_entry  *rt_map_entry_row =
                policy_get_route_map_entry_in_ovsdb(rmp_context.pref,
                                                    rmp_context.name,
                                                    rmp_context.action);

    return policy_set_route_map_set_in_ovsdb(vty, rt_map_entry_row,
                                             "community", NULL);
}

ALIAS (no_set_community,
       no_set_community_val_cmd,
       "no set community .AA:NN",
       NO_STR
       SET_STR
       "BGP community attribute\n"
       "Community number in aa:nn format or local-AS|no-advertise|no-export|internet or additive\n")

void policy_vty_init(void)
{
    install_element (CONFIG_NODE, &ip_prefix_list_seq_cmd);
    install_element (CONFIG_NODE, &no_ip_prefix_list_cmd);
    install_element (CONFIG_NODE, &no_ip_prefix_list_seq_cmd);
    install_element (CONFIG_NODE, &rt_map_cmd);
    install_element (CONFIG_NODE, &no_rt_map_cmd);
    install_element (CONFIG_NODE, &no_rt_map_all_cmd);
    install_element (RMAP_NODE, &rmap_description_cmd);
    install_element (RMAP_NODE, &no_rmap_description_cmd);
    install_element (RMAP_NODE, &match_ip_address_prefix_list_cmd);
    install_element (RMAP_NODE, &no_match_ip_address_prefix_list_cmd);
    install_element (RMAP_NODE, &no_match_ip_address_prefix_list_val_cmd);
    install_element (RMAP_NODE, &set_metric_cmd);
    install_element (RMAP_NODE, &no_set_metric_cmd);
    install_element (RMAP_NODE, &no_set_metric_val_cmd);
    install_element (RMAP_NODE, &set_community_cmd);
    install_element (RMAP_NODE, &no_set_community_cmd);
    install_element (RMAP_NODE, &no_set_community_val_cmd);
}
