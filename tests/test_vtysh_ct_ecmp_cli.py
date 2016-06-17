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

from opsvsi.docker import *
from opsvsi.opsvsitest import *


class ecmpCLITest(OpsVsiTest):

    def setupNet(self):
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=VsiOpenSwitch,
                           host=OpsVsiHost,
                           link=OpsVsiLink,
                           controller=None,
                           build=True)

    def test_ecmp_enable_disable(self):
        '''
            Test ECMP enable disable validations
        '''
        info('\n########## ECMP enable/disable validations ##########\n')
        s1 = self.net.switches[0]

        # Verify default state of ECMP is enabled
        ret = s1.cmdCLI("show ip ecmp")
        assert 'ECMP Status        : Enabled' in ret, \
            'ECMP is not enabled by default'
        assert 'Resilient Hashing  : Enabled' in ret, \
            'Resilient not enabled by default'
        assert 'Source IP          : Enabled' in ret, \
            'Hashing using source IP not enabled by default'
        assert 'Destination IP     : Enabled' in ret, \
            'Hashing using destination IP not enabled by default'
        assert 'Source Port        : Enabled' in ret, \
            'Hashing using source IP not enabled by default'
        assert 'Destination Port   : Enabled' in ret, \
            'Hashing using destination port enabled by default'
        info('### ECMP enabled by default validation passed ###\n')

        # Verify ecmp disable operation, multiple hash disable/enable
        s1.cmdCLI("configure terminal")
        # Disable hash function dst-ip and src-ip
        s1.cmdCLI("ip ecmp load-balance dst-ip disable")
        s1.cmdCLI("ip ecmp load-balance src-port disable")
        ret = s1.cmdCLI("do show ip ecmp")
        assert 'Source IP          : Enabled' in ret, \
            'Hashing using source ip disabled unexpectedly'
        assert 'Destination IP     : Disabled' in ret, \
            'Hashing using destination ip enabled even after it is disabled'
        assert 'Source Port        : Disabled' in ret, \
            'Hashing using source IP is enabled even after it is disabled'
        assert 'Destination Port   : Enabled' in ret, \
            'Hashing using destination port has disabled unexpectedly'
        info('### ECMP configuration validation passed ###\n')

        # Cleanup
        s1.cmdCLI("exit")

    def test_show_running_config(self):
        '''
            Test show running-config for ecmp changes
        '''
        info('\n########## Testing show running-config output ##########\n')
        s1 = self.net.switches[0]

        # Modifying interface data to test show running-config
        ret = s1.cmdCLI("show running-config")
        # CLI(self.net)
        assert 'ip ecmp load-balance src-port disable' in ret and \
               'ip ecmp load-balance dst-ip disable' in ret, \
               'show running-config does not show ecmp configuration'
        info('### ECMP configuration validation in '
             'running configuration passed ###\n')

    def test_ecmp_cmd(self, sub_cmd, show_string):
        '''
            Test ECMP feature enable/disable and show running-config output
            1. Check that the feature is enabled by default
        '''
        s1 = self.net.switches[0]

        # Check that the feature is enabled by default
        ret = s1.cmdCLI("show ip ecmp")
        assert ("%s: Enabled" % show_string) in ret, \
            ('%sis not enabled by default' % show_string)

        # Cleanup
        s1.cmdCLI("exit")


#@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_vtysh_ecmp:

    def setup_class(cls):
        # Create a test topology
        Test_vtysh_ecmp.test = ecmpCLITest()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology
        Test_vtysh_ecmp.test.net.stop()

    def test_ecmp_global(self):
        self.test.test_ecmp_cmd("", "ECMP Status        ")

    def test_ecmp_src_ip(self):
        self.test.test_ecmp_cmd("load-balance src-ip ",
                                "Source IP          ")

    def test_ecmp_src_port(self):
        self.test.test_ecmp_cmd("load-balance src-port ",
                                "Source Port        ")

    def test_ecmp_dst_ip(self):
        self.test.test_ecmp_cmd("load-balance dst-ip ",
                                "Destination IP     ")

    def test_ecmp_dst_port(self):
        self.test.test_ecmp_cmd("load-balance dst-port ",
                                "Destination Port   ")

    def test_ecmp_resilient(self):
        self.test.test_ecmp_cmd("load-balance resilient ",
                                "Resilient Hashing  ")

    # def test_ecmp_enable_disable(self):
        # self.test.test_ecmp_enable_disable()

    # def test_show_running_config(self):
        # self.test.test_show_running_config()

    def __del__(self):
        del self.test
