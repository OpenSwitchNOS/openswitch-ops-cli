 /*
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
 * File: l3routes_vty.c
 *
 * Purpose:  To add l3 routes configuration and display commands.
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file l3routes_vty.c
 * Source to configure l3 static routes into ovsdb tables.
 *
 ***************************************************************************/

#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "openswitch-dflt.h"
#include "ovsdb-idl.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "prefix.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "l3routes_vty.h"

VLOG_DEFINE_THIS_MODULE (vtysh_l3routes_cli);
extern struct ovsdb_idl *idl;
#define DEFAULT_DISTANCE 1

/*
 * Check if port is part of any VRF and return the VRF row.
 */
const struct ovsrec_vrf*
port_find_vrf (const struct ovsrec_port *port_row)
{
  const struct ovsrec_vrf *vrf_row = NULL;
  size_t i;
  OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
      for (i = 0; i < vrf_row->n_ports; i++)
        {
          if (vrf_row->ports[i] == port_row)
            return vrf_row;
        }
    }
  return NULL;
}

struct ovsrec_nexthop *
set_nexthop_entry (struct ovsdb_idl_txn *status_txn, char * nh_entry,
                   bool prefix_match, bool static_match, char * dist_entry,
                   const struct ovsrec_route * row, char * ip_addr_family)
{
  struct ovsrec_nexthop *row_nh = NULL;
  const struct ovsrec_port *row_port = NULL;
  const struct ovsrec_vrf *row_vrf = NULL;
  struct in_addr nexthop;
  struct in6_addr nexthop_ipv6;
  int ret = 0;
  int64_t distance;

  row_nh = ovsrec_nexthop_insert (status_txn);

  if (!strcmp ("ipv4", ip_addr_family))
    ret = inet_pton (AF_INET, nh_entry, &nexthop);
  else if (!strcmp ("ipv6", ip_addr_family))
    ret = inet_pton (AF_INET6, nh_entry, &nexthop_ipv6);

  if (ret)
    ovsrec_nexthop_set_ip_address (row_nh, nh_entry);
  else
    {
      OVSREC_PORT_FOR_EACH (row_port, idl)
        {
          if (!strcmp (row_port->name, nh_entry))
            {
              row_vrf = port_find_vrf (row_port);
              break;
            }
        }

      if (row_port == NULL)
        {
          vty_out (vty, "\nInterface %s not configured%s", nh_entry,
                   VTY_NEWLINE);
          return NULL;
        }

      if (row_vrf == NULL)
        {
          vty_out (vty, "\nInterface %s is not L3%s", nh_entry, VTY_NEWLINE);
          return NULL;
        }
      ovsrec_nexthop_set_ports (row_nh, (struct ovsrec_port**) &row_port,
                                row_nh->n_ports + 1);
    }

  if (!prefix_match)
    {
      if (dist_entry == NULL)
        {
          /*
           * Hardcode the n_distance param to 1 for static routes
           */
          distance = DEFAULT_DISTANCE;
          ovsrec_route_set_distance (row, &distance, 1);
        }
      else
        {
          distance = atoi (dist_entry);
          ovsrec_route_set_distance (row, &distance, 1);
        }
    }
  else
    {
      if (static_match)
        {
          if (dist_entry == NULL)
            {
              /*
               * Hardcode the n_distance param to 1 for static routes
               */
              if (*row->distance != DEFAULT_DISTANCE)
                {
                  vty_out (
                          vty,
                          "\nCannot configure default distance for this nexthop%s",
                          VTY_NEWLINE);
                  vty_out (vty, "Distance for this route is set to %ld, ",
                           *row->distance);
                  vty_out (
                          vty,
                          "decided by the distance entered for the first nexthop\n%s",
                          VTY_NEWLINE);
                  vty_out (vty,
                           "Please enter the new route with distance %ld%s",
                           *row->distance, VTY_NEWLINE);
                  return NULL;
                }
            }
          else if (*row->distance != atoi (dist_entry))
            {
              vty_out (vty,
                       "\nCannot configure new distance for this nexthop%s",
                       VTY_NEWLINE);
              vty_out (vty, "Distance for this route is already set to %ld, ",
                       *row->distance);
              vty_out (
                      vty,
                      "decided by the distance entered for the first nexthop\n%s",
                      VTY_NEWLINE);
              vty_out (vty, "Please enter the new route with distance %ld%s",
                       *row->distance, VTY_NEWLINE);
              return NULL;
            }
        }
      else
        return NULL;
    }
  return row_nh;
}

