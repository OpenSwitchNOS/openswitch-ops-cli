#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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

import pytest
from time import sleep
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class InterfaceCommandsTests(OpsVsiTest):

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

    def interfaceConfigCliTest(self):
        print '''
########## Test to verify interface configuration clis  ##########
'''
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 2')
        out = s1.cmdCLI('mtu 2500')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'mtu 2500' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('speed 4000')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'speed 4000' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('duplex half')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'duplex half' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('autonegotiation off')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'autonegotiation off' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('flowcontrol send on')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'flowcontrol send on' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('flowcontrol receive on')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'flowcontrol receive on' in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('split')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'split' not in out, \
            'Test to verify interface configuration clis - FAILED!'

        s1.cmdCLI('interface 49')
        s1.cmdCLI('split')
        out = s1.cmdCLI('do show running-config interface 49')
        assert 'split' in out, \
            'Test to verify interface configuration clis - FAILED!'
        out = s1.cmdCLI('end')
        return True


class Test_interfaceCommands:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_interfaceCommands.test = InterfaceCommandsTests()

    def test_interfaceConfigCli(self):
        if self.test.interfaceConfigCliTest():
            print '''
########## Test to verify interface configuration clis - SUCCESS! ##########
'''

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
