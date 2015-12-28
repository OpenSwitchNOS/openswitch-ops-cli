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
from opsvsi.docker import *
from opsvsi.opsvsitest import *

class ShowRunningConfigTests(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        config_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(config_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def enablelldpTest(self):
        print '''
########## Test to verify show running-config for lldp enable ##########
'''
        enable_lldp = False
        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        out = s1.cmdCLI('lldp enable')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'lldp enable' in line:
                enable_lldp = True
        assert enable_lldp is True, \
            'Test to verify show running-config for lldp enable - FAILED!'
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
        assert set_lldp_hold_time is True, \
            'Test to verify show running-config for lldp holdtime - FAILED!'
        return True

    def disablelldptxdirTest(self):
        print '''
########## Test to verify show running-config for lldp transmission ##########
'''
        lldp_txrx_disabled = False
        s1 = self.net.switches[0]
        ovsout = s1.ovscmd('/usr/bin/ovs-vsctl list interface 1')
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
        assert lldp_txrx_disabled is True, \
            'Test to verify show running-config for ' \
            'lldp transmission - FAILED!'
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
                'Default behavior: logrotate period should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate period none')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate period none' not in line, \
                'Default behavior: logrotate period none should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate period hourly')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate period hourly' in line:
                logrotate_period_set = True
        assert logrotate_period_set is True, \
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
                'Default behavior: logrotate maxsize should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate maxsize 10')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate maxsize 10' not in line, \
                'Default behavior: logrotate maxsize 10 should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate maxsize 20')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate maxsize 20' in line:
                logrotate_max_size_set = True
        assert logrotate_max_size_set is True, \
            'Test to verify show running-config for ' \
            'logrotate maxsize - FAILED!'
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
                'Default behavior: logrotate target should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate target local')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        for line in lines:
            assert 'logrotate target local' not in line, \
                'Default behavior: logrotate target local should ' \
                'not be part of running config'

        s1.cmdCLI('logrotate target tftp://1.1.1.1')
        s1.cmdCLI(' ')
        s1.cmdCLI(' ')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        lines = out.split('\n')
        for line in lines:
            if 'logrotate target tftp://1.1.1.1' in line:
                logrotate_target_set = True
        assert logrotate_target_set is True, \
            'Test to verify show running-config for logrotate target - FAILED!'
        return True

    def setsessionTimeoutTest(self):
        print '''
########## Test to verify show running-config for session timeout #########
'''
        s1 = self.net.switches[0]
        s1.cmdCLI('configure terminal')
        out1 = s1.cmdCLI('session-timeout')
        out2 = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        assert 'session-timeout' not in out2 and \
            'Command incomplete' in out1, \
            'Test to configure session timeout - Failed!'

        s1.cmdCLI('configure terminal')
        s1.cmdCLI('session-timeout 5')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        assert 'session-timeout 5' in out, \
            'Test to configure session timeout - Failed!'
        return True

    def sessionTimeoutRangeTest(self):
        print '''
########## Test to verify show running-config for session timeout \
range test########
'''
        s1 = self.net.switches[0]
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('session-timeout -1')
        out = s1.cmdCLI('do show running-config')
        assert 'session-timeout 5' in out, \
            'Test to configure session timeout - Failed!'
        s1.cmdCLI('configure terminal')

        s1.cmdCLI('session-timeout 43201')
        out = s1.cmdCLI('do show running-config')
        assert 'session-timeout 5' in out, \
            'Test to configure session timeout - Failed!'

        s1.cmdCLI('session-timeout abc')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        assert 'session-timeout 5' in out, \
            'Test to configure session timeout - Failed!'
        return True

    def setdefaultSessionTimeoutTest(self):
        print '''
########## Test to verify show running-config for default session \
timeout #########
'''
        s1 = self.net.switches[0]
        s1.cmdCLI('configure terminal')
        s1.cmdCLI('no session-timeout')
        out = s1.cmdCLI('do show running-config')
        s1.cmdCLI('exit')
        assert 'session-timeout' not in out, \
            'Test to configure no session timeout - Failed!'
        return True

    def restoreConfigFromRunningConfig(self):
        print '''
########## Test to verify show running-config output used \
to restore configuration #########
'''
        s1 = self.net.switches[0]
        commands = ['vlan 900', 'no shutdown', 'interface lag 1', \
                    'no routing', 'vlan access 900', 'interface 1', \
                    'no shutdown', 'no routing', 'vlan access 900', \
                    'interface 2', 'no shutdown', 'lag 1']
        s1.cmdCLI('copy running-config startup-config')
        s1.cmdCLI('configure terminal')
        # Applying configuration for the first time
        for element in commands:
            s1.cmdCLI(element)

        # Saving first configuration
        firstOut = s1.cmdCLI('do show running-config')
        firstLines = firstOut.split('\n')

        # Erasing configuration
        s1.cmdCLI('do copy startup-config running-config')

        # Applying configuration with show running-config output
        for line in firstLines:
            s1.cmdCLI(line)

        # Saving configuration for the second time
        secondOut = s1.cmdCLI('do show running-config')
        secondLines = secondOut.split('\n')

        # Eliminating last element from list, this is the name
        # of the switch that the test should not validate
        del firstLines[-1]
        del secondLines[-1]

        # If list don't have the same elements mean that the
        # configuration was not properly applied
        assert sorted(firstLines) == sorted(secondLines), \
                'Test to restore config with output from show runnin-config - Failed!'

        # Cleaning configuration to not affect other tests
        s1.cmdCLI('do copy startup-config running-config')
        s1.cmdCLI('end')

        return True


class Test_showrunningconfig:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_showrunningconfig.test = ShowRunningConfigTests()

  # show running config tests.

    def test_restore_config(self):
        if self.test.restoreConfigFromRunningConfig():
            print '########## Test to verify show running-config ' \
                  'output used to restore configuration - SUCCESS! ##########'

    def test_enable_lldp_commands(self):
        if self.test.enablelldpTest():
            print '########## Test to verify show running-config ' \
                  'for lldp enable - SUCCESS! ##########'

    def test_set_lldpholdtime_commands(self):
        if self.test.setlldpholdtimeTest():
            print '########## Test to verify show running-config ' \
                  'for lldp holdtime - SUCCESS! ##########'

    def test_disable_lldptxdir_commands(self):
        if self.test.disablelldptxdirTest():
            print '########## Test to verify show running-config ' \
                  'for lldp transmission - SUCCESS! ##########'

    def test_set_logrotatePeriod(self):
        if self.test.setLogrotatePeriodTest():
            print '########## Test to verify show running-config ' \
                  'for logrotate period - SUCCESS! ##########'

    def test_set_logrotateMaxsize(self):
        if self.test.setLogrotateMaxsizeTest():
            print '########## Test to verify show running-config ' \
                  'for logrotate maxsize - SUCCESS! ##########'

    def test_set_logrotateTarget(self):
        if self.test.setLogrotateTargetTest():
            print '########## Test to verify show running-config ' \
                  'for logrotate target - SUCCESS! ##########'

    def test_set_sessionTimeout(self):
        if self.test.setsessionTimeoutTest():
            print '########## Test to verify show running-config ' \
                  'for setting session timeout - SUCCESS! ##########'

    def test_sessionTimeout_range(self):
        if self.test.sessionTimeoutRangeTest():
            print '########## Test to verify show running-config ' \
                  'for session timeout range check - SUCCESS! ##########'

    def test_set_default_sessionTimeout(self):
        if self.test.setdefaultSessionTimeoutTest():
            print '########## Test to verify show running-config ' \
                  'for no session timeout - SUCCESS! ##########'

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
