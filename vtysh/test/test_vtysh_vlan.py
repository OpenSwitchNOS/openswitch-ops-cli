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

class VLANCliTest(HalonTest):

    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=HalonSwitch, host=HalonHost,
                           link=HalonLink, controller=None,
                           build=True)

    def createVlan(self):
        info('\n########## Test to create VLAN ##########\n')
        vlan_created = False
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan 1" in line:
                vlan_created = True
        assert vlan_created == True,'Test to create VLAN - FAILED!'
        return True

    def showVlanSummary(self):
        s1 = self.net.switches[ 0 ]
        info('\n########## Test \"show vlan summary\" command ##########\n')
        vlan_summary_present = False
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 12")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 123")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 1234")
        s1.cmdCLI("exit")
        out = s1.cmdCLI("do show vlan summary")
        lines = out.split('\n')
        for line in lines:
            if "Number of existing VLANs: 4" in line:
                vlan_summary_present = True
        assert vlan_summary_present == True,'Test \"show vlan summary\" command - FAILED!'
        return True

    def deleteVlan(self):
        s1 = self.net.switches[ 0 ]
        info('\n########## Test to delete VLAN ##########\n')
        vlan_deleted = True
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 99")
        s1.cmdCLI("exit")
        s1.cmdCLI("no vlan 99")
        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan99" in line:
                vlan_deleted = False
        assert vlan_deleted == True,'Test to delete VLAN - FAILED!'
        return True

    def addAccessVlanToInterface(self):
        info('\n########## Test \"vlan access\" command ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 21")
        out = s1.cmdCLI("vlan access 1")
        success = 0;
        assert "Disable routing on the interface" in out,'Test \"vlan access\" command - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1" in line and "21" in line:
                success += 1

        assert 'success == 2','Test \"vlan access\" command - FAILED!'

        vlan_access_cmd_found = False
        s1.cmdCLI("no vlan access")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                vlan_access_cmd_found = True
        assert vlan_access_cmd_found == False,'Test \"vlan access\" command - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 21")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan access 1")
        assert "Can't configure VLAN. Interface is part of LAG" in out,'Test \"vlan access\" command - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        return True

    def addTrunkVlanToInterface(self):
        info('\n########## Test to add VLAN to interface ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 12")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        success = 0
        assert "Disable routing on the interface" in out,'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        assert "The interface is in access mode" in out,'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk allowed 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 1" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1" in line and "52-1" in line:
                success += 1
        assert success == 2,'Test to add VLAN to interface - FAILED!'

        vlan_trunk_allowed_cmd_found = True
        s1.cmdCLI("no vlan trunk allowed 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 1" in line:
                vlan_trunk_allowed_cmd_found = False

        assert vlan_trunk_allowed_cmd_found == True,'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-1")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        assert "Can't configure VLAN. Interface is part of LAG" in out,'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        return True

    def addTrunkNativeVlanToInterface(self):
        info('\n########## Test to add trunk native to interface ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 77")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-2")
        out = s1.cmdCLI("vlan trunk native 1")
        success = 0;
        assert "Disable routing on the interface" in out,'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk native 1")
        assert "The interface is in access mode" in out,'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk native 1")
        s1.cmdCLI("vlan trunk allowed 12")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native 1" in line:
                success += 1
            if "vlan trunk allowed 12" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1" in line and "52-2" in line:
                success += 1
            if "vlan77" in line and "52-2" in line:
                success += 1

        assert success == 4,'Test to add trunk native to interface - FAILED!'

        vlan_trunk_native_cmd_found = False
        s1.cmdCLI("no vlan trunk native")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native" in line:
                vlan_trunk_native_cmd_found = True
        assert vlan_trunk_native_cmd_found == False,'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-2")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk native 1")
        assert "Can't configure VLAN. Interface is part of LAG" in out,'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        return True

    def addTrunkNativeTagVlanToInterface(self):
        info('\n########## Test add trunk native tag vlan to interface ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1789")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 88")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-3")
        out = s1.cmdCLI("vlan trunk native tag")
        success = 0;
        assert "Disable routing on the interface" in out,'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1789")
        out = s1.cmdCLI("vlan trunk native tag")
        assert "The interface is in access mode" in out,'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk native 1789")
        s1.cmdCLI("vlan trunk allowed 88")
        s1.cmdCLI("vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native 1789" in line:
                success += 1
            if "vlan trunk allowed 88" in line:
                success += 1
            if "vlan trunk native tag" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1789" in line and "52-3" in line:
                success += 1
            if "vlan88" in line and "52-3" in line:
                success += 1

        assert success == 5,'Test add trunk native tag vlan to interface - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI("no vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native tag" in line:
                return True
        assert vlan_trunk_native_tag_present == False,'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-3")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk native tag")
        assert "Can't configure VLAN. Interface is part of LAG" in out,'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        return True

    def addAccessVlanToLAG(self):
        info('\n########## Test to add access vlan to LAG ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 21")
        out = s1.cmdCLI("vlan access 1")
        success = 0;
        assert "Disable routing on the LAG" in out,'Test to add access vlan to LAG - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1" in line and "lag21" in line:
                success += 1

        assert success == 2,'Test to add access vlan to LAG - FAILED!'
        vlan_access_cmd_present = False
        s1.cmdCLI("no vlan access")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                vlan_access_cmd_present = True

        assert vlan_access_cmd_present == False,'Test to add access vlan to LAG - FAILED!'
        return True

    def addTrunkVlanToLAG(self):
        info('\n########## Test to add trunk vlan to LAG ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 2345")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 55")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 31")
        out = s1.cmdCLI("vlan trunk allowed 55")
        success = 0;
        assert "Disable routing on the LAG" in out,'Test to add trunk vlan to LAG - FAILED'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 2345")
        out = s1.cmdCLI("vlan trunk allowed 55")
        assert "The LAG is in access mode" in out,'Test to add trunk vlan to LAG - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk allowed 55")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 55" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan55" in line and "lag31" in line:
                success += 1
        assert success == 2,'Test to add trunk vlan to LAG - FAILED!'
        vlan_trunk_allowed_cmd_present = False
        s1.cmdCLI("no vlan trunk allowed 55")
        s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 55" in line:
                vlan_trunk_allowed_cmd_present = True
        assert vlan_trunk_allowed_cmd_present == False,'Test to add trunk vlan to LAG - FAILED!'
        return True

    def addTrunkNativeVlanToLAG(self):
        info('\n########## Test to add trunk native vlan to LAG ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1234")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 66")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 41")
        out = s1.cmdCLI("vlan trunk native 1234")
        success = 0;
        assert "Disable routing on the LAG" in out,'Test to add trunk native vlan to LAG - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1234")
        out = s1.cmdCLI("vlan trunk native 1234")
        assert "The LAG is in access mode" in out,'Test to add trunk native vlan to LAG - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk native 1234")
        s1.cmdCLI("vlan trunk allowed 66")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native 1234" in line:
                success += 1
            if "vlan trunk allowed 66" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1234" in line and "lag41" in line:
                success += 1
            if "vlan66" in line and "lag41" in line:
                success += 1

        s1.cmdCLI("no vlan trunk native")
        out = s1.cmdCLI("do show running-config")

        assert success == 4,'Test to add trunk native vlan to LAG - FAILED!'
        return True

    def addTrunkNativeTagVlanToLAG(self):
        info('\n########## Test to add trunk native tag vlan to LAG ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1567")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 44")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 51")
        out = s1.cmdCLI("vlan trunk native tag")
        success = 0;
        assert "Disable routing on the LAG" in out,'Test to add trunk native tag vlan to LAG - FAILED!'

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk native tag")
        assert "The LAG is in access mode" in out,'Test to add trunk native tag vlan to LAG - FAILED!'

        s1.cmdCLI("no vlan access")
        s1.cmdCLI("vlan trunk native 1567")
        s1.cmdCLI("vlan trunk allowed 44")
        s1.cmdCLI("vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native 1567" in line:
                success += 1
            if "vlan trunk allowed 44" in line:
                success += 1
            if "vlan trunk native tag" in line:
                success += 1

        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan1567" in line and "lag51" in line:
                success += 1
            if "vlan44" in line and "lag51" in line:
                success += 1
        assert success == 5,'Test to add trunk native tag vlan to LAG - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI("no vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native tag" in line:
                vlan_trunk_native_tag_present = True
        assert vlan_trunk_native_tag_present == False,'Test to add trunk native tag vlan to LAG - FAILED!'
        return True

    def vlanCommands(self):
        info('\n########## Test to check VLAN commands ##########\n')
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("no shutdown")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        success = 0
        for line in lines:
            if "no shutdown" in line:
                success += 1

        s1.cmdCLI("description asdf")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "description asdf" in line:
                success += 1

        assert success == 2,'Test to check VLAN commands - FAILED!'
        return True

class Test_vlan_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_vlan_cli.test = VLANCliTest()

    def test_createVlan(self):
        if self.test.createVlan():
            info('\n########## Test to create VLAN - SUCCESS! ##########\n')

    def test_showVlanSummary(self):
        if self.test.showVlanSummary():
            info('\n########## Test \"show vlan summary\" command - SUCCESS! ##########\n')

    def test_deleteVlan(self):
        if self.test.deleteVlan():
            info('\n########## Test to delete VLAN - SUCCESS! ##########\n')

    def test_addAccessVlanToInterface(self):
        if self.test.addAccessVlanToInterface():
            info('\n########## Test \"vlan access\" command - SUCCESS! ##########\n')

    def test_addTrunkVlanToInterface(self):
        if self.test.addTrunkVlanToInterface():
            info('\n########## Test to add VLAN to interface - SUCCESS! ##########\n')

    def test_addTrunkNativeVlanToInterface(self):
        if self.test.addTrunkNativeVlanToInterface():
            info('\n########## Test to add trunk native to interface - SUCCESS! ##########\n')

    def test_addTrunkNativeTagVlanToInterface(self):
        if self.test.addTrunkNativeTagVlanToInterface():
            info('\n########## Test to add trunk native to interface - SUCCESS! ##########\n')

    def test_addAccessVlanToLAG(self):
        if self.test.addAccessVlanToLAG():
            info('\n########## Test add trunk native tag vlan to interface - SUCCESS! ##########\n')

    def test_addTrunkVlanToLAG(self):
        if self.test.addTrunkVlanToLAG():
            info('\n########## Test to add access vlan to LAG - SUCCESS! ##########\n')

    def test_addTrunkNativeVlanToLAG(self):
        if self.test.addTrunkNativeVlanToLAG():
            info('\n########## Test to add trunk native vlan to LAG - SUCCESS! ##########\n')

    def test_addTrunkNativeTagVlanToLAG(self):
        if self.test.addTrunkNativeTagVlanToLAG():
            info('\n########## Test to add trunk native vlan to LAG - SUCCESS!  ##########\n')

    def test_vlanCommands(self):
        if self.test.vlanCommands():
            info('\n########## Test to add trunk native tag vlan to LAG - SUCCESS! ##########\n')

    def teardown_class(cls):
        Test_vlan_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
