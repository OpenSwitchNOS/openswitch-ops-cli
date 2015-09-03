#!/usr/bin/env python

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import time
import pytest
from halonvsi.docker import *
from halonvsi.halon import *

script_path = "/etc/cron.hourly/log_rotate"
logrotateCnfFile = "/etc/logrotate.ovs"
shLogrotateCnfFile = "cat /etc/logrotate.ovs"

class LogrotateTests( HalonTest ):

    def setupNet(self):
    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls
        self.net = Mininet(topo=SingleSwitchTopo(
        k=1,
        hopts=self.getHostOpts(),
        sopts=self.getSwitchOpts()),
        switch=HalonSwitch,
        host=HalonHost,
        link=HalonLink, controller=None,
        build=True)

    @staticmethod
    def parseCLI(cliOutput):
        "Parse the cli output"

    def checkPattern(self,lines,value,fileName):
        for line in lines:
            if value in line:
                return True

        print ("Could not find "+ value + " in" + fileName + "\n")
        return False

    def testLogrotateConfig(self,value):
        switch = self.net.switches[0]

        out = switch.cmd(script_path)
        lines = out.split('\n')
        for line in lines:
            if 'error' in line or 'Error' in line or 'ERROR' in line:
                print line

        out = switch.cmd(shLogrotateCnfFile)
        lines = out.split('\n')
        for line in lines:
            if 'No such file' in line:
                print(logrotateCnfFile + " not generated\n")
                return False

        if not self.checkPattern(lines,value,logrotateCnfFile):
            print("Config file:check failed")
            return False
 #       if not self.checkPattern(lines,maxsize_value,logrotateCnfFile):
 #           print("Default config file: maxsize check failed")
 #           return False

        return True

    def testLogrotation(self):
        switch = self.net.switches[0]

#        out = switch.cmd(script_path)
#        lines = out.split('\n')

        out = switch.cmd("ls /var/log/messages*.gz")
        lines = out.split('\n')
        for line in lines:
            print line
            if 'No such file' in line:
                print("No *.gz file. Logrotation failed\n")
                return False

        print ("Logrotation test:passed\n")
        return True

    def confLogrotateCliGetPeriod(self, switch):
        switch.cmdCLI("end")
        #out = switch.cmdCLI("show logrotate")
        #out = switch.cmdCLI("exit")

        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'period=hourly' in line:
                return True
        return False

    def confLogrotateCliGetMaxsize(self, switch):
        #switch.cmdCLI("end")
        #out = switch.cmdCLI("show logrotate")
        #out = switch.cmdCLI("exit")

        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'maxsize=\"10\"' in line:
                return True
        return False

    def confLogrotateCliGetTarget(self, switch):
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'target=\"tftp://1.1.1.1\"' in line:
                return True
        return False


    def confLogrotateCliGetIP(self, switch):
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'ip=\"1.1.1.1\"' in line:
                return True
        return False

    def LogrotateCliPeriodTest(self):
        switch = self.net.switches[0]
        print('\n=========================================')
        print('*** Test to verify logrotate commands ***')
        print('=========================================')
        switch.cmdCLI("conf t")
        switch.cmdCLI("logrotate period hourly")

        if self.confLogrotateCliGetPeriod(switch):
                print('Set period: test passed\n')
                return True
        print('Set period: test failed\n')
        return False

    def LogrotateCliMaxsizeTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI("conf t")
        switch.cmdCLI("logrotate maxsize 10")

        if self.confLogrotateCliGetMaxsize(switch):
                print('Set maxsize: test passed\n')
                return True
        print('Set maxsize: test failed\n')
        return False

    def LogrotateCliTargetTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI("conf t")
        switch.cmdCLI("logrotate target tftp://1.1.1.1")

        if self.confLogrotateCliGetTarget(switch):
                print('Set target: test passed\n')
                return True
        print('Set target: test failed\n')
        return False

    def LogrotateCliIPTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI("conf t")
        switch.cmdCLI("logrotate target remote ip 1.1.1.1")

        if self.confLogrotateCliGetIP(switch):
                print('Set IP: test passed\n')
                return True
        print('Set IP: test failed\n')
        return False

    def LogrotateCompleteCliTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI("conf t")
        switch.cmdCLI("logrotate maxsize 10K period weekly target remote ip 1.1.1.1")

        if not self.confLogrotateCliGetPeriod(switch):
                print('Complete CLI test: set period failed\n')
                return False

        if not self.confLogrotateCliGetMaxsize(switch):
                print('Complete CLI test: set maxsize failed\n')
                return False

        if not self.confLogrotateCliGetTarget(switch):
                print('Complete CLI test: set target failed\n')
                return False

        print('Complete CLI test: passed\n')
        return True

    def testLogrotationPeriod(self):
        switch = self.net.switches[0]
        self.LogrotateCliPeriodTest()
        now = switch.cmd('date +"%F %T"')
        switch.cmd("date --set='2015-06-26 11:21:42'")
        self.testLogrotateConfig('hourly')
        switch.cmd("date --set='2015-06-26 12:21:42'")
        self.testLogrotateConfig('hourly')
        self.testLogrotation()
        switch.cmd('date --set=' + "\"" +now+"\"")

class Test_logrotate:

  def setup(self):
    pass

  def teardown(self):
    pass

  def setup_class(cls):
    Test_logrotate.test = LogrotateTests()

  def teardown_class(cls):
    # Stop the Docker containers, and
    # mininet topology
    Test_logrotate.test.net.stop()

  def setup_method(self, method):
    pass

  def teardown_method(self, method):
    pass

  def __del__(self):
    del self.test

  # Logrotate tests.

  def test_LogrotateDefaultConfig(self):
    if self.test.testLogrotateConfig('10M'):
        print("Test Default Config file: passed\n")

 # def test_LogrotationSize(self):
 #   self.test.LogrotateCliMaxsizeTest()
 #   self.test.testLogrotateConfig('10K')
 #   self.test.testLogrotation()

  def test_LogrotateCliPeriodTest(self):
    self.test.LogrotateCliPeriodTest()

  def test_LogrotateCliMaxsizeTest(self):
    self.test.LogrotateCliMaxsizeTest()

  def test_LogrotateCliTargetTest(self):
    self.test.LogrotateCliTargetTest()

#  def test_LogrotateCliIPTest(self):
#    self.test.LogrotateCliIPTest()

 # def test_LogrotateCompleteCliTest(self):
 #   self.test.LogrotateCompleteCliTest()

  def test_LogrotateDBConfig(self):
    if self.test.testLogrotateConfig('hourly'):
        print("Test DB Config file: passed\n")

#  @pytest.mark.skipif(True, reason="Modifies system clock. Needs to be fixed.")
#  def test_LogrotationPeriod(self):
#    self.test.testLogrotationPeriod()
