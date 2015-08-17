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

class VtyshInfraCommandsTests( HalonTest ):

  def setupNet(self):
    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls
    self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                             sopts=self.getSwitchOpts()),
                       switch=HalonSwitch, host=HalonHost,
                       link=HalonLink, controller=None,
                       build=True)

  def hideCliCommandTest(self):
        print('\n=========================================================')
        print(  '***       Test to verify changes to CLI infra         ***')
        print(  '=========================================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        s1.cmdCLI("demo_cli to_be_hidden")
        out = s1.cmdCLI(" ")
        sleep(1)
        if 'Demo Cli executed' not in out:
            return False
        s1.cmdCLI("hide demo_cli level 2")
        out = s1.cmdCLI("demo_cli to_be_hidden")
        sleep(1)
        if 'Unknown command.' not in out:
            print out
            return False
        s1.cmdCLI("hide demo_cli level 3")
        out = s1.cmdCLI("demo_cli to_be_hidden")
        sleep(1)
        if 'Unknown command.' not in out:
            print out
            return False
        out = s1.cmdCLI("hide demo_cli level 0")
        out = s1.cmdCLI("end")
        return True

  def aliasCliCommandTest(self):
        print('\n=========================================================')
        print(  '***           Test to verify alias clis               ***')
        print(  '=========================================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        s1.cmdCLI("alias abc hostname MyTest")
        out = s1.cmdCLI("do show alias");
        sleep(1)
        if 'abc' not in out:
                print out
                assert 0, "Failed to get the alias"
                return False
        s1.cmdCLI("alias 12345678901234567890123456789012 demo_cli level 2")
        out = s1.cmdCLI("demo_cli to_be_hidden")
        sleep(1)
        if 'Max length exceeded' not in out:
                assert 0, "Failed to check max length"
                return False
        s1.cmdCLI("alias llht lldp holdtime $1; hostname $2")
        s1.cmdCLI("llht 6 TestHName")
        s1.cmdCLI("do show running")
        out = s1.cmdCLI("do show running")
        sleep(1)
        if 'hostname TestHName' not in out:
                assert 0, "Failed to check hostname in show running"
                return False
        if 'lldp holdtime 6' not in out:
                assert 0, "Failed to check lldp hostname"
                return False
        out = s1.cmdCLI("no hostname")
        out = s1.cmdCLI("no lldp hostname")
        out = s1.cmdCLI("do show running ")
        sleep(1)
        if 'alias llht lldp holdtime $1; hostname $2' not in out:
                assert 0, "Failed to check alias in show running"
                return False
        return True

@pytest.mark.skipif(True, reason="Does not work")
class Test_vtyshInfraCommands:

  def setup(self):
    pass

  def teardown(self):
    pass

  def setup_class(cls):
    Test_vtyshInfraCommands.test = VtyshInfraCommandsTests()

  def teardown_class(cls):
    # Stop the Docker containers, and
    # mininet topology
    Test_vtyshInfraCommands.test.net.stop()

  def setup_method(self, method):
    pass

  def teardown_method(self, method):
    pass

  def __del__(self):
    del self.test

  def test_hideCliCommand(self):
    if self.test.hideCliCommandTest():
      print 'Passed hideCliCommandTest'
    else:
      assert 0, "Failed hideCliCommandTest"

  def test_aliasCliCommand(self):
    if self.test.aliasCliCommandTest():
      print 'Passed aliasCliCommandTest'
    else:
      assert 0, "Failed aliasCliCommandTest"
