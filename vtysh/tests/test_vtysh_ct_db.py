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
import pytest
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class DBTests(OpsVsiTest):

    def setupNet(self):

    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls
    # self.setSwitchCliCountOpts(2)

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        db_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(db_topo, switch=VsiOpenSwitch,
                       host=Host, link=OpsVsiLink,
                       controller=None, build=True)

    def createdbTest(self):
        print '\n========================================================='
        print '***            Test method to create db                 ***'
        print '==========================================================='
        s1 = self.net.switches[0]
        s1.ovscmd('/usr/bin/ovs-vsctl add-br br0')

        for i in range(1, 4095):
            ovsout = s1.ovscmd('/usr/bin/ovs-vsctl add-vlan br0 ' + str(i))
        return True

    def retrievedbTest(self):
        print '\n========================================================='
        print '***          Test method to retrieve db                 ***'
        print '==========================================================='
        s1 = self.net.switches[0]
        ovsout = s1.cmdCLI('show running-config')
        print ovsout
        return True

    def killDBTest(self):
        print '\n============================================================'
        print '***          Test to kill DB and verify CLI                ***'
        print '=============================================================='
        s1 = self.net.switches[0]
        s1.cmdCLI('conf t')
        out = s1.cmd('ps -aux')
        lines = out.split('\n')
        pid = 0
        for line in lines:
            if '/usr/sbin/ovsdb' in line:
                words = line.split(' ')
                words = filter(None, words)
                pid = words[1]
        s1.cmd('kill -9 ' + pid)
        out = s1.cmdCLI('feature lldp')
        if 'Command failed' in out:
            s1.cmdCLI('end')
            return True
        else:
            s1.cmdCLI('end')
            return False

    def disablelldptxdirTest(self):
        print '\n=============================================================='
        print '*** Test to verify show running-config for lldp transmission ***'
        print '================================================================'
        s1 = self.net.switches[0]
        ovsout = s1.ovscmd('/usr/bin/ovs-vsctl list interface 1')
        if '_uuid' not in ovsout:
            print '\nUnable to find Interface 1 in OVSBD'
            return False
        out = s1.cmdCLI('configure terminal')
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI('interface 1')
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI('no lldp transmission')
        if 'Unknown command' in out:
            print out
            return False
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'no lldp transmission' in line:
                return True
        return False

@pytest.mark.skipif(True, reason="Takes too long and not needed for CIT")
# This test was mainly needed for performance analysis of DB.
# As this is not testing any functionality, skipping this file from CIT execution.
class Test_db:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_db.test = DBTests()

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
            assert 0, 'Failed createdbTest'

    def test_retrieve_db(self):
        if self.test.retrievedbTest():
            print 'Passed retrievedbTest'
        else:
            assert 0, 'Failed retrievedbTest'

    def test_kill_db(self):
        if self.test.killDBTest():
            print 'Passed killDBTest'
        else:
            assert 0, 'Failed killDBTest'
