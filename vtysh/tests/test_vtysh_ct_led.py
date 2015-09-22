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


class PlatformLedTests(HalonTest):

    uuid = ''

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

    def initLedTable(self):

        # Add dummy data for LED in subsystem and led table for simulation.
        # Assume there would be only one entry in subsystem table

        s1 = self.net.switches[0]
        out = s1.cmd('ovs-vsctl list subsystem')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                PlatformLedTests.uuid = _id[1].strip()
                s1.cmd('ovs-vsctl -- set Subsystem '
                       + PlatformLedTests.uuid
                       + ' leds=@led1 -- --id=@led1 create led '
                       + ' id=base1 state=flashing status=ok'
                       )

    def deinitLedTable(self):
        s1 = self.net.switches[0]

        # Delete dummy data from subsystem and led table to avoid
        # clash with other CT scripts.

        s1.cmd('ovs-vsctl clear subsystem ' + PlatformLedTests.uuid
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
        out = s1.cmd('ovs-vsctl list led base1')
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
        out = s1.cmd('ovs-vsctl list led base1')
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