static int
#ifdef VRF_ENABLE
ip_route_common (struct vty *vty, char **argv, char *distance, char *vrf)
#else
ip_route_common (struct vty *vty, char **argv, char *distance)
#endif
{
  const struct ovsrec_route *row = NULL;
  struct ovsrec_nexthop *row_nh = NULL;
  const struct ovsrec_vrf *row_vrf = NULL;

  struct prefix p;
  int ret, i;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = NULL;
  char prefix_str[256];
  bool prefix_match = false;
  bool nh_match = false;
  bool static_match = false;
  #ifdef VRF_ENABLE
  char *vrf_name = NULL;
  char *ovs_rt_vrf = NULL;
  #endif
  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  #ifdef VRF_ENABLE
  if (!vrf)
    vrf_name = DEFAULT_VRF_NAME;
  else
    vrf_name = vrf;
  #endif

  ret = str2prefix (argv[0], &p);
  if (ret <= 0)
    {
      vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_WARNING;
    }
  /*
   * Convert to the final/optimized format before storing to DB
   */
  apply_mask (&p);
  memset (prefix_str, 0, sizeof(prefix_str));
  prefix2str ((const struct prefix*) &p, prefix_str, sizeof(prefix_str));

  if (strcmp (prefix_str, argv[0]))
    {
      vty_out (vty, "Invalid prefix. Valid prefix: %s", prefix_str);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  OVSREC_ROUTE_FOR_EACH (row, idl)
    {
      if (row->prefix != NULL)
        {
          #ifdef VRF_ENABLE
          if (row->vrf == NULL)
            ovs_rt_vrf = DEFAULT_VRF_NAME;
          else
            ovs_rt_vrf = row->vrf->name;

          if (strncmp(ovs_rt_vrf,vrf_name,OVSDB_VRF_NAME_MAXLEN))
            continue;
          #endif
          if (!strcmp (row->prefix, argv[0])
              && !strcmp (row->from, OVSREC_ROUTE_FROM_STATIC))
            {
              if (row->n_nexthops > MAX_NEXTHOPS_PER_ROUTE - 1)
                {
                  vty_out (vty, "Maximum %d nexthops per route",
                           MAX_NEXTHOPS_PER_ROUTE);
                  cli_do_config_abort (status_txn);
                  return CMD_OVSDB_FAILURE;
                }
              prefix_match = true;
              static_match = true;
              break;
            }
        }
    }

  if (row == NULL)
    {
     #ifdef VRF_ENABLE
     OVSREC_VRF_FOR_EACH(row_vrf,idl)
       {
         if (!strncmp(row_vrf->name,vrf_name,OVSDB_VRF_NAME_MAXLEN))
           break;
       }

     if (!row_vrf)
       {
         vty_out (vty, "VRF %s does not exist.%s", vrf_name,VTY_NEWLINE);
         VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
         cli_do_config_abort (status_txn);
         return CMD_OVSDB_FAILURE;
       }
     #else
      row_vrf = ovsrec_vrf_first (idl);
      if (!row_vrf)
        {
          VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
          cli_do_config_abort (status_txn);
          return CMD_OVSDB_FAILURE;
        }
     #endif

      row = ovsrec_route_insert (status_txn);

      ovsrec_route_set_vrf (row, row_vrf);

      ovsrec_route_set_address_family (row, OVSREC_ROUTE_ADDRESS_FAMILY_IPV4);

      ovsrec_route_set_sub_address_family (
          row, OVSREC_ROUTE_SUB_ADDRESS_FAMILY_UNICAST);

      ovsrec_route_set_prefix (row, (const char *) prefix_str);

      ovsrec_route_set_from (row, OVSREC_ROUTE_FROM_STATIC);

      row_nh = set_nexthop_entry (status_txn, argv[1], prefix_match,
                                  static_match, distance, row, "ipv4");
      if (row_nh == NULL)
        {
          cli_do_config_abort (status_txn);
          return CMD_OVSDB_FAILURE;
        }
      ovsrec_route_set_nexthops (row, &row_nh, row->n_nexthops + 1);

    }
  else
    {
      for (i = 0; i < row->n_nexthops; i++)
        {
          if (row->nexthops[i] != NULL && static_match)
            {
              if (row->nexthops[i]->ip_address != NULL)
                {
                  if (!strcmp (row->nexthops[i]->ip_address, argv[1]))
                    {
                      nh_match = true;
                      break;
                    }
                }
              else if (row->nexthops[i]->ports[0]->name != NULL)
                {
                  if (!strcmp (row->nexthops[i]->ports[0]->name, argv[1]))
                    {
                      nh_match = true;
                      break;
                    }
                }
            }
        }

      if (!nh_match)
        {
          row_nh = set_nexthop_entry (status_txn, argv[1], prefix_match,
                                      static_match, distance, row, "ipv4");
          if (row_nh == NULL)
            {
              cli_do_config_abort (status_txn);
              return CMD_OVSDB_FAILURE;
            }

          struct ovsrec_nexthop **nexthops = NULL;
          nexthops = xmalloc (sizeof *row->nexthops * (row->n_nexthops + 1));

          for (i = 0; i < row->n_nexthops; i++)
            nexthops[i] = row->nexthops[i];

          nexthops[row->n_nexthops] = row_nh;

          ovsrec_route_set_nexthops (row, nexthops, row->n_nexthops + 1);
          free (nexthops);
        }
      else
        {
          vty_out (vty, "\nNexthop already exists\n%s", VTY_NEWLINE);
        }
    }

  status = cli_do_config_finish (status_txn);

  if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED)))
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
  else
    return CMD_SUCCESS;

}

