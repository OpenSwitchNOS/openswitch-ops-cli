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
 * @ingroup quagga/vtysh
 *
 * @file vrf_vty.h
 * To add declarations required for vrf_vty.c
 *
 ***************************************************************************/

#ifndef _VRF_VTY_H
#define _VRF_VTY_H

#define VRF_NAME_MAX_LENGTH 32
#define IP_ADDRESS_LENGTH 18
#define IPV6_ADDRESS_LENGTH 49

void
vrf_vty_init(void);

const struct ovsrec_port* port_check(const char *port_name, bool create,
                               bool attach_to_default_vrf,
                               struct ovsdb_idl_txn *txn);

const struct ovsrec_vrf* port_vrf_lookup(const struct ovsrec_port *port_row);

const struct ovsrec_vrf* vrf_lookup(const char *vrf_name);

#endif /* _VRF_VTY_H */
