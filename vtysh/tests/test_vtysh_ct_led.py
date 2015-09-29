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

from time import sleep
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class PlatformLedTests(OpsVsiTest):

    uuid = ''

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        led_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(led_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def initLedTable(self):

        # Add dummy data for LED in subsystem and led table for simulation.
        # Assume there would be only one entry in subsystem table

        s1 = self.net.switches[0]
        out = s1.ovscmd('ovs-vsctl list subsystem')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                PlatformLedTests.uuid = _id[1].strip()
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformLedTests.uuid
                          + ' leds=@led1 -- --id=@led1 create led '
                          + ' id=base1 state=flashing status=ok'
                          )

    def deinitLedTable(self):
        s1 = self.net.switches[0]

        # Delete dummy data from subsystem and led table to avoid
        # clash with other CT scripts.

        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformLedTests.uuid
                  + ' leds')

    def setLedTest(self):
        print '''
########## Test to verify \'led\' command ##########
'''
        s1 = self.net.switches[0]
        is_led_set = False
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('led base1 on')
        s1.cmdCLI('exit')
        out = s1.ovscmd('ovs-vsctl list led base1')
        lines = out.split('\n')
        for line in lines:
            if 'state' in line:
                if 'on' in line:
                    is_led_set = True
                    break
        assert is_led_set is True, \
            'Test to verify \'led\' command - FAILED!'
        return True

    def showLedTest(self):
        print '''
########## Test to verify \'show system led\' command ##########
'''
        s1 = self.net.switches[0]
        led_config_present = False
        out = s1.cmdCLI('show system led')
        lines = out.split('\n')
        for line in lines:
            if 'base1' in line:
                if 'on' in line:
                    led_config_present = True
                    break
                elif 'off' in line:
                    led_config_present = True
                    break
                elif 'flashing' in line:
                    led_config_present = True
                    break
                else:
                    led_config_present = False

        assert led_config_present is True, \
            'Test to verify \'show system led\' command - FAILED!'
        return True

    def noLedTest(self):
        print '''
########## Test to verify \'no led\' command  ##########
'''
        s1 = self.net.switches[0]
        led_state_off = False
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('no led base1')
        s1.cmdCLI('exit')
        out = s1.ovscmd('ovs-vsctl list led base1')
        lines = out.split('\n')
        for line in lines:
            if 'state' in line:
                if 'off' in line:
                    led_state_off = True
                    break
        assert led_state_off is True, \
            'Test to verify \'no led\' command - FAILED!'
        return True

    def showRunningLedTest(self):
        print '''
########## Test to verify show running-config command ##########
'''
        s1 = self.net.switches[0]
        led_config_present = False
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('led base1 on')
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show running-config')
        lines = out.split('\n')
        for line in lines:
            if 'led base1 on' in line:
                led_config_present = True
        assert led_config_present is True, \
            'Test to verify show running-config command - FAILED!'
        return True


class Test_led:

    # Init test tables??

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):

        # Initialize the led table with dummy value

        Test_led.test = PlatformLedTests()
        Test_led.test.initLedTable()

    # led <led name> on|off|flashing test.

    def test_led_command(self):
        if self.test.setLedTest():
            print '''
########## Test to verify \'led\' command - SUCCESS! ##########
'''

    # show system led test.

    def test_show_system_led_command(self):
        if self.test.showLedTest():
            print '''
########## Test to verify \'show system led\' command - SUCCESS! ##########
'''

    # no led <led name> test

    def test_no_led_command(self):
        if self.test.noLedTest():
            print '''
########## Test to verify \'no led\' command - SUCCESS! ##########
'''

    # no led show running-config test

    def test_show_running_led_command(self):
        if self.test.showRunningLedTest():
            print '''
########## Test to verify show running-config command - SUCCESS! ##########
'''

    def teardown_class(cls):

        # Delete Dummy data to avoid clash with other test scripts

        Test_led.test.deinitLedTable()

        # Stop the Docker containers, and
        # mininet topology

        Test_led.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
