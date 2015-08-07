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
 * @ingroup cli
 *
 * @file vtysh_ovsdb_config_context.h
 * Source for registering client callback with config context.
 *
 ***************************************************************************/

#ifndef VTYSH_OVSDB_CONFIG_CONTEXT_H
#define VTYSH_OVSDB_CONFIG_CONTEXT_H

int vtysh_init_config_context_clients();
vtysh_ret_val vtysh_config_context_global_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_vrf_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_fan_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_led_clientcallback(void *p_private);
vtysh_ret_val vtysh_config_context_staticroute_clientcallback(void *p_private);

#endif /* VTYSH_OVSDB_CONFIG_CONTEXT_H */
