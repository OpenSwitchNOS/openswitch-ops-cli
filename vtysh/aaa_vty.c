/* AAA CLI commands
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
 * File: aaa_vty.c
 *
 * Purpose:  To add AAA CLI configuration and display commands.
 */

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "aaa_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include <arpa/inet.h>
#include <string.h>

extern struct ovsdb_idl *idl;

VLOG_DEFINE_THIS_MODULE(vtysh_aaa_cli);

static int aaa_set_global_status(const char *status)
{
  const struct ovsrec_open_vswitch *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_aaa;

  if (status_txn == NULL) {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_aaa, &row->aaa);

  if (strcmp(OPEN_VSWITCH_AAA_RADIUS, status) == 0) {
      smap_replace(&smap_aaa, OPEN_VSWITCH_AAA_RADIUS , HALON_TRUE_STR);
  }
  else if (strcmp(OPEN_VSWITCH_AAA_RADIUS_LOCAL,status) == 0) {
      smap_replace(&smap_aaa, OPEN_VSWITCH_AAA_RADIUS  ,HALON_FALSE_STR);
  }

  ovsrec_open_vswitch_set_aaa(row, &row->aaa);
  smap_destroy(&smap_aaa);

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to configure either local or radius configuration */
DEFUN (cli_aaa_set_global_status,
       aaa_set_global_status_cmd,
       "aaa authentication login (radius | local)",
       AAA_STR
       "User authentication\n"
       "Switch login\n"
       "Radius authentication\n"
       "Local authentication\n")
{
    return aaa_set_global_status(argv[0]);
}

static int aaa_fallback_option(const char *value)
{
  const struct ovsrec_open_vswitch *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_aaa;

  if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_aaa, &row->aaa);

  if ((strcmp(value,HALON_TRUE_STR) == 0) ) {
      smap_replace(&smap_aaa, OPEN_VSWITCH_AAA_FALLBACK, HALON_TRUE_STR);
  }
  else {
      smap_replace(&smap_aaa, OPEN_VSWITCH_AAA_FALLBACK, HALON_FALSE_STR);
  }

  ovsrec_open_vswitch_set_aaa(row, &row->aaa);
  smap_destroy(&smap_aaa);

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to enable fallback to local authentication */
DEFUN (cli_aaa_remove_fallback,
       aaa_remove_fallback_cmd,
       "aaa authentication login fallback error local",
       AAA_STR
       "User authentication\n"
       "Switch login\n"
       "Fallback authentication\n"
       "Radius server unreachable\n"
       "Local authentication")
{
    return aaa_fallback_option(HALON_TRUE_STR);
}

/* CLI to disable fallback to local authentication */
DEFUN (cli_aaa_no_remove_fallback,
       aaa_no_remove_fallback_cmd,
       "no aaa authentication login fallback error local",
       NO_STR
       AAA_STR
       "User authentication\n"
       "Switch login\n"
       "Fallback authentication\n"
       "Radius server unreachable\n"
       "Local authentication")
{
    return aaa_fallback_option(HALON_FALSE_STR);
}

static int aaa_show_aaa_authenctication()
{
  const struct ovsrec_open_vswitch *row = NULL;

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_OVSDB_FAILURE;
  }
  vty_out(vty, "AAA Authentication:%s", VTY_NEWLINE);
  if (!strcmp(smap_get(&row->aaa,OPEN_VSWITCH_AAA_RADIUS ),HALON_TRUE_STR)){
      vty_out(vty, "  Local authentication\t\t\t: %s%s","Disabled",VTY_NEWLINE);
      vty_out(vty, "  Radius authentication\t\t\t: %s%s","Enabled",VTY_NEWLINE);
  }
  else {
      vty_out(vty, "  Local authentication\t\t\t: %s%s","Enabled",VTY_NEWLINE);
      vty_out(vty, "  Radius authentication\t\t\t: %s%s","Disabled",VTY_NEWLINE);
  }
  if (!strcmp(smap_get(&row->aaa,OPEN_VSWITCH_AAA_FALLBACK),HALON_TRUE_STR)){
      vty_out(vty, "  Fallback to local authentication\t: %s%s","Enabled",VTY_NEWLINE);
  }
  else {
      vty_out(vty, "  Fallback to local authentication\t: %s%s","Disabled",VTY_NEWLINE);
  }

  return CMD_SUCCESS;
}

/* CLI to show authentication mechanism configured in DB */
DEFUN (cli_aaa_show_aaa_authenctication,
       aaa_show_aaa_authenctication_cmd,
       "show aaa authentication",
       SHOW_STR
       "Show authentication options\n"
       "Show aaa authentication information\n")
{
    return aaa_show_aaa_authenctication();
}

