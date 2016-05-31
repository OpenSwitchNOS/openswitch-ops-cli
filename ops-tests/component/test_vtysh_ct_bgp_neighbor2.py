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

bgp1_config = ["config t",
               "router bgp %s" % bgp1_asn,
               "bgp router-id %s" % bgp1_router_id,
               "network %s/%s" % (bgp1_network, bgp_network_pl),
               "neighbor %s remote-as %s" % (bgp1_neighbor, bgp1_neighbor_asn),
               "end"]
bgp2_config = ["config t",
               "router bgp %s" % bgp2_asn,
               "bgp router-id %s" % bgp2_router_id,
               "network %s/%s" % (bgp2_network, bgp_network_pl),
               "neighbor %s remote-as %s" % (bgp2_neighbor, bgp2_neighbor_asn),
               "end"]

bgp_configs = [bgp1_config, bgp2_config]

num_of_switches = 2
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
        switch("configure terminal")
        switch("interface 1")
        switch("no shutdown")
        switch("ip address %s/%s" % (bgp_router_ids[i],
                                     bgp_network_pl))
        switch("end")
        i += 1


def verify_bgp_running(step):
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


def verify_bgp_route_removed(step, switch, network, next_hop):
    step("Verifying route %s --> %s on switch %s removed..." %
         (network, next_hop, switch.name))

    verify_route_exists = False
    found = wait_for_route(switch, network, next_hop, verify_route_exists)

    assert not found


def verify_bgp_routes(step):
    step("Verifying routes...")

    verify_bgp_route(step, dutarray[0], bgp2_network + "/" + bgp_network_pl,
                     bgp2_router_id)

    verify_bgp_route(step, dutarray[1], bgp1_network + "/" + bgp_network_pl,
                     bgp1_router_id)


def verify_bgp_route(step, switch, network, next_hop):
    step("Checking for route: %s --> %s" % (network, next_hop))

    found = wait_for_route(switch, network, next_hop, True)

    assert found


