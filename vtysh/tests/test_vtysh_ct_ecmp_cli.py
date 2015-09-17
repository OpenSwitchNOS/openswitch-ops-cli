#!/usr/bin/env python

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

from opsvsi.docker import *
from opsvsi.opsvsitest import *

class ecmpCLITest( OpsVsiTest ):

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
        s1 = self.net.switches[ 0 ]

        # Verify default state of ECMP is enabled
        ret = s1.cmdCLI("show ip ecmp")
        assert 'ECMP Status        : Enabled' in ret, 'ECMP is not enabled by default'
        assert 'Source IP          : Enabled' in ret, 'Hashing using source IP not enabled by default'
        assert 'Destination IP     : Enabled' in ret, 'Hashing using destination IP not enabled by default'
        assert 'Source Port        : Enabled' in ret, 'Hashing using source IP not enabled by default'
        assert 'Destination Port   : Enabled' in ret, 'Hashing using destination port enabled by default'
        info('### ECMP enabled by default validation passed ###\n')

        # Verify ecmp disable operation, multiple hash disable/enable
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("ip ecmp disable")
        # Disable hash function dst-ip and src-ip
        s1.cmdCLI("ip ecmp load-balance dst-ip disable")
        s1.cmdCLI("ip ecmp load-balance src-port disable")
        ret = s1.cmdCLI("do show ip ecmp")
        assert 'ECMP Status        : Disabled' in ret, 'ECMP is enabled even after disabling it'
        assert 'Source IP          : Enabled' in ret, 'Hashing using source ip disabled unexpectedly'
        assert 'Destination IP     : Disabled' in ret, 'Hashing using destination ip enabled even after it is disabled'
        assert 'Source Port        : Disabled' in ret, 'Hashing using source IP is enabled even after it is disabled'
        assert 'Destination Port   : Enabled' in ret, 'Hashing using destination port has disabled unexpectedly'
        info('### ECMP configuration validation passed ###\n')

        #Cleanup
        s1.cmdCLI("exit")

    def test_show_running_config(self):
        '''
            Test show running-config for ecmp changes
        '''
        info('\n########## Testing show running-config output ##########\n')
        s1 = self.net.switches[ 0 ]

        # Modifying interface data to test show running-config
        ret = s1.cmdCLI("show running-config")
        #CLI(self.net)
        assert 'ip ecmp disable' in ret and \
               'ip ecmp load-balance src-port disable' in ret and \
               'ip ecmp load-balance dst-ip disable' in ret, 'show running-config does not show ecmp configuration'
        info('### ECMP configuration validation in running configuration passed ###\n')

class Test_vtysh_ecmp:

    def setup_class(cls):
        # Create a test topology
        Test_vtysh_ecmp.test = ecmpCLITest()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology
        Test_vtysh_ecmp.test.net.stop()

    def test_ecmp_enable_disable(self):
        self.test.test_ecmp_enable_disable()

    def test_show_running_config(self):
        self.test.test_show_running_config()

    def __del__(self):
        del self.test
