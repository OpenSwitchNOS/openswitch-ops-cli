#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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
import re


class LACPCliTest(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        infra_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(infra_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)
    def createLagPort(self):
        info('''
########## Test to create LAG Port ##########
''')
        lag_port_found = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 1')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if '"lag1"' in line:
                lag_port_found = True
        assert (lag_port_found is True), \
            'Test to create LAG Port - FAILED!!'
        return True

    def deleteLagPort(self):
        info('''
########## Test to delete LAG port ##########
''')
        lag_port_found = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 5004')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 21')
        s1.cmdCLI('interface 20')
        s1.cmdCLI('lag 500')
        s1.cmdCLI('no lag 500')
        s1.cmdCLI('interface lag 3')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 10')
        s1.cmdCLI('lag 3')
        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 3')
        s1.cmdCLI('no interface 21')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if '"lag3"' in line:
                lag_port_found = False
        assert (lag_port_found is True), \
            'Test to delete LAG port - FAILED!'
        lag_port_found = False
        for line in lines:
            if '"interface 20"' in line:
                lag_port_found = True
        assert (lag_port_found is False), \
            'Test to move interface to port table - FAILED!'
        lag_port_found = False
        for line in lines:
            if '"interface 21"' in line:
                lag_port_found = True
        assert (lag_port_found is True), \
            'Test to remove interface from port table - FAILED!'
        out = s1.cmd('do show running')
        lag_port_found = True
        lines = out.split('\n')
        for line in lines:
            if '"lag3"' in line:
                lag_port_found = False
        assert (lag_port_found is True), \
            'Test to delete LAG port - FAILED!'
        return True

class Test_lacp_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_lacp_cli.test = LACPCliTest()

    def test_createLagPort(self):
        if self.test.createLagPort():
            info('''
########## Test to create LAG Port - SUCCESS! ##########
''')

    def test_deleteLagPort(self):
        if self.test.deleteLagPort():
            info('''
########## Test to delete LAG port - SUCCESS! ##########
''')

    def teardown_class(cls):
        Test_lacp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
