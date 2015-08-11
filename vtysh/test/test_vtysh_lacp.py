#!/usr/bin/python
#
# Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
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

from time import sleep
from halonvsi.docker import *
from halonvsi.halon import *

class LACPCliTest(HalonTest):

    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=HalonSwitch, host=HalonHost,
                           link=HalonLink, controller=None,
                           build=True)

    def createLagPort(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface lag 1")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if '\"lag1\"' in line:
                return True
        return False

    def showLacpAggregates(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface lag 2")
        out = s1.cmdCLI("do show lacp aggregates")
        lines = out.split('\n')
        for line in lines:
            if 'lag2' in line:
                return True
        return False

    def deleteLagPort(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface lag 3")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 10")
        s1.cmdCLI("lag 3")
        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 3")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if '\"lag3\"' in line:
                return False
        out = s1.cmd("do show running")
        lines = out.split('\n')
        for line in lines:
            if '\"lag3\"' in line:
                return False
        return True

    def addInterfacesToLags(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface 1")
        s1.cmdCLI("lag 1")
        s1.cmdCLI("interface 2")
        s1.cmdCLI("lag 1")
        s1.cmdCLI("interface 3")
        s1.cmdCLI("lag 2")
        s1.cmdCLI("interface 4")
        s1.cmdCLI("lag 2")
        out = s1.cmdCLI("do show lacp aggregates")
        lines = out.split('\n')
        for line in lines:
            if 'Aggregated-interfaces' in line and '3' in line and '4' in line:
                return True
        return False

    def globalLacpCommands(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lacp system-priority 999")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-system-priority=\"999\"' in line:
                return True
        return False

    def lagContextCommands(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("lacp mode active")
        s1.cmdCLI("lacp fallback")
        s1.cmdCLI("hash l2-src-dst")
        s1.cmdCLI("lacp rate fast")
        success = 0
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp=active' in line:
                success +=1
                break
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-fallback-ab=\"true\"' in line:
                success +=1
                break
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'bond_mode=\"l2-src-dst\"' in line:
                success +=1
                break
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-time=fast' in line:
                success +=1
                break
        if success != 4:
            return False

        success = 0
        #Test "no" forms of commands
        s1.cmdCLI("no lacp mode active")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp=active' in line:
                success +=1
                break
        s1.cmdCLI("no lacp fallback")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-fallback-ab' in line:
                success +=1
                break
        s1.cmdCLI("no hash l2-src-dst")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'bond_mode=\"l2-src-dst\"' in line:
                success +=1
                break
        s1.cmdCLI("no lacp rate fast")
        out = s1.cmd("ovs-vsctl list port")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-time' in line:
                success +=1
                break
        if success != 0:
            return False
        return True


    def interfaceContext(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("interface 1")
        s1.cmdCLI("lacp port-id 999")
        success = 0
        out = s1.cmd("ovs-vsctl list interface 1")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-port-id=\"999\"' in line:
                success +=1
        s1.cmdCLI("lacp port-priority 111")
        out = s1.cmd("ovs-vsctl list interface 1")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-port-priority=\"111\"' in line:
                success +=1
        s1.cmdCLI("lacp aggregation-key 333")
        out = s1.cmd("ovs-vsctl list interface 1")
        lines = out.split('\n')
        for line in lines:
            if 'lacp-aggregation-key=\"333\"' in line:
                success +=1

        if success !=3:
            return False
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
            print '\nPassed createLagPort test.'
        else:
            assert 0, 'Failed createLagPort test.'

    def test_showLacpAggregates(self):
        if self.test.showLacpAggregates():
            print '\nPassed showLacpAggregates test.'
        else:
            assert 0, 'Failed showLacpAggregates test.'

    def test_deleteLagPort(self):
        if self.test.deleteLagPort():
            print '\nPassed deleteLagPort test.'
        else:
            assert 0, 'Failed deleteLagPort test.'

    def test_addInterfacesToLags(self):
        if self.test.addInterfacesToLags():
            print '\nPassed addInterfacesToLags test.'
        else:
            assert 0, 'Failed addInterfacesToLags test.'

    def test_globalLacpCommands(self):
        if self.test.globalLacpCommands():
            print '\nPassed globalLacpCommands test.'
        else:
            assert 0, 'Failed globalLacpCommands test.'

    def test_lagContextCommands(self):
        if self.test.lagContextCommands():
            print '\nPassed lagContextCommands test.'
        else:
            assert 0, 'Failed lagContextCommands test.'

    def test_interfaceContext(self):
        if self.test.interfaceContext():
            print '\nPassed interfaceContext test.'
        else:
            assert 0, 'Failed interfaceContext test.'

    def teardown_class(cls):
        Test_lacp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
