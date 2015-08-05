/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
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
#include "openhalon-idl.h"
#include "openhalon-dflt.h"
#include "openvswitch/vlog.h"
#include "vtysh/vtysh_ovsdb_config.h"

/* vlan length + 1 */
#define VLAN_ID_LEN 5

/* max(strlen("ascending"), strlen("descending")) + 1 */
#define VLAN_POLICY_STR_LEN 11

#define INTERNAL_VLAN_ID_INVALID    -1

void vlan_vty_init(void);

#endif /* _VLAN_VTY_H */