def unconfigure_neighbor_bgp(step):
    step("Unconfiguring neighbor for BGP1...")

    switch = dutarray[0]

    switch("configure terminal")
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s" % bgp1_neighbor)

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def configure_neighbor_soft_reconfiguration_peer(step):
    key = "soft-reconfiguration inbound"
    step("configuring %s for neighbor %s on switch 1..." %
         (key, bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s" % (bgp1_neighbor, key))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_soft_reconfiguration_peer(step):
    key = "soft-reconfiguration inbound"
    key2 = "inbound_soft_reconfiguration"
    step("Verifying %s for neighbor %s on switch 1..." % (key2,
                                                          bgp1_neighbor))
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


def unconfigure_neighbor_soft_reconfiguration_peer(step):
    key = "soft-reconfiguration inbound"
    step("unconfiguring %s for neighbor %s on switch 1..." % (key,
                                                              bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (bgp1_neighbor, key))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_soft_reconfiguration_peer(step):
    key = "soft-reconfiguration inbound"
    key2 = "inbound_soft_reconfiguration"
    step("Verifying no %s for neighbor %s on switch 1..." % (key2,
                                                             bgp1_neighbor))
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
    show_cmd = "show ip bgp neighbors " + bgp1_neighbor
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "%s: Enabled" % (key2)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)

    found = False
    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s" % (bgp1_neighbor, key)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_soft_reconfiguration_peergroup(step):
    key = "soft-reconfiguration inbound"
    step("Configuring %s for peer group %s on switch 1..." % (key,
                                                              peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s peer-group" % peer_group)
    cfg_array.append("neighbor %s peer-group %s" % (bgp1_neighbor,
                                                    peer_group))
    cfg_array.append("neighbor %s %s" % (peer_group, key))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_soft_reconfiguration_peergroup(step):
    key = "soft-reconfiguration inbound"
    step("verifying %s for peer group %s on switch 1..." % (key, peer_group))
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


def unconfigure_neighbor_soft_reconfiguration_peergroup(step):
    key = "soft-reconfiguration inbound"
    step("Unconfiguring %s for peer group %s on switch 1..." % (key,
                                                                peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s" % (peer_group, key))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_soft_reconfiguration_peergroup(step):
    key = "soft-reconfiguration inbound"
    step("Verifying no %s for peer group %s on switch 1..." % (key,
                                                               peer_group))
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


def configure_neighbor_prefix_list(step):
    key = "prefix-list"
    step("Configuring %s for neighbor %s on switch 1..." % (key,
                                                            bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    prefix_list = 'pl_test'
    cfg_array = []
    cfg_array.append("ip prefix-list %s seq 101 permit "
                     "10.0.0.1/8" % prefix_list)
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                               prefix_list, list_in))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_prefix_list(step):
    key = "prefix-list"
    key2 = "prefix_list"
    step("Verifying %s for neighbor %s on switch 1..." % (key2,
                                                          bgp1_neighbor))
    found = False
    prefix_list = 'pl_test'
    switch = dutarray[0]

    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                               prefix_list,
                                               list_in)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_neighbor_prefix_list(step):
    key = "prefix-list"
    step("Unconfiguring %s for neighbor %s on switch 1..." % (key,
                                                              bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    prefix_list = 'pl_test'
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                                  prefix_list, list_in))
    cfg_array.append("no ip prefix-list %s seq 101 permit "
                     "10.0.0.1/8" % prefix_list)

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_prefix_list(step):
    key = "prefix-list"
    key2 = "prefix_list"
    step("Verifying no %s for neighbor %s on switch 1..." % (key2,
                                                             bgp1_neighbor))
    found = False
    prefix_list = 'pl_test'
    switch = dutarray[0]

    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                               prefix_list,
                                               list_in)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_prefix_list_peergroup(step):
    key = "prefix-list"
    step("Configuring %s for peer group %s on switch 1..." % (key,
                                                              peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    prefix_list = 'pl_test'
    cfg_array = []
    cfg_array.append("ip prefix-list %s seq 101 permit "
                     "10.0.0.1/8" % prefix_list)
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s peer-group" % peer_group)
    cfg_array.append("neighbor %s peer-group %s" % (bgp1_neighbor,
                                                    peer_group))
    cfg_array.append("neighbor %s %s %s %s" % (peer_group, key, prefix_list,
                                               list_in))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_prefix_list_peergroup(step):
    key = "prefix-list"
    step("verifying %s for peer group %s on switch 1..." % (key, peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    prefix_list = 'pl_test'
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %s %s" % (peer_group, key,
                                                              prefix_list,
                                                              list_in)
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


def unconfigure_neighbor_prefix_list_peergroup(step):
    key = "prefix-list"
    step("Unconfiguring %s for peer group %s on switch 1..." % (key,
                                                                peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    prefix_list = 'pl_test'
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %s %s" % (peer_group, key,
                                                  prefix_list, list_in))
    cfg_array.append("no ip prefix-list %s seq 101 permit "
                     "10.0.0.1/8" % prefix_list)

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_prefix_list_peergroup(step):
    key = "prefix-list"
    step("Verifying no %s for peer group %s on switch 1..." % (key,
                                                               peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    prefix_list = 'pl_test'
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %s %s" % (peer_group, key,
                                                              prefix_list,
                                                              list_in)
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


def configure_neighbor_filter_list(step):
    key = "filter-list"
    step("Configuring %s for neighbor %s on switch 1..." % (key,
                                                            bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("ip as-path access-list %s permit 123" % filter_list)
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                               filter_list, list_out))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_filter_list(step):
    key = "filter-list"
    key2 = "filter_list"
    step("Verifying %s for neighbor %s on switch 1..." % (key2,
                                                          bgp1_neighbor))
    found = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %s %s" % (bgp1_neighbor, key, filter_list,
                                               list_out)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_neighbor_filter_list(step):
    key = "filter-list"
    step("Unconfiguring %s for neighbor %s on switch 1..." % (key,
                                                              bgp1_neighbor))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %s %s" % (bgp1_neighbor, key,
                                                  filter_list, list_out))
    cfg_array.append("no ip as-path access-list %s permit 123" % filter_list)

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_filter_list(step):
    key = "filter-list"
    key2 = "filter_list"
    step("Verifying no %s for neighbor %s on switch 1..." % (key2,
                                                             bgp1_neighbor))
    found = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "neighbor %s %s %s %s" % (bgp1_neighbor, key, filter_list,
                                               list_out)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_neighbor_filter_list_peergroup(step):
    key = "filter-list"
    step("Configuring %s for peer group %s on switch 1..." % (key,
                                                              peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("ip as-path access-list %s permit 123" % filter_list)
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("neighbor %s peer-group" % peer_group)
    cfg_array.append("neighbor %s peer-group %s" % (bgp1_neighbor,
                                                    peer_group))
    cfg_array.append("neighbor %s %s %s %s" % (peer_group, key, filter_list,
                                               list_out))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_neighbor_filter_list_peergroup(step):
    key = "filter-list"
    step("Verifying %s for peer group %s on switch 1..." % (key, peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %s %s" % (peer_group, key,
                                                              filter_list,
                                                              list_out)
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


def unconfigure_neighbor_filter_list_peergroup(step):
    key = "filter-list"
    step("Unconfiguring %s for peer group %s on switch 1..." % (key,
                                                                peer_group))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("router bgp %s" % bgp1_asn)
    cfg_array.append("no neighbor %s %s %s %s" % (peer_group, key,
                                                  filter_list, list_out))
    cfg_array.append("no ip as-path access-list %s permit 123" % filter_list)

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_neighbor_filter_list_peergroup(step):
    key = "filter-list"
    step("verifying no %s for peer group %s on switch 1..." % (key,
                                                               peer_group))
    foundneighbor = False
    foundpeergroup = False
    foundpeergroupcfg = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    dump = switch("show running-config")
    lines = dump.split('\n')
    search_pattern_neighbor = "neighbor %s peer-group %s" % (bgp1_neighbor,
                                                             peer_group)
    search_pattern_peer_group = "neighbor %s peer-group" % (peer_group)
    search_pattern_peer_group_cfg = "neighbor %s %s %s %s" % (peer_group, key,
                                                              filter_list,
                                                              list_out)
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


def configure_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Configuring %s on switch 1..." % (key))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("ip %s %s permit 123" % (key, filter_list))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Verifying %s on switch 1..." % (key))
    found = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    found = False
    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "ip %s %s permit 123" % (key, filter_list)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is True)


def unconfigure_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Configuring %s on switch 1..." % (key))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    cfg_array = []
    cfg_array.append("no ip %s %s permit 123" % (key, filter_list))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_no_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Verifying %s on switch 1..." % (key))
    found = False
    filter_list = 'fl_test'
    switch = dutarray[0]

    found = False
    show_cmd = "show running-config"
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "ip %s %s permit 123" % (key, filter_list)
    for line in lines:
        if search_pattern in line:
            found = True
    assert (found is False)


def configure_for_show_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Configuring %s on switch 1..." % (key))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    filter_list2 = 'fl_test2'
    cfg_array = []
    cfg_array.append("ip %s %s permit 123" % (key, filter_list))
    cfg_array.append("ip %s %s deny 456" % (key, filter_list2))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def unconfigure_for_show_ip_aspath_access_list(step):
    key = "as-path access-list"
    step("Configuring %s on switch 1..." % (key))

    switch = dutarray[0]
    switch("configure terminal")
    filter_list = 'fl_test'
    filter_list2 = 'fl_test2'
    cfg_array = []
    cfg_array.append("no ip %s %s permit 123" % (key, filter_list))
    cfg_array.append("no ip %s %s deny 456" % (key, filter_list2))

    for cmd in cfg_array:
        switch(cmd)
    switch("end")


def verify_show_ip_aspath_access_list(step):
    key = "as-path access-list"
    key2 = "as-path-access-list"
    step("Verifying show %s on switch 1..." % (key))
    found = False
    filter_list = 'fl_test'
    filter_list2 = 'fl_test2'
    switch = dutarray[0]

    found = False
    show_cmd = "show ip %s" % (key2)
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "ip %s %s" % (key, filter_list)
    search_pattern_action = "permit 123"
    i = 0
    for line in lines:
        if (
            search_pattern in lines[i] and
            search_pattern_action in lines[i + 1]
        ):
            found = True
        i = i + 1
    assert (found is True)

    found = False
    search_pattern = "ip %s %s" % (key, filter_list2)
    search_pattern_action = "deny 456"
    i = 0
    for line in lines:
        if (
            search_pattern in lines[i] and
            search_pattern_action in lines[i + 1]
        ):
            found = True
        i = i + 1
    assert (found is True)


def verify_show_ip_aspath_access_list_name(step):
    key = "as-path access-list"
    key2 = "as-path-access-list"
    step("Verifying show %s on switch 1..." % (key))

    found = False
    filter_list = 'fl_test'
    filter_list2 = 'fl_test2'
    switch = dutarray[0]

    found = False
    show_cmd = "show ip %s %s" % (key2, filter_list)
    dump = switch(show_cmd)
    lines = dump.split('\n')
    search_pattern = "ip %s %s" % (key, filter_list)
    search_pattern_action = "permit 123"
    i = 0
    for line in lines:
        if (
            search_pattern in lines[i] and
            search_pattern_action in lines[i + 1]
        ):
            found = True
        i = i + 1
    assert (found is True)

    found = False
    search_pattern = "ip %s %s" % (key, filter_list2)
    search_pattern_action = "deny 456"
    i = 0
    for line in lines:
        if (
            search_pattern in lines[i] and
            search_pattern_action in lines[i + 1]
        ):
            found = True
        i = i + 1
    assert (found is False)


def test_vtysh_ct_bgp_2_neighbor(topology, step):
    ops1 = topology.get("ops1")
    ops2 = topology.get("ops2")
    assert ops1 is not None
    assert ops2 is not None

    global dutarray
    dutarray = [ops1, ops2]

    configure_switch_ips(step)
    verify_bgp_running(step)
    configure_bgp(step)
    verify_bgp_routes(step)

    configure_neighbor_soft_reconfiguration_peer(step)
    verify_neighbor_soft_reconfiguration_peer(step)
    unconfigure_neighbor_soft_reconfiguration_peer(step)
    verify_no_neighbor_soft_reconfiguration_peer(step)

    configure_neighbor_soft_reconfiguration_peergroup(step)
    verify_neighbor_soft_reconfiguration_peergroup(step)
    unconfigure_neighbor_soft_reconfiguration_peergroup(step)
    verify_no_neighbor_soft_reconfiguration_peergroup(step)

    configure_neighbor_prefix_list(step)
    verify_neighbor_prefix_list(step)
    unconfigure_neighbor_prefix_list(step)
    verify_no_neighbor_prefix_list(step)
    configure_neighbor_prefix_list_peergroup(step)
    verify_neighbor_prefix_list_peergroup(step)
    unconfigure_neighbor_prefix_list_peergroup(step)
    verify_no_neighbor_prefix_list_peergroup(step)

    configure_neighbor_filter_list(step)
    verify_neighbor_filter_list(step)
    unconfigure_neighbor_filter_list(step)
    verify_no_neighbor_filter_list(step)
    configure_neighbor_filter_list_peergroup(step)
    verify_neighbor_filter_list_peergroup(step)
    unconfigure_neighbor_filter_list_peergroup(step)
    verify_no_neighbor_filter_list_peergroup(step)

    configure_ip_aspath_access_list(step)
    verify_ip_aspath_access_list(step)
    unconfigure_ip_aspath_access_list(step)
    verify_no_ip_aspath_access_list(step)

    configure_for_show_ip_aspath_access_list(step)
    verify_show_ip_aspath_access_list(step)
    verify_show_ip_aspath_access_list_name(step)
    unconfigure_for_show_ip_aspath_access_list(step)
