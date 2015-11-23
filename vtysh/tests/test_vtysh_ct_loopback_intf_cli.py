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

import pytest
from time import sleep
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class LoopbackInterfaceCommandsTests(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        intf_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(intf_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def createLoopbackInterface(self):
        info('''
########## Test to create Loopback Interface ##########
''')
        loopback_if_port_found = False
        loopback_if_interface_found = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface loopback 2')
        out = s1.cmd('ovs-vsctl list port')
        lines = out.split('\n')
        for line in lines:
            if '"lo2"' in line:
                loopback_if_port_found = True
        assert (loopback_if_port_found is True), \
            'Test to create Loopback Interface - FAILED!!'

        out = s1.cmd('ovs-vsctl list interface')
        lines = out.split('\n')
        for line in lines:
            if '"lo2"' in line:
                loopback_if_interface_found = True
        assert (loopback_if_interface_found is True), \
            'Test to create Loopback Interface - FAILED!!'
        return True

    def interfaceConfigCliTest(self):
        print '''
########## Test to verify loopback-interface configuration clis  ##########
'''
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface loopback 2')
        out = s1.cmdCLI('ip address 192.168.16.23/16')
        out = s1.cmdCLI('do show running-conf')

        assert 'ip address 192.168.16.23/16' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        s1.cmdCLI('ipv6 address 10:10::10:10/23')
        out = s1.cmdCLI('do show running-conf')
        assert 'ipv6 address 10:10::10:10/23' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        out = s1.cmdCLI('do show interface loopback 2')
        assert 'Interface lo2 is (null)' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        s1.cmdCLI('exit')

        out = s1.cmdCLI('interface loopback 21474836501')
        assert 'Unknown command.' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        s1.cmdCLI('interface loopback 3')

        out = s1.cmdCLI('ip address 0.0.0.0/23')
        assert 'Invalid IP address' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        out = s1.cmdCLI('ip address 255.255.255.255/23')
        assert 'Invalid IP address' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        out = s1.cmdCLI('ip address 192.168.16.23/16')
        assert 'Duplicate IP Address' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        out = s1.cmdCLI('exit')
        out = s1.cmdCLI('no interface loopback 4')

        assert 'Loopback interface does not exist' in out, \
            'Test to verify loopback-interface configuration clis - FAILED!'

        out = s1.cmdCLI('end')
        return True


class Test_interfaceCommands:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_interfaceCommands.test = LoopbackInterfaceCommandsTests()

    def test_createLoopbackInterface(self):
        if self.test.createLoopbackInterface():
            print'''
########## Test to create Loopback Interface - SUCCESS ##########
'''

    def test_interfaceConfigCli(self):
        if self.test.interfaceConfigCliTest():
            print '''
########## Test to verify loopback-interface configuration clis -
 SUCCESS! ########## '''

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_interfaceCommands.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