DEFUN (vtysh_ip_route,
    vtysh_ip_route_cmd,
    "ip route A.B.C.D/M (A.B.C.D|INTERFACE)",
    IP_STR
    "Configure static routes\n"
    "IP destination prefix (e.g. 10.0.0.0/8)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n")
{
#ifdef VRF_ENABLE
  return ip_route_common(vty, (char **)argv, NULL, NULL);
#else
  return ip_route_common(vty, (char **)argv, NULL);
#endif
}

DEFUN (vtysh_ip_route_distance,
    vtysh_ip_route_distance_cmd,
    "ip route A.B.C.D/M (A.B.C.D|INTERFACE) <1-255>",
    IP_STR
    "Configure static routes\n"
    "IP destination prefix (e.g. 10.0.0.0/8)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n"
    "Distance (Default: 1)\n")
{
#ifdef VRF_ENABLE
  return ip_route_common(vty, (char **)argv, (char *)argv[2], NULL);
#else
  return ip_route_common(vty, (char **)argv, (char *)argv[2]);
#endif
}

#ifdef VRF_ENABLE
DEFUN (vtysh_ip_route_vrf,
    vtysh_ip_route_vrf_cmd,
    "ip route A.B.C.D/M (A.B.C.D|INTERFACE) vrf WORD",
    IP_STR
    "Configure static routes\n"
    "IP destination prefix (e.g. 10.0.0.0/8)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n"
    "VRF Information\n"
    "VRF name\n")
{
  return ip_route_common(vty, (char **)argv, NULL, (char *)argv[2]);
}
#endif


#ifdef VRF_ENABLE
static int
show_routes (struct vty *vty, char * ip_addr_family, char* vrf)
#else
static int
show_routes (struct vty *vty, char * ip_addr_family)
#endif
{
  const struct ovsrec_route *row_route = NULL;
  int flag = 0;
  int disp_flag = 1;
  char str[50];
  int i, active_route_next_hops;
  #ifdef VRF_ENABLE
  char *vrf_name = vrf;

  if (NULL == vrf_name)
    vrf_name = DEFAULT_VRF_NAME;
  #endif

  OVSREC_ROUTE_FOR_EACH (row_route, idl)
    {
      #ifdef VRF_ENABLE
      if (strncmp(row_route->vrf->name, vrf_name,OVSDB_VRF_NAME_MAXLEN))
        continue;
      #endif

      if (strcmp (row_route->address_family, ip_addr_family))
        continue;

      if (row_route->selected == NULL || row_route->selected[0] == false)
        continue;

      if (!(row_route->prefix))
        continue;

      if (disp_flag == 1)
        {
          flag = 1;
          if (!strcmp ("ipv4", ip_addr_family))
            {
              vty_out (vty,
                       "\nDisplaying ipv4 routes selected for forwarding%s",
                       VTY_NEWLINE);
            }
          else if (!strcmp ("ipv6", ip_addr_family))
            {
              vty_out (vty,
                       "\nDisplaying ipv6 routes selected for forwarding%s",
                       VTY_NEWLINE);
            }
          vty_out (vty, "\n'[x/y]' denotes [distance/metric]%s\n", VTY_NEWLINE);
          disp_flag = 0;
        }

      memset (str, 0, sizeof(str));
      snprintf (str, sizeof(str), "%s", row_route->prefix);
      vty_out (vty, "%s", str);

      if (row_route->n_nexthops)
        {
          active_route_next_hops = 0;
          for (i = 0; i < row_route->n_nexthops; i++)
            {
              if (row_route->nexthops[i]->selected == NULL ||
                  row_route->nexthops[i]->selected[0] == true)
                active_route_next_hops++;
            }
          vty_out (vty, ",  %zd %s next-hops %s", active_route_next_hops,
                   row_route->sub_address_family, VTY_NEWLINE);
        }

      if (row_route->n_nexthops)
        {
          memset (str, 0, sizeof(str));

          for (i = 0; i < row_route->n_nexthops; i++)
            {
              if (row_route->nexthops[i]->selected == NULL ||
                  row_route->nexthops[i]->selected[0] == true)
                {
                  if (row_route->nexthops[i]->ip_address)
                    {
                      snprintf (str, sizeof(str), " %s",
                                row_route->nexthops[i]->ip_address);
                      vty_out (vty, "\tvia %s", str);
                    }
                  else if (row_route->nexthops[i]->ports[0]->name)
                    {
                      snprintf (str, sizeof(str), " %s",
                                row_route->nexthops[i]->ports[0]->name);
                      vty_out (vty, "\tvia %s", str);
                    }

                  vty_out (vty, ",  [%ld", *row_route->distance);

                  if (row_route->metric)
                    vty_out (vty, "/%ld]", *row_route->metric);
                  else
                    vty_out (vty, "/0]");

                  vty_out (vty, ",  %s", row_route->from);

                  vty_out (vty, VTY_NEWLINE);
                }
            }
        }
    }

  if (flag == 0)
    {
      if (!strcmp ("ipv4", ip_addr_family))
        vty_out (vty, "\nNo ipv4 routes configured %s", VTY_NEWLINE);
      else if (!strcmp ("ipv6", ip_addr_family))
        vty_out (vty, "\nNo ipv6 routes configured %s", VTY_NEWLINE);
      return CMD_SUCCESS;
    }
  else
    return CMD_SUCCESS;
}
#ifdef VRF_ENABLE
DEFUN (vtysh_show_ip_route,
    vtysh_show_ip_route_cmd,
    "show ip route { vrf WORD }",
    SHOW_STR
    IP_STR
    ROUTE_STR
    "VRF Information\n"
    "VRF name\n")
{
  int retval;

  retval = show_routes(vty, "ipv4", argv[0]);
  vty_out(vty, VTY_NEWLINE);

  return retval;
}
#else
DEFUN (vtysh_show_ip_route,
    vtysh_show_ip_route_cmd,
    "show ip route",
    SHOW_STR
    IP_STR
    ROUTE_STR)
{
  int retval;

  retval = show_routes(vty, "ipv4");
  vty_out(vty, VTY_NEWLINE);

  return retval;
}
#endif

