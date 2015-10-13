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


class VLANCliTest(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        vlan_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(vlan_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def createVlan(self):
        info('''
########## Test to create VLAN ##########
''')
        vlan_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan 1' in line:
                vlan_created = True
        assert (vlan_created is True), 'Test to create VLAN - FAILED!'
        return True

    def showVlanSummary(self):
        s1 = self.net.switches[0]
        info('''
########## Test "show vlan summary" command ##########
''')
        vlan_summary_present = False
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 12')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 123')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 1234')
        s1.cmdCLI('exit')
        out = s1.cmdCLI('do show vlan summary')
        lines = out.split('\n')
        for line in lines:
            if 'Number of existing VLANs: 4' in line:
                vlan_summary_present = True
        assert (vlan_summary_present is True), \
            'Test "show vlan summary" command - FAILED!'
        return True

    def deleteVlan(self):
        s1 = self.net.switches[0]
        info('''
########## Test to delete VLAN ##########
''')
        vlan_deleted = True
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 99')
        s1.cmdCLI('exit')
        s1.cmdCLI('no vlan 99')
        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan99' in line:
                vlan_deleted = False
        assert (vlan_deleted is True), 'Test to delete VLAN - FAILED!'
        return True

    def addAccessVlanToInterface(self):
        info('''
########## Test "vlan access" command ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 21')
        out = s1.cmdCLI('vlan access 1')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 1' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1' in line and '21' in line:
                success += 1

        assert 'success == 2', 'Test "vlan access" command - FAILED!'

        vlan_access_cmd_found = False
        s1.cmdCLI('no vlan access')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 1' in line:
                vlan_access_cmd_found = True
        assert (vlan_access_cmd_found is False), \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 21')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan access 1')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test "vlan access" command - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkVlanToInterface(self):
        info('''
########## Test to add VLAN to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 12')
        s1.cmdCLI('exit')
        # Split the parent interface to enable L2/L3 configurations
        # on child interfaces
        s1.cmdCLI('interface 52')
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        s1.cmdCLI('interface 52-1')
        out = s1.cmdCLI('vlan trunk allowed 1')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1')
        out = s1.cmdCLI('vlan trunk allowed 1')
        assert 'The interface is in access mode' in out, \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk allowed 1')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 1' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1' in line and '52-1' in line:
                success += 1
        assert success == 2, 'Test to add VLAN to interface - FAILED!'

        vlan_trunk_allowed_cmd_found = True
        s1.cmdCLI('no vlan trunk allowed 1')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 1' in line:
                vlan_trunk_allowed_cmd_found = False

        assert (vlan_trunk_allowed_cmd_found is True), \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-1')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk allowed 1')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test to add VLAN to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkNativeVlanToInterface(self):
        info('''
########## Test to add trunk native to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 77')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-2')
        out = s1.cmdCLI('vlan trunk native 1')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1')
        out = s1.cmdCLI('vlan trunk native 1')
        assert 'The interface is in access mode' in out, \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk native 1')
        s1.cmdCLI('vlan trunk allowed 12')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1' in line:
                success += 1
            if 'vlan trunk allowed 12' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1' in line and '52-2' in line:
                success += 1
            if 'vlan77' in line and '52-2' in line:
                success += 1

        assert success == 4, \
            'Test to add trunk native to interface - FAILED!'

        vlan_trunk_native_cmd_found = False
        s1.cmdCLI('no vlan trunk native')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native' in line:
                vlan_trunk_native_cmd_found = True
        assert (vlan_trunk_native_cmd_found is False), \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-2')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk native 1')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test to add trunk native to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addTrunkNativeTagVlanToInterface(self):
        info('''
########## Test add trunk native tag vlan to interface ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1789')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 88')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-3')
        out = s1.cmdCLI('vlan trunk native tag')
        success = 0
        assert 'Disable routing on the interface' in out, \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1789')
        out = s1.cmdCLI('vlan trunk native tag')
        assert 'The interface is in access mode' in out, \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk native 1789')
        s1.cmdCLI('vlan trunk allowed 88')
        s1.cmdCLI('vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1789' in line:
                success += 1
            if 'vlan trunk allowed 88' in line:
                success += 1
            if 'vlan trunk native tag' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1789' in line and '52-3' in line:
                success += 1
            if 'vlan88' in line and '52-3' in line:
                success += 1

        assert success == 5, \
            'Test add trunk native tag vlan to interface - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI('no vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native tag' in line:
                return True
        assert (vlan_trunk_native_tag_present is False), \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 52-3')
        s1.cmdCLI('lag 1')
        out = s1.cmdCLI('vlan trunk native tag')
        assert "Can't configure VLAN, interface is part of LAG" in out, \
            'Test add trunk native tag vlan to interface - FAILED!'

        s1.cmdCLI('exit')
        s1.cmdCLI('no interface lag 1')
        return True

    def addAccessVlanToLAG(self):
        info('''
########## Test to add access vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 21')
        out = s1.cmdCLI('vlan access 1')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add access vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 1' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1' in line and 'lag21' in line:
                success += 1

        assert success == 2, 'Test to add access vlan to LAG - FAILED!'
        vlan_access_cmd_present = False
        s1.cmdCLI('no vlan access')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan access 1' in line:
                vlan_access_cmd_present = True

        assert (vlan_access_cmd_present is False), \
            'Test to add access vlan to LAG - FAILED!'
        return True

    def addTrunkVlanToLAG(self):
        info('''
########## Test to add trunk vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 2345')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 55')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 31')
        out = s1.cmdCLI('vlan trunk allowed 55')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk vlan to LAG - FAILED'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 2345')
        out = s1.cmdCLI('vlan trunk allowed 55')
        assert 'The LAG is in access mode' in out, \
            'Test to add trunk vlan to LAG - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk allowed 55')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 55' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan55' in line and 'lag31' in line:
                success += 1
        assert success == 2, 'Test to add trunk vlan to LAG - FAILED!'
        vlan_trunk_allowed_cmd_present = False
        s1.cmdCLI('no vlan trunk allowed 55')
        s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk allowed 55' in line:
                vlan_trunk_allowed_cmd_present = True
        assert (vlan_trunk_allowed_cmd_present is False), \
            'Test to add trunk vlan to LAG - FAILED!'
        return True

    def addTrunkNativeVlanToLAG(self):
        info('''
########## Test to add trunk native vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1234')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 66')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 41')
        out = s1.cmdCLI('vlan trunk native 1234')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk native vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1234')
        out = s1.cmdCLI('vlan trunk native 1234')
        assert 'The LAG is in access mode' in out, \
            'Test to add trunk native vlan to LAG - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk native 1234')
        s1.cmdCLI('vlan trunk allowed 66')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1234' in line:
                success += 1
            if 'vlan trunk allowed 66' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1234' in line and 'lag41' in line:
                success += 1
            if 'vlan66' in line and 'lag41' in line:
                success += 1

        s1.cmdCLI('no vlan trunk native')
        out = s1.cmdCLI('do show running-config')

        assert success == 4, \
            'Test to add trunk native vlan to LAG - FAILED!'
        return True

    def addTrunkNativeTagVlanToLAG(self):
        info('''
########## Test to add trunk native tag vlan to LAG ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1567')
        s1.cmdCLI('exit')
        s1.cmdCLI('vlan 44')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface lag 51')
        out = s1.cmdCLI('vlan trunk native tag')
        success = 0
        assert 'Disable routing on the LAG' in out, \
            'Test to add trunk native tag vlan to LAG - FAILED!'

        s1.cmdCLI('no routing')
        s1.cmdCLI('vlan access 1')
        out = s1.cmdCLI('vlan trunk native tag')
        assert 'The LAG is in access mode' in out, \
            'Test to add trunk native tag vlan to LAG - FAILED!'

        s1.cmdCLI('no vlan access')
        s1.cmdCLI('vlan trunk native 1567')
        s1.cmdCLI('vlan trunk allowed 44')
        s1.cmdCLI('vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native 1567' in line:
                success += 1
            if 'vlan trunk allowed 44' in line:
                success += 1
            if 'vlan trunk native tag' in line:
                success += 1

        out = s1.cmdCLI('do show vlan')
        lines = out.split('\n')
        for line in lines:
            if 'vlan1567' in line and 'lag51' in line:
                success += 1
            if 'vlan44' in line and 'lag51' in line:
                success += 1
        assert success == 5, \
            'Test to add trunk native tag vlan to LAG - FAILED!'

        vlan_trunk_native_tag_present = False
        s1.cmdCLI('no vlan trunk native tag')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'vlan trunk native tag' in line:
                vlan_trunk_native_tag_present = True
        assert (vlan_trunk_native_tag_present is False), \
            'Test to add trunk native tag vlan to LAG - FAILED!'
        return True

    def vlanCommands(self):
        info('''
########## Test to check VLAN commands ##########
''')
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('vlan 1')
        s1.cmdCLI('no shutdown')
        out = s1.cmd('ovs-vsctl list vlan vlan1')
        lines = out.split('\n')
        success = 0
        for line in lines:
            if 'admin' in line:
                if 'up' in line:
                    success += 1

        s1.cmdCLI('description asdf')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'description asdf' in line:
                success += 1

        assert success == 2, 'Test to check VLAN commands - FAILED!'
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
            info('''
########## Test to create VLAN - SUCCESS! ##########
''')

    def test_showVlanSummary(self):
        if self.test.showVlanSummary():
            info('''
########## Test "show vlan summary" command - SUCCESS! ##########
''')

    def test_deleteVlan(self):
        if self.test.deleteVlan():
            info('''
########## Test to delete VLAN - SUCCESS! ##########
''')

    def test_addAccessVlanToInterface(self):
        if self.test.addAccessVlanToInterface():
            info('''
########## Test "vlan access" command - SUCCESS! ##########
''')

    def test_addTrunkVlanToInterface(self):
        if self.test.addTrunkVlanToInterface():
            info('''
########## Test to add VLAN to interface - SUCCESS! ##########
''')

    def test_addTrunkNativeVlanToInterface(self):
        if self.test.addTrunkNativeVlanToInterface():
            info('''
########## Test to add trunk native to interface - SUCCESS! ##########
''')

    def test_addTrunkNativeTagVlanToInterface(self):
        if self.test.addTrunkNativeTagVlanToInterface():
            info('''
########## Test to add trunk native to interface - SUCCESS! ##########
''')

    def test_addAccessVlanToLAG(self):
        if self.test.addAccessVlanToLAG():
            info('''
########## Test add trunk native tag vlan to interface - SUCCESS! ##########
''')

    def test_addTrunkVlanToLAG(self):
        if self.test.addTrunkVlanToLAG():
            info('''
########## Test to add access vlan to LAG - SUCCESS! ##########
''')

    def test_addTrunkNativeVlanToLAG(self):
        if self.test.addTrunkNativeVlanToLAG():
            info('''
########## Test to add trunk native vlan to LAG - SUCCESS! ##########
''')

    def test_addTrunkNativeTagVlanToLAG(self):
        if self.test.addTrunkNativeTagVlanToLAG():
            info('''
########## Test to add trunk native vlan to LAG - SUCCESS!  ##########
''')

    def test_vlanCommands(self):
        if self.test.vlanCommands():
            info('''
########## Test to add trunk native tag vlan to LAG - SUCCESS! ##########
''')

    def teardown_class(cls):
        Test_vlan_cli.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
