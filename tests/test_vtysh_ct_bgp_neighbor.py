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
#   * neighbor <peer> advertisement-interval <interval>
#   * no neighbor <peer> advertisement-interval <interval>
#   * neighbor <peer> ebgp_multihop
#   * neighbor <peer-group> ebgp_multihop
#   * no neighbor <peer> ebgp_multihop
#   * no neighbor <peer-group> ebgp_multihop
#   * neighbor <peer> ttl_security_hops
#   * neighbor <peer-group> ttl_security_hops
#   * no neighbor <peer> ttl_security_hops
#   * no neighbor <peer-group> ttl_security_hops
#   * neighbor <peer> update_source
#   * neighbor <peer-group> update_source
#   * no neighbor <peer> update_source
#   * no neighbor <peer-group> update_source
#
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

    def configure_neighbor_advertisement_interval(self):
        key = "advertisement-interval"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
             (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        advertisement_interval = 50
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %d" % (BGP1_NEIGHBOR,
                         key, advertisement_interval))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_advertisement_interval(self):
        key = "advertisement-interval"
        key2 = "advertisement_interval"
        info("\n########## Verifying %s for neighbor %s"
             " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))
        found = False
        advertisement_interval = 50
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(advertisement_interval) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(advertisement_interval) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %d" % (BGP1_NEIGHBOR,
                                                key,
                                                advertisement_interval)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_advertisement_interval(self):
        key = "advertisement-interval"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        advertisement_interval = 50
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %d" % (BGP1_NEIGHBOR,
                                                   key,
                                                   advertisement_interval))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_advertisement_interval_alias(self):
        key = "advertisement-interval"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
                                                key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_advertisement_interval(self):
        key = "advertisement-interval"
        key2 = "advertisement_interval"
        info("\n########## Verifying no %s for neighbor %s"
             " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))
        found = False
        advertisement_interval = 50
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(advertisement_interval) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(advertisement_interval) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %d" % (BGP1_NEIGHBOR,
                                                key,
                                                advertisement_interval)
        for line in lines:
            if search_pattern in line:
                found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def configure_neighbor_ebgp_multihop(self):
        key = "ebgp-multihop"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
             (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_ebgp_multihop(self):
        key = "ebgp-multihop"
        key2 = "ebgp_multihop"
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

    def unconfigure_neighbor_ebgp_multihop(self):
        key = "ebgp-multihop"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_ebgp_multihop(self):
        key = "ebgp-multihop"
        key2 = "ebgp_multihop"
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

    def configure_neighbor_ebgp_multihop_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1 to test dependency on ttl-security... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %d" % (BGP1_NEIGHBOR, \
            key2, ttl_security))
        cfg_array.append("neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_ebgp_multihop_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1 to test dependency on ttl-security... ##########\n" %
            (key2, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %d" % (BGP1_NEIGHBOR, \
            key2, ttl_security))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def configure_neighbor_ebgp_multihop_peer_group_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1 to test dependency on ttl-security... ##########\n" %
            (key, self.peer_group))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %d" % (self.peer_group, \
            key2, ttl_security))
        cfg_array.append("neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_ebgp_multihop_peer_group_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1 to test dependency on ttl-security... ##########\n" %
            (key2, self.peer_group))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %d" % (self.peer_group, \
            key2, ttl_security))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def configure_neighbor_ebgp_multihop_peergroup(self):
        key = "ebgp-multihop"
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

    def verify_neighbor_ebgp_multihop_peergroup(self):
        key = "ebgp-multihop"
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

    def unconfigure_neighbor_ebgp_multihop_peergroup(self):
        key = "ebgp-multihop"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_ebgp_multihop_peergroup(self):
        key = "ebgp-multihop"
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

    def configure_neighbor_ttl_security_hops(self):
        key = "ttl-security hops"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        ttl_security_hops = 123
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %d" % (BGP1_NEIGHBOR, \
            key, ttl_security_hops))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_ttl_security_hops(self):
        key = "ttl-security hops"
        key2 = "ttl_security hops"
        info("\n########## Verifying %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        ttl_security_hops = 123
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(ttl_security_hops) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(ttl_security_hops) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %d"% (BGP1_NEIGHBOR, \
            key, ttl_security_hops)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_ttl_security_hops(self):
        key = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        ttl_security_hops = 123
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %d" % (BGP1_NEIGHBOR, \
            key, ttl_security_hops))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_ttl_security_hops_alias(self):
        key = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_ttl_security_hops(self):
        key = "ttl-security hops"
        key2 = "ttl_security hops"
        info("\n########## Verifying no %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        ttl_security_hops = 123
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(ttl_security_hops) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(ttl_security_hops) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %d"% (BGP1_NEIGHBOR, \
            key, ttl_security_hops)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

    def configure_neighbor_ttl_security_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1 to test dependency on ebgp-multihop... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))
        cfg_array.append("neighbor %s %s %d" % (BGP1_NEIGHBOR, \
            key2, ttl_security))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_ttl_security_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1 to test dependency on ebgp-multihop... ##########\n" %
            (key2, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def configure_neighbor_ttl_security_peer_group_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1 to test dependency on ebgp-multihop... ##########\n" %
            (key, self.peer_group))

        switch = self.net.switches[0]
        ttl_security = 250
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s" % (self.peer_group, \
            key))
        cfg_array.append("neighbor %s %s %d" % (self.peer_group, \
            key2, ttl_security))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_ttl_security_peer_group_test_dependency(self):
        key = "ebgp-multihop"
        key2 = "ttl-security hops"
        info("\n########## Unconfiguring %s for neighbor %s"
             " on switch 1 to test dependency on ebgp-multihop... ##########\n" %
            (key2, self.peer_group))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def configure_neighbor_ttl_security_hops_peergroup(self):
        key = "ttl-security hops"
        info("\n########## Configuring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        ttl_security_hops = 50
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s peer-group" % self.peer_group)
        cfg_array.append("neighbor %s peer-group %s" % (BGP1_NEIGHBOR,
            self.peer_group))
        cfg_array.append("neighbor %s %s %d" % (self.peer_group, \
            key, ttl_security_hops))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_ttl_security_hops_peergroup(self):
        key = "ttl-security hops"
        info("\n########## Verifying %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        ttl_security_hops = 50
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %d"% (self.peer_group, key, ttl_security_hops)
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

    def unconfigure_neighbor_ttl_security_hops_peergroup(self):
        key = "ttl-security hops"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        ttl_security_hops = 50
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s %d" % (self.peer_group, \
            key, ttl_security_hops))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_ttl_security_hops_peergroup(self):
        key = "ttl-security hops"
        info("\n########## Verifying no %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        ttl_security_hops = 50
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %d"% (self.peer_group, key, ttl_security_hops)
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

    def configure_neighbor_update_source(self):
        key = "update-source"
        info("\n########## Configuring %s for neighbor %s"
             " on switch 1... ##########\n" %
            (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        update_source = 'loopback'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s %s %s" % (BGP1_NEIGHBOR, \
            key, update_source))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_update_source(self):
        key = "update-source"
        key2 = "update_source"
        info("\n########## Verifying %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        update_source = 'loopback'
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(update_source) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(update_source) in line:
                        found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s"% (BGP1_NEIGHBOR, \
            key, update_source)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == True), \
            "Error in verifying %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

        return True

    def unconfigure_neighbor_update_source(self):
        key = "update-source"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        update_source = 'loopback'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def unconfigure_neighbor_update_source_alias(self):
        key = "update-source"
        info("\n########## Unconfiguring %s for neighbor %s"
            " on switch 1... ##########\n" % (key, BGP1_NEIGHBOR))

        switch = self.net.switches[0]
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (BGP1_NEIGHBOR, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_update_source(self):
        key = "update-source"
        key2 = "update_source"
        info("\n########## Verifying no %s for neighbor %s"
            " on switch 1... ##########\n" % (key2, BGP1_NEIGHBOR))
        found = False
        update_source = 'loopback'
        switch = self.net.switches[0]
        dump = switch.cmdCLI("show ip bgp neighbors")
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(update_source) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show ip bgp neighbors "+BGP1_NEIGHBOR
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        for line in lines:
            if key2 in line \
                and ":" in line \
                    and str(update_source) in line:
                        found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key2, BGP1_NEIGHBOR)

        found = False
        show_cmd = "show running-config"
        dump = switch.cmdCLI(show_cmd)
        lines = dump.split('\n')
        search_pattern = "neighbor %s %s %s"% (BGP1_NEIGHBOR, \
            key, update_source)
        for line in lines:
            if search_pattern in line:
                   found = True
        assert (found == False), \
            "Error in verifying no %s for neighbor %s" \
            " on switch 1\n" % (key, BGP1_NEIGHBOR)

    def configure_neighbor_update_source_peergroup(self):
        key = "update-source"
        info("\n########## Configuring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        update_source = 'loopback'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("neighbor %s peer-group" % self.peer_group)
        cfg_array.append("neighbor %s peer-group %s" % (BGP1_NEIGHBOR,
            self.peer_group))
        cfg_array.append("neighbor %s %s %s" % (self.peer_group, \
            key, update_source))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_neighbor_update_source_peergroup(self):
        key = "update-source"
        info("\n########## Verifying %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        update_source = 'loopback'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s"% (self.peer_group, key, update_source)
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

    def unconfigure_neighbor_update_source_peergroup(self):
        key = "update-source"
        info("\n########## Unconfiguring %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))

        switch = self.net.switches[0]
        update_source = 'loopback'
        cfg_array = []
        cfg_array.append("router bgp %s" % BGP1_ASN)
        cfg_array.append("no neighbor %s %s" % (self.peer_group, \
            key))

        SwitchVtyshUtils.vtysh_cfg_cmd(switch, cfg_array)

    def verify_no_neighbor_update_source_peergroup(self):
        key = "update-source"
        info("\n########## Verifying no %s for peer group %s"
            " on switch 1... ##########\n" % (key, self.peer_group))
        foundNeighbor = False
        foundpeerGroup = False
        foundpeerGroupCfg = False
        update_source = 'loopback'
        switch = self.net.switches[0]

        dump = switch.cmdCLI("show running-config")
        lines = dump.split('\n')
        search_pattern_neighbor = "neighbor %s peer-group %s"% (BGP1_NEIGHBOR,
            self.peer_group)
        search_pattern_peer_group = "neighbor %s peer-group"% (self.peer_group)
        search_pattern_peer_group_cfg = \
            "neighbor %s %s %s"% (self.peer_group, key, update_source)
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

        self.test_var.configure_neighbor_advertisement_interval()
        self.test_var.verify_neighbor_advertisement_interval()
        self.test_var.unconfigure_neighbor_advertisement_interval()
        self.test_var.verify_no_neighbor_advertisement_interval()

        self.test_var.configure_neighbor_advertisement_interval()
        self.test_var.verify_neighbor_advertisement_interval()
        self.test_var.unconfigure_neighbor_advertisement_interval_alias()
        self.test_var.verify_no_neighbor_advertisement_interval()

        self.test_var.configure_neighbor_ebgp_multihop()
        self.test_var.verify_neighbor_ebgp_multihop()
        self.test_var.unconfigure_neighbor_ebgp_multihop()
        self.test_var.verify_no_neighbor_ebgp_multihop()
        self.test_var.configure_neighbor_ebgp_multihop_test_dependency()
        self.test_var.verify_no_neighbor_ebgp_multihop()
        self.test_var.unconfigure_neighbor_ebgp_multihop_test_dependency()

        self.test_var.configure_neighbor_ebgp_multihop_peergroup()
        self.test_var.verify_neighbor_ebgp_multihop_peergroup()
        self.test_var.unconfigure_neighbor_ebgp_multihop_peergroup()
        self.test_var.verify_no_neighbor_ebgp_multihop_peergroup()
        self.test_var.configure_neighbor_ebgp_multihop_peer_group_test_dependency()
        self.test_var.verify_no_neighbor_ebgp_multihop_peergroup()
        self.test_var.unconfigure_neighbor_ebgp_multihop_peer_group_test_dependency()

        self.test_var.configure_neighbor_ttl_security_hops()
        self.test_var.verify_neighbor_ttl_security_hops()
        self.test_var.unconfigure_neighbor_ttl_security_hops()
        self.test_var.verify_no_neighbor_ttl_security_hops()
        self.test_var.configure_neighbor_ttl_security_test_dependency()
        self.test_var.verify_no_neighbor_ttl_security_hops()
        self.test_var.unconfigure_neighbor_ttl_security_test_dependency()

        self.test_var.configure_neighbor_ttl_security_hops_peergroup()
        self.test_var.verify_neighbor_ttl_security_hops_peergroup()
        self.test_var.unconfigure_neighbor_ttl_security_hops_peergroup()
        self.test_var.verify_no_neighbor_ttl_security_hops_peergroup()
        self.test_var.configure_neighbor_ttl_security_peer_group_test_dependency()
        self.test_var.verify_no_neighbor_ttl_security_hops_peergroup()
        self.test_var.unconfigure_neighbor_ttl_security_peer_group_test_dependency()

        self.test_var.configure_neighbor_update_source()
        self.test_var.verify_neighbor_update_source()
        self.test_var.unconfigure_neighbor_update_source()
        self.test_var.verify_no_neighbor_update_source()

        self.test_var.configure_neighbor_update_source_peergroup()
        self.test_var.verify_neighbor_update_source_peergroup()
        self.test_var.unconfigure_neighbor_update_source_peergroup()
        self.test_var.verify_no_neighbor_update_source_peergroup()


#    def test_mininet_cli(self):
#        self.test_var.mininet_cli()
