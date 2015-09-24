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
            ovsout = s1.ovscmd('/usr/bin/ovs-vsctl list interface '
                            + str(j))
            assert '_uuid' in ovsout, '\nUnable to find interface ' \
                + str(j) + 'in DB'

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
            assert ret == True, '\nUnable to find interface ' + str(i)

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
