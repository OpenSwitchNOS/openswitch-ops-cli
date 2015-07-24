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

class DBTests( HalonTest ):

  def setupNet(self):
    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls
    self.setSwitchCliCountOpts(2)
    self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                             sopts=self.getSwitchOpts()),
                       switch=HalonSwitch, host=HalonHost,
                       link=HalonLink, controller=None,
                       build=True)

  def createdbTest(self):
    print('\n=========================================================')
    print('***            Test method to create db                 ***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    for j in range(1, 49):
        ovsout = s1.cmd("/usr/bin/ovs-vsctl list interface " + str(j))
        if '_uuid' not in ovsout:
            assert 0, "\nUnable to find interface " + str(j) + "in DB"

    s1.cmdCLI("configure terminal")
    for i in range(1, 49):
        s1.cmdCLI("interface " + str(i), 0)
        s1.cmdCLI("no shutdown", 0)
        s1.cmdCLI("exit", 0)
    return True

  def retrievedbTest(self):
    print('\n=========================================================')
    print('***          Test method to retrieve db                 ***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    ovsout = s1.cmdCLI("show running-config", 1)
    print ovsout
    ovssptout = ovsout.split('\n')
    for i in range(1, 49):
        lines = ovssptout
        linechk = "interface " + str(i)
        ret = False
        for line in lines:
            if linechk in line:
                ret = True
                break
        if ret == False:
            print ("\nUnable to find interface "+str(i))
            return False
    return True

class Test_db:
  # Create the Mininet topology based on mininet.
  test = DBTests()

  def setup(self):
    pass

  def teardown(self):
    pass

  def setup_class(cls):
    pass

  def teardown_class(cls):
    # Stop the Docker containers, and
    # mininet topology
    Test_db.test.net.stop()

  def setup_method(self, method):
    pass

  def teardown_method(self, method):
    pass

  def __del__(self):
    del self.test

  # DB config tests.
  def test_create_db(self):
    if self.test.createdbTest():
      print 'Passed createdbTest'
    else:
      assert 0, "Failed createdbTest"

  def test_retrieve_db(self):
    if self.test.retrievedbTest():
      print 'Passed retrievedbTest'
    else:
      assert 0, "Failed retrievedbTest"
