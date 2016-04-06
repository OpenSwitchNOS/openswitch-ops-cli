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

        s1.cmdCLI('speed 1000')
        out = s1.cmdCLI('do show running-conf interface 2')
        assert 'speed 1000' in out, \
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
        s1.cmdCLI('split', False)
        s1.cmdCLI('y')
        out = s1.cmdCLI('do show running-config interface 49')
        assert 'split' in out, \
            'Test to verify interface configuration clis - FAILED!'
        out = s1.cmdCLI('end')
        return True

    def dynHelpStr_intfSpeedTest(self):
        print '''
########## Test to verify dynamic helpstr for interface speed cli  ##########
'''

        s1 = self.net.switches[0]
        s1.ovscmd('ovs-vsctl set interface 1 hw_intf_info:speeds="1000"')
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 1')
        out = s1.cmdCLI('speed ?')
        assert '1000    Mb/s supported' in out and \
            '10000   Mb/s not supported' in out and \
            '40000   Mb/s not supported' in out, \
            'Test to verify dyn helpstr for int speeds=1000 - FAILED!'
        out = s1.cmdCLI('end')

        s1.ovscmd('ovs-vsctl set interface 1 hw_intf_info:speeds="1000,10000"')
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 1')
        out = s1.cmdCLI('speed ?')
        assert '1000    Mb/s supported' in out and \
            '10000   Mb/s supported' in out and \
            '40000   Mb/s not supported' in out, \
            'Test to verify dyn helpstr for int speeds="1000,10000" - FAILED!'
        out = s1.cmdCLI('end')

        s1.ovscmd('ovs-vsctl set interface 1 hw_intf_info:speeds="40000"')
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 1')
        out = s1.cmdCLI('speed ?')
        assert '1000    Mb/s not supported' in out and \
            '10000   Mb/s not supported' in out and \
            '40000   Mb/s supported' in out, \
            'Test to verify dynamic helpstr for interface speed cli - FAILED!'
        out = s1.cmdCLI('end')
        return True

    def display_interface_in_numerical_order(self):
        s1 = self.net.switches[0]
        s1.cmdCLI('end')
        out = s1.cmdCLI('show interface brief')
        list_interface = out.split("\n")
        result_sortted = []
        result_orig = []
        for x in list_interface:
            test = re.search('\s+\d+ | \s+\d+-\d+', x)
            if test:
                test_number = test.group(0)
                if '-' in test_number:
                    number = test_number
                    number = number.replace('-', '.')
                    result_orig.append(float(number))
                    result_sortted.append(float(number))
                else:
                    result_orig.append(int(test_number))
                    result_sortted.append(int(test_number))

        result_sortted.sort()

        assert(result_orig == result_sortted), 'Test to display \
               interface in numerical order -FAILED'
        return True

    def dynHelpStr_intfmtuTest(self):
        print '''
########## Test to verify dynamic helpstr for mtu  ##########
'''

        s1 = self.net.switches[0]
        s1.ovscmd('ovs-vsctl set Subsystem base other_info:max_transmission_unit="2500"')
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 1')
        out = s1.cmdCLI('mtu ?')
        assert 'WORD  Enter MTU (in bytes) in the range <576-2500>' in out, \
            'Test to verify dyn helpstr for mtu 2500 - FAILED!'
        out = s1.cmdCLI('end')

        return True

    def showlacpPortPriority(self):
        info('''
########## Test show running-config interface Port Priority ##########
''')
        lag_interface = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface 2')
        s1.cmdCLI('lacp port-priority 1')
        s1.cmdCLI('exit')
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show running-config')
        if "lacp port-priority 1" in out:
            lag_interface = True
        assert (lag_interface is True), \
            'Test show running-config lacp port priority showed - FAIL!'

        return True

    def showRunInterfaceLag(self):
        info('''
########## Test show running-config interface with LAG ##########
''')
        lag_interface = False
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        s1.cmdCLI('interface lag 1')
        s1.cmdCLI('no routing')
        s1.cmdCLI('lacp mode active')
        s1.cmdCLI('hash l2-src-dst')
        s1.cmdCLI('lacp fallback')
        s1.cmdCLI('lacp rate fast')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 2')
        s1.cmdCLI('lag 1')
        s1.cmdCLI('lacp port-id 2')
        s1.cmdCLI('lacp port-priority 2')
        s1.cmdCLI('exit')
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show run interface')
        if "interface lag 1" in out and "no routing" in out \
            and "lacp mode active" in out \
            and "hash l2-src-dst" in out and "lacp fallback" in out \
            and "lacp rate fast" in out and "lacp port-id 2" in out \
            and "lacp port-priority 2" in out and "lag 1" in out:
            lag_interface = True
        assert (lag_interface is True), \
            'Test show running-config interface with LAG port - FAILED!'
        return True

@pytest.mark.skipif(True, reason="Disabling old tests")
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

    def test_dynHelpStr_intfSpeed(self):
        if self.test.dynHelpStr_intfSpeedTest():
            print '''
########## Test to verify dyn helpstr for int speed cli - SUCCESS! ##########
'''
    def test_display_interface_in_numerical_order(self):
        if self.test.display_interface_in_numerical_order():
            info('''
########## Test to verify that interface id displaying in numerical order \
##########\n ''')

    def test_dynHelpStr_intfmtu(self):
        if self.test.dynHelpStr_intfmtuTest():
            print '''
########## Test to verify dyn helpstr for mtu - SUCCESS! ##########
'''
    def showlacpPortPriority(self):
        if self.test.showlacpPortPriority():
            info('''
########## Test show running-config port priority default - SUCCESS! ##########
''')

    def test_showRunInterfaceLag(self):
        if self.test.showRunInterfaceLag():
            info('''
########## Test show running-config interface with LAG - SUCCESS! ##########
''')
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
