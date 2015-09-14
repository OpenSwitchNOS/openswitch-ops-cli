#!/usr/bin/python
# -*- coding: utf-8 -*-

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

from halonvsi.docker import *
from halonvsi.halon import *


class vlanInternalCT(HalonTest):

    def setupNet(self):
        self.net = Mininet(
            topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                  sopts=self.getSwitchOpts()),
            switch=HalonSwitch,
            host=HalonHost,
            link=HalonLink,
            controller=None,
            build=True,
            )

    def test_vlan_internal_cli(self):
        '''
            Test VLAN internal CLI working
        '''

        info('''
########## Internal VLAN CLI validations ##########

''')
        s1 = self.net.switches[0]

        s1.cmdCLI('configure terminal')

        # Checking internal VLAN range at bootup

        info('### Checking Internal VLAN range at bootup ###\n')
        ret = s1.cmdCLI('do show vlan internal')
        assert 'Internal VLAN range  : 1024-4094' in ret \
            and 'Internal VLAN policy : ascending' in ret, \
            'Internal VLAN range at bootup validation failed'
        info('''### Internal VLAN range at bootup validation passed ###

''')

        # Checking invalid internal VLAN range

        info('### Checking Internal VLAN range with (start > end) ###\n'
             )
        ret = s1.cmdCLI('vlan internal range 100 10 ascending')
        assert 'Invalid VLAN range. End VLAN must be greater or equal to start VLAN' \
            in ret, \
            'Internal VLAN range (start > end) validation failed'
        info('''### Internal VLAN range (start > end) validation passed ###

''')

        # Checking equal start & end internal VLAN range

        info('### Checking Internal VLAN range with (start = end) ###\n'
             )
        s1.cmdCLI('vlan internal range 10 10 ascending')
        ret = s1.cmdCLI('do show vlan internal')
        assert 'Internal VLAN range  : 10-10' in ret \
            and 'Internal VLAN policy : ascending', \
            'Internal VLAN range (start = end) validation failed'
        info('''### Internal VLAN range (start = end) validation passed ###

''')

        # Checking ascending internal VLAN range

        info('### Checking Ascending Internal VLAN range ###\n')
        s1.cmdCLI('vlan internal range 10 100 ascending')
        ret = s1.cmdCLI('do show vlan internal')
        assert 'Internal VLAN range  : 10-100' in ret \
            and 'Internal VLAN policy : ascending' in ret, \
            'Ascending Internal VLAN range validation failed'
        info('''### Ascending Internal VLAN range validation passed ###

''')

        # Checking descending internal VLAN range

        info('### Checking Descending Internal VLAN range ###\n')
        s1.cmdCLI('vlan internal range 100 200 descending')
        ret = s1.cmdCLI('do show vlan internal')
        assert 'Internal VLAN range  : 100-200' in ret \
            and 'Internal VLAN policy : descending' in ret, \
            'Descending Internal VLAN range validation failed'
        info('''### Descending Internal VLAN range validation passed ###

''')

        # Checking default internal VLAN range

        info('### Checking Default Internal VLAN range ###\n')
        s1.cmdCLI('no vlan internal range')
        ret = s1.cmdCLI('do show vlan internal')
        assert 'Internal VLAN range  : 1024-4094' in ret \
            and 'Internal VLAN policy : ascending' in ret, \
            'Default Internal VLAN range validation failed'
        info('''### Default Internal VLAN range validation passed ###

''')

        # Cleanup

        s1.cmdCLI('exit')
        info('''########## Internal VLAN CLI validations passed ##########

''')


class Test_vtysh_vlan_int:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_vlan_int.test = vlanInternalCT()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_vlan_int.test.net.stop()

    def test_vlan_internal_cli(self):
        self.test.test_vlan_internal_cli()

    def __del__(self):
        del self.test
