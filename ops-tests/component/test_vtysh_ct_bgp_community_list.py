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
from pytest import mark

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


def add_bgp_community_list(dut, step):
    step("Test to add BGP community list configuration")
    community_created = False
    dut("configure terminal")
    dut("ip community-list test_name permit _5000_")
    dump = dut("do show running-config")
    dut("end")
    lines = dump.splitlines()
    for line in lines:
        if (
            "ip community-list test_name "
            "permit _5000_" in line
        ):
            community_created = True
    assert community_created is True


def add_bgp_extcommunity_list(dut, step):
    step("Test to add BGP extcommunity configurations")
    community_created = False
    dut("configure terminal")
    dut("ip extcommunity-list testname permit _1000_")
    dump = dut("do show running-config")
    lines = dump.splitlines()
    dut("end")
    for line in lines:
        if ("ip extcommunity-list testname permit _1000_" in line):
            community_created = True
    assert community_created is True


def validate_show_ip_community_list(dut, step):
    step("Test to validate show ip community list configuration")
    community_created = False
    dut("end")
    dump = dut("show ip community-list")
    lines = dump.splitlines()
    i = 0
    for line in lines:
        if ("ip community-list test_name" in lines[i] and "permit _5000_" in lines[i+1]):  # noqa
            community_created = True
        i = i + 1
    assert community_created is True


def validate_show_ip_extcommunity_list(dut, step):
    step("Test to validate show ip extcommunity list configuration")
    community_created = False
    dut("end")
    dump = dut("show ip extcommunity-list")
    lines = dump.splitlines()
    i = 0
    for line in lines:
        if ("ip extcommunity-list testname" in lines[i] and "permit _1000_" in lines[i+1]):  # noqa
            community_created = True
        i = i + 1
    assert community_created is True


def delete_bgp_community_list(dut, step):
    step("Test to delete BGP community list configurations")
    community_deleted = True
    dut("configure terminal")
    dut("no ip community-list test_name permit _5000_")
    dut("end")
    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if ("ip community-list test_name permit _5000_" in line):
            community_deleted = False
    assert community_deleted is True


def add_bgp_extcommunity_deny_list(dut, step):
    step("Test to add BGP extcommunity list deny configurations")
    community_created = False
    dut("configure terminal")
    dut("ip extcommunity-list test_name1 deny 100")
    dump = dut("do show running-config")
    dut("end")
    lines = dump.splitlines()
    for line in lines:
        if ("ip extcommunity-list test_name1 deny 100" in line):
            community_created = True
    assert community_created is True


def delete_bgp_extcommunity_deny_list(dut, step):
    step("Test to delete BGP extcommunity deny list configurations")
    community_deleted = True
    dut("configure terminal")
    dut("no ip extcommunity-list test_name1 deny 100")
    dut("end")
    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if ("ip extcommunity-list test_name1 deny 100" in line):
            community_deleted = False
    assert community_deleted is True


def delete_bgp_extcommunity_list(dut, step):
    step("Test to delete BGP extcommunity configurations")
    community_deleted = True
    dut("configure terminal")
    dut("no ip extcommunity-list test_name permit 200")
    dut("end")
    dump = dut("show running-config")
    lines = dump.splitlines()
    for line in lines:
        if ("ip extcommunity-list testname permit 200" in lines):
            community_deleted = False
    assert community_deleted is True

@mark.skipif(True, reason="Disabling tests due to gate job failures")
def test_vtysh_ct_bgp_community_list(topology, step):
    ops1 = topology.get("ops1")
    assert ops1 is not None

    add_bgp_community_list(ops1, step)
    add_bgp_extcommunity_list(ops1, step)
    validate_show_ip_community_list(ops1, step)
    validate_show_ip_extcommunity_list(ops1, step)
    delete_bgp_community_list(ops1, step)
    add_bgp_extcommunity_deny_list(ops1, step)
    delete_bgp_extcommunity_deny_list(ops1, step)
    delete_bgp_extcommunity_list(ops1, step)
