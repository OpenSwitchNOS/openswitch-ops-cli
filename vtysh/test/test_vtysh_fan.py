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

class FanSystemTests( HalonTest ):
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
    def initFanTable(self):
        # Add dummy data for fans in subsystem and fan table for simulation.
        # Assume there would be only one entry in subsystem table
        s1 = self.net.switches[ 0 ]
        print("\n")
        out = s1.cmd("ovs-vsctl list subsystem")
        lines = out.split('\n')
        for line in lines:
            if "_uuid" in line:
                _id = line.split(':')
                FanSystemTests.uuid = _id[1].strip()
                out=s1.cmd("/usr/bin/ovs-vsctl -- set Subsystem "+FanSystemTests.uuid+" fans=@fan1 -- --id=@fan1 create Fan "
                           "name=base-FAN-1L direction=f2b speed=normal status=ok rpm=9000")

    def deinitFanTable(self):
        s1 = self.net.switches[ 0 ]
        # Delete dummy data from subsystem and led table to avoid clash with other CT scripts.
        s1.cmd("ovs-vsctl clear subsystem "+FanSystemTests.uuid+" fans")
    def showSystemFanTest(self):
        # Test to verify show system command
        s1 = self.net.switches[ 0 ]
        print('\n########## Test to verify \'show system fan\' command ##########\n')
        fan_keywords_found = False
        out = s1.cmdCLI("show system fan")
        lines = out.split('\n')
        for line in lines:
            if 'base-FAN-1L' and 'front-to-back' and 'normal' and 'ok' and '9000' in line:
                fan_keywords_found = True
        assert fan_keywords_found == True,' Test to verify \'show system fan\' command - FAILED!'
        return True

    def setSystemFanSpeedTest(self):
        # Test to verify fan-speed command
        s1 = self.net.switches[ 0 ]
        print('\n########## Test to verify \'fan-speed\' command  ##########\n')
        fan_speed_set = False
        out = s1.cmdCLI("configure terminal")
        out = s1.cmdCLI("fan-speed slow")
        out = s1.cmdCLI("do show system fan")
        s1.cmdCLI("exit")
        lines = out.split('\n')
        for line in lines:
            if "Fan speed override is set to : slow" in line:
                fan_speed_set = True
        assert fan_speed_set == True,'Test to verify \'fan-speed\' command - FAILED!'
        return True

    def showrunningFanSpeed(self):
        # Test to verify if the fan-speed config is reflected in show running config
        s1 = self.net.switches[ 0 ]
        print('\n########## Test to verify \'show running\' command for fan-speed config ##########\n')
        fan_speed_keyword_found = False
        out = s1.cmdCLI("show running-config")
        lines = out.split('\n')
        for line in lines:
            if "fan-speed slow" in line:
                fan_speed_keyword_found = True
        assert fan_speed_keyword_found == True, 'Test to verify \'show running\' command for fan-speed config - FAILED!'
        return True

    def unsetSystemFanSpeedTest(self):
        # Test to verify no fan-speed command
        s1 = self.net.switches[ 0 ]
        print('\n########## Test to verify \'no fan-speed\' command ##########\n')
        fan_speed_unset = False
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI("no fan-speed")
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI("do show system fan")
        s1.cmdCLI("exit")
        lines = out.split('\n')
        for line in lines:
            if "Fan speed override is not configured" in line:
                fan_speed_unset = True
        assert fan_speed_unset == True, 'Test to verify \'no fan-speed\' command - FAILED!'
        return True

class Test_sys_fan:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        # Initialize the led table with dummy value
        Test_sys_fan.test = FanSystemTests()
        Test_sys_fan.test.initFanTable()

    # show system fan test.
    def test_show_system_fan_command(self):
       if self.test.showSystemFanTest():
           print '\n########## Test to verify \'show system fan\' command - SUCCESS! ##########\n'

    # set system fan speed test.
    def test_set_system_fan_speed_command(self):
        if self.test.setSystemFanSpeedTest():
            print '\n########## Test to verify \'fan-speed\' command - SUCCESS! ##########\n'

    # showrunningFanSpeed
    def test_show_run_for_fan_speed_config(self):
      if self.test.showrunningFanSpeed():
            print '\n########## Test to verify \'show running\' command for fan-speed config - SUCCESSS! ##########\n'

    # unset system fan speed test
    def test_unset_system_fan_speed_command(self):
        if self.test.unsetSystemFanSpeedTest():
            print '\n########## Test to verify \'no fan-speed\' command - SUCCESS! ##########\n'

    def teardown_class(cls):
        # Delete Dummy data to avoid clash with other test scripts
        Test_sys_fan.test.deinitFanTable()
        # Stop the Docker containers, and
        # mininet topology
        Test_sys_fan.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
