/* BGP CLI implementation with Halon vtysh.
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
 * File: bgp_vtysh.c
 *
 * Purpose: This file contains implementation of all BGP related CLI commands.
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
#include "bgpd/bgp_vty.h"
#include "vswitch-idl.h"
#include "openvswitch/vlog.h"


extern struct ovsdb_idl *idl;
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

#define BGP_SHOW_SCODE_HEADER "Status codes: s suppressed, d damped, "\
                              "h history, * valid, > best, = multipath,%s"\
                "              i internal, r RIB-failure, S Stale, R Removed%s"
#define BGP_SHOW_OCODE_HEADER "Origin codes: i - IGP, e - EGP, ? - incomplete%s%s"
#define BGP_SHOW_HEADER "   Network          Next Hop            Metric LocPrf Weight Path%s"
VLOG_DEFINE_THIS_MODULE(bgp_vty);

static void
print_route_status(struct vty *vty, int64_t flags)
{
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

  /* Internal route. TODO */
  /*
    if ((binfo->peer->as) && (binfo->peer->as == binfo->peer->local_as))
      vty_out (vty, "i");
    else
    vty_out (vty, " "); */
}

/* Function to print route status code */
static void show_routes (struct vty *vty)
{
  const struct ovsrec_rib *ribRow = NULL;
  int ii;

  // Read RIB flags column from RIB table
  OVSREC_RIB_FOR_EACH(ribRow, idl) {
    if (!ribRow)
      continue;
    print_route_status(vty, ribRow->flags);
    if (ribRow->prefix) {
      char str[17] ;
      int len = 0;
      len = snprintf(str, sizeof(str), " %s/%d", ribRow->prefix, ribRow->prefix_len);
      vty_out(vty, "%s", str);
      if (len < 18)
	vty_out (vty, "%*s", 18-len, " ");

      // nexthop
      if (!strcmp(ribRow->address_family, OVSREC_RIB_ADDRESS_FAMILY_IPV4)) {
	if (ribRow->n_nexthop_list) {
	  // Get the nexthop list
	  //VLOG_INFO("No. of next hops : %d", ribRow->n_nexthop_list);
	  const struct ovsdb_datum *datum = NULL;
	  datum = ovsrec_rib_get_nexthop_list(ribRow, OVSDB_TYPE_STRING);
	  for (ii = 0; ii < ribRow->n_nexthop_list; ii++) {
	    vty_out (vty, "%-16s", datum->keys[ii].string);
	  }
	} else {
	  vty_out (vty, "%-16s", " ");
	  syslog(LOG_DEBUG, "%s:No next hops for this route\n", __FUNCTION__);
	} // if 'ribRow->n_nexthop_list'

      } else {
	// TODO ipv6
	VLOG_INFO("Address family not supported\n");
      }
      vty_out (vty, "%10d", ribRow->metric);
      // TODO weight, local pref, aspath
      vty_out (vty, "%7d", 0);
      vty_out (vty, "%7d ", 0);
      vty_out(vty, "%s", " ");
      // print origin
      if (ribRow->from_protocol)
	vty_out(vty, "%s", ribRow->from_protocol);
      vty_out (vty, VTY_NEWLINE);
    }

  }
}

DEFUN(vtysh_show_ip_bgp,
      vtysh_show_ip_bgp_cmd,
      "show ip bgp",
      SHOW_STR
      IP_STR
      BGP_STR)
{
  const struct ovsrec_bgp_router *bgpRow = NULL;
  ovsdb_idl_run(idl);
  ovsdb_idl_wait(idl);

  vty_out (vty, "BGP table version is 0\n", VTY_NEWLINE);
  vty_out (vty, BGP_SHOW_SCODE_HEADER, VTY_NEWLINE, VTY_NEWLINE);
  vty_out (vty, BGP_SHOW_OCODE_HEADER, VTY_NEWLINE, VTY_NEWLINE);

  OVSREC_BGP_ROUTER_FOR_EACH(bgpRow, idl) {
    if (!bgpRow) {
      vty_out(vty, "No bgp instance configured");
      continue;
    }
    char *id = bgpRow->router_id;
    if (id) {
      vty_out (vty, "Local router-id %s\n", id);
    } else {
      vty_out (vty, "Router-id not configured");
    }
  }
  vty_out (vty, BGP_SHOW_HEADER, VTY_NEWLINE);
  show_routes(vty);
  return CMD_SUCCESS;
}

void bgp_vty_init(void)
{
  install_element (ENABLE_NODE, &vtysh_show_ip_bgp_cmd);
}
