/*
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro
 * Copyright (C) 2015 - 2016 Hewlett Packard Enterprise Development LP
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
 */

/****************************************************************************
 * @ingroup cli
 *
 * @file vtysh_ovsdb_router_context.c
 * Source for registering client callback with router context.
 *
 ***************************************************************************/

#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_router_context.h"
#include "openswitch-dflt.h"
#include "command.h"
#include "ospf_vty.h"

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_neighbor_callback
| Responsibility : Neighbor commands
| Parameters :
|     vtysh_ovsdb_cbmsg_ptr p_msg: struct vtysh_ovsdb_cbmsg_struct *
| Return : void
-----------------------------------------------------------------------------*/

void vtysh_router_context_bgp_neighbor_callback(vtysh_ovsdb_cbmsg_ptr p_msg)
{
    const struct ovsrec_bgp_router *bgp_router_context=NULL;
    int i = 0, n_neighbors = 0, k = 0;
    const struct ovsrec_bgp_neighbor *nbr_table=NULL;
  /* To consider all router entries. */
    OVSREC_BGP_ROUTER_FOR_EACH(bgp_router_context, p_msg->idl)
    {
        for (n_neighbors = 0; n_neighbors < bgp_router_context->n_bgp_neighbors;
             n_neighbors++) {
          /* Neighbor peer group commands. */
            if (*(bgp_router_context->value_bgp_neighbors[n_neighbors]->
                  is_peer_group))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "peer-group");
        }

        for (n_neighbors = 0; n_neighbors<bgp_router_context->n_bgp_neighbors;
             n_neighbors++) {
            nbr_table =  bgp_router_context->value_bgp_neighbors[n_neighbors];
            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_remote_as)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %lu", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "remote-as", *(bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      remote_as));

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                description)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "description", bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      description);

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->password)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "password", bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      password);

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_advertisement_interval)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s %d", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "advertisement-interval", *(bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      advertisement_interval));

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_timers > 0)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %d %d","","neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors], "timers",
                                      bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      value_timers[1], bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      value_timers[0]);

            i=0;
            while (i < bgp_router_context->value_bgp_neighbors[n_neighbors]->
                   n_route_maps) {
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s %s", "",
                                      "neighbor", bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "route-map", bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      value_route_maps[i]->name,
                                      bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      key_route_maps[i]);
                i++;
            }

            i=0;
            while (i < nbr_table->n_prefix_lists) {
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s %s", "",
                                      "neighbor", bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "prefix-list", nbr_table->value_prefix_lists[i]->name,
                                      nbr_table->key_prefix_lists[i]);
                i++;
            }

            i=0;
            while (i < nbr_table->n_aspath_filters) {
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s %s", "",
                                      "neighbor", bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "filter-list", nbr_table->value_aspath_filters[i]->name,
                                      nbr_table->key_aspath_filters[i]);
                i++;
            }

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_allow_as_in)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %d", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "allowas-in",
                                      *(bgp_router_context->
                                        value_bgp_neighbors[n_neighbors]->
                                        allow_as_in));

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_remove_private_as)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "remove-private-AS");

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_inbound_soft_reconfiguration)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "soft-reconfiguration inbound");

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_ebgp_multihop)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "ebgp-multihop");

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                n_ttl_security_hops)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s %d", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "ttl-security hops",
                                      *(bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      ttl_security_hops));

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                update_source)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s %s", "", "neighbor",
                                      bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "update-source",
                                      (bgp_router_context->
                                      value_bgp_neighbors[n_neighbors]->
                                      update_source));

            if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                bgp_peer_group) {
                for (k = 0; k < bgp_router_context->n_bgp_neighbors; k++) {
                    if (bgp_router_context->value_bgp_neighbors[n_neighbors]->
                        bgp_peer_group == bgp_router_context->
                        value_bgp_neighbors[k])
                        vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s", "",
                                              "neighbor",
                                              bgp_router_context->
                                              key_bgp_neighbors[n_neighbors],
                                              "peer-group",
                                              bgp_router_context->
                                              key_bgp_neighbors[k]);
                }
            }
        }
        vtysh_ovsdb_cli_print(p_msg,"!");
    }
}


 /*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_ip_filter_list_clientcallback
| Responsibility : ip as-path access-list command
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_bgp_ip_filter_list_clientcallback(void *p_private)
{
    const struct ovsrec_bgp_aspath_filter *ovs_filter_list = NULL;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    int itr;

    OVSREC_BGP_ASPATH_FILTER_FOR_EACH(ovs_filter_list, p_msg->idl)
    {
        if (ovs_filter_list->name) {
            for(itr = 0; itr < ovs_filter_list->n_permit; itr++) {
                vtysh_ovsdb_cli_print(p_msg,"ip as-path access-list %s %s %s",
                                      ovs_filter_list->name,
                                      "permit", ovs_filter_list->permit[itr]);
            }
            for(itr = 0; itr < ovs_filter_list->n_deny; itr++) {
                vtysh_ovsdb_cli_print(p_msg,"ip as-path access-list %s %s %s",
                                      ovs_filter_list->name,
                                      "deny", ovs_filter_list->deny[itr]);
            }
        }
    }
    vtysh_ovsdb_cli_print(p_msg,"!");
    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_ip_community_filter_clientcallback
| Responsibility : ip community-filter lists commands
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_bgp_ip_community_filter_clientcallback(void *p_private)
{
    const struct ovsrec_bgp_community_filter *ovs_community_list = NULL;
    int i;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    OVSREC_BGP_COMMUNITY_FILTER_FOR_EACH(ovs_community_list, p_msg->idl)
    {

        if ( ovs_community_list->name
             && ovs_community_list->type) {
            for (i =0 ; i<ovs_community_list->n_permit; i++) {
                vtysh_ovsdb_cli_print(p_msg,"ip %s %s permit %s",
                                  ovs_community_list->type,
                                  ovs_community_list->name,
                                  ovs_community_list->permit[i]);
            }
            for (i =0 ; i<ovs_community_list->n_deny; i++) {
                vtysh_ovsdb_cli_print(p_msg,"ip %s %s deny %s",
                                  ovs_community_list->type,
                                  ovs_community_list->name,
                                  ovs_community_list->deny[i]);
            }
        }

    }
    vtysh_ovsdb_cli_print(p_msg,"!");
    return e_vtysh_ok;

}

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_ip_prefix_clientcallback
| Responsibility : ip prefix-list command
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/

vtysh_ret_val
vtysh_router_context_bgp_ip_prefix_clientcallback(void *p_private)
{
    const struct ovsrec_prefix_list *ovs_prefix_list = NULL;
    const struct ovsrec_prefix_list_entry *ovs_prefix_list_entry = NULL;
    struct in6_addr addrv6;
    int j = 0;
    char *temp_prefix;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    OVSREC_PREFIX_LIST_FOR_EACH(ovs_prefix_list, p_msg->idl)
    {

        if (ovs_prefix_list->name &&
            strlen(ovs_prefix_list->description) !=0 ) {
            vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list %s "
                                               "description %s",
                                               ovs_prefix_list->name,
                                               ovs_prefix_list->
                                               description);
        }
        for (j = 0; j < ovs_prefix_list->n_prefix_list_entries; j++) {

            if (ovs_prefix_list->name) {

                temp_prefix = (char *)malloc(sizeof(ovs_prefix_list->
                                  value_prefix_list_entries[j]->prefix));
                strcpy(temp_prefix,ovs_prefix_list->
                           value_prefix_list_entries[j]->prefix);
                strtok(temp_prefix,"/");

                if (strcmp(ovs_prefix_list->
                        value_prefix_list_entries[j]->prefix,"any") == 0
                        || (ovs_prefix_list->
                        value_prefix_list_entries[j]->ge[0] == 0
                        && ovs_prefix_list->
                        value_prefix_list_entries[j]->le[0] == 0 )) {

                    if (ovs_prefix_list->
                        value_prefix_list_entries[j]->le[0] == 128) {

                        vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list"
                                          " %s seq %lu %s %s",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix);
                    } else if (inet_pton(AF_INET6,temp_prefix,
                                             &addrv6) == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list %s"
                                          " seq %lu %s %s",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"ip prefix-list %s"
                                          " seq %lu %s %s",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix);
                    }
                } else if (strcmp(ovs_prefix_list->
                               value_prefix_list_entries[j]->prefix,"any") != 0
                               && ovs_prefix_list->
                               value_prefix_list_entries[j]->le[0] == 0 ) {

                    if (inet_pton(AF_INET6,temp_prefix,&addrv6) == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list %s "
                                          "seq %lu %s %s ge %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->ge[0]);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"ip prefix-list %s "
                                          "seq %lu %s %s ge %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->ge[0]);
                    }
                } else if (strcmp(ovs_prefix_list->
                               value_prefix_list_entries[j]->prefix,"any") != 0
                               && ovs_prefix_list->
                               value_prefix_list_entries[j]->ge[0] == 0 ) {
                    if (inet_pton(AF_INET6,temp_prefix,&addrv6) == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list %s "
                                          "seq %lu %s %s le %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->le[0]);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"ip prefix-list %s seq "
                                          "%lu %s %s le %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->le[0]);
                    }

                } else {
                    if (inet_pton(AF_INET6,temp_prefix,&addrv6) == 1) {
                        vtysh_ovsdb_cli_print(p_msg,"ipv6 prefix-list %s "
                                          "seq %lu %s %s ge %d le %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->ge[0],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->le[0]);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"ip prefix-list %s "
                                          "seq %lu %s %s ge %d le %d ",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->ge[0],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->le[0]);

                    }
                }
                free(temp_prefix);
            }
        }
    }
    vtysh_ovsdb_cli_print(p_msg,"!");
    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_routemap_clientcallback
| Responsibility : Routemap commands
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/

vtysh_ret_val
vtysh_router_context_bgp_routemap_clientcallback(void *p_private)
{
    const struct ovsrec_route_map *ovs_route_map = NULL;
    int j = 0;
    const char *match_val, *set_val;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    OVSREC_ROUTE_MAP_FOR_EACH(ovs_route_map, p_msg->idl)
    {
        for (j = 0; j < ovs_route_map->n_route_map_entries; j++) {
            if (ovs_route_map->name)
                vtysh_ovsdb_cli_print(p_msg, "route-map %s %s %d",
                                      ovs_route_map->name,
                                      ovs_route_map->
                                      value_route_map_entries[j]->action,
                                      ovs_route_map->key_route_map_entries[j]);

            if (ovs_route_map->value_route_map_entries[j]->description)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "", "description",
                                      ovs_route_map->
                                      value_route_map_entries[j]->description);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "prefix_list");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match ip address prefix-list",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "ipv6_prefix_list");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match ipv6 address prefix-list",
                                      match_val);
            match_val = smap_get(
                        &(ovs_route_map->value_route_map_entries[j]->match),
                        "community");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match community",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "extcommunity");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match extcommunity",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "as_path");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match as-path",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "ipv6_next_hop");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match ipv6 next-hop",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "metric");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match metric",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "origin");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match origin",
                                      match_val);
            match_val = smap_get(
                         &(ovs_route_map->value_route_map_entries[j]->match),
                         "probability");

            if (match_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match probability",
                                      match_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "community");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "", "set community",
                                      set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "metric");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "", "set metric",
                                      set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "aggregator_as");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set aggregator as",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "as_path_exclude");
            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set as-path exclude",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "as_path_prepend");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set as-path prepend",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "atomic_aggregate");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s", "",
                    "set atomic-aggregate");

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "comm_list");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s delete", "",
                    "set comm-list",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "extcommunity rt");
            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set extcommunity rt",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "extcommunity soo");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set extcommunity soo",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "ipv6_next_hop_global");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set ipv6 next-hop global",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "local_preference");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "",
                    "set local-preference",
                        set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "origin");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "", "set origin",
                    set_val);

            set_val = smap_get(
                       &ovs_route_map->value_route_map_entries[j]->set,
                       "weight");

            if (set_val)
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "", "set weight",
                    set_val);

        }
    }

    vtysh_ovsdb_cli_print(p_msg,"!");
    return e_vtysh_ok;
}
/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_bgp_clientcallback
| Responsibility : client callback routine
| Parameters :
     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/

vtysh_ret_val
vtysh_router_context_bgp_clientcallback(void *p_private)
{
    const struct ovsrec_vrf *ovs_vrf = NULL;
    int i = 0, j = 0, k = 0;
    const struct ovsrec_bgp_router *bgp_router_row = NULL;

    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                             "vtysh_context_router_bgp_clientcallback entered");

    OVSREC_VRF_FOR_EACH(ovs_vrf, p_msg->idl)
    {
        for (j = 0; j < ovs_vrf->n_bgp_routers; j++) {
            vtysh_ovsdb_cli_print(p_msg, "%s %lu", "router bgp",
                                  ovs_vrf->key_bgp_routers[j]);

            if (ovs_vrf->value_bgp_routers[j]->router_id)
                if(strcmp(ovs_vrf->value_bgp_routers[j]->router_id,"0.0.0.0"))
                    vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                          "bgp router-id", ovs_vrf->
                                          value_bgp_routers[j]->router_id);

            while (i < ovs_vrf->value_bgp_routers[j]->n_networks) {
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "", "network",
                                      ovs_vrf->value_bgp_routers[j]->
                                      networks[i]);
                i++;
            }

            if (ovs_vrf->value_bgp_routers[j]->n_maximum_paths)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %d", "", "maximum-paths",
                                      *(ovs_vrf->value_bgp_routers[j]->
                                        maximum_paths));

            if (ovs_vrf->value_bgp_routers[j]->n_timers > 0)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %d %d", "", "timers bgp",
                                      ovs_vrf->value_bgp_routers[j]->
                                      value_timers[1], ovs_vrf->
                                      value_bgp_routers[j]->value_timers[0]);

            if (ovs_vrf->value_bgp_routers[j]->n_redistribute > 0) {
                for (k = 0; k < ovs_vrf->value_bgp_routers[j]->n_redistribute;
                     k++) {
                        if (strlen(ovs_vrf->value_bgp_routers[j]->
                            value_redistribute[k]->name) == 0)
                            vtysh_ovsdb_cli_print(p_msg,"%4s %s %s","",
                                                  "redistribute",
                                                  ovs_vrf->value_bgp_routers[j]
                                                  ->key_redistribute[k]);
                        else
                            vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s %s","",
                                                  "redistribute",
                                                  ovs_vrf->value_bgp_routers[j]
                                                  ->key_redistribute[k],
                                                  "route-map",
                                                  ovs_vrf->value_bgp_routers[j]
                                                  ->value_redistribute[k]
                                                  ->name);
                }
            }
            if (ovs_vrf->value_bgp_routers[j]->n_fast_external_failover)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s", "", "bgp fast-external-failover");

            if (ovs_vrf->value_bgp_routers[j]->n_log_neighbor_changes)
                vtysh_ovsdb_cli_print(p_msg, "%4s %s", "", "bgp log-neighbor-changes");
        }
    }
    vtysh_router_context_bgp_neighbor_callback(p_msg);
    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_ospf_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
void vtysh_router_context_ospf_area_callback(vtysh_ovsdb_cbmsg_ptr p_msg,
                                    const struct ovsrec_ospf_router *router_row)
{
    const struct ovsrec_ospf_area *area_row = NULL;
    int i = 0;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];
    char disp_str[OSPF_SHOW_STR_LEN];

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
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s authentication "
                                      "message-digest", "",
                                      area_str);
            }
            else if (!strcmp(area_row->ospf_auth_type,
                             OVSREC_OSPF_AREA_OSPF_AUTH_TYPE_TEXT))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s authentication", "",
                                      area_str);
            }
        }

        /* area type */
        strncpy(disp_str, "", OSPF_SHOW_STR_LEN);
        if (area_row->nssa_translator_role)
        {
            if (!strcmp(area_row->nssa_translator_role,
                OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_NEVER))
            {
                strncpy(disp_str, "translate-never", OSPF_SHOW_STR_LEN);
            }
            else if (!strcmp(area_row->nssa_translator_role,
                OVSREC_OSPF_AREA_NSSA_TRANSLATOR_ROLE_ALWAYS))
            {
                strncpy(disp_str, "translate-always", OSPF_SHOW_STR_LEN);
            }
        }

        if (area_row->area_type)
        {
            if (!strcmp(area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_NSSA))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s nssa %s", "", area_str,
                                      disp_str);
            }
            else if (!strcmp(area_row->area_type,
                            OVSREC_OSPF_AREA_AREA_TYPE_NSSA_NO_SUMMARY))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s nssa %s no-summary", "",
                                      area_str, disp_str);
            }
            else if (!strcmp(area_row->area_type,
                        OVSREC_OSPF_AREA_AREA_TYPE_STUB))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s stub", "", area_str);
            }
            else if (!strcmp(area_row->area_type,
                            OVSREC_OSPF_AREA_AREA_TYPE_STUB_NO_SUMMARY))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4sarea %s stub no-summary", "",
                                      area_str);
            }
        }
    }
}