static int
#ifdef VRF_ENABLE
no_ip_route_common (struct vty *vty, char **argv, char *distance, char *vrf)
#else
no_ip_route_common (struct vty *vty, char **argv, char *distance)
#endif
{
  int ret;
  const struct ovsrec_route *row_route = NULL;
  int flag = 0;
  struct prefix p;
  char prefix_str[256];
  int found_flag = 0;
  char str[17];
  int distance_match = 0;
  int i, n;
  #ifdef VRF_ENABLE
  char *vrf_name = NULL;
  char *ovs_rt_vrf = NULL;
  #endif

  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = NULL;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  ret = str2prefix (argv[0], &p);
  if (ret <= 0)
    {
      vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_WARNING;
    }
  /*
   * Convert to the final/optimized format before storing to DB
   */
  apply_mask (&p);
  memset (prefix_str, 0, sizeof(prefix_str));
  prefix2str ((const struct prefix*) &p, prefix_str, sizeof(prefix_str));

  if (strcmp (prefix_str, argv[0]))
    {
      vty_out (vty, "Invalid prefix. Valid prefix: %s", prefix_str);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  #ifdef VRF_ENABLE
  if (!vrf)
    vrf_name = DEFAULT_VRF_NAME;
  else
    vrf_name = vrf;
  #endif

  OVSREC_ROUTE_FOR_EACH (row_route, idl)
    {
     #ifdef VRF_ENABLE
     if (row_route->vrf == NULL)
       ovs_rt_vrf = DEFAULT_VRF_NAME;
     else
       ovs_rt_vrf = row_route->vrf->name;

     if (strncmp(ovs_rt_vrf,vrf_name,OVSDB_VRF_NAME_MAXLEN))
       continue;
     #endif

      if (row_route->address_family != NULL)
        {
          if (strcmp (row_route->address_family, "ipv4"))
            continue;
        }

      if (row_route->from != NULL)
        {
          if (strcmp (row_route->from, OVSREC_ROUTE_FROM_STATIC))
            continue;
        }

      if (row_route->prefix != NULL)
        {
          /* Checking for presence of Prefix and Nexthop entries in a row */
          if (0 == strcmp (prefix_str, row_route->prefix))
            {
              if (row_route->n_nexthops)
                {
                  memset (str, 0, sizeof(str));
                  distance_match = 0;

                  if (distance != NULL)
                    {
                      (atoi (distance) == *row_route->distance) ?
                          (distance_match = 1) : (distance_match = 0);
                    }
                  else
                    distance_match = 1;

                  /* Checking for presence of Nexthop IP or Interface*/
                  struct ovsrec_nexthop **nexthops = NULL;
                  nexthops = xmalloc (
                      sizeof *row_route->nexthops
                          * (row_route->n_nexthops - 1));

                  for (i = n = 0; i < row_route->n_nexthops; i++)
                    {
                      if ((row_route->nexthops[i]->ip_address) ||
                          (row_route->nexthops[i]->ports[0]->name))
                        {
                          if (row_route->nexthops[i]->ip_address != NULL)
                            {
                              if (0 == strcmp (argv[1],
                                               row_route->nexthops[i]->ip_address)
                                               && (distance_match == 1))
                                {
                                  found_flag = 1;
                                  ovsrec_nexthop_delete (
                                      row_route->nexthops[i]);
                                  if (row_route->n_nexthops == 1)
                                    ovsrec_route_delete (row_route);
                                }
                              else
                                nexthops[n++] = row_route->nexthops[i];
                            }
                          else if (row_route->nexthops[i]->ports[0]->name
                              != NULL)
                            {
                              if (0 == strcmp (argv[1],
                                               row_route->nexthops[i]->ports[0]->name)
                                               && (distance_match == 1))
                                {
                                  found_flag = 1;
                                  ovsrec_nexthop_delete (
                                      row_route->nexthops[i]);
                                  if (row_route->n_nexthops == 1)
                                    ovsrec_route_delete (row_route);
                                }
                              else
                                nexthops[n++] = row_route->nexthops[i];
                            }
                        }
                    }
                  if (row_route->n_nexthops != 1 && found_flag == 1)
                    ovsrec_route_set_nexthops (row_route, nexthops,
                                               row_route->n_nexthops - 1);
                  free (nexthops);
                }
            }
        }
      flag = 1;
    }

  if (flag == 0)
    vty_out (vty, "No ip routes configured %s", VTY_NEWLINE);

  if (found_flag == 0)
    vty_out (vty, "\nNo such ip route found %s\n", VTY_NEWLINE);

  status = cli_do_config_finish (status_txn);

  if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED)))
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
  else
    return CMD_SUCCESS;
}

