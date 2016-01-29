/*
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro
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
#include "command.h"
#include "ospf_vty.h"

#define CLEANUP_SHOW_RUN

char routercontextbgpclientname[] = "vtysh_router_context_bgp_clientcallback";
char routercontextospfclientname[] = "vtysh_router_context_ospf_clientcallback";
char routercontextbgpipprefixclientname[] =
                          "vtysh_router_context_bgp_ip_prefix_clientcallback";
char routercontextbgproutemapclientname[] =
                          "vtysh_router_context_bgp_routemap_clientcallback";

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
    struct ovsrec_bgp_neighbor *nbr_table=NULL;
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
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %d", "", "neighbor",
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
            while (i < nbr_table->n_prefix_lists) {
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s %s %s %s", "",
                                      "neighbor", bgp_router_context->
                                      key_bgp_neighbors[n_neighbors],
                                      "prefix-list", nbr_table->value_prefix_lists[i]->name,
                                      nbr_table->key_prefix_lists[i]);
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
                                          " %s seq %d %s %s",
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
                                          " seq %d %s %s",
                                          ovs_prefix_list->name,
                                          ovs_prefix_list->
                                          key_prefix_list_entries[j],
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->action,
                                          ovs_prefix_list->
                                          value_prefix_list_entries[j]->prefix);
                    } else {
                        vtysh_ovsdb_cli_print(p_msg,"ip prefix-list %s"
                                          " seq %d %s %s",
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
                                          "seq %d %s %s ge %d ",
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
                                          "seq %d %s %s ge %d ",
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
                                          "seq %d %s %s le %d ",
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
                                          "%d %s %s le %d ",
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
                                          "seq %d %s %s ge %d le %d ",
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
                                          "seq %d %s %s ge %d le %d ",
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

            if (smap_get(&(ovs_route_map->value_route_map_entries[j]->match),
                "prefix_list"))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match ip address prefix-list",
                                      smap_get(&(ovs_route_map->
                                                 value_route_map_entries[j]->
                                                 match), "prefix_list"));

            if (smap_get(&(ovs_route_map->value_route_map_entries[j]->match),
                "ipv6_prefix_list"))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match ipv6 address prefix-list",
                                      smap_get(&(ovs_route_map->
                                                 value_route_map_entries[j]->
                                                 match), "ipv6_prefix_list"));

            if (smap_get(&(ovs_route_map->value_route_map_entries[j]->match),
                "community"))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match community",
                                      smap_get(&(ovs_route_map->
                                                 value_route_map_entries[j]->
                                                 match), "community"));

            if (smap_get(&(ovs_route_map->value_route_map_entries[j]->match),
                "extcommunity"))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "",
                                      "match extcommunity",
                                      smap_get(&(ovs_route_map->
                                                 value_route_map_entries[j]->
                                                 match), "extcommunity"));

            if (smap_get(&ovs_route_map->value_route_map_entries[j]->set,
                "community"))
                vtysh_ovsdb_cli_print(p_msg, "%4s %s %s", "", "set community",
                                      smap_get(&ovs_route_map->
                                               value_route_map_entries[j]->set,
                                               "community"));

            if (smap_get(&ovs_route_map->value_route_map_entries[j]->set,
                "metric"))
                vtysh_ovsdb_cli_print(p_msg,"%4s %s %s", "", "set metric",
                                      smap_get(&ovs_route_map->
                                               value_route_map_entries[j]->set,
                                               "metric"));
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
            vtysh_ovsdb_cli_print(p_msg, "%s %d", "router bgp",
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

            /*OVSREC_BGP_ROUTER_FOR_EACH(bgp_router_row, p_msg->idl)
            {
                for (k = 0; k < bgp_router_row->n_redistribute; k++) {
                    if (bgp_router_row->key_redistribute[k]) {
                        if (strlen(bgp_router_row->
                            value_redistribute[k]->name) == 0)
                            vtysh_ovsdb_cli_print(p_msg,"%4s %s %s","",
                                                  "redistribute",
                                                  bgp_router_row->
                                                  key_redistribute[k]);
                        else
                            vtysh_ovsdb_cli_print(p_msg,"%4s %s %s %s %s","",
                                                  "redistribute",
                                                  bgp_router_row->
                                                  key_redistribute[k],
                                                  "route-map",
                                                  bgp_router_row->
                                                  value_redistribute[k]->name);
                    }
                }
            }*/

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
vtysh_ret_val
vtysh_router_context_ospf_clientcallback(void *p_private)
{
    const struct ovsrec_vrf *ovs_vrf = NULL;
    const struct ovsrec_ospf_router *ospf_router_row = NULL;
    int i = 0, j = 0;
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const char *val = NULL;
    char area_str[OSPF_SHOW_STR_LEN];

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
        }
    }

    //vtysh_router_context_ospf_neighbor_callback(p_msg);
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
    vtysh_context_client client;

    vtysh_ret_val retval = e_vtysh_error;
    client.p_client_name = routercontextbgpclientname;
    client.client_id = e_vtysh_router_context_bgp;
    client.p_callback = &vtysh_router_context_bgp_clientcallback;
    retval = vtysh_context_addclient(
              e_vtysh_router_context, e_vtysh_router_context_bgp, &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add bgp callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    client.p_client_name = routercontextbgpipprefixclientname;
    client.client_id = e_vtysh_router_context_bgp_ip_prefix;
    client.p_callback = &vtysh_router_context_bgp_ip_prefix_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_router_context,
                                     e_vtysh_router_context_bgp_ip_prefix,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add "
                                  "bgp ip prefix callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    client.p_client_name = routercontextbgproutemapclientname;
    client.client_id = e_vtysh_router_context_bgp_routemap;
    client.p_callback = &vtysh_router_context_bgp_routemap_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_router_context,
                                     e_vtysh_router_context_bgp_routemap,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add bgp "
                                  "routemap callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    memset(&client, 0, sizeof(vtysh_context_client));
    client.p_client_name = routercontextospfclientname;
    client.client_id = e_vtysh_router_context_ospf;
    client.p_callback = &vtysh_router_context_ospf_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_router_context,
                                     e_vtysh_router_context_ospf, &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "router context unable to add ospf callback");
        assert(0);
        return retval;
    }

    return e_vtysh_ok;
}
