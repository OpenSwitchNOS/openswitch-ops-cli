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

from time import sleep
from halonvsi.docker import *
from halonvsi.halon import *


class LLDPCliTest(HalonTest):

    uuid = ''

    def initLLDPNeighborinfo(self):
        s1 = self.net.switches[0]
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_capability_available=Bridge,Router \n '
                     )
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_description=Halon_0.1.0_basil_Linux_3.9.11_#1_SMP_Mon_Aug_24_14:38:01_UTC_2015_x86_64'
                     )
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_id=70:72:cf:fd:e9:26 \n'
                     )
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_id_len= 6 \n')
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_id_subtype=link_local_addr \n'
                     )
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_index=1 \n')
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_name=as5712 \n')
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_protocol=LLDP \n')
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_refcount=1 \n')
        out = s1.cmd('ovs-vsctl  set interface ' + LLDPCliTest.uuid
                     + ' lldp_neighbor_info:chassis_ttl=120 \n')
        time.sleep(1)

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        self.net = Mininet(
            topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                  sopts=self.getSwitchOpts()),
            switch=HalonSwitch,
            host=HalonHost,
            link=HalonLink,
            controller=None,
            build=True,
            )

    def enableLLDPFeatureTest(self):
        info('''
########## Test to enable LLDP feature ##########
''')
        lldp_feature_enabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('feature lldp')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                lldp_feature_enabled = True
        assert lldp_feature_enabled == True, \
            'Test to enable LLDP feature - FAILED'
        return True

    def disableLLDPFeatureTest(self):
        info('''
########## Test to disable LLDP feature ##########
''')
        lldp_feature_enabled = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('no feature lldp')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                lldp_feature_enabled = False
        assert lldp_feature_enabled == True, \
            'Test to disable LLDP feature - FAILED!'
        return True

    def setLLDPholdtimeTest(self):
        info('''
########## Test setting LLDP holdtime ##########
''')
        lldp_hold_time_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp holdtime 7')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold="7"' in line:
                lldp_hold_time_set = True
        assert lldp_hold_time_set == True, \
            'Test setting LLDP holdtime - FAILED!'
        return True

    def unsetLLDPholdtimeTest(self):
        s1 = self.net.switches[0]
        info('''
########## Test unsetting LLDP holdtime ##########
''')
        lldp_holdtime_set = False
        s1.cmdCLI('conf t')
        s1.cmdCLI('no lldp holdtime')
        out = s1.cmd('do show running')
        lines = out.split('\n')
        for line in lines:
            if 'lldp holdtime' in line:
                lldp_holdtime_set = True
        assert lldp_holdtime_set == False, \
            'Test unsetting LLDP holdtime - FAILED!'
        return True

    def setLLDPDefaultHoldtimeTest(self):
        info('''
########## Test setting LLDP default holdtime ##########
''')
        lldp_default_hold_time_set = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp holdtime 4')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold' in line:
                lldp_default_hold_time_set = False
        assert lldp_default_hold_time_set == True, \
            'Test setting LLDP default holdtime - FAILED!'
        return True

    def setLLDPTimerTest(self):
        info('''
########## Test setting LLDP timer ##########
''')
        lldp_timer_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp timer 100')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval="100"' in line:
                lldp_timer_set = True
        assert lldp_timer_set == True, \
            'Test setting LLDP timer - FAILED!'
        return True

    def unsetLLDPTimerTest(self):
        info('''
########## Test unsetting LLDP timer ##########
''')
        lldp_timer_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('no lldp timer')
        out = s1.cmd('do show running')
        lines = out.split('\n')
        for line in lines:
            if 'lldp timer' in line:
                lldp_timer_set = True
        assert lldp_timer_set == False, \
            'Test unsetting LLDP timer - FAILED!'
        return True

    def setLLDPDefaultTimerTest(self):
        info('''
########## Test setting default LLDP timer ##########
''')
        lldp_default_timer_set = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp timer 30')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval' in line:
                lldp_default_timer_set = False
        assert lldp_default_timer_set == True, \
            'Test setting default LLDP timer - FAILED!'
        return True

    def setLLDPMgmtAddressTest(self):
        info('''
########## Test setting LLDP management address ##########
''')
        lldp_mgmt_addr_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp management-address 1.1.1.1')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr="1.1.1.1"' in line:
                lldp_mgmt_addr_set = True
        assert lldp_mgmt_addr_set == True, \
            'Test setting LLDP management address - FAILED!'
        return True

    def unsetLLDPMgmtAddressTest(self):
        info('''
########## Test unsetting LLDP management address ##########
''')
        lldp_mgmt_addr_set = True
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('no lldp management-address')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr' in line:
                lldp_mgmt_addr_set = False
        assert lldp_mgmt_addr_set == True, \
            'Test unsetting LLDP management address'
        return True

    def setLLDPClearCountersTest(self):
        info('''
########## Test LLDP clear counters ##########
''')
        lldp_clear_counters_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp clear counters')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_counters_requested="1"' in line:
                lldp_clear_counters_set = True
        assert lldp_clear_counters_set == True, \
            'Test LLDP clear counters - FAILED!'
        return True

    def setLLDPClearNeighborsTest(self):
        info('''
########## Test LLDP clear neighbors ##########
''')
        lldp_clear_neighbors_set = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('lldp clear neighbors')
        out = s1.cmd('ovs-vsctl list open_vswitch')
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_table_requested="1"' in line:
                lldp_clear_neighbors_set = True
        assert lldp_clear_neighbors_set == True, \
            'Test LLDP clear neighbors - FAILED!'
        return True

    def LLDPNeighborsinfoTest(self):
        s1 = self.net.switches[0]
        info('''
########## Test LLDP neighbor info command ##########
''')
        counter = 0
        s1.cmdCLI('conf t')
        s1.cmdCLI('feature lldp')
        out = s1.cmd('ovs-vsctl list interface 1')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                LLDPCliTest.uuid = _id[1].strip()
                self.initLLDPNeighborinfo()
                out = s1.cmdCLI('do show lldp neighbor-info 1')
                lines = out.split('\n')
                for line in lines:
                    if 'Neighbor Chassis-Name          : as5712' \
                        in line:
                        counter += 1
                    if 'Neighbor Chassis-Description   : Halon_0.1.0_basil_Linux_3.9.11_#1_SMP_Mon_Aug_24_14:38:01_UTC_2015_x86_64' \
                        in line:
                        counter += 1
                    if 'Neighbor Chassis-ID            : 70:72:cf:fd:e9:26' \
                        in line:
                        counter += 1
                    if 'Chassis Capabilities Available : Bridge,Router' \
                        in line:
                        counter += 1
                assert counter == 4, \
                    'Test LLDP neighbor info command - FAILED!'
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
            info('''
########## Test to enable LLDP feature - SUCCESS ##########
''')

    def test_disableLLDPFeatureTest(self):
        if self.test.disableLLDPFeatureTest():
            info('''
########## Test to disable LLDP feature - SUCCESS ##########
''')

    def test_setLLDPholdtimeTest(self):
        if self.test.setLLDPholdtimeTest():
            info('''
########## Test setting LLDP holdtime - SUCCESS ##########
''')

    def test_unsetLLDPholdtimeTest(self):
        if self.test.unsetLLDPholdtimeTest():
            info('''
########## Test unsetting LLDP holdtime - SUCCESS ##########
''')

    def test_setLLDPDefaultHoldtimeTest(self):
        if self.test.setLLDPDefaultHoldtimeTest():
            info('''
########## Test setting LLDP default holdtime - SUCCESS ##########
''')

    def test_setLLDPTimerTest(self):
        if self.test.setLLDPTimerTest():
            info('''
########## Test setting LLDP timer - SUCCESS ##########
''')

    def test_unsetLLDTimerTest(self):
        if self.test.unsetLLDPholdtimeTest():
            info('''
########## Test unsetting LLDP timer - SUCCESS ##########
''')

    def test_setLLDPDefaultTimerTest(self):
        if self.test.setLLDPDefaultTimerTest():
            info('''
########## Test setting default LLDP timer - SUCCESS ##########
''')

    def test_setLLDPMgmtAddressTest(self):
        if self.test.setLLDPMgmtAddressTest():
            info('''
########## Test setting LLDP management address - SUCCESS ##########
''')

    def test_unsetLLDPMgmtAddressTest(self):
        if self.test.unsetLLDPMgmtAddressTest():
            info('''
########## Test unsetting LLDP management address - SUCCESS ##########
''')

    def test_setLLDPClearCountersTest(self):
        if self.test.setLLDPClearCountersTest():
            info('''
########## Test LLDP clear counters - SUCCESS ##########
''')

    def test_setLLDPClearNeighborsTest(self):
        if self.test.setLLDPClearNeighborsTest():
            info('''
########## Test LLDP clear neighbors - SUCCESS ##########
''')

    def test_LLDPNeighborsinfoTest(self):
        if self.test.LLDPNeighborsinfoTest():
            info('''
########## Test LLDP neighbor info command - SUCCESS ##########
''')

    def teardown_class(cls):
        Test_lldp_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