static int radius_server_add_host(const char *ipv4)
{
  const char  *passkey = RADIUS_SERVER_DEFAULT_PASSKEY;
  int64_t udp_port = 0, timeout = 0, retries = 0, i = 0, priority = 1;
  const struct ovsrec_radius_server *tempRow = NULL, **radius_info = NULL;
  struct ovsrec_radius_server *row = NULL;
  const struct ovsrec_open_vswitch *ovs = NULL;
  struct in_addr addr;
  struct ovsdb_idl_txn *status_txn = NULL;
  enum ovsdb_idl_txn_status txn_status;

  if (inet_pton(AF_INET, ipv4, &addr) <= 0) {
      VLOG_ERR("Invalid IPv4 address\n");
      cli_do_config_abort(status_txn);
      return CMD_ERR_NOTHING_TODO;
  }

  if (!IS_VALID_IPV4(htonl(addr.s_addr))) {
      VLOG_ERR("Broadcast, multicast and loopback addresses are not allowed\n");
      cli_do_config_abort(status_txn);
      return CMD_ERR_NOTHING_TODO;
  }

  udp_port = RADIUS_SERVER_DEFAULT_PORT;
  timeout = RADIUS_SERVER_DEFAULT_TIMEOUT;
  retries = RADIUS_SERVER_DEFAULT_RETRIES;

  status_txn = cli_do_config_start();

  if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(tempRow, idl)
  {
      if (!strcmp(tempRow->ip_address,ipv4)) {
          cli_do_config_abort(status_txn);
          status_txn = NULL;
          return CMD_SUCCESS;
      }
      retries = *(tempRow->retries);
      timeout = *(tempRow->timeout);
      priority += 1;
  }

  ovs = ovsrec_open_vswitch_first(idl);
  if (ovs == NULL) {
      assert(0);
      cli_do_config_abort(status_txn);
      status_txn = NULL;
      return CMD_OVSDB_FAILURE;
  }
  if (ovs->n_radius_servers == MAX_RADIUS_SERVERS) {
      vty_out(vty, "Exceeded maximum radius servers support%s",VTY_NEWLINE);
      cli_do_config_abort(status_txn);
      status_txn = NULL;
      return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_radius_server_insert(status_txn);

  ovsrec_radius_server_set_ip_address(row,ipv4);
  ovsrec_radius_server_set_passkey(row,passkey);
  ovsrec_radius_server_set_udp_port(row,&udp_port,1);
  ovsrec_radius_server_set_retries(row,&retries,1);
  ovsrec_radius_server_set_timeout(row,&timeout,1);
  ovsrec_radius_server_set_priority(row,priority);

  radius_info = xmalloc(sizeof *ovs->radius_servers * (ovs->n_radius_servers +1));
  for (i = 0; i < ovs->n_radius_servers; i++) {
      radius_info[i] = ovs->radius_servers[i];
  }
  radius_info[ovs->n_radius_servers] = row;
  ovsrec_open_vswitch_set_radius_servers(ovs, (struct ovsrec_radius_server**)radius_info, ovs->n_radius_servers + 1);
  free(radius_info);

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to add host */
DEFUN (cli_radius_server_add_host,
       radius_server_add_host_cmd,
       "radius-server host A.B.C.D",
       "Radius server configuration\n"
       "Host IP address\n"
       "Radius server IPv4 address\n")
{
    return radius_server_add_host(argv[0]);
}

static int radius_server_remove_host(const char *ipv4)
{
  int n = 0,i = 0;
  int64_t priority = 0;
  const struct ovsrec_radius_server *row = NULL, *tempRow = NULL;
  const struct ovsrec_radius_server **radius_info = NULL;
  const struct ovsrec_open_vswitch *ovs = NULL;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct in_addr addr;
  enum ovsdb_idl_txn_status txn_status;

  if (status_txn == NULL) {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  if (inet_pton(AF_INET, ipv4, &addr) <= 0) {
      VLOG_ERR("Invalid IPv4 address\n");
      cli_do_config_abort(status_txn);
      return CMD_ERR_NOTHING_TODO;
  }

  if (!IS_VALID_IPV4(htonl(addr.s_addr))) {
      VLOG_ERR("Broadcast, multicast and loopback addresses are not allowed\n");
      cli_do_config_abort(status_txn);
      return CMD_ERR_NOTHING_TODO;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, idl)
  {
      if (!strcmp(row->ip_address,ipv4)) {
          tempRow = row;
          break;
      }
  }

  if (!tempRow) {
      vty_out(vty, "No radius server configured with this IP%s",VTY_NEWLINE);
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }
  else {
      OVSREC_RADIUS_SERVER_FOR_EACH(row, idl) {
          if (tempRow->priority < row->priority) {
              priority = row->priority - 1;
              ovsrec_radius_server_set_priority(row, priority);
         }
     }
  }

  ovs = ovsrec_open_vswitch_first(idl);

  ovsrec_radius_server_delete(tempRow);
  radius_info = xmalloc(sizeof *ovs->radius_servers * ovs->n_radius_servers );

  for (i = n = 0; i < ovs->n_radius_servers; i++) {
      if (ovs->radius_servers[i] != tempRow) {
          radius_info[n++] = ovs->radius_servers[i];
      }
  }
  ovsrec_open_vswitch_set_radius_servers(ovs, (struct ovsrec_radius_server**)radius_info, n);
  free(radius_info);

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
       return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to remove radius server */
DEFUN (cli_radius_server_remove_host,
       radius_server_remove_host_cmd,
       "no radius-server host A.B.C.D",
       NO_STR
       "Radius server configuration\n"
       "Host IP address\n"
       "Radius server IPv4 address\n")
{
    return radius_server_remove_host(argv[0]);
}

static int radius_server_passkey_host(const char *ipv4, const char *passkey)
{
  const struct ovsrec_radius_server *row= NULL;
  int ret = 0;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = NULL;

  ret = radius_server_add_host(ipv4);
  if (CMD_SUCCESS != ret) {
      return ret;
  }

  status_txn = cli_do_config_start();

  if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, idl)
  {
      if (!strcmp(row->ip_address,ipv4)) {
          ovsrec_radius_server_set_passkey(row,passkey);
      }
  }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to set passkey */
DEFUN (cli_radius_server_passkey_host,
       radius_server_passkey_host_cmd,
       "radius-server host A.B.C.D key WORD",
       "Radius server configuration\n"
       "Host IP address\n"
       "Radius server IPv4 address\n"
       "Set shared secret\n"
       "Radius shared secret\n")
{
    return radius_server_passkey_host(argv[0],argv[1]);
}

static int radius_server_set_retries(const char  *retries)
{
  int64_t val = atoi(retries);
  const struct ovsrec_radius_server *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();

  if (status_txn == NULL)
  {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_radius_server_first(idl);

  if (!row) {
     vty_out(vty, "No radius servers configured %s",VTY_NEWLINE);
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, idl)
  {
      ovsrec_radius_server_set_retries(row,&val,1);
  }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_radius_server_retries,
       radius_server_retries_cmd,
       "radius-server retries <0-5>",
       "Radius server configuration\n"
       "Set the number of retries\n"
       "Set the range from 0 to 5. (Default: 1)\n")
{
  return radius_server_set_retries(argv[0]);
}


static int radius_server_set_timeout(const char *timeout)
{
  int64_t time_out = atoi(timeout);
  const struct ovsrec_radius_server *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();

  if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_radius_server_first(idl);
  if (!row) {
      vty_out(vty, "No radius servers configured%s",VTY_NEWLINE);
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, idl)
  {
      ovsrec_radius_server_set_timeout(row,&time_out,1);
  }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}


DEFUN (cli_radius_server_configure_timeout,
       radius_server_configure_timeout_cmd,
       "radius-server timeout <1-60>",
       "Radius server configuration\n"
       "Set the transmission timeout interval\n"
       "Timeout interval 1 to 60 seconds. (Default: 5)\n")
{
    return radius_server_set_timeout(argv[0]);
}


static int radius_server_set_auth_port(const char *ipv4, const char *port)
{
  int64_t udp_port = atoi(port);
  int ret = 0;
  const struct ovsrec_radius_server *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = NULL;

  ret = radius_server_add_host(ipv4);
  if (CMD_SUCCESS != ret) {
      return ret;
  }

  status_txn = cli_do_config_start();
  if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
  }

  OVSREC_RADIUS_SERVER_FOR_EACH(row, idl)
  {
      if (!strcmp(row->ip_address,ipv4)) {
          ovsrec_radius_server_set_udp_port(row,&udp_port,1);
      }
  }

  txn_status = cli_do_config_finish(status_txn);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

DEFUN (cli_radius_server_set_auth_port,
       radius_server_set_auth_port_cmd,
       "radius-server host A.B.C.D auth-port <0-65535>",
       "Radius server configuration\n"
       "Host IP address\n"
       "Radius server IPv4 address\n"
       "Set authentication port\n"
       "UDP port range is 0 to 65535. (Default: 1812)\n")
{
    return radius_server_set_auth_port(argv[0],argv[1]);
}

static int show_radius_server_info()
{
  const struct ovsrec_radius_server *row = NULL;
  int count = 1;
  char ip[1000], *pp,*udp,*timeout,*passkey;
  char file_name[]="/etc/raddb/server";
  FILE *fp;

  fp = fopen(file_name,"r");
  if (fp == NULL) {
      vty_out(vty,"Error while opening the file:%s",VTY_NEWLINE);
      fclose(fp);
      return CMD_SUCCESS;
  }

  row = ovsrec_radius_server_first(idl);
  if (row == NULL) {
      vty_out(vty, "No Radius Servers configured%s",VTY_NEWLINE);
      fclose(fp);
      return CMD_SUCCESS;
  }

  vty_out(vty, "***** Radius Server information ****** :%s",VTY_NEWLINE);
  while (fgets(ip,100 ,fp) != NULL) {
      vty_out(vty,"Radius-server:%d%s",count++,VTY_NEWLINE);
      pp=strtok(ip,":");
      udp=strtok(NULL," ");
      passkey=strtok(NULL," ");
      timeout=strtok(NULL, " ");
      vty_out(vty, " Host IP address\t: %s%s",pp,VTY_NEWLINE);
      vty_out(vty, " Shared secret\t\t: %s%s",passkey,VTY_NEWLINE);
      vty_out(vty, " Auth port\t\t: %s%s",udp,VTY_NEWLINE);
      vty_out(vty, " Retries\t\t: %ld%s",*(row->retries),VTY_NEWLINE);
      vty_out(vty, " Timeout\t\t: %s%s",timeout,VTY_NEWLINE);
  }

  fclose(fp);
  return CMD_SUCCESS;

}

DEFUN (cli_show_radius_server,
       show_radius_server_cmd,
       "show radius-server",
       SHOW_STR
       "Show radius server configuration\n")
{
    return show_radius_server_info();
}

static int show_auto_provisioning()
{
  const struct ovsrec_open_vswitch *row = NULL;

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_OVSDB_FAILURE;
  }

  if (smap_get(&row->auto_provisioning_status, "performed") != NULL) {
      if (!strcmp(smap_get(&row->auto_provisioning_status,"performed"), "True")) {
          vty_out(vty, " Performed : %s%s", "Yes", VTY_NEWLINE);
          vty_out(vty, " URL       : %s%s", smap_get(&row->auto_provisioning_status, "url"), VTY_NEWLINE);
      }
      else {
          vty_out(vty, " Performed : %s%s", "No", VTY_NEWLINE);
      }
  }

  return CMD_SUCCESS;
}

/* CLI to show auto provisioning status */
DEFUN (cli_show_auto_provisioning,
       show_auto_provisioning_cmd,
       "show autoprovisioning",
       SHOW_STR
       "Show auto provisioning status\n")
{
    return show_auto_provisioning();
}

static int show_ssh_auth_method()
{
  const struct ovsrec_open_vswitch *row = NULL;

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
      VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
      return CMD_OVSDB_FAILURE;
  }

  if (!strcmp(smap_get(&row->aaa,SSH_PUBLICKEY_AUTHENTICATION ),SSH_AUTH_ENABLE)){
      vty_out(vty, " SSH publickey authentication : %s%s", "Enabled", VTY_NEWLINE);
  }
  else {
      vty_out(vty, " SSH publickey authentication : %s%s", "Disabled", VTY_NEWLINE);
  }

  if (!strcmp(smap_get(&row->aaa,SSH_PASSWORD_AUTHENTICATION ),SSH_AUTH_ENABLE)){
      vty_out(vty, " SSH password authentication  : %s%s", "Enabled", VTY_NEWLINE);
  }
  else {
      vty_out(vty, " SSH password authentication  : %s%s", "Disabled", VTY_NEWLINE);
  }

  return CMD_SUCCESS;
}

/* CLI to show authentication mechanism configured in DB */
DEFUN (cli_show_ssh_auth_method,
       show_ssh_auth_method_cmd,
       "show ssh authentication-method",
       SHOW_STR
       "Show SSH configuration\n"
       "Show authentication method\n")
{
    return show_ssh_auth_method();
}

static int set_ssh_publickey_auth(const char *status)
{
  const struct ovsrec_open_vswitch *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_aaa;

  if (status_txn == NULL) {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_aaa, &row->aaa);

  if (strcmp(SSH_AUTH_ENABLE, status) == 0) {
      smap_replace(&smap_aaa, SSH_PUBLICKEY_AUTHENTICATION, SSH_AUTH_ENABLE);
  }
  else if (strcmp(SSH_AUTH_DISABLE, status) == 0) {
      smap_replace(&smap_aaa, SSH_PUBLICKEY_AUTHENTICATION, SSH_AUTH_DISABLE);
  }

  ovsrec_open_vswitch_set_aaa(row, &smap_aaa);

  txn_status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_aaa);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to enable ssh public key authentication */
DEFUN (cli_set_ssh_publickey_auth,
       set_ssh_publickey_auth_cmd,
       "ssh public-key-authentication",
       "SSH authentication\n"
       "Enable publickey authentication method\n")
{
    return set_ssh_publickey_auth(SSH_AUTH_ENABLE);
}

/* CLI to disable ssh public key authentication */
DEFUN (cli_no_set_ssh_publickey_auth,
       no_set_ssh_publickey_auth_cmd,
       "no ssh public-key-authentication",
       "SSH authentication\n"
       "Disable publickey authentication method\n")
{
    return set_ssh_publickey_auth(SSH_AUTH_DISABLE);
}


static int set_ssh_password_auth(const char *status)
{
  const struct ovsrec_open_vswitch *row = NULL;
  enum ovsdb_idl_txn_status txn_status;
  struct ovsdb_idl_txn *status_txn = cli_do_config_start();
  struct smap smap_aaa;

  if (status_txn == NULL) {
    VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
    cli_do_config_abort(status_txn);
    return CMD_OVSDB_FAILURE;
  }

  row = ovsrec_open_vswitch_first(idl);

  if (!row) {
     VLOG_ERR(OVSDB_ROW_FETCH_ERROR);
     cli_do_config_abort(status_txn);
     return CMD_OVSDB_FAILURE;
  }

  smap_clone(&smap_aaa, &row->aaa);

  if (strcmp(SSH_AUTH_ENABLE, status) == 0) {
      smap_replace(&smap_aaa, SSH_PASSWORD_AUTHENTICATION, SSH_AUTH_ENABLE);
  }
  else if (strcmp(SSH_AUTH_DISABLE, status) == 0) {
      smap_replace(&smap_aaa, SSH_PASSWORD_AUTHENTICATION, SSH_AUTH_DISABLE);
  }

  ovsrec_open_vswitch_set_aaa(row, &smap_aaa);

  txn_status = cli_do_config_finish(status_txn);
  smap_destroy(&smap_aaa);

  if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
      return CMD_SUCCESS;
  }
  else {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
  }
}

/* CLI to enable ssh password athentication */
DEFUN (cli_set_ssh_password_auth,
       set_ssh_password_auth_cmd,
       "ssh password-authentication",
       "SSH authentication\n"
       "Enable password authentication method\n")
{
    return set_ssh_password_auth(SSH_AUTH_ENABLE);
}

/* CLI to disable ssh password athentication */
DEFUN (cli_no_set_ssh_password_auth,
       no_set_ssh_password_auth_cmd,
       "no ssh password-authentication",
       "SSH authentication\n"
       "Disable password authentication method\n")
{
    return set_ssh_password_auth("disable");
}

/* Install AAA related vty commands. */
void
aaa_vty_init (void)
{
    install_element (ENABLE_NODE, &aaa_show_aaa_authenctication_cmd);
    install_element (CONFIG_NODE, &aaa_set_global_status_cmd);
    install_element (CONFIG_NODE, &aaa_remove_fallback_cmd);
    install_element (CONFIG_NODE, &aaa_no_remove_fallback_cmd);
    install_element (CONFIG_NODE, &radius_server_add_host_cmd);
    install_element (CONFIG_NODE, &radius_server_remove_host_cmd);
    install_element (CONFIG_NODE, &radius_server_passkey_host_cmd);
    install_element (CONFIG_NODE, &radius_server_retries_cmd);
    install_element (CONFIG_NODE, &radius_server_configure_timeout_cmd);
    install_element (CONFIG_NODE, &radius_server_set_auth_port_cmd);
    install_element (ENABLE_NODE, &show_radius_server_cmd);
    install_element (ENABLE_NODE, &show_auto_provisioning_cmd);
    install_element (ENABLE_NODE, &show_ssh_auth_method_cmd);
    install_element (CONFIG_NODE, &set_ssh_publickey_auth_cmd);
    install_element (CONFIG_NODE, &no_set_ssh_publickey_auth_cmd);
    install_element (CONFIG_NODE, &set_ssh_password_auth_cmd);
    install_element (CONFIG_NODE, &no_set_ssh_password_auth_cmd);
}
