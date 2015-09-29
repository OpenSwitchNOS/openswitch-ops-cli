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


class vlanInternalCT(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        vlan_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(vlan_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

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
        assert 'Invalid VLAN range. End VLAN must be greater ' \
               'or equal to start VLAN' in ret, \
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
