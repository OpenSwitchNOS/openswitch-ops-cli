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
        self.test.configure_bgp_router_flags()
        time.sleep(60)
        self.test.unconfigure_bgp_router_flags()
        time.sleep(60)
        self.test.configure_bgp_network()
        time.sleep(60)
        self.test.unconfigure_bgp_network()
        time.sleep(60)
