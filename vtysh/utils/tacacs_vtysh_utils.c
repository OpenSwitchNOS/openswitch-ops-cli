/*
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <lib/version.h>
#include "shash.h"
#include "command.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/utils/system_vtysh_utils.h"
#include "vtysh/vtysh.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh/vtysh_utils.h"

/* Utility functions for tacacs server display*/

/* qsort comparator function: default_priority*/
int
compare_nodes_by_tacacs_server_default_priority (const void *a, const void *b)
{
    const struct shash_node *const *node_a = a;
    const struct shash_node *const *node_b = b;
    const struct ovsrec_tacacs_server *server_a =
                      (const struct ovsrec_tacacs_server *)(*node_a)->data;
    const struct ovsrec_tacacs_server *server_b =
                      (const struct ovsrec_tacacs_server *)(*node_b)->data;

    return (server_a->default_priority - server_b->default_priority);
}


/* qsort comparator function: group_priority*/
int
compare_nodes_by_tacacs_server_group_priority (const void *a, const void *b)
{
    const struct shash_node *const *node_a = a;
    const struct shash_node *const *node_b = b;
    const struct ovsrec_tacacs_server *server_a =
                      (const struct ovsrec_tacacs_server *)(*node_a)->data;
    const struct ovsrec_tacacs_server *server_b =
                      (const struct ovsrec_tacacs_server *)(*node_b)->data;

    return (server_a->group_priority - server_b->group_priority);
}

/*
 * Sorting function for tacacs servers
 * on success, returns sorted tacacs server list.
 */
const struct shash_node **
sort_tacacs_server(const struct shash *list, bool by_default_priority)
{
    if (shash_is_empty(list))
    {
        return NULL;
    }
    else
    {
        const struct shash_node **nodes;
        struct shash_node *node;
        size_t iter = 0;
        size_t count = 0;

        count = shash_count(list);
        nodes = malloc(count * sizeof(*nodes));
        if (nodes == NULL)
          return NULL;
        SHASH_FOR_EACH (node, list) {
            nodes[iter++] = node;
        }
        if (by_default_priority)
            qsort(nodes, count, sizeof(*nodes), compare_nodes_by_tacacs_server_default_priority);
        else
            qsort(nodes, count, sizeof(*nodes), compare_nodes_by_tacacs_server_group_priority);
        return nodes;
    }
}
