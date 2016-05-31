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

import os
import sys
from time import sleep
from pytest import mark
import subprocess
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class VtyshInfraCommandsTests(OpsVsiTest):

    def setupNet(self):

    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        infra_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(infra_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def aliasCliCommandTest(self):
        print '\n========================================================='
        print '***           Test to verify alias clis               ***'
        print '========================================================='
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        s1.cmdCLI('alias abc hostname MyTest')
        out = s1.cmdCLI('do show alias')
        sleep(1)
        if 'abc' not in out:
            print out
            assert 0, 'Failed to get the alias'
            return False
        out = \
            s1.cmdCLI('alias 123456789012345678901234567890123 '
                      'demo_cli level 2')
        if 'Max length exceeded' not in out:
            assert 0, 'Failed to check max length'
            return False
        s1.cmdCLI('alias llht lldp holdtime $1; hostname $2')
        s1.cmdCLI('llht 6 TestHName')
        out = s1.cmdCLI('do show running')
        if 'lldp holdtime 6' not in out:
            assert 0, 'Failed to check lldp hostname'
            return False
        out = s1.cmdCLI('no hostname')
        out = s1.cmdCLI('no lldp holdtime')
        out = s1.cmdCLI('do show running ')
        if 'alias llht lldp holdtime $1; hostname $2' not in out:
            assert 0, 'Failed to check alias in show running'
            return False
        return True

@mark.skipif(True, reason="Disabling test as modular FW test is enabled")
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

    def test_aliasCliCommand(self):
        if self.test.aliasCliCommandTest():
            print 'Passed aliasCliCommandTest'
        else:
            assert 0, 'Failed aliasCliCommandTest'
