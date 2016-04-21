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
[type=openswitch name="OpenSwitch 2"] ops2

ops1:if01 -- ops2:if01
"""


dutarray = []


def add_bgp_ipv6_prefix_list_permit_prefix(step):
    step("Test to add ipv6 prefix-list permit prefix configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#1 seq 101 permit 2001:0DB8:0000::/48")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test#1 seq 101 permit "
           "2001:0DB8:0000::/48" in line):
            plist_created = True
    assert plist_created is True


def validate_show_ipv6_prefix_list(step):
    step("Test to validate show ipv6 prefix-list configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if ("ipv6 prefix-list test#1: 1 entries" in lines[i] and
           "seq 101 permit 2001:0DB8:0000::/48" in lines[i+1]):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_word(step):
    step("Test to validate show ipv6 prefix-list word configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list test#1")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if ("ipv6 prefix-list test#1: 1 entries" in lines[i] and
           "seq 101 permit 2001:0DB8:0000::/48" in lines[i+1]):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_seq(step):
    step("Test to validate show ipv6 prefix-list seq configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list test#1 seq 101")
    lines = dump.split('\n')
    for line in lines:
        if "seq 101 permit 2001:0DB8:0000::/48" in line:
            plist_created = True
    assert plist_created is True


def validate_show_ipv6_prefix_list_detail(step):
    step("Test to validate show ipv6 prefix-list detail configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list detail")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if (
            "ipv6 prefix-list test#1:" in lines[i] and
            "count: 1, sequences: 101 - 101" in lines[i+1] and
            "seq 101 permit 2001:0DB8:0000::/48" in lines[i+2]
        ):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_detail_word(step):
    step("Test to validate show ipv6 prefix-list detail word configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list detail test#1")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if (
            "ipv6 prefix-list test#1:" in lines[i] and
            "count: 1, sequences: 101 - 101" in lines[i+1] and
            "seq 101 permit 2001:0DB8:0000::/48" in lines[i+2]
        ):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_summary(step):
    step("Test to validate show ipv6 prefix-list summary configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list summary")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if ("ipv6 prefix-list test#1:" in lines[i] and
           "count: 1, sequences: 101 - 101" in lines[i+1]):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_summary_word(step):
    step("Test to validate show ipv6 prefix-list summary word configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list summary test#1")
    lines = dump.split('\n')
    i = 0
    for line in lines:
        if ("ipv6 prefix-list test#1:" in lines[i] and
           "count: 1, sequences: 101 - 101" in lines[i+1]):
            plist_created = True
        i = i + 1
    assert plist_created is True


def validate_show_ipv6_prefix_list_word_prefix(step):
    step("Test to validate show ipv6 prefix-list word prefix configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list test#1 2001:0DB8:0000::/48")
    lines = dump.split('\n')
    for line in lines:
        if "seq 101 permit 2001:0DB8:0000::/48" in line:
            plist_created = True
    assert plist_created is True


def delete_bgp_ipv6_prefix_list_permit_prefix(step):
    step("Test to delete ipv6 prefix-list permit prefix configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#1 seq 101 permit 2001:0DB8:0000::/48")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test#1 seq 101  permit "
           "2001:0DB8:0000::/48" in line):
            plist_deleted = False
    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_permit_prefix(step):
    step("Test to delete ipv6 prefix-list permit WORD configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#1")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test#1" in line:
            plist_deleted = False
    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_deny_prefix(step):
    step("Test to add ipv6 prefix-list deny prefix configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#2 seq 102 deny 2001:0DB8:0000::/48")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#2 seq 102 deny "
           "2001:0DB8:0000::/48" in line):
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_deny_prefix(step):
    step("Test to delete ipv6 prefix-list deny prefix configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#2 seq 102 deny 2001:0DB8:0000::/48")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#2 seq 102 deny"
           " 2001:0DB8:0000::/48" in line):
            plist_deleted = False

    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_deny_prefix(step):
    step("Test to delete ipv6 prefix-list permit WORD deny prefix "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#2")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if "ipv6 prefix-list test#2" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_permit_prefix_ge(step):
    step("Test to add ipv6 prefix-list permit prefix ge configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#3 seq 103 permit 2001:0DB8:0000::/48 ge 100")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if (
            "ipv6 prefix-list test#3 seq 103 permit"
            " 2001:0DB8:0000::/48 ge 100" in line
        ):
            plist_created = True
    assert plist_created is True


def delete_bgp_ipv6_prefix_list_permit_prefix_ge(step):
    step("Test to delete ipv6 prefix-list permit prefix ge configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#3 seq 103 permit 2001:0DB8:0000::/48 ge 100")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if (
            "ipv6 prefix-list test#3 seq 103 permit"
            " 2001:0DB8:0000::/48 ge 100" in line
        ):
            plist_deleted = False
    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_permit_ge(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix ge "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#3")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test#3" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_deny_prefix_ge(step):
    step("Test to add ipv6 prefix-list deny prefix ge configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#4 seq 104 deny 2001:0DB8:0000::/48 ge 110")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test#4 seq 104 deny"
           " 2001:0DB8:0000::/48 ge 110" in line):
            plist_created = True
    assert plist_created is True


def delete_bgp_ipv6_prefix_list_deny_prefix_ge(step):
    step("Test to delete ipv6 prefix-list deny prefix ge configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#4 seq 104 deny 2001:0DB8:0000::/48 ge 110")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if (
            "ipv6 prefix-list test#4 seq 104 deny"
            " 2001:0DB8:0000::/48 ge 110" in line
        ):
            plist_deleted = False

    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_deny_ge(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix ge 2"
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#4")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if "ipv6 prefix-list test#4" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_permit_prefix_le(step):
    step("Test to add ipv6 prefix-list permit prefix le configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#5 seq 105 permit 2001:0DB8:0000::/48 le 100")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#5 seq 105 permit"
           " 2001:0DB8:0000::/48 le 100" in line):
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_permit_prefix_le(step):
    step("Test to delete ipv6 prefix-list permit prefix le configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#5 seq 105 permit 2001:0DB8:0000::/48 le 100")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if (
            "ipv6 prefix-list test#5 seq 105 permit"
            " 2001:0DB8:0000::/48 le 100" in line
        ):
            plist_deleted = False

    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_permit_le(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix le "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#5")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if "ipv6 prefix-list test#5" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_deny_prefix_le(step):
    step("Test to add ipv6 prefix-list deny prefix le configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#6 seq 106 deny"
       " 2001:0DB8:0000::/48 le 110")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if (
            "ipv6 prefix-list test#6 seq 106 deny"
            " 2001:0DB8:0000::/48 le 110" in line
        ):
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_deny_prefix_le(step):
    step("Test to delete ipv6 prefix-list deny prefix le configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#6 seq 106 deny"
       " 2001:0DB8:0000::/48 le 110")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#6 seq 106 deny"
           " 2001:0DB8:0000::/48 le 110" in line):
            plist_deleted = False

    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_deny_le(step):
    step("Test to delete ipv6 prefix-list permit WORD deny prefix le "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#6")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if "ipv6 prefix-list test#6" in line:
            plist_deleted = False
    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_permit_prefix_ge_le(step):
    step("Test to add ipv6 prefix-list permit prefix ge le configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#5 seq 105 permit"
       " 2001:0DB8:0000::/48 ge 110 le 120")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#5 seq 105 permit 2001:0DB8:0000::/48 ge "
           "110 le 120" in line):
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_permit_prefix_ge_le(step):
    step("Test to delete ipv6 prefix-list permit prefix ge le configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#5 seq 105 permit "
       "2001:0DB8:0000::/48 ge 110 le 120")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if ("ipv6 prefix-list test#5 seq 105 permit 2001:0DB8:0000::/48 ge "
           "110 le 120" in line):
            plist_deleted = False

    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_permit_ge_le(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix ge le "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#5")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')

    for line in lines:
        if "ipv6 prefix-list test#5" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_deny_prefix_ge_le(step):
    step("Test to add ipv6 prefix-list deny prefix ge le configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#6 seq 106 deny 2001:0DB8:0000::/48 ge 111 le "
       "121")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test#6 seq 106 deny 2001:0DB8:0000::/48"
           " ge 111 le 121" in line):
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_deny_prefix_ge_le(step):
    step("Test to delete ipv6 prefix-list deny prefix ge le configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#6 seq 106 deny 2001:0DB8:0000::/48 ge 111 le"
       " 121")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test#6 seq 106 deny"
           " 2001:0DB8:0000::/48 ge 111 le 121" in line):
            plist_deleted = False
    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_deny_ge_le(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix ge le"
         " configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test#6")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test#6" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_permit_prefix_any(step):
    step("Test to add ipv6 prefix-list permit prefix any configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test seq 105 permit any")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if ("ipv6 prefix-list test seq 105 permit any" in line):
            plist_created = True
    assert plist_created is True


def delete_bgp_ipv6_prefix_list_permit_prefix_any(step):
    step("Test to delete ipv6 prefix-list permit prefix any configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test seq 105 permit any")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test seq 105 permit any" in line:
            plist_deleted = False
    assert plist_deleted is True


def delete_bgp_ipv6_prefix_list_word_permit_any(step):
    step("Test to delete ipv6 prefix-list permit WORD permit prefix any "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test" in line:
            plist_deleted = False

    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_deny_prefix_any(step):
    step("Test to add ipv6 prefix-list deny prefix any configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test_delete seq 105 deny any")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test_delete seq 105 deny any" in line:
            plist_created = True
    assert plist_created is True


def delete_bgp_ipv6_prefix_list_deny_prefix_any(step):
    step("Test to delete ipv6 prefix-list deny prefix any configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test_delete seq 105 deny any")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list delete seq 105 deny any" in line:
            plist_deleted = False
    assert plist_deleted is True


def add_bgp_ipv6_prefix_list_line(step):
    step("Test to add ipv6 prefix-list WORD description line configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test description regular expression")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test description regular expression" in line:
            plist_created = True

    assert plist_created is True


def delete_bgp_ipv6_prefix_list_line(step):
    step("Test to delete ipv6 prefix-list WORD description line "
         "configurations")
    plist_deleted = True
    s1 = dutarray[0]
    s1("configure terminal")
    s1("no ipv6 prefix-list test description regular expression")
    s1("end")
    dump = s1("show running-config")
    lines = dump.split('\n')
    for line in lines:
        if "ipv6 prefix-list test description regular expression" in line:
            plist_deleted = False

    assert plist_deleted is True


def validate_show_ipv6_prefix_list_word_prefix_first_match(step):
    step("Test to validate show ipv6 prefix-list word prefix first match "
         "configurations")
    plist_created = False
    s1 = dutarray[0]
    s1("configure terminal")
    s1("ipv6 prefix-list test#1 seq 101 permit 2001:0DB8:0000::/48")
    s1("ipv6 prefix-list test#1 seq 102 permit 2001:0DB8:0000::/48 ge 50 le"
       " 60")
    s1("ipv6 prefix-list test#1 seq 103 permit 2001:0DB8:0000::/50")
    s1("ipv6 prefix-list test#1 seq 104 permit 2001:0DB8:0000::/47")
    s1("end")
    dump = s1("show ipv6 prefix-list test#1 2001:0DB8:0000::/48 "
              "first-match")
    lines = dump.split('\n')
    for line in lines:
        if "seq 101 permit 2001:0DB8:0000::/48 ge 50 le 60" not in line:
            plist_created = True
    assert plist_created is True


def validate_show_ipv6_prefix_list_word_prefix_longer(step):
    step("Test to validate show ipv6 prefix-list word prefix longer "
         "configurations")
    plist_created = False
    s1 = dutarray[0]
    dump = s1("show ipv6 prefix-list test#1 2001:0DB8:0000::/48"
              " longer")
    lines = dump.split('\n')
    for line in lines:
        if "seq 101 permit 2001:0DB8:0000::/47" not in line:
            plist_created = True
    assert plist_created is True

@mark.skipif(True, reason="Disabling due to gate job failures")
def test_vtysh_ct_bgp_ipv6_prefix_cli(topology, step):
    ops1 = topology.get("ops1")
    assert ops1 is not None

    global dutarray
    dutarray = [ops1]

    add_bgp_ipv6_prefix_list_permit_prefix(step)
    validate_show_ipv6_prefix_list(step)
    validate_show_ipv6_prefix_list_word(step)
    validate_show_ipv6_prefix_list_seq(step)
    validate_show_ipv6_prefix_list_seq(step)
    validate_show_ipv6_prefix_list_detail(step)
    validate_show_ipv6_prefix_list_detail_word(step)
    validate_show_ipv6_prefix_list_summary(step)
    validate_show_ipv6_prefix_list_summary_word(step)
    validate_show_ipv6_prefix_list_word_prefix(step)

    delete_bgp_ipv6_prefix_list_permit_prefix(step)
    delete_bgp_ipv6_prefix_list_word_permit_prefix(step)
    add_bgp_ipv6_prefix_list_deny_prefix(step)
    delete_bgp_ipv6_prefix_list_deny_prefix(step)
    delete_bgp_ipv6_prefix_list_word_deny_prefix(step)
    add_bgp_ipv6_prefix_list_permit_prefix_ge(step)
    delete_bgp_ipv6_prefix_list_permit_prefix_ge(step)
    delete_bgp_ipv6_prefix_list_word_permit_ge(step)
    add_bgp_ipv6_prefix_list_deny_prefix_ge(step)
    delete_bgp_ipv6_prefix_list_deny_prefix_ge(step)

    delete_bgp_ipv6_prefix_list_word_deny_ge(step)
    add_bgp_ipv6_prefix_list_permit_prefix_le(step)
    delete_bgp_ipv6_prefix_list_permit_prefix_le(step)
    delete_bgp_ipv6_prefix_list_word_permit_le(step)
    add_bgp_ipv6_prefix_list_deny_prefix_le(step)
    delete_bgp_ipv6_prefix_list_deny_prefix_le(step)
    delete_bgp_ipv6_prefix_list_word_deny_le(step)
    add_bgp_ipv6_prefix_list_permit_prefix_ge_le(step)
    delete_bgp_ipv6_prefix_list_permit_prefix_ge_le(step)
    delete_bgp_ipv6_prefix_list_word_permit_ge_le(step)

    add_bgp_ipv6_prefix_list_deny_prefix_ge_le(step)
    delete_bgp_ipv6_prefix_list_deny_prefix_ge_le(step)
    delete_bgp_ipv6_prefix_list_word_deny_ge_le(step)
    add_bgp_ipv6_prefix_list_permit_prefix_any(step)
    delete_bgp_ipv6_prefix_list_permit_prefix_any(step)
    delete_bgp_ipv6_prefix_list_word_permit_any(step)
    add_bgp_ipv6_prefix_list_deny_prefix_any(step)
    delete_bgp_ipv6_prefix_list_deny_prefix_any(step)
    add_bgp_ipv6_prefix_list_line(step)
    delete_bgp_ipv6_prefix_list_line(step)

    validate_show_ipv6_prefix_list_word_prefix_first_match(step)
    validate_show_ipv6_prefix_list_word_prefix_longer(step)
