/*
 * Copyright (C) 1997 Kunihiro Ishiguro
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
/****************************************************************************
 * @ingroup  quagga
 *
 * @file vtysh_ovsdb_config.h
 * Source for config infra to walkthrough the ovsdb tables.
 *
 ***************************************************************************/

#ifndef VTYSH_OVSDB_CONFIG_H
#define VTYSH_OVSDB_CONFIG_H

#include "vty.h"
#include <stdbool.h>

/* general vtysh return type */
typedef enum vtysh_ret_val_enum
{
  e_vtysh_error = -1,
  e_vtysh_ok = 0
} vtysh_ret_val;

/* vtysh ovsdb table_id type */
typedef enum vtysh_context_idenum
{
  e_vtysh_config_context = 0,
  e_vtysh_router_context,
  e_vtysh_vlan_context,
  e_vtysh_interface_lag_context,
  e_vtysh_interface_context,
  e_vtysh_mgmt_interface_context,
  e_vtysh_dependent_config,
  e_vtysh_source_interface_context,
  e_vtysh_dhcp_tftp_context,
  e_vtysh_sftp_server_context,
  e_vtysh_dhcp_relay_context,
  e_vtysh_udp_forwarder_context,
  e_vtysh_qos_apply_global_context,
  e_vtysh_qos_trust_global_context,
  e_vtysh_qos_cos_map_context,
  e_vtysh_qos_dscp_map_context,
  e_vtysh_access_list_context,
} vtysh_contextid;

/* Config Context Client ID type */
typedef enum vtysh_config_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_config_context_global = 0,
  e_vtysh_config_context_vrf,
  e_vtysh_config_context_sflow,
  e_vtysh_config_context_fan,
  e_vtysh_config_context_led,
  e_vtysh_config_context_staticroute,
  e_vtysh_config_context_ecmp,
  e_vtysh_config_context_ntp,
  e_vtysh_config_context_mstp,
  e_vtysh_config_context_loop_protect,
  e_vtysh_config_context_syslog,
  e_vtysh_config_context_snmp,
  e_vtysh_config_context_access_list,
} vtysh_config_context_clientid;

/* Router Context Client ID type */
typedef enum vtysh_router_context_client_idenum
{
  e_vtysh_router_context_bgp_ip_prefix = 0,
  e_vtysh_router_context_bgp_ip_community_filter,
  e_vtysh_router_context_bgp_ip_filter_list,
  e_vtysh_router_context_bgp_routemap,
  e_vtysh_router_context_bgp,
  e_vtysh_router_context_ospf,
} vtysh_router_context_clientid;

/* Interface Context client-id type */
typedef enum vtysh_interface_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_interface_context_config = 0,
  e_vtysh_interface_context_lldp,
  e_vtysh_interface_context_lacp,
  e_vtysh_interface_context_lag,
  e_vtysh_interface_context_vlan,
  e_vtysh_interface_context_vrf,
  e_vtysh_interface_context_mstp,
  e_vtysh_interface_context_loop_protect,
  e_vtysh_interface_context_ospf,
  e_vtysh_interface_context_access_list,
  e_vtysh_intf_context_client_id_max
} vtysh_interface_context_clientid;

/* Vlan Context client-id type */
typedef enum vtysh_vlan_context_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_vlan_context_client_id_first = 0,
  e_vtysh_vlan_context_config,
  e_vtysh_vlan_context_access_list,
  e_vtysh_vlan_context_client_id_max
} vtysh_vlan_context_clientid;

/* Dependent Config Client ID type */
typedef enum vtysh_dependent_config_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_dependent_config_staticroute,
} vtysh_dependent_config_clientid;

/* Dependent Config Client ID type */
typedef enum vtysh_dhcp_tftp_config_client_idenum
{
  /* client callback based on client-id value */
  e_vtysh_dhcp_tftp_context_dhcp,
  e_vtysh_dhcp_tftp_context_tftp,
} vtysh_dhcp_tftp_context_clientid;

typedef struct vtysh_ovsdb_cbmsg_struct
{
  FILE *fp;
  struct ovsdb_idl *idl;
  vtysh_contextid contextid;
  int clientid;
  bool disp_header_cfg;
  void *feature_row;
  bool skip_subcontext_list;
}vtysh_ovsdb_cbmsg;

typedef struct vtysh_ovsdb_cbmsg_struct *vtysh_ovsdb_cbmsg_ptr;

#define VTYSH_STR_EQ(s1, s2)      ((strlen((s1)) == strlen((s2))) && (!strncmp((s1), (s2), strlen((s2)))))

#define VTYSH_OVSDB_CONFIG_ERR 1
#define VTYSH_OVSDB_CONFIG_WARN 2
#define VTYSH_OVSDB_CONFIG_INFO 3
#define VTYSH_OVSDB_CONFIG_DBG 4

extern struct ovsdb_idl *vtysh_ovsdb_idl;
extern unsigned int vtysh_ovsdb_idl_seqno;

vtysh_ret_val vtysh_sh_run_iteratecontextlist(FILE *fp);

vtysh_ret_val vtysh_ovsdb_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...);

/* All log/debug/err logging functions */
void vtysh_ovsdb_config_logmsg(int loglevel, char *fmt,  ...);

struct ovsdb_idl_txn * cli_do_config_start(void);

enum ovsdb_idl_txn_status cli_do_config_finish(struct ovsdb_idl_txn* txn);

void cli_do_config_abort(struct ovsdb_idl_txn* txn);

struct vtysh_context_feature_row_list {
    void * row;
    struct vtysh_context_feature_row_list *next;
};
typedef struct vtysh_context_feature_row_list feature_row_list;

struct feature_sorted_list {
    const struct shash_node **nodes;
    int count;
};

struct vtysh_contextlist_struct {
    /* Enum value of the context */
    int index;
    /* Context callback function */
    vtysh_ret_val (*vtysh_context_callback) (void* p_private);
    /* Init callback function to be called before vtysh_context_callback */
    struct feature_sorted_list * (*context_callback_init) (void* p_private);
    /* Exit callback function to be called after vtysh_context_callback */
    void (*context_callback_exit) (struct feature_sorted_list * row_list);
    /* Sub-context list for running-config context */
    struct vtysh_contextlist_struct * subcontext_list;
    /* Pointer to next context callback node */
    struct vtysh_contextlist_struct * next;
};

typedef struct vtysh_contextlist_struct vtysh_contextlist;

vtysh_ret_val install_show_run_config_context(vtysh_contextid index,
                          vtysh_ret_val (*funcptr) (void* p_private),
                          struct feature_sorted_list * (*init_funcptr) (void* p_private),
                          void (*exit_funcptr) (struct feature_sorted_list * head));
vtysh_ret_val install_show_run_config_subcontext(vtysh_contextid index,
                          vtysh_contextid subcontext_index,
                          vtysh_ret_val (*funcptr) (void* p_private),
                          struct feature_sorted_list * (*init_funcptr) (void* p_private),
                          void (*exit_funcptr) (struct feature_sorted_list * head));
#endif /* VTYSH_OVSDB_CONFIG_H */
