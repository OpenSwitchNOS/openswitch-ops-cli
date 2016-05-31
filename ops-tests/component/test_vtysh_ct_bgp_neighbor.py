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

from time import sleep

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
[type=openswitch name="OpenSwitch 2"] ops2

ops1:if01 -- ops2:if01
"""


peer_group = "peerGroupTest"

bgp1_asn = "1"
bgp1_router_id = "9.0.0.1"
bgp1_network = "11.0.0.0"

bgp2_asn = "2"
bgp2_router_id = "9.0.0.2"
bgp2_network = "12.0.0.0"

bgp1_neighbor = bgp2_router_id
bgp1_neighbor_asn = bgp2_asn

bgp2_neighbor = bgp1_router_id
bgp2_neighbor_asn = bgp1_asn

bgp_network_pl = "8"
bgp_network_mask = "255.0.0.0"
bgp_router_ids = [bgp1_router_id, bgp2_router_id]

bgp1_config = ["router bgp %s" % bgp1_asn,
               "bgp router-id %s" % bgp1_router_id,
               "network %s/%s" % (bgp1_network, bgp_network_pl),
               "neighbor %s remote-as %s" % (bgp1_neighbor, bgp1_neighbor_asn),
               "end"]

bgp2_config = ["router bgp %s" % bgp2_asn,
               "bgp router-id %s" % bgp2_router_id,
               "network %s/%s" % (bgp2_network, bgp_network_pl),
               "neighbor %s remote-as %s" % (bgp2_neighbor, bgp2_neighbor_asn),
               "end"]

bgp_configs = [bgp1_config, bgp2_config]

num_of_swtiches = 2
num_hosts_per_switch = 0

switch_prefix = "s"

list_in = "in"
list_out = "out"

dutarray = []


def wait_for_route(dut, network, nexthop, condition):
    result = {}
    flag = True
    for i in range(300):
        print("Attempt: %s" % str(i + 1))
        raw = dut.libs.vtysh.show_ip_bgp()

        for routes in raw:
            print(routes["network"] + "=" + network)
            print(routes["next_hop"] + "=" + nexthop)
            if (routes["network"] == network and
               routes["next_hop"] == nexthop):
                result["network"] = routes["network"]
                result["nexthop"] = routes["next_hop"]
                flag = False
                break

        if not flag:
            print("Route found...")
            break
        else:
            print("Route not found...")
            sleep(1)

    return result


def configure_switch_ips(step):
    step("Configuring switch IPs...")

    i = 0
    for switch in dutarray:
        # Configure the IPs between the switches
        switch("configure terminal")
        switch("interface 1")
        switch("no shutdown")
        switch("ip address %s/%s" % (bgp_router_ids[i], bgp_network_pl))
        switch("exit")
        i += 1


def verify_bgp_running(dut, step):
    step("Verifying bgp processes...")

    for switch in dutarray:
        pid = switch("pgrep -f bgpd", shell="bash").strip()
        assert (pid != "")


def configure_bgp(step):
    step("Applying BGP configurations...")

    i = 0
    for switch in dutarray:
        cfg_array = bgp_configs[i]
        i += 1
        for cmd in cfg_array:
            switch(cmd)


def verify_bgp_route_removed(dut, step, network, next_hop):
    step("Verifying route %s --> %s on switch %s removed..." %
         (network, next_hop, dut.name))

    verify_route_exists = False
    found = wait_for_route(dut, network, next_hop, verify_route_exists)

    assert not found


def verify_bgp_routes(step):
    step("Verifying routes...")

    verify_bgp_route(dutarray[0], step, bgp2_network + "/" + bgp_network_pl,
                     bgp2_router_id)

    verify_bgp_route(dutarray[1], step, bgp1_network + "/" + bgp_network_pl,
                     bgp1_router_id)


def verify_bgp_route(dut, step, network, next_hop):
    step("Checking for route: %s --> %s" % (network, next_hop))

    dut("end")
    found = wait_for_route(dut, network, next_hop, True)

    assert found


def unconfigure_neighbor_bgp(step):
    step("Unconfiguring neighbor for BGP1...")

    switch = dutarray[0]

    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s" % bgp1_neighbor)

    for cmd in cfg_array:
        switch(cmd)


def configure_neighbor_advertisement_interval(step):
    key = "advertisement-interval"
    step("Configuring %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    advertisement_interval = 50
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %d" % (bgp1_neighbor,
                     key, advertisement_interval))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_neighbor_advertisement_interval(step):
    key = "advertisement-interval"
    key2 = "advertisement_interval"
    step("Verifying %s for neighbor %s on switch 1... ##########\n" %
         (key, bgp1_neighbor))
    found = False
    advertisement_interval = 50
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(advertisement_interval) in line):
            found = True
    assert (found is True)

    found = False
    dump = switch("show ip bgp neighbors " + bgp1_neighbor)
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(advertisement_interval) in line):
            found = True
    assert (found is True)

    found = False
    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %d" % (bgp1_neighbor, key,
                                            advertisement_interval)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_neighbor_advertisement_interval(step):
    key = "advertisement-interval"
    step("Unconfiguring %s for neighbor %s on switch 1... ##########\n" %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    advertisement_interval = 50
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %d" % (bgp1_neighbor,
                                               key,
                                               advertisement_interval))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_advertisement_interval_alias(step):
    key = "advertisement-interval"
    step("Unconfiguring %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (bgp1_neighbor,
                                            key))
    cfg_array.append("cend")

    for cmd in cfg_array:
        switch(cmd)


def verify_no_neighbor_advertisement_interval(step):
    key = "advertisement-interval"
    key2 = "advertisement_interval"
    step("Verifying no %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))
    found = False
    advertisement_interval = 50
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(advertisement_interval) in line):
                    found = True
    assert (found is False)

    found = False
    dump = switch("show ip bgp neighbors " + bgp1_neighbor)
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(advertisement_interval) in line):
                    found = True
    assert (found is False)

    found = False
    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %d" % (bgp1_neighbor, key,
                                            advertisement_interval)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_ebgp_multihop(step):
    key = "ebgp-multihop"
    step("Configuring %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_neighbor_ebgp_multihop(step):
    key = "ebgp-multihop"
    key2 = "ebgp_multihop"
    step("Verifying %s for neighbor %s on switch 1..." %
         (key2, bgp1_neighbor))
    found = False
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    search_pattern = "%s: Enabled" % (key2)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)

    found = False
    show_cmd = "show ip bgp neighbors " + bgp1_neighbor
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "%s: Enabled" % (key2)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)

    found = False
    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s" % (bgp1_neighbor, key)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_neighbor_ebgp_multihop(step):
    key = "ebgp-multihop"
    step("Unconfiguring %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_no_neighbor_ebgp_multihop(step):
    key = "ebgp-multihop"
    key2 = "ebgp_multihop"
    step("Verifying no %s for neighbor %s on switch 1..." %
         (key2, bgp1_neighbor))
    found = False
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    search_pattern = "%s: Enabled" % (key2)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)

    found = False
    dump = switch("show ip bgp neighbors " + bgp1_neighbor)
    lines = dump.split('\n')
    search_pattern = "%s: Enabled" % (key2)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)

    found = False
    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s" % (bgp1_neighbor, key)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_ebgp_multihop_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Configuring %s for neighbor %s on switch 1 to test dependency on"
         " ttl-security..." % (key, bgp1_neighbor))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %d" % (bgp1_neighbor, key2,
                                            ttl_security))
    cfg_array.append("neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_ebgp_multihop_test_dependency(step):
    key2 = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1 to test dependency on"
         " ttl-security..." % (key2, bgp1_neighbor))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %d" % (bgp1_neighbor, key2,
                                               ttl_security))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def configure_neighbor_ebgp_multihop_peer_group_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Configuring %s for neighbor %s on switch 1 to test dependency on"
         " ttl-security..." % (key, peer_group))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %d" % (peer_group, key2,
                                            ttl_security))
    cfg_array.append("neighbor %s %s" % (peer_group, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_ebgp_multihop_peer_group_test_dependency(step):
    key2 = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1 to test dependency on"
         " ttl-security..." % (key2, peer_group))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %d" % (peer_group, key2,
                                               ttl_security))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def configure_neighbor_ebgp_multihop_peergroup(step):
    key = "ebgp-multihop"
    step("Configuring %s for peer group %s on switch 1..." % (key,
                                                              peer_group))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s peer-group" % peer_group)
    cfg_array.append("neighbor %s peer-group %s" % (bgp1_neighbor,
                                                    peer_group))
    cfg_array.append("neighbor %s %s" % (peer_group, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_neighbor_ebgp_multihop_peergroup(step):
    key = "ebgp-multihop"
    step("Verifying %s for peer group %s on switch 1..." % (key, peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s" % (peer_group, key)
    for line in lines:
        if search_pattern_neighbor in line:
            foundneighbor = True
        elif search_pattern_peer_group in line:
            foundpeergroup = True
        elif search_pattern_peer_group_cfg in line:
            foundpeergroupcfg = True

    assert (
        foundneighbor is True or foundpeergroup is True or
        foundpeergroupcfg is True
    )


def unconfigure_neighbor_ebgp_multihop_peergroup(step):
    key = "ebgp-multihop"
    step("Unconfiguring %s for peer group %s on switch 1..." % (key,
                                                                peer_group))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (peer_group, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_no_neighbor_ebgp_multihop_peergroup(step):
    key = "ebgp-multihop"
    step("Verifying no %s for peer group %s on switch 1..." %
         (key, peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s" % (peer_group, key)
    for line in lines:
        if search_pattern_neighbor in line:
            foundneighbor = True
        elif search_pattern_peer_group in line:
            foundpeergroup = True
        elif search_pattern_peer_group_cfg in line:
            foundpeergroupcfg = True

    assert (
        foundneighbor is True or foundpeergroup is True or
        foundpeergroupcfg is False
    )


def configure_neighbor_ttl_security_hops(step):
    key = "ttl-security hops"
    step("Configuring %s for neighbor %s on switch 1..." % (key,
                                                            bgp1_neighbor))

    switch = dutarray[0]
    ttl_security_hops = 123
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %d" % (bgp1_neighbor, key,
                                            ttl_security_hops))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_neighbor_ttl_security_hops(step):
    key = "ttl-security hops"
    key2 = "ttl_security hops"
    step("Verifying %s for neighbor %s on switch 1..." % (key2, bgp1_neighbor))
    found = False
    ttl_security_hops = 123
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(ttl_security_hops) in line):
            found = True
    assert (found is True)

    found = False
    dump = switch("show ip bgp neighbors " + bgp1_neighbor)
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(ttl_security_hops) in line):
            found = True
    assert (found is True)

    found = False
    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %d" % (bgp1_neighbor, key,
                                            ttl_security_hops)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_neighbor_ttl_security_hops(step):
    key = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1..." % (key,
                                                              bgp1_neighbor))

    switch = dutarray[0]
    ttl_security_hops = 123
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %d" % (bgp1_neighbor, key,
                                               ttl_security_hops))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_ttl_security_hops_alias(step):
    key = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1..." % (key,
                                                              bgp1_neighbor))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_no_neighbor_ttl_security_hops(step):
    key = "ttl-security hops"
    key2 = "ttl_security hops"
    step("Verifying no %s for neighbor %s on switch 1..." % (key2,
                                                             bgp1_neighbor))
    found = False
    ttl_security_hops = 123
    switch = dutarray[0]
    dump = switch("show ip bgp neighbors")
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(ttl_security_hops) in line):
            found = True
    assert (found is False)

    found = False
    show_cmd = "show ip bgp neighbors " + bgp1_neighbor
    dump = switch(show_cmd)
    lines = dump.split('\n')
    for line in lines:
        if (key2 in line and ":" in line and
           str(ttl_security_hops) in line):
            found = True
    assert (found is False)

    found = False
    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %d" % (bgp1_neighbor, key,
                                            ttl_security_hops)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_ttl_security_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Configuring %s for neighbor %s on switch 1 to test dependency on"
         " ebgp-multihop..." % (key, bgp1_neighbor))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("neighbor %s %s %d" % (bgp1_neighbor, key2,
                                            ttl_security))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_ttl_security_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1 to test dependency on "
         "ebgp-multihop..." % (key2, bgp1_neighbor))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (bgp1_neighbor, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def configure_neighbor_ttl_security_peer_group_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Configuring %s for neighbor %s on switch 1 to test dependency on "
         "ebgp-multihop..." % (key, peer_group))

    switch = dutarray[0]
    ttl_security = 250
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s" % (peer_group, key))
    cfg_array.append("neighbor %s %s %d" % (peer_group, key2,
                                            ttl_security))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def unconfigure_neighbor_ttl_security_peer_group_test_dependency(step):
    key = "ebgp-multihop"
    key2 = "ttl-security hops"
    step("Unconfiguring %s for neighbor %s on switch 1 to test dependency on "
         "ebgp-multihop..." % (key2, peer_group))

    switch = dutarray[0]
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (peer_group, key))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def configure_neighbor_ttl_security_hops_peergroup(step):
    key = "ttl-security hops"
    step("Configuring %s for peer group %s on switch 1..." % (key, peer_group))

    switch = dutarray[0]
    ttl_security_hops = 50
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s peer-group" % peer_group)
    cfg_array.append("neighbor %s peer-group %s" % (bgp1_neighbor,
                                                    peer_group))
    cfg_array.append("neighbor %s %s %d" % (peer_group, key,
                                            ttl_security_hops))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_neighbor_ttl_security_hops_peergroup(step):
    key = "ttl-security hops"
    step("Verifying %s for peer group %s on switch 1..." % (key, peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    ttl_security_hops = 50
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %d" % (peer_group, key,
                                                           ttl_security_hops)
    for line in lines:
        if search_pattern_neighbor in line:
            foundneighbor = True
        elif search_pattern_peer_group in line:
            foundpeergroup = True
        elif search_pattern_peer_group_cfg in line:
            foundpeergroupcfg = True

    assert (
        foundneighbor is True or foundpeergroup is True or
        foundpeergroupcfg is True
    )


def unconfigure_neighbor_ttl_security_hops_peergroup(step):
    key = "ttl-security hops"
    step("Unconfiguring %s for peer group %s on switch 1..." % (key,
                                                                peer_group))

    switch = dutarray[0]
    ttl_security_hops = 50
    cfg_array = []
    cfg_array.append("conf t")
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %d" % (peer_group, key,
                                               ttl_security_hops))
    cfg_array.append("end")

    for cmd in cfg_array:
        switch(cmd)


def verify_no_neighbor_ttl_security_hops_peergroup(step):
    key = "ttl-security hops"
    step("Verifying no %s for peer group %s on switch 1..." % (key,
                                                               peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    ttl_security_hops = 50
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %d" % (peer_group, key,
                                                           ttl_security_hops)
    for line in lines:
        if search_pattern_neighbor in line:
            foundneighbor = True
        elif search_pattern_peer_group in line:
            foundpeergroup = True
        elif search_pattern_peer_group_cfg in line:
            foundpeergroupcfg = True

    assert (
        foundneighbor is True or foundpeergroup is True or
        foundpeergroupcfg is False
    )


def test_vtysh_ct_bgp_neighbor(topology, step):
    ops1 = topology.get("ops1")
    ops2 = topology.get("ops2")
    assert ops1 is not None
    assert ops2 is not None

    global dutarray
    dutarray = [ops1, ops2]

    configure_switch_ips(step)
    verify_bgp_running(ops1, step)
    configure_bgp(step)
    verify_bgp_routes(step)

    configure_neighbor_advertisement_interval(step)
    verify_neighbor_advertisement_interval(step)
    unconfigure_neighbor_advertisement_interval(step)
    verify_no_neighbor_advertisement_interval(step)

    configure_neighbor_advertisement_interval(step)
    verify_neighbor_advertisement_interval(step)
    unconfigure_neighbor_advertisement_interval_alias(step)
    verify_no_neighbor_advertisement_interval(step)

    configure_neighbor_ebgp_multihop(step)
    verify_neighbor_ebgp_multihop(step)
    unconfigure_neighbor_ebgp_multihop(step)
    verify_no_neighbor_ebgp_multihop(step)
    configure_neighbor_ebgp_multihop_test_dependency(step)
    verify_no_neighbor_ebgp_multihop(step)
    unconfigure_neighbor_ebgp_multihop_test_dependency(step)

    configure_neighbor_ebgp_multihop_peergroup(step)
    verify_neighbor_ebgp_multihop_peergroup(step)
    unconfigure_neighbor_ebgp_multihop_peergroup(step)
    verify_no_neighbor_ebgp_multihop_peergroup(step)
    configure_neighbor_ebgp_multihop_peer_group_test_dependency(step)
    verify_no_neighbor_ebgp_multihop_peergroup(step)
    unconfigure_neighbor_ebgp_multihop_peer_group_test_dependency(step)

    configure_neighbor_ttl_security_hops(step)
    verify_neighbor_ttl_security_hops(step)
    unconfigure_neighbor_ttl_security_hops(step)
    verify_no_neighbor_ttl_security_hops(step)
    configure_neighbor_ttl_security_test_dependency(step)
    verify_no_neighbor_ttl_security_hops(step)
    unconfigure_neighbor_ttl_security_test_dependency(step)

    configure_neighbor_ttl_security_hops_peergroup(step)
    verify_neighbor_ttl_security_hops_peergroup(step)
    unconfigure_neighbor_ttl_security_hops_peergroup(step)
    verify_no_neighbor_ttl_security_hops_peergroup(step)
    configure_neighbor_ttl_security_peer_group_test_dependency(step)
    verify_no_neighbor_ttl_security_hops_peergroup(step)
    unconfigure_neighbor_ttl_security_peer_group_test_dependency(step)