/*-----------------------------------------------------------------------------
| Function : vtysh_router_context_ospf_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_router_context_ospf_clientcallback(void *p_private)
{
    const struct ovsrec_vrf *ovs_vrf = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    int i = 0, j = 0;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];
    int64_t distance = 0;
    bool redist_def_orig = false;
    bool redist_def_orig_always = false;
    int metric, timers;

    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                             "vtysh_context_router_ospf_clientcallback entered");

    OVSREC_VRF_FOR_EACH(ovs_vrf, p_msg->idl)
    {
        for (j = 0; j < ovs_vrf->n_ospf_routers; j++)
        {

            vtysh_ovsdb_cli_print(p_msg, "%s", "router ospf");

            ospf_router_row = ovs_vrf->value_ospf_routers[j];

            /* Router id */
            val = smap_get((const struct smap *)&ospf_router_row->router_id,
                            OSPF_KEY_ROUTER_ID_VAL);
            if (val && (strcmp(val, OSPF_DEFAULT_STR) != 0))
                vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "",
                                      "router-id", val);

            /* network <range> area <area-id>*/
            while (i < ospf_router_row->n_networks)
            {
                memset(area_str,'\0', OSPF_SHOW_STR_LEN);
                OSPF_IP_STRING_CONVERT(area_str, ntohl(ospf_router_row->value_networks[i]));
                vtysh_ovsdb_cli_print(p_msg, "%4snetwork %s area %s", "",
                                      ospf_router_row->key_networks[i],
                                      area_str);
                i++;
            }

            /* max-metric  admin */
            val = smap_get(&ospf_router_row->stub_router_adv,
                            OSPF_KEY_ROUTER_STUB_ADMIN);
            if(val && (strcmp(val, "true") == 0))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s", "",
                        "max-metric router-lsa");
            }

            /* max-metric  startup */
            val = smap_get(&ospf_router_row->stub_router_adv,
                            OSPF_KEY_ROUTER_STUB_STARTUP);
            if(val)
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "",
                        "max-metric router-lsa on-startup", val);
            }

            /*Compatible rfc1583*/
            redist_def_orig = smap_get_bool(&ospf_router_row->other_config,
                                            OSPF_KEY_RFC1583_COMPATIBLE,
                                            OSPF_RFC1583_COMPATIBLE_DEFAULT);
            if (redist_def_orig)
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s", "",
                        "compatible rfc1583");
            }

            /*Default metric*/
            metric = smap_get_int(&ospf_router_row->other_config,
                                  OSPF_KEY_ROUTER_DEFAULT_METRIC,
                                  OSPF_DEFAULT_METRIC_DEFAULT);
            if ((metric > 0) && (metric != atoi(OSPF_DEFAULT_METRIC_DEFAULT)))
            {
                 vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "default-metric",
                                                                        metric);
            }

            /*Timers lsa-group-pacing*/
            for (i = 0; i < ospf_router_row->n_lsa_timers; i++) {
                if (strcmp(ospf_router_row->key_lsa_timers[i],
                                             OSPF_KEY_LSA_GROUP_PACING) == 0) {
                    timers = ospf_router_row->value_lsa_timers[i];
                }
            }
            if ((timers > 0) && (timers != OSPF_LSA_GROUP_PACING_DEFAULT))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "",
                                             "timers lsa-group-pacing", timers);
            }

            /* Distance */
            distance = ospf_get_distance(ospf_router_row,
                                         OVSREC_OSPF_ROUTER_DISTANCE_ALL);
            if ((distance > 0) && (distance != OSPF_ROUTER_DISTANCE_DEFAULT))
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s %d", "", "distance",
                                      distance);
            }
            for (i = 0 ; i < ospf_router_row->n_redistribute ; i++)
            {
                if (!strcmp(ospf_router_row->redistribute[i],OVSREC_OSPF_ROUTER_REDISTRIBUTE_CONNECTED))
                    vtysh_ovsdb_cli_print(p_msg, "%4s%s", "",
                    "redistribute connected");
                else if (!strcmp(ospf_router_row->redistribute[i],OVSREC_OSPF_ROUTER_REDISTRIBUTE_STATIC))
                    vtysh_ovsdb_cli_print(p_msg, "%4s%s", "",
                    "redistribute static");
                else if (!strcmp(ospf_router_row->redistribute[i],OVSREC_OSPF_ROUTER_REDISTRIBUTE_BGP))
                    vtysh_ovsdb_cli_print(p_msg, "%4s%s", "",
                    "redistribute bgp");
            }
            redist_def_orig = smap_get_bool(&(ospf_router_row->default_information),
                                OSPF_DEFAULT_INFO_ORIGINATE,OSPF_DEFAULT_INFO_ORIG_DEFAULT);
            redist_def_orig_always = smap_get_bool(&(ospf_router_row->default_information),
                                OSPF_DEFAULT_INFO_ORIGINATE_ALWAYS,OSPF_DEFAULT_INFO_ORIG_DEFAULT);
            if (redist_def_orig)
            {
                vtysh_ovsdb_cli_print(p_msg, "%4s%s%s", "",
                        "default-information originate",
                        (redist_def_orig_always)?" always":"");
            }
            vtysh_router_context_ospf_area_callback(p_msg, ospf_router_row);

        }
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_init_router_contextclients
| Responsibility : Registers the client callback routines for router context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_router_context_clients()
{
    vtysh_ret_val retval = e_vtysh_error;

    retval = install_show_run_config_context(e_vtysh_router_context,
                  NULL, NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                "router context unable to add router context");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_bgp,
                  &vtysh_router_context_bgp_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add bgp callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_bgp_ip_prefix,
                  &vtysh_router_context_bgp_ip_prefix_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add "
                                  "bgp ip prefix callback");
        assert(0);
        return retval;
    }


    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_bgp_ip_community_filter,
                  &vtysh_router_context_bgp_ip_community_filter_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add "
                                  "bgp ip community filter callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_bgp_ip_filter_list,
                  &vtysh_router_context_bgp_ip_filter_list_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add "
                                  "bgp ip filter list callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_bgp_routemap,
                  &vtysh_router_context_bgp_routemap_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add bgp "
                                  "routemap callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    retval = install_show_run_config_subcontext(e_vtysh_router_context,
                  e_vtysh_router_context_ospf,
                  &vtysh_router_context_ospf_clientcallback,
                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                 "router context unable to add ospf callback");
        assert(0);
        return retval;
    }

    return e_vtysh_ok;
}
