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
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan 1" in line:
                return True
        return False

    def showVlanSummary(self):
        s1 = self.net.switches[ 0 ]
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
                return True
        return False

    def deleteVlan(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 99")
        s1.cmdCLI("exit")
        s1.cmdCLI("no vlan 99")
        out = s1.cmdCLI("do show vlan")
        lines = out.split('\n')
        for line in lines:
            if "vlan99" in line:
                return False
        return True

    def addAccessVlanToInterface(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 21")
        out = s1.cmdCLI("vlan access 1")
        success = 0;
        if "Disable routing on the interface" not in out:
            return False

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

        s1.cmdCLI("no vlan access")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                return False

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 21")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan access 1")
        if "Can't configure VLAN. Interface is part of LAG" not in out:
            return False

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        if success == 2:
            return True
        return False

    def addTrunkVlanToInterface(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 12")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        success = 0
        if "Disable routing on the interface" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        if "The interface is in access mode" not in out:
            return False

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

        s1.cmdCLI("no vlan trunk allowed 1")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 1" in line:
                return False

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-1")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk allowed 1")
        if "Can't configure VLAN. Interface is part of LAG" not in out:
            return False

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        if success == 2:
            return True
        return False

    def addTrunkNativeVlanToInterface(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 77")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-2")
        out = s1.cmdCLI("vlan trunk native 1")
        success = 0;
        if "Disable routing on the interface" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk native 1")
        if "The interface is in access mode" not in out:
            return False

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

        s1.cmdCLI("no vlan trunk native")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native" in line:
                return False

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-2")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk native 1")
        if "Can't configure VLAN. Interface is part of LAG" not in out:
            return False

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        if success == 4:
            return True
        return False

    def addTrunkNativeTagVlanToInterface(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1789")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 88")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-3")
        out = s1.cmdCLI("vlan trunk native tag")
        success = 0;
        if "Disable routing on the interface" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1789")
        out = s1.cmdCLI("vlan trunk native tag")
        if "The interface is in access mode" not in out:
            return False

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

        s1.cmdCLI("no vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native tag" in line:
                return False

        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 52-3")
        s1.cmdCLI("lag 1")
        out = s1.cmdCLI("vlan trunk native tag")
        if "Can't configure VLAN. Interface is part of LAG" not in out:
            return False

        s1.cmdCLI("exit")
        s1.cmdCLI("no interface lag 1")
        if success == 5:
            return True
        return False

    def addAccessVlanToLAG(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 21")
        out = s1.cmdCLI("vlan access 1")
        success = 0;
        if "Disable routing on the LAG" not in out:
            return False

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

        s1.cmdCLI("no vlan access")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan access 1" in line:
                return False

        if success == 2:
            return True
        return False

    def addTrunkVlanToLAG(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 2345")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 55")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 31")
        out = s1.cmdCLI("vlan trunk allowed 55")
        success = 0;
        if "Disable routing on the LAG" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 2345")
        out = s1.cmdCLI("vlan trunk allowed 55")
        if "The LAG is in access mode" not in out:
            return False

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

        s1.cmdCLI("no vlan trunk allowed 55")
        s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk allowed 55" in line:
                return False

        if success == 2:
            return True
        return False

    def addTrunkNativeVlanToLAG(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1234")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 66")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 41")
        out = s1.cmdCLI("vlan trunk native 1234")
        success = 0;
        if "Disable routing on the LAG" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1234")
        out = s1.cmdCLI("vlan trunk native 1234")
        if "The LAG is in access mode" not in out:
            return False

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

        if success == 4:
            return True
        return False

    def addTrunkNativeTagVlanToLAG(self):
        s1 = self.net.switches[ 0 ]
        s1.cmdCLI("conf t")
        s1.cmdCLI("vlan 1567")
        s1.cmdCLI("exit")
        s1.cmdCLI("vlan 44")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface lag 51")
        out = s1.cmdCLI("vlan trunk native tag")
        success = 0;
        if "Disable routing on the LAG" not in out:
            return False

        s1.cmdCLI("no routing")
        s1.cmdCLI("vlan access 1")
        out = s1.cmdCLI("vlan trunk native tag")
        if "The LAG is in access mode" not in out:
            return False

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

        s1.cmdCLI("no vlan trunk native tag")
        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        for line in lines:
            if "vlan trunk native tag" in line:
                return False

        if success == 5:
            return True
        return False

    def vlanCommands(self):
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

        if success == 2:
            return True
        return False

class Test_vlan_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_vlan_cli.test = VLANCliTest()

    def test_createVlan(self):
        if self.test.createVlan():
            print '\nPassed createVlan test.'
        else:
            assert 0, 'Failed createVlan test.'

    def test_showVlanSummary(self):
        if self.test.showVlanSummary():
            print '\nPassed showVlanSummary test.'
        else:
            assert 0, 'Failed showVlanSummary test.'

    def test_deleteVlan(self):
        if self.test.deleteVlan():
            print '\nPassed deleteVlan test.'
        else:
            assert 0, 'Failed deleteVlan test.'

    def test_addAccessVlanToInterface(self):
        if self.test.addAccessVlanToInterface():
            print '\nPassed addAccessVlanToInterface test.'
        else:
            assert 0, 'Failed addAccessVlanToInterface test.'

    def test_addTrunkVlanToInterface(self):
        if self.test.addTrunkVlanToInterface():
            print '\nPassed addTrunkVlanToInterface test.'
        else:
            assert 0, 'Failed addTrunkVlanToInterface test.'

    def test_addTrunkNativeVlanToInterface(self):
        if self.test.addTrunkNativeVlanToInterface():
            print '\nPassed addTrunkNativeVlanToInterface test.'
        else:
            assert 0, 'Failed addTrunkNativeVlanToInterface test.'

    def test_addTrunkNativeTagVlanToInterface(self):
        if self.test.addTrunkNativeTagVlanToInterface():
            print '\nPassed addTrunkNativeTagVlanToInterface test.'
        else:
            assert 0, 'Failed addTrunkNativeTagVlanToInterface test.'

    def test_addAccessVlanToLAG(self):
        if self.test.addAccessVlanToLAG():
            print '\nPassed addAccessVlanToLAG test.'
        else:
            assert 0, 'Failed addAccessVlanToLAG test.'

    def test_addTrunkVlanToLAG(self):
        if self.test.addTrunkVlanToLAG():
            print '\nPassed addTrunkVlanToLAG test.'
        else:
            assert 0, 'Failed addTrunkVlanToLAG test.'

    def test_addTrunkNativeVlanToLAG(self):
        if self.test.addTrunkNativeVlanToLAG():
            print '\nPassed addTrunkNativeVlanToLAG test.'
        else:
            assert 0, 'Failed addTrunkNativeVlanToLAG test.'

    def test_addTrunkNativeTagVlanToLAG(self):
        if self.test.addTrunkNativeTagVlanToLAG():
            print '\nPassed addTrunkNativeTagVlanToLAG test.'
        else:
            assert 0, 'Failed addTrunkNativeTagVlanToLAG test.'

    def test_vlanCommands(self):
        if self.test.vlanCommands():
            print '\nPassed vlanCommands test.'
        else:
            assert 0, 'Failed vlanCommands test.'

    def teardown_class(cls):
        Test_vlan_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
