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
        print('***    Test to verify interface congfiguration clis     ***')
        print('===========================================================')
        s1 = self.net.switches[ 0 ]
        out = s1.cmdCLI("configure terminal")
        s1.cmdCLI("demo_cli to_be_hidden")
        out = s1.cmdCLI(" ");
        sleep(1);
        if 'Demo Cli executed' not in out:
        print out
                return False
        s1.cmdCLI("hide demo_cli level 2")
        out = s1.cmdCLI("demo_cli to_be_hidden")
        sleep(1);
        if 'Unknown command.' not in out:
        print out;
                return False
        s1.cmdCLI("hide demo_cli level 3")
        out = s1.cmdCLI("demo_cli to_be_hidden")
        sleep(1);
        if 'Unknown command.' not in out:
        print out;
                return False
        out = s1.cmdCLI("hide demo_cli level 0")
        out = s1.cmdCLI("end")
        return True;

class Test_vtyshInfraCommands:
  # Create the Mininet topology based on mininet.
  test = VtyshInfraCommandsTests()

  def setup(self):
    pass

  def teardown(self):
    pass

  def setup_class(cls):
    pass

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

  def hideCliCommandTest(self):
    if self.test.hideCliCommandTest():
      print 'Passed hideCliCommandTest'
    else:
      assert 0, "Failed hideCliCommandTest"