DEFUN (vtysh_no_ip_route,
    vtysh_no_ip_route_cmd,
    "no ip route A.B.C.D/M (A.B.C.D|INTERFACE)",
    NO_STR
    IP_STR
    "Configure static route\n"
    "IP destination prefix (e.g. 10.0.0.0)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n")
{
#ifdef VRF_ENABLE
  return no_ip_route_common(vty, (char **)argv, NULL, NULL);
#else
  return no_ip_route_common(vty, (char **)argv, NULL);
#endif
}

DEFUN (vtysh_no_ip_route_distance,
    vtysh_no_ip_route_distance_cmd,
    "no ip route A.B.C.D/M (A.B.C.D|INTERFACE) <1-255>",
    NO_STR
    IP_STR
    "Configure static route\n"
    "IP destination prefix (e.g. 10.0.0.0)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n"
    "Distance (Default: 1)\n")
{
#ifdef VRF_ENABLE
  return no_ip_route_common(vty, (char **)argv, (char *)argv[2], NULL);
#else
  return no_ip_route_common(vty, (char **)argv, (char *)argv[2]);
#endif
}

#ifdef VRF_ENABLE
DEFUN (vtysh_no_ip_route_vrf,
    vtysh_no_ip_route_vrf_cmd,
    "no ip route A.B.C.D/M (A.B.C.D|INTERFACE) vrf WORD",
    NO_STR
    IP_STR
    "Configure static routes\n"
    "IP destination prefix (e.g. 10.0.0.0/8)\n"
    "Nexthop IP (eg. 10.0.0.1)\n"
    "Outgoing interface\n"
    "VRF Information\n"
    "VRF name\n")
{
  return no_ip_route_common(vty, (char **)argv, NULL, (char *)argv[2]);
}
#endif

/* IPv6 CLIs*/

