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

import pytest
from time import sleep
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
        print '''
########## Test method to create db ##########
'''
        s1 = self.net.switches[0]
        for j in range(1, 49):
            ovsout = \
                s1.ovscmd('/usr/bin/ovs-vsctl list interface ' + str(j))
            assert '_uuid' in ovsout, \
                'Unable to find interface ' + str(j) + 'in DB'

        s1.cmdCLI('configure terminal')
        for i in range(1, 49):
            s1.cmdCLI('interface ' + str(i))
            s1.cmdCLI('no shutdown')
            s1.cmdCLI('exit')
        return True

    def retrievedbTest(self):
        print '''
########## Test method to retrieve db ##########
'''
        s1 = self.net.switches[0]
        ovsout = s1.cmdCLI('do show running-config')
        ovssptout = ovsout.split('\n')
        for i in range(1, 49):
            lines = ovssptout
            linechk = 'interface ' + str(i)
            ret = False
            for line in lines:
                if linechk in line:
                    ret = True
                    break
            assert ret is True, 'Unable to find interface ' + str(i)

        return True


class Test_db:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_db.test = DBTests()

  # DB config tests.

    def test_create_db(self):
        if self.test.createdbTest():
            print '''
########## Test method to create db - PASSED ##########
'''

    def test_retrieve_db(self):
        if self.test.retrievedbTest():
            print '''
########## Test method to retrieve db - PASSED ##########
'''

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
