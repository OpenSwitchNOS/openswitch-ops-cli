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

import os
import sys
from time import sleep
import pytest
import subprocess
from halonvsi.docker import *
from halonvsi.halon import *

class PlatformLedTests( HalonTest ):
    uuid = ""

    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=HalonSwitch, host=HalonHost,
                           link=HalonLink, controller=None,
                           build=True)

    def initLedTable(self):
        # Add dummy data for LED in subsystem and led table for simulation.
        # Assume there would be only one entry in subsystem table
        s1 = self.net.switches[ 0 ]
        out = s1.cmd("ovs-vsctl list subsystem")
        lines = out.split('\n')
        for line in lines:
            if "_uuid" in line:
                _id = line.split(':')
                PlatformLedTests.uuid = _id[1].strip()
                s1.cmd("ovs-vsctl -- set Subsystem "+PlatformLedTests.uuid+" leds=@led1 -- --id=@led1 create led "
                "id=base1 state=flashing status=ok")


    def deinitLedTable(self):
        s1 = self.net.switches[ 0 ]
        # Delete dummy data from subsystem and led table to avoid clash with other CT scripts.
        s1.cmd("ovs-vsctl clear subsystem "+PlatformLedTests.uuid+" leds")


    def setLedTest(self):
        print('\n=====================================')
        print('*** Test to verify \'led\' command ***')
        print('=======================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI("led base1 on")
        if 'Cannot Find LED' in out:
            print out
            s1.cmdCLI("exit")
            return False
        s1.cmdCLI("exit")
        out = s1.cmd("ovs-vsctl list led base1")
        lines = out.split('\n')
        for line in lines:
            if "state" in line:
                if "on" in line:
                    return True
                else:
                    return False

        return False

    def showLedTest(self):
        print('\n=================================================')
        print('*** Test to verify \'show system led\' command ***')
        print('===================================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("show system led")
        if 'Unknown command' in out:
            print out
            return False
        lines = out.split('\n')
        for line in lines:
            if "base1" in line:
                if "on" in line:
                    return True
                elif "off" in line:
                    return True
                elif "flashing" in line:
                    return True
                else:
                    return False

        return False

    def noLedTest(self):
        print('\n=====================================')
        print('*** Test to verify \'no led\' command ***')
        print('=======================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI("no led base1")
        if 'Cannot Find LED' in out:
            print out
            s1.cmdCLI("exit")
            return False

        s1.cmdCLI("exit")
        out = s1.cmd("ovs-vsctl list led base1")
        lines = out.split('\n')
        for line in lines:
            if "state" in line:
                if "off" in line:
                    return True
                else:
                    return False

        return False

    def showRunningLedTest(self):
        print('\n=====================================')
        print('*** Test to verify show running-config command ***')
        print('=======================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI("led base1 on")
        if 'Cannot Find LED!!' in out:
            print out
            s1.cmdCLI("exit")
            return False

        s1.cmdCLI("exit")
        out = s1.cmdCLI("show running-config")
        lines = out.split('\n')
        for line in lines:
            if "led base1 on" in line:
                return True

        return False

class Test_led:
    test = PlatformLedTests()

    # Init test tables??
    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        # Initialize the led table with dummy value
        Test_led.test.initLedTable()

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

    # led <led name> on|off|flashing test.
    def test_led_command(self):
       if self.test.setLedTest():
           print 'Passed led Test'
       else:
           assert 0, "Failed led Test"

    # show system led test.
    def test_show_system_led_command(self):
        if self.test.showLedTest():
            print 'Passed show system led Test'
        else:
            assert 0, "Failed show system led Test"

    #no led <led name> test
    def test_no_led_command(self):
        if self.test.noLedTest():
            print 'Passed no led Test'
        else:
            assert 0, "Failed no led Test"

    #no led show running-config test
    def test_show_running_led_command(self):
        if self.test.showRunningLedTest():
            print 'Passed Show running-config Test'
        else:
            assert 0, "Failed Show running-config Test"
