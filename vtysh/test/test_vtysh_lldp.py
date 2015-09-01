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
        info('\n########## Test to enable LLDP feature ##########\n')
        lldp_feature_enabled = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("feature lldp")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                lldp_feature_enabled = True
        assert lldp_feature_enabled == True, 'Test to enable LLDP feature - FAILED'
        return True

    def disableLLDPFeatureTest(self):
        info('\n########## Test to disable LLDP feature ##########\n')
        lldp_feature_enabled = True
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("no feature lldp")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                lldp_feature_enabled = False
        assert lldp_feature_enabled == True,'Test to disable LLDP feature - FAILED!'
        return True

    def setLLDPholdtimeTest(self):
        info('\n########## Test setting LLDP holdtime ##########\n')
        lldp_hold_time_set = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp holdtime 7")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold=\"7\"' in line:
                lldp_hold_time_set = True
        assert lldp_hold_time_set == True, 'Test setting LLDP holdtime - FAILED!'
        return True

    def setLLDPDefaultHoldtimeTest(self):
        info('\n########## Test setting LLDP default holdtime ##########\n')
        lldp_default_hold_time_set = True
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp holdtime 4")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold' in line:
                lldp_default_hold_time_set = False
        assert lldp_default_hold_time_set == True,'Test setting LLDP default holdtime - FAILED!'
        return True

    def setLLDPTimerTest(self):
        info('\n########## Test setting LLDP timer ##########\n')
        lldp_timer_set = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp timer 100")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval=\"100\"' in line:
                lldp_timer_set = True
        assert lldp_timer_set == True,'Test setting LLDP timer - FAILED!'
        return True

    def setLLDPDefaultTimerTest(self):
        info('\n########## Test setting default LLDP timer ##########\n')
        lldp_default_timer_set = True
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp timer 30")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval' in line:
                lldp_default_timer_set = False
        assert lldp_default_timer_set == True, 'Test setting default LLDP timer - FAILED!'
        return True

    def setLLDPMgmtAddressTest(self):
        info('\n########## Test setting LLDP management address ##########\n')
        lldp_mgmt_addr_set = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp management-address 1.1.1.1")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr=\"1.1.1.1\"' in line:
                lldp_mgmt_addr_set = True
        assert lldp_mgmt_addr_set == True, 'Test setting LLDP management address - FAILED!'
        return True

    def unsetLLDPMgmtAddressTest(self):
        info('\n########## Test unsetting LLDP management address ##########\n')
        lldp_mgmt_addr_set = True
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("no lldp management-address")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr' in line:
                lldp_mgmt_addr_set = False
        assert lldp_mgmt_addr_set == True, 'Test unsetting LLDP management address'
        return True

    def setLLDPClearCountersTest(self):
        info('\n########## Test LLDP clear counters ##########\n')
        lldp_clear_counters_set = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp clear counters")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_counters_requested=\"1\"' in line:
                lldp_clear_counters_set = True
        assert lldp_clear_counters_set == True,'Test LLDP clear counters - FAILED!'
        return True

    def setLLDPClearNeighborsTest(self):
        info('\n########## Test LLDP clear neighbors ##########\n')
        lldp_clear_neighbors_set = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("lldp clear neighbors")
        out = s1.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_table_requested=\"1\"' in line:
                lldp_clear_neighbors_set = True
        assert lldp_clear_neighbors_set == True, 'Test LLDP clear neighbors - FAILED!'
        return True


class Test_lldp_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_lldp_cli.test = LLDPCliTest()

    def test_enableLLDPFeatureTest(self):
        if self.test.enableLLDPFeatureTest():
            info('\n########## Test to enable LLDP feature - SUCCESS ##########\n')

    def test_disableLLDPFeatureTest(self):
        if self.test.disableLLDPFeatureTest():
            info('\n########## Test to disable LLDP feature - SUCCESS ##########\n')

    def test_setLLDPholdtimeTest(self):
        if self.test.setLLDPholdtimeTest():
            info('\n########## Test setting LLDP holdtime - SUCCESS ##########\n')

    def test_setLLDPDefaultHoldtimeTest(self):
        if self.test.setLLDPDefaultHoldtimeTest():
            info('\n########## Test setting LLDP default holdtime - SUCCESS ##########\n')

    def test_setLLDPTimerTest(self):
        if self.test.setLLDPTimerTest():
            info('\n########## Test setting LLDP timer - SUCCESS ##########\n')

    def test_setLLDPDefaultTimerTest(self):
        if self.test.setLLDPDefaultTimerTest():
            info('\n########## Test setting default LLDP timer - SUCCESS ##########\n')

    def test_setLLDPMgmtAddressTest(self):
        if self.test.setLLDPMgmtAddressTest():
            info('\n########## Test setting LLDP management address - SUCCESS ##########\n')

    def test_unsetLLDPMgmtAddressTest(self):
        if self.test.unsetLLDPMgmtAddressTest():
            info('\n########## Test unsetting LLDP management address - SUCCESS ##########\n')

    def test_setLLDPClearCountersTest(self):
        if self.test.setLLDPClearCountersTest():
            info('\n########## Test LLDP clear counters - SUCCESS ##########\n')

    def test_setLLDPClearNeighborsTest(self):
        if self.test.setLLDPClearNeighborsTest():
            info('\n########## Test LLDP clear neighbors - SUCCESS ##########\n')

    def teardown_class(cls):
        Test_lldp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
