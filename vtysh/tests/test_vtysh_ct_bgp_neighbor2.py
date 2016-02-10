#!/usr/bin/python

# Copyright (C) 2016 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import pytest
from opsvsiutils.vtyshutils import *
from opsvsiutils.bgpconfig import *
from opsvsi.docker import *
from opsvsi.opsvsitest import *

#
# This case tests the most basic configuration between two BGP instances by
# verifying that the advertised routes are received on both instances running
# BGP.
#
# The following commands are tested:
#   * router bgp <asn>
#   * bgp router-id <router-id>
#   * network <network>
#   * neighbor <peer> remote-as <asn>
#   * neighbor <peer> soft-reconfiguration inbound
#   * neighbor <peer-group> soft-reconfiguration inbound
#   * no neighbor <peer> soft-reconfiguration inbound
#   * no neighbor <peer-group> soft-reconfiguration inbound
#   * neighbor <peer> prefix-list
#   * neighbor <peer-group> prefix-list
#   * no neighbor <peer> prefix-list
#   * no neighbor <peer-group> prefix-list
#   * neighbor <peer> filter-list
#   * neighbor <peer-group> filter-list
#   * no neighbor <peer> filter-list
#   * no neighbor <peer-group> filter-list
#   * ip as-path access-list WORD (deny|permit) .LINE
#   * no ip as-path access-list WORD (deny|permit) .LINE
#   * show ip as-path access-list
#   * show ip as-path access-list WORD
#
# S1 [interface 1]<--->[interface 1] S2


BGP1_ASN = "1"
BGP1_ROUTER_ID = "9.0.0.1"
BGP1_NETWORK = "11.0.0.0"

BGP2_ASN = "2"
BGP2_ROUTER_ID = "9.0.0.2"
BGP2_NETWORK = "12.0.0.0"

BGP1_NEIGHBOR = BGP2_ROUTER_ID
BGP1_NEIGHBOR_ASN = BGP2_ASN

BGP2_NEIGHBOR = BGP1_ROUTER_ID
BGP2_NEIGHBOR_ASN = BGP1_ASN

BGP_NETWORK_PL = "8"
BGP_NETWORK_MASK = "255.0.0.0"
BGP_ROUTER_IDS = [BGP1_ROUTER_ID, BGP2_ROUTER_ID]

BGP1_CONFIG = ["router bgp %s" % BGP1_ASN,
               "bgp router-id %s" % BGP1_ROUTER_ID,
               "network %s/%s" % (BGP1_NETWORK, BGP_NETWORK_PL),
               "neighbor %s remote-as %s" % (BGP1_NEIGHBOR, BGP1_NEIGHBOR_ASN)]

BGP2_CONFIG = ["router bgp %s" % BGP2_ASN,
               "bgp router-id %s" % BGP2_ROUTER_ID,
               "network %s/%s" % (BGP2_NETWORK, BGP_NETWORK_PL),
               "neighbor %s remote-as %s" % (BGP2_NEIGHBOR, BGP2_NEIGHBOR_ASN)]

BGP_CONFIGS = [BGP1_CONFIG, BGP2_CONFIG]

NUM_OF_SWITCHES = 2
NUM_HOSTS_PER_SWITCH = 0

SWITCH_PREFIX = "s"

LIST_IN = "in"
LIST_OUT = "out"


class myTopo(Topo):
    def build(self, hsts=0, sws=2, **_opts):
        self.hsts = hsts
        self.sws = sws

        switch = self.addSwitch("%s1" % SWITCH_PREFIX)
        switch = self.addSwitch(name="%s2" % SWITCH_PREFIX,
                                cls=PEER_SWITCH_TYPE,
                                **self.sopts)

        # Connect the switches
        for i in irange(2, sws):
            self.addLink("%s%s" % (SWITCH_PREFIX, i-1),
                         "%s%s" % (SWITCH_PREFIX, i))


