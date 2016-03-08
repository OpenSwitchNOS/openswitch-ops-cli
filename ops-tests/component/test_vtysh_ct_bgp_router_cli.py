# -*- coding: utf-8 -*-
# (C) Copyright 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
##########################################################################

"""
OpenSwitch Test for switchd related configurations.
"""

# from pytest import set_trace
# from time import sleep

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


def verify_bgp_router_table(dut, step):
    step("Test to verify BGP router table")

    out = dut("show ip bgp summary")
    assert "No bgp router configured." in out


def configure_bgp_router_flags(dut, step):
    step("Test to configure BGP router flags")

    fast_ext_failover_str = "bgp fast-external-failover"
    fast_ext_failover_flag = False
    log_neighbor_changes_str = "bgp log-neighbor-changes"
    log_neighbor_changes_flag = False

    dut("configure terminal")
    dut("router bgp 100")
    dut(fast_ext_failover_str)
    dut(log_neighbor_changes_str)
    dut("end")

    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if fast_ext_failover_str in line:
            fast_ext_failover_flag = True
        elif log_neighbor_changes_str in line:
            log_neighbor_changes_flag = True

    if fast_ext_failover_flag is False:
        print("###  BGP fast-external-failover flag not set ###")
    elif log_neighbor_changes_flag is False:
        print("###  BGP log-neighbor-changes flag not set ###")

    if (
        fast_ext_failover_flag is False or log_neighbor_changes_flag is False
    ):
        print("### Test to set BGP Router flags-FAILED! ###")


def unconfigure_bgp_router_flags(dut, step):
    step("Test to unconfigure BGP router flags")

    fast_ext_failover_str = "bgp fast-external-failover"
    no_fast_ext_failover_str = "no bgp fast-external-failover"
    fast_ext_failover_flag = False
    log_neighbor_changes_str = "bgp log-neighbor-changes"
    no_log_neighbor_changes_str = "no bgp log-neighbor-changes"
    log_neighbor_changes_flag = False

    dut("configure terminal")
    dut("router bgp 100")
    dut(no_fast_ext_failover_str)
    dut(no_log_neighbor_changes_str)
    dut("end")

    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if fast_ext_failover_str in line:
            fast_ext_failover_flag = True
        elif log_neighbor_changes_str in line:
            log_neighbor_changes_flag = True

    if fast_ext_failover_flag is True:
        print("###  BGP fast-external-failover flag is set ###")
    elif log_neighbor_changes_flag is True:
        print("###  BGP log-neighbor-changes flag is set ###")

    if (
        fast_ext_failover_flag is True or log_neighbor_changes_flag is True
    ):
        print("### Test to unconfigure BGP Router flags-FAILED! ###")


def configure_bgp_network(dut, step):
    step("Test to configure BGP network")

    network_str = "network 3001::/32"
    network_str_flag = False

    dut("configure terminal")
    dut("router bgp 100")
    dut("network 3001::1/32")
    dut("end")

    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if network_str in line:
            network_str_flag = True

    assert network_str_flag is True


def unconfigure_bgp_network(dut, step):
    step("Test to unconfigure BGP network")

    network_str = "network 3001::/32"
    network_str_flag = False

    dut("configure terminal")
    dut("router bgp 100")
    dut("no network 3001::1/32")
    dut("end")

    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if network_str in line:
            network_str_flag = True

    assert network_str_flag is False


def configure_routemap_match(dut, step):
    step("Test to configure Route-Map Match commands")

    match_ipv6_prefix_list_str = "match ipv6 address prefix-list 5"
    match_ipv6_prefix_list_flag = False
    match_community_str = "match community 100"
    match_community_str_flag = False
    match_extcommunity_str = "match extcommunity e1"
    match_extcommunity_str_flag = False

    dut("configure terminal")
    dut("route-map r1 permit 10")
    dut(match_ipv6_prefix_list_str)
    dut(match_community_str)
    dut(match_extcommunity_str)
    dut("end")

    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if match_ipv6_prefix_list_str in line:
            match_ipv6_prefix_list_flag = True
        elif match_community_str in line:
            match_community_str_flag = True
        elif match_extcommunity_str in line:
            match_extcommunity_str_flag = True

    if match_ipv6_prefix_list_flag is False:
        print("###  Error configuring 'match ipv6 address prefix-list' ###")
    elif match_community_str_flag is False:
        print("###  Error configuring 'match community' ###\n")
    elif match_extcommunity_str_flag is False:
        print("###  Error configuring 'match extcommunity' ###\n")

    if match_ipv6_prefix_list_flag is False or \
       match_community_str_flag is False or \
       match_extcommunity_str_flag is False:
        print("### Test to configure Route-Map match commands FAILED! ###")


def unconfigure_routemap_match(dut, step):
    step("Test to unconfigure Route-Map Match commands")

    match_ipv6_prefix_list_str = "match ipv6 address prefix-list 5"
    no_match_ipv6_prefix_list_str = "no match ipv6 address prefix-list 5"
    match_ipv6_prefix_list_flag = False
    match_community_str = "match community 100"
    no_match_community_str = "no match community 100"
    match_community_str_flag = False
    match_extcommunity_str = "match extcommunity e1"
    no_match_extcommunity_str = "no match extcommunity e1"
    match_extcommunity_str_flag = False

    dut("configure terminal")
    dut("route-map r1 permit 10")
    dut(no_match_ipv6_prefix_list_str)
    dut(no_match_community_str)
    dut(no_match_extcommunity_str)
    dut("end")

    dump = dut("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if match_ipv6_prefix_list_str in line:
            match_ipv6_prefix_list_flag = True
        elif match_community_str in line:
            match_community_str_flag = True
        elif match_extcommunity_str in line:
            match_extcommunity_str_flag = True

    if match_ipv6_prefix_list_flag is True:
        print("###  Error unconfiguring 'match ipv6 address prefix-list' ###")
    elif match_community_str_flag is True:
        print("###  Error unconfiguring 'match community' ###")
    elif match_extcommunity_str_flag is True:
        print("###  Error unconfiguring 'match extcommunity' ###")

    if (
        match_ipv6_prefix_list_flag is True or
        match_community_str_flag is True or
        match_extcommunity_str_flag is True
    ):
        print("### Test to unconfigure Route-Map match commands FAILED! ###")


def test_vtysh_ct_bgp_router_cli(topology, step):
    ops1 = topology.get("ops1")
    assert ops1 is not None

    verify_bgp_router_table(ops1, step)
    configure_bgp_router_flags(ops1, step)
    unconfigure_bgp_router_flags(ops1, step)
    configure_bgp_network(ops1, step)
    unconfigure_bgp_network(ops1, step)
    configure_routemap_match(ops1, step)
    unconfigure_routemap_match(ops1, step)
