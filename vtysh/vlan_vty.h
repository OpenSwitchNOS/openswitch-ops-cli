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
 */
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file vlan_vty.h
 *
 ***************************************************************************/

#ifndef _VLAN_VTY_H
#define _VLAN_VTY_H

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "openswitch-idl.h"
#include "openswitch-dflt.h"
#include "openvswitch/vlog.h"
#include "vtysh/vtysh_ovsdb_config.h"

/* vlan length + 1 */
#define VLAN_ID_LEN 5

/* max(strlen("ascending"), strlen("descending")) + 1 */
#define VLAN_POLICY_STR_LEN 11

#define INTERNAL_VLAN_ID_INVALID    -1

#define DEFAULT_VLAN    1

#define OVSDB_VLAN_SHUTDOWN_ERROR "Failed to shutdown VLAN%s"
#define OVSDB_VLAN_NO_SHUTDOWN_ERROR "Failed to enable VLAN%s"
#define OVSDB_INTF_VLAN_ACCESS_ERROR "Failed to set access VLAN %d%s"
#define OVSDB_INTF_VLAN_REMOVE_ACCESS_ERROR "Failed to remove access VLAN%s"
#define OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR "Failed to set allowed trunk VLAN %d%s"
#define OVSDB_INTF_VLAN_REMOVE_TRUNK_ALLOWED_ERROR "Failed to remove allowed trunk VLAN %d%s"
#define OVSDB_INTF_VLAN_TRUNK_NATIVE_ERROR "Failed to set native VLAN %d%s"
#define OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_ERROR "Failed to remove native VLAN%s"
#define OVSDB_INTF_VLAN_TRUNK_NATIVE_TAG_ERROR "Failed to set native VLAN tagging on the interface%s"
#define OVSDB_INTF_VLAN_REMOVE_TRUNK_NATIVE_TAG_ERROR "Failed to remove native VLAN tagging on the interface%s"

void vlan_vty_init(void);
extern int
compare_nodes_by_vlan_id_in_numerical(const void *a_, const void *b_);
extern const struct shash_node **
sort_vlan_id(const struct shash *sh);

#endif /* _VLAN_VTY_H */
