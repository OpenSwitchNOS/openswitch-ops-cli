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
        print '\n=====================================' \
              '========================='
        print '*** Test to verify show running-config ' \
              'for lldp transmission ***'
        print '=======================================' \
              '========================='
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
# As this is not testing any functionality,
# skipping this file from CIT execution.
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