class bgpTest(OpsVsiTest):
    peer_group = "peerGroupTest"
    def setupNet(self):
        self.net = Mininet(topo=myTopo(hsts=NUM_HOSTS_PER_SWITCH,
                                       sws=NUM_OF_SWITCHES,
                                       hopts=self.getHostOpts(),
                                       sopts=self.getSwitchOpts()),
                           switch=SWITCH_TYPE,
                           host=OpsVsiHost,
                           link=OpsVsiLink,
                           controller=None,
                           build=True)

    def mininet_cli(self):
        CLI(self.net)

    def configure_switch_ips(self):
        info("\n########## Configuring switch IPs.. ##########\n")

        i = 0
        for switch in self.net.switches:
            # Configure the IPs between the switches
            if isinstance(switch, VsiOpenSwitch):
                switch.cmdCLI("configure terminal")
                switch.cmdCLI("interface 1")
                switch.cmdCLI("no shutdown")
                switch.cmdCLI("ip address %s/%s" % (BGP_ROUTER_IDS[i],
                                                    BGP_NETWORK_PL))
                switch.cmdCLI("exit")
            else:
                switch.setIP(ip=BGP_ROUTER_IDS[i],
                             intf="%s-eth1" % switch.name)
            i += 1

    def verify_bgp_running(self):
        info("\n########## Verifying bgp processes.. ##########\n")

        for switch in self.net.switches:
            pid = switch.cmd("pgrep -f bgpd").strip()
            assert (pid != ""), "bgpd process not running on switch %s" % \
                                switch.name

            info("### bgpd process exists on switch %s ###\n" % switch.name)

    def configure_bgp(self):
        info("\n########## Applying BGP configurations... ##########\n")

        i = 0
        for switch in self.net.switches:
            cfg_array = BGP_CONFIGS[i]
            i += 1

            SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_bgp_route_removed(self, switch, network, next_hop):
        info("\n########## Verifying route %s --> %s "
             "on switch %s removed... ##########\n" %
             (network, next_hop, switch.name))

        verify_route_exists = False
        found = SwitchVtyshUtils.wait_for_route(switch, network, next_hop,
                                                verify_route_exists)

        assert not found, "Route (%s) was not successfully removed" % network

        info("### Route successfully removed ###\n")

    def verify_bgp_routes(self):
        info("\n########## Verifying routes... ##########\n")

        self.verify_bgp_route(self.net.switches[0], BGP2_NETWORK,
                              BGP2_ROUTER_ID)

        self.verify_bgp_route(self.net.switches[1], BGP1_NETWORK,
                              BGP1_ROUTER_ID)

    def verify_bgp_route(self, switch, network, next_hop):
        info("### Checking for route: %s --> %s ###\n" % (network, next_hop))

        found = SwitchVtyshUtils.wait_for_route(switch, network, next_hop)

        assert found, "Could not find route (%s -> %s) on %s" % \
                      (network, next_hop, switch.name)

    def unconfigure_neighbor_bgp(self):
        info("\n########## Unconfiguring neighbor for BGP1... ##########\n")

        switch = self.net.switches[0]

        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s" % BGP1_NEIGHBOR)

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)


    def configure_neighbor_soft_reconfiguration_peer(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Configuring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)


    def verify_neighbor_soft_reconfiguration_peer(self):
        key = "soft-reconfiguration inbound"
        key2 = "inbound_soft_reconfiguration"
        info("\n########## Verifying %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        search_pattern = "%s: Enabled"% (key2)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "%s: Enabled"% (key2)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s"% (BGP1_NEIGHBOR, \
            key)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_soft_reconfiguration_peer(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_soft_reconfiguration_peer(self):
        key = "soft-reconfiguration inbound"
        key2 = "inbound_soft_reconfiguration"
        info("\n########## Verifying no %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        search_pattern = "%s: Enabled"% (key2)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "%s: Enabled"% (key2)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s"% (BGP1_NEIGHBOR, \
            key)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)


    def configure_neighbor_soft_reconfiguration_peergroup(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Configuring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s peer-group" % self.peer_group)
        cfg_array.append("neighbor %s peer-group %s" % (BGP1_NEIGHBOR,
            self.peer_group))
        cfg_array.append("neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_soft_reconfiguration_peergroup(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Verifying %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s"% (self.peer_group, key)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == True), \
            "Error in verifying %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

    def unconfigure_neighbor_soft_reconfiguration_peergroup(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_soft_reconfiguration_peergroup(self):
        key = "soft-reconfiguration inbound"
        info("\n########## Verifying no %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s"% (self.peer_group, key)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == False), \
            "Error in verifying no %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

    def configure_neighbor_prefix_list(self):
        key = "prefix-list"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        prefix_list = 'pl_test'
        cfg_array = []
        cfg_array.append("ip prefix-list %s seq 101 permit 10.0.0.1/8" % prefix_list)
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %s %s" % (BGP1_NEIGHBOR, \
            key, prefix_list, LIST_IN))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_prefix_list(self):
        key = "prefix-list"
        key2 = "prefix_list"
        info("\n########## Verifying %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        prefix_list = 'pl_test'
        switch = self.net.switches[0]

        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s %s"% (BGP1_NEIGHBOR, \
            key, prefix_list, LIST_IN)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_prefix_list(self):
        key = "prefix-list"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        prefix_list = 'pl_test'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %s %s" % (BGP1_NEIGHBOR, \
            key, prefix_list, LIST_IN))
        cfg_array.append("no ip prefix-list %s seq 101 permit 10.0.0.1/8" % prefix_list)

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_prefix_list(self):
        key = "prefix-list"
        key2 = "prefix_list"
        info("\n########## Verifying no %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        prefix_list = 'pl_test'
        switch = self.net.switches[0]

        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s %s"% (BGP1_NEIGHBOR, \
            key, prefix_list, LIST_IN)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

    def configure_neighbor_prefix_list_peergroup(self):
        key = "prefix-list"
        info("\n########## Configuring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        prefix_list = 'pl_test'
        cfg_array = []
        cfg_array.append("ip prefix-list %s seq 101 permit 10.0.0.1/8" % prefix_list)
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s peer-group" % self.peer_group)
        cfg_array.append("neighbor %s peer-group %s" % (BGP1_NEIGHBOR,
            self.peer_group))
        cfg_array.append("neighbor %s %s %s %s" % (self.peer_group, \
            key, prefix_list, LIST_IN))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_prefix_list_peergroup(self):
        key = "prefix-list"
        info("\n########## Verifying %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        prefix_list = 'pl_test'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s %s"% (self.peer_group, key, prefix_list, LIST_IN)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == True), \
            "Error in verifying %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

    def unconfigure_neighbor_prefix_list_peergroup(self):
        key = "prefix-list"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        prefix_list = 'pl_test'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %s %s" % (self.peer_group, \
            key, prefix_list, LIST_IN))
        cfg_array.append("no ip prefix-list %s seq 101 permit 10.0.0.1/8" % prefix_list)

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_prefix_list_peergroup(self):
        key = "prefix-list"
        info("\n########## Verifying no %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        prefix_list = 'pl_test'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s %s"% (self.peer_group, key, prefix_list, LIST_IN)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == False), \
            "Error in verifying no %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

        return True

    def configure_neighbor_filter_list(self):
        key = "filter-list"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("ip as-path access-list %s permit 123" % filter_list)
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %s %s" % (BGP1_NEIGHBOR, \
            key, filter_list, LIST_OUT))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_filter_list(self):
        key = "filter-list"
        key2 = "filter_list"
        info("\n########## Verifying %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s %s"% (BGP1_NEIGHBOR, \
            key, filter_list, LIST_OUT)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_filter_list(self):
        key = "filter-list"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %s %s" % (BGP1_NEIGHBOR, \
            key, filter_list, LIST_OUT))
        cfg_array.append("no ip as-path access-list %s permit 123" % filter_list)

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_filter_list(self):
        key = "filter-list"
        key2 = "filter_list"
        info("\n########## Verifying no %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s %s"% (BGP1_NEIGHBOR, \
            key, filter_list, LIST_OUT)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

    def configure_neighbor_filter_list_peergroup(self):
        key = "filter-list"
        info("\n########## Configuring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("ip as-path access-list %s permit 123" % filter_list)
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s peer-group" % self.peer_group)
        cfg_array.append("neighbor %s peer-group %s" % (BGP1_NEIGHBOR,
            self.peer_group))
        cfg_array.append("neighbor %s %s %s %s" % (self.peer_group, \
            key, filter_list, LIST_OUT))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_filter_list_peergroup(self):
        key = "filter-list"
        info("\n########## Verifying %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s %s"% (self.peer_group, key, filter_list, LIST_OUT)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == True), \
            "Error in verifying %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

    def unconfigure_neighbor_filter_list_peergroup(self):
        key = "filter-list"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %s %s" % (self.peer_group, \
            key, filter_list, LIST_OUT))
        cfg_array.append("no ip as-path access-list %s permit 123" % filter_list)

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_filter_list_peergroup(self):
        key = "filter-list"
        info("\n########## Verifying no %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s %s"% (self.peer_group, key, filter_list, LIST_OUT)
        for line in lines:
            if search_pattern_neighbor in line:
                foundNeighbor = True
            elif search_pattern_peer_group in line:
                foundpeerGroup = True
            elif search_pattern_peer_group_cfg in line:
                foundpeerGroupCfg = True

        assert (foundNeighbor == True or foundpeerGroup == True or \
            foundpeerGroupCfg == False), \
            "Error in verifying no %s for peer group %s" \
            " on switch 1\n" % (key, self.peer_group)

        return True

    def configure_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Configuring %s on switch 1... ##########\n" %(key))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("ip %s %s permit 123" % (key, filter_list))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Verifying %s on switch 1... ##########\n" %(key))
        found = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "ip %s %s permit 123"% (key, \
            filter_list)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s on switch 1\n" % (key)

        return True

    def unconfigure_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Configuring %s on switch 1... ##########\n" %(key))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        cfg_array = []
        cfg_array.append("no ip %s %s permit 123" % (key, filter_list))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Verifying %s on switch 1... ##########\n" % (key))
        found = False
        filter_list = 'fl_test'
        switch = self.net.switches[0]

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "ip %s %s permit 123"% (key, \
            filter_list)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying %s on switch 1\n" % (key)

        return True

    def configure_for_show_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Configuring %s on switch 1... ##########\n" % (key))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        filter_list2 = 'fl_test2'
        cfg_array = []
        cfg_array.append("ip %s %s permit 123" % (key, filter_list))
        cfg_array.append("ip %s %s deny 456" % (key, filter_list2))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)


    def unconfigure_for_show_ip_aspath_access_list(self):
        key = "as-path access-list"
        info("\n########## Configuring %s on switch 1... ##########\n" % (key))

        switch = self.net.switches[0]
        filter_list = 'fl_test'
        filter_list2 = 'fl_test2'
        cfg_array = []
        cfg_array.append("no ip %s %s permit 123" % (key, filter_list))
        cfg_array.append("no ip %s %s deny 456" % (key, filter_list2))


    def verify_show_ip_aspath_access_list(self):
        key = "as-path access-list"
        key2 = "as-path-access-list"
        info("\n########## Verifying %s on switch 1... ##########\n" %(key))
        found = False
        filter_list = 'fl_test'
        filter_list2 = 'fl_test2'
        switch = self.net.switches[0]

        found = False
        show_cmd = "show ip %s" %(key2)
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "ip %s %s"% (key, filter_list)
        search_pattern_action = "permit 123"
        i = 0
        for line in lines:
            if search_pattern in lines[i]\
                and search_pattern_action in lines[i+1]:
                found = True
            i = i + 1
        assert (found == True), \
            "Error in verifying %s on switch 1\n" % (key)

        found = False
        search_pattern = "ip %s %s"% (key, filter_list2)
        search_pattern_action = "deny 456"
        i = 0
        for line in lines:
            if search_pattern in lines[i]\
                and search_pattern_action in lines[i+1]:
                found = True
            i = i + 1
        assert (found == True), \
            "Error in verifying %s on switch 1\n" % (key)

        return True

    def verify_show_ip_aspath_access_list_name(self):
        key = "as-path access-list"
        key2 = "as-path-access-list"
        info("\n########## Verifying %s on switch 1... ##########\n" %(key))

        found = False
        filter_list = 'fl_test'
        filter_list2 = 'fl_test2'
        switch = self.net.switches[0]

        found = False
        show_cmd = "show ip %s %s" %(key2, filter_list)
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "ip %s %s"% (key, filter_list)
        search_pattern_action = "permit 123"
        i = 0
        for line in lines:
            if search_pattern in lines[i]\
                and search_pattern_action in lines[i+1]:
                found = True
            i = i + 1
        assert (found == True), \
            "Error in verifying %s on switch 1\n" % (key)

        found = False
        search_pattern = "ip %s %s"% (key, filter_list2)
        search_pattern_action = "deny 456"
        i = 0
        for line in lines:
            if search_pattern in lines[i]\
                and search_pattern_action in lines[i+1]:
                found = True
            i = i + 1
        assert (found == False), \
            "Error in verifying %s on switch 1\n" % (key)

        return True


class Test_bgpd_neighbor_cmds:
    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_bgpd_neighbor_cmds.test_var = bgpTest()

    def teardown_class(cls):
        Test_bgpd_neighbor_cmds.test_var.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test_var

    # the actual test function
    def test_bgp_full(self):
        self.test_var.configure_switch_ips()
        self.test_var.verify_bgp_running()
        self.test_var.configure_bgp()
        self.test_var.verify_bgp_routes()

        self.test_var.configure_neighbor_soft_reconfiguration_peer()
        self.test_var.verify_neighbor_soft_reconfiguration_peer()
        self.test_var.unconfigure_neighbor_soft_reconfiguration_peer()
        self.test_var.verify_no_neighbor_soft_reconfiguration_peer()

        self.test_var.configure_neighbor_soft_reconfiguration_peergroup()
        self.test_var.verify_neighbor_soft_reconfiguration_peergroup()
        self.test_var.unconfigure_neighbor_soft_reconfiguration_peergroup()
        self.test_var.verify_no_neighbor_soft_reconfiguration_peergroup()

        self.test_var.configure_neighbor_prefix_list()
        self.test_var.verify_neighbor_prefix_list()
        self.test_var.unconfigure_neighbor_prefix_list()
        self.test_var.verify_no_neighbor_prefix_list()
        self.test_var.configure_neighbor_prefix_list_peergroup()
        self.test_var.verify_neighbor_prefix_list_peergroup()
        self.test_var.unconfigure_neighbor_prefix_list_peergroup()
        self.test_var.verify_no_neighbor_prefix_list_peergroup()

        self.test_var.configure_neighbor_filter_list()
        self.test_var.verify_neighbor_filter_list()
        self.test_var.unconfigure_neighbor_filter_list()
        self.test_var.verify_no_neighbor_filter_list()
        self.test_var.configure_neighbor_filter_list_peergroup()
        self.test_var.verify_neighbor_filter_list_peergroup()
        self.test_var.unconfigure_neighbor_filter_list_peergroup()
        self.test_var.verify_no_neighbor_filter_list_peergroup()

        self.test_var.configure_ip_aspath_access_list()
        self.test_var.verify_ip_aspath_access_list()
        self.test_var.unconfigure_ip_aspath_access_list()
        self.test_var.verify_no_ip_aspath_access_list()

        self.test_var.configure_for_show_ip_aspath_access_list()
        self.test_var.verify_show_ip_aspath_access_list()
        self.test_var.verify_show_ip_aspath_access_list_name()
        self.test_var.unconfigure_for_show_ip_aspath_access_list()


#    def test_mininet_cli(self):
#        self.test_var.mininet_cli()
