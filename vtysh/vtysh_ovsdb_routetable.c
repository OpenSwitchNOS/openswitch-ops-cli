/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distrouteuted under the License is distrouteuted on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
*/
/****************************************************************************
 *
 * @file vtysh_ovsdb_routetable.c
 * Source for registering client callback with route table.
 *
 ***************************************************************************/

#include "vswitch-idl.h"
#include "openhalon-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_routetable.h"

char routeclientname[] = "vtysh_ovsdb_routetable_clientcallback";

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_routetable_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_routetable_clientcallback(void *p_private)
{
  vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

  const struct ovsrec_route *row_route;
  char str_temp[80];
  vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
                           "vtysh_ovsdb_routetable_clientcallback entered");

  OVSREC_ROUTE_FOR_EACH(row_route, p_msg->idl) {
      int ipv4_flag = 0;
      int ipv6_flag = 0;
      if (row_route->address_family != NULL) {
          if (!strcmp(row_route->address_family, "ipv4")) {
              ipv4_flag = 1;
          } else if (!strcmp(row_route->address_family, "ipv6")) {
              ipv6_flag = 1;
          }
      } else {
          break;
      }

      if (ipv4_flag == 1 || ipv6_flag == 1) {
          if (row_route->prefix) {
              char str[50];
              int len = 0;
              len = snprintf(str, sizeof(str), "%s", row_route->prefix);
              if (ipv4_flag == 1 && ipv6_flag == 0) {
                  snprintf(str_temp, sizeof(str_temp), "ip route %s", str);
              }
              else {
                  snprintf(str_temp, sizeof(str_temp), "ipv6 route %s", str);
              }
          } else {
              return e_vtysh_error;
          }

          if(row_route->distance != NULL) {
          if (row_route->n_nexthops && row_route->nexthops[0]->ip_address &&
              row_route->distance) {
              if (*row_route->distance == 1) {
                  vtysh_ovsdb_cli_print(p_msg,"%s %s", str_temp,
                      row_route->nexthops[0]->ip_address);
              } else {
                  vtysh_ovsdb_cli_print(p_msg,"%s %s %d", str_temp,
                      row_route->nexthops[0]->ip_address, *row_route->distance);
              }

          } else if (row_route->n_nexthops && row_route->nexthops[0]->ports
              && row_route->distance) {
              if (*row_route->distance == 1) {
                  vtysh_ovsdb_cli_print(p_msg,"%s %s", str_temp,
                      row_route->nexthops[0]->ports);
              } else {
                  vtysh_ovsdb_cli_print(p_msg,"%s %s %d", str_temp,
                      row_route->nexthops[0]->ports, *row_route->distance);
              }
          } else {
              return e_vtysh_error;
          }
          }

      }

  }
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_init_routetableclients
| Responsibility : registers the client callbacks for routetable
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_ovsdb_init_routetableclients()
{
  vtysh_ovsdb_client client_route;

  client_route.p_client_name = routeclientname;
  client_route.client_id = e_vtysh_route_table_config;
  client_route.p_callback = &vtysh_ovsdb_routetable_clientcallback;

  vtysh_ovsdbtable_addclient(e_route_table, e_vtysh_route_table_config, &client_route);

  return e_vtysh_ok;
}
