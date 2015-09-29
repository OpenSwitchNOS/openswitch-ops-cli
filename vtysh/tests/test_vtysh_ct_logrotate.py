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

import time
import pytest
from opsvsi.docker import *
from opsvsi.opsvsitest import *

script_path = '/etc/cron.hourly/ops-log-rotate'
logrotateCnfFile = '/etc/logrotate.ovs'
shLogrotateCnfFile = 'cat /etc/logrotate.ovs'


class LogrotateTests(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        logrotate_topo = SingleSwitchTopo(
            k=1, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(logrotate_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    @staticmethod
    def parseCLI(cliOutput):
        '''Parse the cli output'''

    def checkPattern(
        self,
        lines,
        value,
        fileName,
    ):
        for line in lines:
            if value in line:
                return True

        print 'Could not find ' + value + ' in' + fileName + '\n'
        return False

    def testLogrotateConfig(self, value):
        switch = self.net.switches[0]

        out = switch.cmd(script_path)
        lines = out.split('\n')
        for line in lines:
            if 'error' in line or 'Error' in line or 'ERROR' in line:
                print line

        out = switch.cmd(shLogrotateCnfFile)
        lines = out.split('\n')
        for line in lines:
            assert 'No such file' not in line,\
                   logrotateCnfFile + ' not generated\n'

        assert self.checkPattern(lines, value, logrotateCnfFile),\
            "Configuration file check: failed"
        return True

    def testLogrotation(self):
        switch = self.net.switches[0]

        out = switch.cmd('ls /var/log/messages*.gz')
        lines = out.split('\n')
        for line in lines:
            print line
            if 'No such file' in line:
                print 'No *.gz file. Logrotation failed\n'
                return False

        print 'Logrotation test:passed\n'
        return True

    def confLogrotateCliGetPeriod(self, switch):
        switch.cmdCLI('end')

        out = switch.cmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'period=hourly' in line:
                return True
        return False

    def confLogrotateCliGetMaxsize(self, switch):

        out = switch.cmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'maxsize="10"' in line:
                return True
        return False

    def confLogrotateCliGetTarget(self, switch):
        out = switch.cmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate_config' in line and 'target="tftp://1.1.1.1"' \
                    in line:
                return True
        return False

    def LogrotateCliPeriodTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI('conf t')
        switch.cmdCLI('logrotate period hourly')

        assert self.confLogrotateCliGetPeriod(switch),\
            "Test to set period: failed"
        info("### Test to set period: passed ###\n")
        return True

    def LogrotateCliMaxsizeTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI('conf t')
        switch.cmdCLI('logrotate maxsize 10')

        assert self.confLogrotateCliGetMaxsize(switch),\
            "Test to set maxsize: failed"
        info("### Test to set maxsize: passed ###\n")
        return True

    def LogrotateCliTargetTest(self):
        switch = self.net.switches[0]
        switch.cmdCLI('conf t')
        switch.cmdCLI('logrotate target tftp://1.1.1.1')

        assert self.confLogrotateCliGetTarget(switch),\
            "Test to set target: failed"
        info("### Test to set target: passed ###\n")
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
        switch.cmd('date --set=' + '"' + now + '"')


class Test_logrotate:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_logrotate.test = LogrotateTests()

    # Stop the Docker containers, and
    # mininet topology
    def teardown_class(cls):
        Test_logrotate.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test

  # Logrotate tests.

    def test_LogrotateDefaultConfig(self):
        info("\n########## Test to verify logrotate commands "
             "##########\n")
        if self.test.testLogrotateConfig('10M'):
            info("### Test default Config file: passed ###\n")

    def test_LogrotateCliPeriodTest(self):
        self.test.LogrotateCliPeriodTest()

    def test_LogrotateCliMaxsizeTest(self):
        self.test.LogrotateCliMaxsizeTest()

    def test_LogrotateCliTargetTest(self):
        self.test.LogrotateCliTargetTest()

    def test_LogrotateDBConfig(self):
        if self.test.testLogrotateConfig('hourly'):
            info("### Test config file generation from DB: passed ###\n")


#  @pytest.mark.skipif(True, \
#  reason="Modifies system clock. Needs to be fixed.")
#  def test_LogrotationPeriod(self):
#    self.test.testLogrotationPeriod()