static int
ipv6_route_common (struct vty *vty, char **argv, char *distance)
{
  const struct ovsrec_route *row = NULL;
  const struct ovsrec_nexthop *row_nh = NULL;
  const struct ovsrec_vrf *row_vrf = NULL;

  struct prefix p;
  int ret, i;
  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = NULL;
  char prefix_str[256];
  bool prefix_match = false;
  bool nh_match = false;
  bool static_match = false;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  ret = str2prefix (argv[0], &p);
  if (ret <= 0)
    {
      vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_WARNING;
    }
  /*
   * Convert to the final/optimized format before storing to DB
   */
  apply_mask (&p);
  memset (prefix_str, 0, sizeof(prefix_str));
  prefix2str ((const struct prefix*) &p, prefix_str, sizeof(prefix_str));

  if (strcmp (prefix_str, argv[0]))
    {
      vty_out (vty, "Invalid prefix. Valid prefix: %s", prefix_str);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  OVSREC_ROUTE_FOR_EACH (row, idl)
    {
      if (row->prefix != NULL)
        {
          if (!strcmp (row->prefix, argv[0]) &&
              !strcmp (row->from, OVSREC_ROUTE_FROM_STATIC))
            {
              if (row->n_nexthops != 0)
                {
                  if (row->n_nexthops > MAX_NEXTHOPS_PER_ROUTE - 1)
                    {
                      vty_out (vty, "Maximum %d nexthops per route",
                               MAX_NEXTHOPS_PER_ROUTE);
                      cli_do_config_abort (status_txn);
                      return CMD_OVSDB_FAILURE;
                    }
                }
              prefix_match = true;
              static_match = true;
              break;
            }
        }
    }

  if (row == NULL)
    {
      row_vrf = ovsrec_vrf_first (idl);
      if (!row_vrf)
        {
          VLOG_ERR (OVSDB_ROW_FETCH_ERROR);
          cli_do_config_abort (status_txn);
          return CMD_OVSDB_FAILURE;
        }

      row = ovsrec_route_insert (status_txn);

      ovsrec_route_set_vrf (row, row_vrf);

      ovsrec_route_set_address_family (row, OVSREC_ROUTE_ADDRESS_FAMILY_IPV6);

      ovsrec_route_set_sub_address_family (
          row, OVSREC_ROUTE_SUB_ADDRESS_FAMILY_UNICAST);

      ovsrec_route_set_prefix (row, (const char *) prefix_str);

      ovsrec_route_set_from (row, OVSREC_ROUTE_FROM_STATIC);

      row_nh = set_nexthop_entry (status_txn, (char*) argv[1], prefix_match,
                                  static_match, distance, row, "ipv6");
      if (row_nh == NULL)
        {
          cli_do_config_abort (status_txn);
          return CMD_OVSDB_FAILURE;
        }

      ovsrec_route_set_nexthops (row, (struct ovsrec_nexthop**) &row_nh,
                                 row->n_nexthops + 1);
    }
  else
    {
      for (i = 0; i < row->n_nexthops; i++)
        {
          if (row->nexthops[i] != NULL && static_match)
            {
              if (row->nexthops[i]->ip_address != NULL)
                {
                  if (!strcmp (row->nexthops[i]->ip_address, argv[1]))
                    {
                      nh_match = true;
                      break;
                    }
                }
              else if (row->nexthops[i]->ports[0]->name != NULL)
                {
                  if (!strcmp (row->nexthops[i]->ports[0]->name, argv[1]))
                    {
                      nh_match = true;
                      break;
                    }
                }
            }
        }

      if (!nh_match)
        {
          row_nh = set_nexthop_entry (status_txn, (char*) argv[1], prefix_match,
                                      static_match, distance, row, "ipv6");

          if (row_nh == NULL)
            {
              cli_do_config_abort (status_txn);
              return CMD_OVSDB_FAILURE;
            }

          struct ovsrec_nexthop **nexthops = NULL;
          nexthops = xmalloc (sizeof *row->nexthops * (row->n_nexthops + 1));

          for (i = 0; i < row->n_nexthops; i++)
            {
              nexthops[i] = row->nexthops[i];
            }
          nexthops[row->n_nexthops] = (struct ovsrec_nexthop*) row_nh;

          ovsrec_route_set_nexthops (row, nexthops, row->n_nexthops + 1);
          free (nexthops);
        }
      else
        vty_out (vty, "\nNexthop already exists\n%s", VTY_NEWLINE);
    }

  status = cli_do_config_finish (status_txn);

  if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED)))
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
  else
    return CMD_SUCCESS;

}

DEFUN (vtysh_ipv6_route,
    vtysh_ipv6_route_cmd,
    "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE)",
    IP_STR
    "Configure static routes\n"
    "IPv6 destination prefix (e.g. 2010:bd9::/32)\n"
    "Nexthop IPv6 (eg. 2010:bda::)\n"
    "Outgoing interface\n")
{
  return ipv6_route_common(vty, (char **)argv, NULL);
}

DEFUN (vtysh_ipv6_route_distance,
    vtysh_ipv6_route_distance_cmd,
    "ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) <1-255>",
    IP_STR
    "Configure static routes\n"
    "IPv6 destination prefix (e.g. 2010:bd9::/32)\n"
    "Nexthop IPv6 (eg. 2010:bda::)\n"
    "Outgoing interface\n"
    "Distance (Default: 1)\n")
{
  return ipv6_route_common(vty, (char **)argv, (char *)argv[2]);
}

DEFUN (vtysh_show_ipv6_route,
    vtysh_show_ipv6_route_cmd,
    "show ipv6 route",
    SHOW_STR
    IP_STR
    ROUTE_STR)
{
  int retval;
#ifdef VRF_ENABLE
  retval = show_routes(vty, "ipv6", NULL);
#else
  retval = show_routes(vty, "ipv6");
#endif
  vty_out(vty, VTY_NEWLINE);

  return retval;
}

