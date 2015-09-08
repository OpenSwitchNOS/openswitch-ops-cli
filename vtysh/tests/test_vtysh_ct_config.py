#!/usr/bin/python
# -*- coding: utf-8 -*-

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

from time import sleep
from halonvsi.docker import *
from halonvsi.halon import *


class ShowRunningConfigTests(HalonTest):

    def setupNet(self):

    # if you override this function, make sure to
    # either pass getNodeOpts() into hopts/sopts of the topology that
    # you build or into addHost/addSwitch calls

        self.net = Mininet(
            topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                  sopts=self.getSwitchOpts()),
            switch=HalonSwitch,
            host=HalonHost,
            link=HalonLink,
            controller=None,
            build=True,
            )

    def enablelldpTest(self):
        print '''
########## Test to verify show running-config for feature lldp ##########
'''
        enable_lldp = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('feature lldp')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'feature lldp' in line:
                enable_lldp = True
        assert enable_lldp == True, \
            'Test to verify show running-config for feature lldp - FAILED!'
        return True

    def setlldpholdtimeTest(self):
        print '''
########## Test to verify show running-config for lldp holdtime ##########
'''
        set_lldp_hold_time = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('lldp holdtime 5')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'lldp holdtime 5' in line:
                set_lldp_hold_time = True
        assert set_lldp_hold_time == True, \
            'Test to verify show running-config for lldp holdtime - FAILED!'
        return True

    def disablelldptxdirTest(self):
        print '''
########## Test to verify show running-config for lldp transmission ##########
'''
        lldp_txrx_disabled = False
        s1 = self.net.switches[0]
        ovsout = s1.cmd('/usr/bin/ovs-vsctl list interface 1')
        assert '_uuid' in ovsout, 'Unable to find Interface 1 in OVSDB'
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('interface 1')
        out = s1.cmdCLI('no lldp transmission')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'no lldp transmission' in line:
                lldp_txrx_disabled = True
        assert lldp_txrx_disabled == True, \
            'Test to verify show running-config for lldp transmission - FAILED!'
        return True

    def setLogrotatePeriodTest(self):
        print '''
########## Test to verify show running-config for logrotate period ##########
'''
        logrotate_period_set = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')

        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate period' not in line, \
                'Default behavior: logrotate period should not be part of running config'

        s1.cmdCLI('logrotate period none')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate period none' not in line, \
                'Default behavior: logrotate period none should not be part of running config'

        s1.cmdCLI('logrotate period hourly')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate period hourly' in line:
                logrotate_period_set = True
        assert logrotate_period_set == True, \
            'Test to verify show running-config for logrotate period'
        return True

    def setLogrotateMaxsizeTest(self):
        print '''
########## Test to verify show running-config for logrotate maxsize ##########
'''
        logrotate_max_size_set = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate maxsize' not in line, \
                'Default behavior: logrotate maxsize should not be part of running config'

        s1.cmdCLI('logrotate maxsize 10')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate maxsize 10' not in line, \
                'Default behavior: logrotate maxsize 10 should not be part of running config'

        s1.cmdCLI('logrotate maxsize 20')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate maxsize 20' in line:
                logrotate_max_size_set = True
        assert logrotate_max_size_set == True, \
            'Test to verify show running-config for logrotate maxsize - FAILED!'
        return True

    def setLogrotateTargetTest(self):
        print '''
########## Test to verify show running-config for logrotate target ##########
'''
        logrotate_target_set = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate target' not in line, \
                'Default behavior: logrotate target should not be part of running config'

        s1.cmdCLI('logrotate target local')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate target local' not in line, \
                'Default behavior: logrotate target local should not be part of running config'

        s1.cmdCLI('logrotate target tftp://1.1.1.1')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate target tftp://1.1.1.1' in line:
                logrotate_target_set = True
        assert logrotate_target_set == True, \
            'Test to verify show running-config for logrotate target - FAILED!'
        return True


class Test_showrunningconfig:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_showrunningconfig.test = ShowRunningConfigTests()

  # show running config tests.

    def test_enable_lldp_commands(self):
        if self.test.enablelldpTest():
            print '''
########## Test to verify show running-config for feature lldp - SUCCESS! ##########
'''

    def test_set_lldpholdtime_commands(self):
        if self.test.setlldpholdtimeTest():
            print '''
########## Test to verify show running-config for lldp holdtime - SUCCESS! ##########
'''

    def test_disable_lldptxdir_commands(self):
        if self.test.disablelldptxdirTest():
            print '''
########## Test to verify show running-config for lldp transmission - SUCCESS! ##########
'''

    def test_set_logrotatePeriod(self):
        if self.test.setLogrotatePeriodTest():
            print '''
########## Test to verify show running-config for logrotate period - SUCCESS! ##########
'''

    def test_set_logrotateMaxsize(self):
        if self.test.setLogrotateMaxsizeTest():
            print '''
########## Test to verify show running-config for logrotate maxsize - SUCCESS! ##########
'''

    def test_set_logrotateTarget(self):
        if self.test.setLogrotateTargetTest():
            print '''
########## Test to verify show running-config for logrotate target - SUCCESS! ##########
'''

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
