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

class LLDPCliTest(HalonTest):

    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=HalonSwitch, host=HalonHost,
                           link=HalonLink, controller=None,
                           build=True)

    def enableLLDPFeatureTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("feature lldp")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                return True
        return False

    def disableLLDPFeatureTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("no feature lldp")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                return False
        return True

    def setLLDPholdtimeTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp holdtime 7")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold=\"7\"' in line:
                return True
        return False

    def setLLDPDefaultHoldtimeTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp holdtime 4")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold' in line:
                return False
        return True

    def setLLDPTimerTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp timer 100")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval=\"100\"' in line:
                return True
        return False

    def setLLDPDefaultTimerTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp timer 30")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval' in line:
                return False
        return True

    def setLLDPMgmtAddressTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp management-address 1.1.1.1")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr=\"1.1.1.1\"' in line:
                return True
        return False

    def unsetLLDPMgmtAddressTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("no lldp management-address")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr' in line:
                return False
        return True

    def setLLDPClearCountersTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp clear counters")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_counters_requested=\"1\"' in line:
                return True
        return False

    def setLLDPClearNeighborsTest(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp clear neighbors")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_table_requested=\"1\"' in line:
                return True
        return False


class Test_lldp_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_lldp_cli.test = LLDPCliTest()

    def test_enableLLDPFeatureTest(self):
        if self.test.enableLLDPFeatureTest():
            print '\nPassed enableLLDPFeatureTest'
        else:
            assert 0, 'Failed enableLLDPFeatureTest'

    def test_disableLLDPFeatureTest(self):
        if self.test.disableLLDPFeatureTest():
            print 'Passed disableLLDPFeatureTest'
        else:
            assert 0, 'Failed disableLLDPFeatureTest'

    def test_setLLDPholdtimeTest(self):
        if self.test.setLLDPholdtimeTest():
            print 'Passed setLLDPholdtimeTest'
        else:
            assert 0, 'Failed setLLDPholdtimeTest'

    def test_setLLDPDefaultHoldtimeTest(self):
        if self.test.setLLDPDefaultHoldtimeTest():
            print 'Passed setLLDPDefaultHoldtimeTest'
        else:
            assert 0, 'Failed setLLDPDefaultHoldtimeTest'

    def test_setLLDPTimerTest(self):
        if self.test.setLLDPTimerTest():
            print 'Passed setLLDPTimerTest'
        else:
            assert 0, 'Failed setLLDPTimerTest'

    def test_setLLDPDefaultHoldtimeTest(self):
        if self.test.setLLDPDefaultHoldtimeTest():
            print 'Passed setLLDPDefaultTimerTest'
        else:
            assert 0, 'Failed setLLDPDefaultTimerTest'

    def test_setLLDPMgmtAddressTest(self):
        if self.test.setLLDPMgmtAddressTest():
            print 'Passed setLLDPMgmtAddressTest'
        else:
            assert 0, 'Failed setLLDPMgmtAddressTest'

    def test_unsetLLDPMgmtAddressTest(self):
        if self.test.unsetLLDPMgmtAddressTest():
            print 'Passed unsetLLDPMgmtAddressTest'
        else:
            assert 0, 'Failed unsetLLDPMgmtAddressTest'

    def test_setLLDPClearCountersTest(self):
        if self.test.setLLDPClearCountersTest():
            print 'Passed setLLDPClearCountersTest'
        else:
            assert 0, 'Failed setLLDPClearCountersTest'

    def test_setLLDPClearNeighborsTest(self):
        if self.test.setLLDPClearNeighborsTest():
            print 'Passed setLLDPClearNeighborsTest'
        else:
            assert 0, 'Failed setLLDPClearNeighborsTest'

    def teardown_class(cls):
        Test_lldp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