static int
no_ipv6_route_common (struct vty *vty, char **argv, char *distance)
{
  int ret;
  const struct ovsrec_route *row_route = NULL;
  int flag = 0;
  struct prefix p;
  char prefix_str[256];
  int found_flag = 0;
  char str[17];
  int distance_match = 0;
  int i, n;

  enum ovsdb_idl_txn_status status;
  struct ovsdb_idl_txn *status_txn = NULL;

  status_txn = cli_do_config_start ();

  if (status_txn == NULL)
    {
      VLOG_ERR (OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  ret = str2prefix (argv[0], &p);
  if (ret <= 0)
    {
      vty_out (vty, "%% Malformed address format%s", VTY_NEWLINE);
      cli_do_config_abort (status_txn);
      return CMD_WARNING;
    }
  /*
   * Convert to the final/optimized format before storing to DB
   */
  apply_mask (&p);
  memset (prefix_str, 0, sizeof(prefix_str));
  prefix2str ((const struct prefix*) &p, prefix_str, sizeof(prefix_str));

  if (strcmp (prefix_str, argv[0]))
    {
      vty_out (vty, "Invalid prefix. Valid prefix: %s", prefix_str);
      cli_do_config_abort (status_txn);
      return CMD_OVSDB_FAILURE;
    }

  OVSREC_ROUTE_FOR_EACH (row_route, idl)
    {
      if (row_route->address_family != NULL)
        {
          if (strcmp (row_route->address_family, "ipv6"))
            continue;
        }

      if (row_route->from != NULL)
        {
          if (strcmp (row_route->from, OVSREC_ROUTE_FROM_STATIC))
            continue;
        }

      if (row_route->prefix != NULL)
        {
          /* Checking for presence of Prefix and Nexthop entries in a row */
          if (0 == strcmp (prefix_str, row_route->prefix))
            {
              if (row_route->n_nexthops)
                {
                  memset (str, 0, sizeof(str));
                  distance_match = 0;

                  if (distance != NULL)
                    {
                      (atoi (distance) == *row_route->distance) ?
                          (distance_match = 1) : (distance_match = 0);
                    }
                  else
                    distance_match = 1;

                  /* Checking for presence of Nexthop IP or Interface*/
                  struct ovsrec_nexthop **nexthops = NULL;
                  nexthops = xmalloc (
                      sizeof *row_route->nexthops
                          * (row_route->n_nexthops - 1));

                  for (i = n = 0; i < row_route->n_nexthops; i++)
                    {
                      if ((row_route->nexthops[i]->ip_address) ||
                          (row_route->nexthops[i]->ports[0]->name))
                        {
                          if (row_route->nexthops[i]->ip_address != NULL)
                            {
                              if (0 == strcmp (argv[1],
                                               row_route->nexthops[i]->ip_address)
                                               && (distance_match == 1))
                                {
                                  found_flag = 1;
                                  ovsrec_nexthop_delete (
                                      row_route->nexthops[i]);
                                  if (row_route->n_nexthops == 1)
                                    ovsrec_route_delete (row_route);
                                }
                              else
                                nexthops[n++] = row_route->nexthops[i];
                            }
                          else if (row_route->nexthops[i]->ports[0]->name
                              != NULL)
                            {
                              if (0 == strcmp (argv[1],
                                               row_route->nexthops[i]->ports[0]->name)
                                               && (distance_match == 1))
                                {
                                  found_flag = 1;
                                  ovsrec_nexthop_delete (
                                      row_route->nexthops[i]);
                                  if (row_route->n_nexthops == 1)
                                    {
                                      ovsrec_route_delete (row_route);
                                    }
                                }
                              else
                                nexthops[n++] = row_route->nexthops[i];
                            }
                        }
                    }
                  if (row_route->n_nexthops != 1 && found_flag == 1)
                    {
                      ovsrec_route_set_nexthops (row_route, nexthops,
                                                 row_route->n_nexthops - 1);
                    }
                  free (nexthops);
                }
            }
        }
      flag = 1;
    }

  if (flag == 0)
    vty_out (vty, "No ipv6 routes configured %s", VTY_NEWLINE);

  if (found_flag == 0)
    vty_out (vty, "\nNo such ipv6 route found %s\n", VTY_NEWLINE);

  status = cli_do_config_finish (status_txn);

  if (((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED)))
    {
      VLOG_ERR (OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
    }
  else
    return CMD_SUCCESS;
}

DEFUN (vtysh_no_ipv6_route,
    vtysh_no_ipv6_route_cmd,
    "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE)",
    NO_STR
    IP_STR
    "Configure static route\n"
    "IP destination prefix (e.g. 2010:bd9::)\n"
    "Nexthop IP (eg. 2010:bda::)\n"
    "Outgoing interface\n")
{
  return no_ipv6_route_common(vty, (char **)argv, NULL);
}

DEFUN (vtysh_no_ipv6_route_distance,
    vtysh_no_ipv6_route_distance_cmd,
    "no ipv6 route X:X::X:X/M (X:X::X:X|INTERFACE) <1-255>",
    NO_STR
    IP_STR
    "Configure static route\n"
    "IP destination prefix (e.g. 2010:bd9::)\n"
    "Nexthop IP (eg. 2010:bda::)\n"
    "Outgoing interface\n"
    "Distance (Default: 1)\n")
{
  return no_ipv6_route_common(vty, (char **)argv, (char *)argv[2]);
}

#ifdef VRF_ENABLE
static int
show_rib (struct vty *vty, char * ip_addr_family, char *vrf)
#else
static int
show_rib (struct vty *vty, char * ip_addr_family)
#endif
{
  const struct ovsrec_route *row_route = NULL;
  int flag = 0;
  int disp_flag = 1;
  char str[50];
  int i;
  #ifdef VRF_ENABLE
  char *vrf_name = vrf;

  if (NULL == vrf_name)
    vrf_name = DEFAULT_VRF_NAME;
  #endif

  OVSREC_ROUTE_FOR_EACH (row_route, idl)
    {
      if (row_route->protocol_private != NULL)
        {
          if (row_route->protocol_private[0] == true)
            continue;
        }

      #ifdef VRF_ENABLE
      if (strncmp(row_route->vrf->name, vrf_name,OVSDB_VRF_NAME_MAXLEN))
        continue;
      #endif

      if (strcmp (row_route->address_family, ip_addr_family))
        continue;

      if (disp_flag == 1)
        {
          flag = 1;
          if (!strcmp ("ipv4", ip_addr_family))
            {
              vty_out (vty, "\nDisplaying ipv4 rib entries %s", VTY_NEWLINE);
            }
          else if (!strcmp ("ipv6", ip_addr_family))
            {
              vty_out (
                  vty,
                  "\n\n-----------------------------------------------------%s\n",
                  VTY_NEWLINE);
              vty_out (vty, "\nDisplaying ipv6 rib entries %s", VTY_NEWLINE);
            }
          vty_out (vty, "\n'*' denotes selected%s", VTY_NEWLINE);
          vty_out (vty, "'[x/y]' denotes [distance/metric]%s\n", VTY_NEWLINE);
          disp_flag = 0;
        }

      if (row_route->prefix)
        {
          memset (str, 0, sizeof(str));
          snprintf (str, sizeof(str), "%s", row_route->prefix);
          if (row_route->selected != NULL && row_route->selected[0] == true)
            vty_out (vty, "*%s", str);
          else
            vty_out (vty, "%s", str);
        }

      if (row_route->n_nexthops)
        {
          vty_out (vty, ",  %zd %s next-hops %s", row_route->n_nexthops,
                   row_route->sub_address_family, VTY_NEWLINE);
        }

      if (row_route->n_nexthops)
        {
          memset (str, 0, sizeof(str));

          for (i = 0; i < row_route->n_nexthops; i++)
            {
              if (row_route->nexthops[i]->ip_address)
                snprintf (str, sizeof(str), " %s",
                          row_route->nexthops[i]->ip_address);
              else if (row_route->nexthops[i]->ports[0]->name)
                snprintf (str, sizeof(str), " %s",
                          row_route->nexthops[i]->ports[0]->name);

              if (row_route->nexthops[i]->selected == NULL ||
                  row_route->nexthops[i]->selected[0] == true)
                vty_out (vty, "\t*via %s", str);
              else
                vty_out (vty, "\tvia %s", str);

              vty_out (vty, ",  [%ld", *row_route->distance);

              if (row_route->metric)
                vty_out (vty, "/%ld]", *row_route->metric);
              else
                vty_out (vty, "/0]");

              vty_out (vty, ",  %s", row_route->from);

              vty_out (vty, VTY_NEWLINE);
            }
        }
    }

  if (flag == 0)
    {
      if (!strcmp ("ipv4", ip_addr_family))
        vty_out (vty, "\nNo ipv4 rib entries %s", VTY_NEWLINE);
      else if (!strcmp ("ipv6", ip_addr_family))
        vty_out (vty, "\nNo ipv6 rib entries %s", VTY_NEWLINE);
      return CMD_SUCCESS;
    }
  else
    return CMD_SUCCESS;

}

#ifdef VRF_ENABLE
DEFUN (vtysh_show_rib,
    vtysh_show_rib_cmd,
    "show rib {vrf WORD}",
    SHOW_STR
    RIB_STR
    "VRF Information\n"
    "VRF name\n")
{
  int retval;

  retval = show_rib(vty, "ipv4", argv[0]);
  retval = show_rib(vty, "ipv6", NULL);
  vty_out(vty, VTY_NEWLINE);

  return retval;
}
#else
DEFUN (vtysh_show_rib,
    vtysh_show_rib_cmd,
    "show rib",
    SHOW_STR
    RIB_STR)
{
  int retval;

  retval = show_rib(vty, "ipv4");
  retval = show_rib(vty, "ipv6");
  vty_out(vty, VTY_NEWLINE);

  return retval;
}
#endif

void
l3routes_vty_init (void)
{
  install_element (CONFIG_NODE, &vtysh_ip_route_cmd);
  install_element (CONFIG_NODE, &vtysh_ip_route_distance_cmd);
#ifdef VRF_ENABLE
  install_element (CONFIG_NODE, &vtysh_ip_route_vrf_cmd);
#endif
  install_element (ENABLE_NODE, &vtysh_show_ip_route_cmd);
  install_element (CONFIG_NODE, &vtysh_no_ip_route_cmd);
  install_element (CONFIG_NODE, &vtysh_no_ip_route_distance_cmd);
#ifdef VRF_ENABLE
  install_element (CONFIG_NODE, &vtysh_no_ip_route_vrf_cmd);
#endif

  install_element (CONFIG_NODE, &vtysh_ipv6_route_cmd);
  install_element (CONFIG_NODE, &vtysh_ipv6_route_distance_cmd);
  install_element (ENABLE_NODE, &vtysh_show_ipv6_route_cmd);
  install_element (CONFIG_NODE, &vtysh_no_ipv6_route_cmd);
  install_element (CONFIG_NODE, &vtysh_no_ipv6_route_distance_cmd);

  install_element (ENABLE_NODE, &vtysh_show_rib_cmd);
}
