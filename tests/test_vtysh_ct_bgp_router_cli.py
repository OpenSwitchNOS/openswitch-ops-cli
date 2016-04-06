#!/usr/bin/python

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


from opsvsi.docker import *
from opsvsi.opsvsitest import *
import time


class bgpCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        bgp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(bgp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def verify_bgp_router_table(self):
        info("\n##########  Test to verify BGP router table"
             " ##########\n")

        s1 = self.net.switches[0]
        out = s1.cmdCLI("show ip bgp summary")
        assert "No bgp router configured." in out, \
            "Test to verify BGP router table FAILED!"
        info("\n##########  Test to verify BGP router table successfull"
             " ##########\n")

    def configure_bgp_router_flags(self):
        info("\n##########  Test to configure BGP router flags"
             " ##########\n")

        fast_ext_failover_str = "bgp fast-external-failover"
        fast_ext_failover_flag = False
        log_neighbor_changes_str = "bgp log-neighbor-changes"
        log_neighbor_changes_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("router bgp 100")
        s1.cmdCLI(fast_ext_failover_str)
        s1.cmdCLI(log_neighbor_changes_str)
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if fast_ext_failover_str in line:
                fast_ext_failover_flag = True
            elif log_neighbor_changes_str in line:
                log_neighbor_changes_flag = True

        if fast_ext_failover_flag is False:
            info("###  BGP fast-external-failover flag not set ###\n")
        elif log_neighbor_changes_flag is False:
            info("###  BGP log-neighbor-changes flag not set ###\n")

        if fast_ext_failover_flag is False or \
           log_neighbor_changes_flag is False:
            info("### Test to set BGP Router flags-FAILED! ###\n")

    def unconfigure_bgp_router_flags(self):
        info("\n##########  Test to unconfigure BGP router flags"
             " ##########\n")

        fast_ext_failover_str = "bgp fast-external-failover"
        no_fast_ext_failover_str = "no bgp fast-external-failover"
        fast_ext_failover_flag = False
        log_neighbor_changes_str = "bgp log-neighbor-changes"
        no_log_neighbor_changes_str = "no bgp log-neighbor-changes"
        log_neighbor_changes_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("router bgp 100")
        s1.cmdCLI(no_fast_ext_failover_str)
        s1.cmdCLI(no_log_neighbor_changes_str)
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if fast_ext_failover_str in line:
                fast_ext_failover_flag = True
            elif log_neighbor_changes_str in line:
                log_neighbor_changes_flag = True

        if fast_ext_failover_flag is True:
            info("###  BGP fast-external-failover flag is set ###\n")
        elif log_neighbor_changes_flag is True:
            info("###  BGP log-neighbor-changes flag is set ###\n")

        if fast_ext_failover_flag is True or \
           log_neighbor_changes_flag is True:
            info("### Test to unconfigure BGP Router flags-FAILED! ###\n")

    def configure_bgp_network(self):
        info("\n##########  Test to configure BGP network"
             " ##########\n")

        network_str = "network 3001::/32"
        network_str_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("router bgp 100")
        s1.cmdCLI("network 3001::1/32")
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if network_str in line:
                network_str_flag = True

        assert network_str_flag is True, \
            'Test to configure BGP network FAILED!'

    def unconfigure_bgp_network(self):
        info("\n##########  Test to unconfigure BGP network"
             " ##########\n")

        network_str = "network 3001::/32"
        network_str_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("router bgp 100")
        s1.cmdCLI("no network 3001::1/32")
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if network_str in line:
                network_str_flag = True

        assert network_str_flag is False, \
            'Test to unconfigure BGP network FAILED!'

    def configure_routemap_match(self):
        info("\n##########  Test to configure Route-Map Match commands"
             " ##########\n")

        match_ipv6_prefix_list_str = "match ipv6 address prefix-list 5"
        match_ipv6_prefix_list_flag = False
        match_community_str = "match community 100"
        match_community_str_flag = False
        match_extcommunity_str = "match extcommunity e1"
        match_extcommunity_str_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("route-map r1 permit 10")
        s1.cmdCLI(match_ipv6_prefix_list_str)
        s1.cmdCLI(match_community_str)
        s1.cmdCLI(match_extcommunity_str)
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if match_ipv6_prefix_list_str in line:
                match_ipv6_prefix_list_flag = True
            elif match_community_str in line:
                match_community_str_flag = True
            elif match_extcommunity_str in line:
                match_extcommunity_str_flag = True

        if match_ipv6_prefix_list_flag is False:
            info("###  Error configuring 'match ipv6 address prefix-list' ###\n")
        elif match_community_str_flag is False:
            info("###  Error configuring 'match community' ###\n")
        elif match_extcommunity_str_flag is False:
            info("###  Error configuring 'match extcommunity' ###\n")

        if match_ipv6_prefix_list_flag is False or \
           match_community_str_flag is False or \
           match_extcommunity_str_flag is False:
            info("### Test to configure Route-Map match commands FAILED! ###\n")

    def unconfigure_routemap_match(self):
        info("\n##########  Test to unconfigure Route-Map Match commands"
             " ##########\n")

        match_ipv6_prefix_list_str = "match ipv6 address prefix-list 5"
        no_match_ipv6_prefix_list_str = "no match ipv6 address prefix-list 5"
        match_ipv6_prefix_list_flag = False
        match_community_str = "match community 100"
        no_match_community_str = "no match community 100"
        match_community_str_flag = False
        match_extcommunity_str = "match extcommunity e1"
        no_match_extcommunity_str = "no match extcommunity e1"
        match_extcommunity_str_flag = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("route-map r1 permit 10")
        s1.cmdCLI(no_match_ipv6_prefix_list_str)
        s1.cmdCLI(no_match_community_str)
        s1.cmdCLI(no_match_extcommunity_str)
        s1.cmdCLI("end")

        dump = s1.cmdCLI("show running-config")
        lines = dump.split('\n')
        for line in lines:
            if match_ipv6_prefix_list_str in line:
                match_ipv6_prefix_list_flag = True
            elif match_community_str in line:
                match_community_str_flag = True
            elif match_extcommunity_str in line:
                match_extcommunity_str_flag = True

        if match_ipv6_prefix_list_flag is True:
            info("###  Error unconfiguring 'match ipv6 address prefix-list' ###\n")
        elif match_community_str_flag is True:
            info("###  Error unconfiguring 'match community' ###\n")
        elif match_extcommunity_str_flag is True:
            info("###  Error unconfiguring 'match extcommunity' ###\n")

        if match_ipv6_prefix_list_flag is True or \
           match_community_str_flag is True or \
           match_extcommunity_str_flag is True:
            info("### Test to unconfigure Route-Map match commands FAILED! ###\n")


@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_bgpd_router_cmds:
    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_bgpd_router_cmds.test = bgpCLItest()

    def teardown_class(cls):
        Test_bgpd_router_cmds.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test

    def test_bgp_router_cmds(self):
        self.test.verify_bgp_router_table()
        self.test.configure_bgp_router_flags()
        self.test.unconfigure_bgp_router_flags()
        self.test.configure_bgp_network()
        self.test.unconfigure_bgp_network()
        self.test.configure_routemap_match()
        self.test.unconfigure_routemap_match()
