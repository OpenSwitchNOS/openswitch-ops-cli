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
import time

hosts = 0
switches = 2

class myTopo(Topo):

    '''
        Custom Topology Example
        S1[1][2]<--->[1][2]S2
    '''

    def build(self, hsts=0, sws=2, **_opts):
        self.sws = sws

        # Add list of switches
        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

        # Add links between nodes based on custom topo

        self.addLink('s1', 's2')
        self.addLink('s1', 's2')

class LACPCliTest(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        static_topo = myTopo(hsts=hosts, sws=switches, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(static_topo, switch=VsiOpenSwitch,
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


    def showInterfaceLagBrief(self):
        info('''
########## Test show interface lag brief command ##########
''')
        show_interface_brief = True
        show_interface_lag_brief = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')

        # Configure lag with undefined mode
        s1.cmdCLI('interface lag 3')
        s1.cmdCLI('exit')

        # Configure lag in passive mode
        s1.cmdCLI('interface lag 4')
        s1.cmdCLI('lacp mode passive')
        s1.cmdCLI('exit')

        # Configure lag in active mode
        s1.cmdCLI('interface lag 5')
        s1.cmdCLI('lacp mode active')
        s1.cmdCLI('exit')
        s1.cmdCLI('exit')

        # Verify show interface brief shows the lags created before
        success = 0
        out = s1.cmdCLI('show interface brief')
        # info('''%s \n''', out)
        lines = out.split('\n')
        for line in lines:
            if 'lag3         --      --  --      --     --                        auto     --' in line:
                success += 1
            if 'lag4         --      --  passive --     --                        auto     --' in line:
                success += 1
            if 'lag5         --      --  active  --     --                        auto     --' in line:
                success += 1
        if success != 3:
            show_interface_brief = False
        assert (show_interface_brief is True), \
            'Test show interface brief command - FAILED!'

        # Verify show interface lag4 brief shows only lag 4
        success = 0;
        out = s1.cmdCLI('show interface lag4 brief')
        lines = out.split('\n')
        for line in lines:
            if 'lag4         --      --  passive --     --                        auto     --' in line:
                success += 1
            if 'lag1' in line or 'lag2' in line or 'lag3' in line or 'lag5' in line:
                success -= 1
        if success != 1:
            show_interface_lag_brief = False
        assert (show_interface_lag_brief is True), \
            'Test show interface lag4 brief command - FAILED!'

        return True

    def showInterfaceLag(self):
        info('''
########## Test show interface lag command ##########
''')
        show_interface_lag1 = True
        show_interface_lag4 = True
        show_interface_lag5 = True
        s1 = self.net.switches[0]

        # Verify 'show interface lag1' shows correct  information about lag1
        success = 0;
        out = s1.cmdCLI('show interface lag1')
        lines = out.split('\n')
        for line in lines:
            if 'Aggregate-name lag1 ' in line:
                success += 1
            if 'Aggregated-interfaces' in line and '1' in line and '2' in line:
                success += 1
            if 'Aggregate mode' in line and 'off' in line:
                success += 1
            if 'Speed' in line and '0 Mb/s' in line:
                success += 1
        if success != 4:
            show_interface_lag1 = False
        assert (show_interface_lag1 is True), \
            'Test show interface lag1 command - FAILED!'

        # Verify 'show interface lag4' shows correct  information about lag4
        success = 0;
        out = s1.cmdCLI('show interface lag4')
        lines = out.split('\n')
        for line in lines:
            if 'Aggregate-name lag4 ' in line:
                success += 1
            if 'Aggregated-interfaces : ' in line:
                success += 1
            if 'Aggregate mode' in line and 'passive' in line:
                success += 1
            if 'Speed' in line and '0 Mb/s' in line:
                success += 1
        if success != 4:
            show_interface_lag4 = False
        assert (show_interface_lag4 is True), \
            'Test show interface lag4 command - FAILED!'

        # Verify 'show interface lag5' shows correct  information about lag5
        success = 0;
        out = s1.cmdCLI('show interface lag5')
        lines = out.split('\n')
        for line in lines:
            if 'Aggregate-name lag5 ' in line:
                success += 1
            if 'Aggregated-interfaces : ' in line:
                success += 1
            if 'Aggregate mode' in line and 'active' in line:
                success += 1
            if 'Speed' in line and '0 Mb/s' in line:
                success += 1
        if success != 4:
            show_interface_lag5 = False
        assert (show_interface_lag5 is True), \
            'Test show interface lag5 command - FAILED!'

        return True

    def testLACPConnectivity(self):

        s1 = self.net.switches[0]
        s2 = self.net.switches[1]

        s1SystemId = ''
        s2SystemId = ''

        # Getting System ID from both switches
        # After splitting all words from
        # 'show lacp configuration' command, the System ID
        # for this switch is store in index 5.
        # If the output for this command change in the future,
        # this needs to be changed
        out = s1.cmdCLI('show lacp configuration')
        s1SystemId = out.split()[5]
        out = s2.cmdCLI('show lacp configuration')
        s2SystemId = out.split()[5]

        # Configuring LAG 1 for S1
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('lacp mode active')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('no shutdown')
        s1.cmdCLI('interface 2')
        s1.cmdCLI('no shutdown')

        # Configuring LAG 1 for S2
        s2.cmdCLI('configure terminal')
        s2.cmdCLI('interface lag 1')
        s2.cmdCLI('lacp mode active')
        s2.cmdCLI('interface 1')
        s2.cmdCLI('no shutdown')
        s2.cmdCLI('lag 1')
        s2.cmdCLI('interface 2')
        s2.cmdCLI('lag 1')
        s2.cmdCLI('no shutdown')

        # Giving time to LAG to form correctly
        time.sleep(15)

        # Getting current info for LAG in both switches
        s1Out = s1.cmdCLI('do show lacp interface')
        s2Out = s2.cmdCLI('do show lacp interface')
        s1Lines = s1Out.split('\n')
        s2Lines = s2Out.split('\n')

        # If the LAG is correclty formed,
        # the systemID from the opposite switch
        # will appear in the command
        success = 0
        for line in s1Lines:
            if s2SystemId in line:
                success += 1

        for line in s2Lines:
            if s1SystemId in line:
                success += 1

        assert (success == 4), \
            'Test LACP connectivity - FAILED!'

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

    def test_showInterfaceLagBrief(self):
        if self.test.showInterfaceLagBrief():
            info('''
########## Test show interface lag brief command - SUCCESS! ##########
''')

    def test_showInterfaceLag(self):
        if self.test.showInterfaceLag():
            info('''
########## Test show interface lag command - SUCCESS! ##########
''')
    def test_showLACPConnectivity(self):
        if self.test.testLACPConnectivity():
            info ('''
########## Test LACP connectivity - SUCCESS! ##########
''')


    def teardown_class(cls):
        Test_lacp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
