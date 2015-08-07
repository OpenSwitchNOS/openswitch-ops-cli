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

class ShowRunningConfigTests( HalonTest ):

  def setupNet(self):
    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls
    self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                             sopts=self.getSwitchOpts()),
                       switch=HalonSwitch, host=HalonHost,
                       link=HalonLink, controller=None,
                       build=True)

  def enablelldpTest(self):
    print('\n=========================================================')
    print('*** Test to verify show running-config for feature lldp ***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    out = s1.cmdCLI("configure terminal")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("feature lldp")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "feature lldp" in line:
            return True
    return False

  def setlldpholdtimeTest(self):
    print('\n=========================================================')
    print('*** Test to verify show running-config for lldp holdtime***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    out = s1.cmdCLI("configure terminal")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("lldp holdtime 5")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "lldp holdtime 5" in line:
            return True
    return False

  def disablelldptxdirTest(self):
    print('\n==============================================================')
    print('*** Test to verify show running-config for lldp transmission ***')
    print('================================================================')
    s1 = self.net.switches[ 0 ]
    ovsout = s1.cmd("/usr/bin/ovs-vsctl list interface 1")
    if '_uuid' not in ovsout:
        print('\nUnable to find Interface 1 in OVSBD')
        return False
    out = s1.cmdCLI("configure terminal")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("interface 1")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("no lldp transmission")
    if 'Unknown command' in out:
        print out
        return False
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "no lldp transmission" in line:
            return True
    return False

  def setLogrotatePeriodTest(self):
    print('\n=========================================================')
    print('*** Test to verify show running-config for logrotate period***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    out = s1.cmdCLI("configure terminal")

    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate period" in line:
            print("Default behavior: logrotate period should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate period none")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate period none" in line:
            print("Default behavior: logrotate period none should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate period hourly")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "logrotate period hourly" in line:
            return True
    return False

  def setLogrotateMaxsizeTest(self):
    print('\n=========================================================')
    print('*** Test to verify show running-config for logrotate maxsize***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    out = s1.cmdCLI("configure terminal")
    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate maxsize" in line:
            print("Default behavior: logrotate maxsize should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate maxsize 10")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate maxsize 10" in line:
            print("Default behavior: logrotate maxsize 10 should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate maxsize 20")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "logrotate maxsize 20" in line:
            return True
    return False

  def setLogrotateTargetTest(self):
    print('\n=========================================================')
    print('*** Test to verify show running-config for logrotate target***')
    print('===========================================================')
    s1 = self.net.switches[ 0 ]
    out = s1.cmdCLI("configure terminal")
    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate target" in line:
            print("Default behavior: logrotate target should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate target local")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    lines = out.split('\n')
    for line in lines:
        if "logrotate target local" in line:
            print("Default behavior: logrotate target local should not be part of running config\n")
            return False

    s1.cmdCLI("logrotate target tftp://1.1.1.1")
    s1.cmdCLI(" ")
    s1.cmdCLI(" ")
    out = s1.cmdCLI("do show running-config")
    s1.cmdCLI("exit")
    lines = out.split('\n')
    for line in lines:
        if "logrotate target tftp://1.1.1.1" in line:
            return True
    return False

class Test_showrunningconfig:
  # Create the Mininet topology based on mininet.
  test = ShowRunningConfigTests()

  def setup(self):
    pass

  def teardown(self):
    pass

  def setup_class(cls):
    pass

  def teardown_class(cls):
    # Stop the Docker containers, and
    # mininet topology
    Test_showrunningconfig.test.net.stop()

  def setup_method(self, method):
    pass

  def teardown_method(self, method):
    pass

  def __del__(self):
    del self.test

  # show running config tests.
  def test_enable_lldp_commands(self):
    if self.test.enablelldpTest():
      print 'Passed enablelldpTest'
    else:
      assert 0, "Failed enablelldpTest"

  def test_set_lldpholdtime_commands(self):
    if self.test.setlldpholdtimeTest():
      print 'Passed setlldpholdtimeTest'
    else:
      assert 0, "Failed setlldpholdtimeTest"

  def test_disable_lldptxdir_commands(self):
    if self.test.disablelldptxdirTest():
      print 'Passed disablelldptxdirTest'
    else:
      assert 0, "disablelldptxdirTest"

  def test_set_logrotatePeriod(self):
    if self.test.setLogrotatePeriodTest():
      print 'Passed : running config logrotate period test'
    else:
      assert 0, "setLogrotatePeriodTest"

  def test_set_logrotateMaxsize(self):
    if self.test.setLogrotateMaxsizeTest():
      print 'Passed : running config logrotate maxsize test'
    else:
      assert 0, "setLogrotateMaxsizeTest"

  def test_set_logrotateTarget(self):
    if self.test.setLogrotateTargetTest():
      print 'Passed : running config logrotate target test'
    else:
      assert 0, "setLogrotateTargetTest"
