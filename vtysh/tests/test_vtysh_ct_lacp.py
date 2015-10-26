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

    def showLacpAggregates(self):
        info('''
########## Test Show lacp aggregates command ##########
''')
        lag_port_found_in_cmd = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 2')
        out = s1.cmdCLI('do show lacp aggregates')
        lines = out.split('\n')
        for line in lines:
            if 'lag2' in line:
                lag_port_found_in_cmd = True
        assert (lag_port_found_in_cmd is True), \
            'Test Show lacp aggregates command - FAILED!!'
        return True

    def deleteLagPort(self):
        info('''
########## Test to delete LAG port ##########
''')
        lag_port_found = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 3')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 10')
        s1.cmdCLI('lag 3')
        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 3')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if '"lag3"' in line:
                lag_port_found = False
        assert (lag_port_found is True), \
            'Test to delete LAG port - FAILED!'
        out = s1.cmd('do show running')
        lag_port_found = True
        lines = out.split('\n')
        for line in lines:
            if '"lag3"' in line:
                lag_port_found = False
        assert (lag_port_found is True), \
            'Test to delete LAG port - FAILED!'
        return True

    def addInterfacesToLags(self):
        info('''
########## Test to add interfaces to LAG ports ##########
''')
        interface_found_in_lag = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('lag 1')
        s1.cmdCLI('interface 2')
        s1.cmdCLI('lag 1')
        s1.cmdCLI('interface 3')
        s1.cmdCLI('lag 2')
        s1.cmdCLI('interface 4')
        s1.cmdCLI('lag 2')
        out = s1.cmdCLI('do show lacp aggregates')
        lines = out.split('\n')
        for line in lines:
            if 'Aggregated-interfaces' in line and '3' in line and '4' in line:
                interface_found_in_lag = True

        assert (interface_found_in_lag is True), \
            'Test to add interfaces to LAG ports - FAILED!'
        return True

    def globalLacpCommands(self):
        info('''
########## Test global LACP commands ##########
''')
        global_lacp_cmd_found = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lacp system-priority 999')
        out = s1.cmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-system-priority="999"' in line:
                global_lacp_cmd_found = True
        assert (global_lacp_cmd_found is True), \
            'Test global LACP commands - FAILED!'
        return True

    def lagContextCommands(self):
        info('''
########## Test LAG context commands ##########
''')
        lag_context_cmds_found = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('lacp mode active')
        s1.cmdCLI('lacp fallback')
        s1.cmdCLI('hash l2-src-dst')
        s1.cmdCLI('lacp rate fast')
        success = 0
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if re.search('lacp * : active', line) is not None:
                success += 1
                break
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-fallback-ab="true"' in line:
                success += 1
                break
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'bond_mode="l2-src-dst"' in line:
                success += 1
                break
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-time=fast' in line:
                success += 1
                break
        if success != 4:
            lag_context_cmds_found = False
        assert (lag_context_cmds_found is True), \
            'Test LAG context commands - FAILED!'

        success = 0

        # Test "no" forms of commands

        s1.cmdCLI('no lacp mode active')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'lacp=active' in line:
                success += 1
                break
        s1.cmdCLI('no lacp fallback')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-fallback-ab' in line:
                success += 1
                break
        s1.cmdCLI('no hash l2-src-dst')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'bond_mode="l2-src-dst"' in line:
                success += 1
                break
        s1.cmdCLI('no lacp rate fast')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-time' in line:
                success += 1
                break
        if success != 0:
            lag_context_cmds_found = False
        assert (lag_context_cmds_found is True), \
            'Test LAG context commands - FAILED!'
        return True

    def interfaceContext(self):
        info('''
########## Test interface context commands ##########
''')
        interface_context_cmds_found = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('lacp port-id 999')
        success = 0
        out = s1.cmd('ovs-vsctl list interface 1')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-port-id="999"' in line:
                success += 1
        s1.cmdCLI('lacp port-priority 111')
        out = s1.cmd('ovs-vsctl list interface 1')
        lines = out.split('\n')
        for line in lines:
            if 'lacp-port-priority="111"' in line:
                success += 1

        if success != 2:
            interface_context_cmds_found = False
        assert (interface_context_cmds_found is True), \
            'Test interface context commands - FAILED!'
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

    def test_showLacpAggregates(self):
        if self.test.showLacpAggregates():
            info('''
########## Test Show lacp aggregates command - SUCCESS! ##########
''')

    def test_deleteLagPort(self):
        if self.test.deleteLagPort():
            info('''
########## Test to delete LAG port - SUCCESS! ##########
''')

    def test_addInterfacesToLags(self):
        if self.test.addInterfacesToLags():
            info('''
########## Test to add interfaces to LAG ports - SUCCESS! ##########
''')

    def test_globalLacpCommands(self):
        if self.test.globalLacpCommands():
            info('''
########## Test global LACP commands - SUCCESS! ##########
''')

    def test_lagContextCommands(self):
        if self.test.lagContextCommands():
            info('''
########## Test LAG context commands - SUCCESS! ##########
''')

    def test_interfaceContext(self):
        if self.test.interfaceContext():
            info('''
########## Test interface context commands - SUCCESS! ##########
''')

    def teardown_class(cls):
        Test_lacp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
