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

from halonvsi.docker import *
from halonvsi.halon import *

first_interface = "12"
second_interface = "13"

class vrfCLITest( HalonTest ):

    def setupNet(self):
        self.net = Mininet(topo=SingleSwitchTopo(k=0,
                                                 hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                                                 switch=HalonSwitch,
                                                 host=HalonHost,
                                                 link=HalonLink, controller=None,
                                                 build=True)

    def test_vrf(self):
        '''
            Test VRF add and delete validations
        '''
        info('\n######## VRF add/delete validations ########\n')
        s1 = self.net.switches[ 0 ]

        # HALON_TODO: When multiple VRF support is added, change the script to include required validations.
        s1.cmdCLI("configure terminal")

        ret = s1.cmdCLI("vrf thisisavrfnamewhichismorethan32characters")
        assert 'Error: VRF name cannot be more than 32 characters.' in ret, 'VRF name validation failed'
        info('VRF name validation passed\n')

        ret = s1.cmdCLI("vrf vrf0")
        assert 'Error: Command not supported. Default VRF already exists.' in ret, 'VRF add validation failed'
        info('VRF add validation passed\n')

        ret = s1.cmdCLI("no vrf vrf_default")
        assert 'Error: Cannot delete default VRF.' in ret, 'VRF delete validation failed'
        info('VRF delete validation passed\n')

        ret = s1.cmdCLI("no vrf abcd")
        assert 'Error: VRF abcd not found.' in ret, 'VRF lookup validation failed'
        info('VRF lookup validation passed\n')

        #Cleanup
        s1.cmdCLI("exit")

    def test_interface(self):
        '''
            Test attaching/detaching interface to/from VRF
        '''
        info('\n######## Attaching/detaching interface to/from vrf #######\n')
        s1 = self.net.switches[ 0 ]
        intf_cmd = "interface " + first_interface

        s1.cmdCLI("configure terminal")
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("vrf attach vrf_default")
        ret = s1.cmdCLI("do show vrf")
        expected_output = '\t' + first_interface
        assert expected_output in ret, 'Interface attach failed'
        info('Interface attached successfully\n')

        ret = s1.cmdCLI("vrf attach vrf_default")
        expected_output = "Error: Interface " + first_interface + " has already been added to vrf."
        assert expected_output in ret, 'VRF attach interface validation failed'
        info('VRF attach interface validation passed\n')

        s1.cmdCLI("no vrf attach vrf_default")
        ret = s1.cmdCLI("do show vrf")
        assert '\t12' not in ret, 'Interface detach failed'
        info('Interface detached successfully\n')

        s1.cmdCLI("exit")
        intf_cmd = "interface " + second_interface
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("no vrf attach vrf_default")
        expected_output = "Error: Interface " + second_interface + " not attached to given VRF."
        assert expected_output in ret, 'Interface detach validation failed'
        info('Interface detach validation passed\n')

        #Cleanup
        s1.cmdCLI("exit")
        s1.cmdCLI("exit")

    def test_ip(self):
        '''
            Test configuration of IP address for port
        '''
        info('\n####### Assign/remove IP address to/from interface #######\n')
        s1 = self.net.switches[ 0 ]
        intf_cmd = "interface " + first_interface

        s1.cmdCLI("configure terminal")
        s1.cmdCLI(intf_cmd)
        ret = s1.cmdCLI("ip address 10.0.20.2/24")
        expected_output = "Error: Interface " + first_interface + " is not L3."
        assert expected_output in ret, 'IP address validation failed'
        info('IP address validation passed\n')

        s1.cmdCLI("vrf attach vrf_default")
        ret = s1.cmdCLI("no ip address 10.0.30.2/24")
        expected_output = "Error: No IP Address configured on interface " + first_interface
        assert expected_output in ret, 'IP address presence validation failed'
        info('IP address presence validation passed\n')

        s1.cmdCLI("ip address 10.0.20.2/24")
        ip = s1.ovscmd("/usr/bin/ovs-vsctl get port 12 ip_address").strip()
        assert ip == "10.0.20.2/24", 'IP address configuration failed'
        info('IP address configured successfully\n')

        ret = s1.cmdCLI("no ip address 10.0.30.2/24")
        assert "Error: IP Address 10.0.30.2/24 not found." in ret, 'IP address delete validation failed'
        info('IP address delete validation passed\n')

        ret = s1.cmdCLI("no ip address 10.0.20.2/24")
        ip = s1.ovscmd("/usr/bin/ovs-vsctl get port 12 ip_address").strip()
        assert ip == "[]", 'IP address remove failed'
        info('IP address removed successfully\n')

        #Cleanup
        s1.cmdCLI("no vrf attach vrf_default")
        s1.cmdCLI("exit")
        s1.cmdCLI("exit")

    def test_ipv6(self):
        '''
            Test configuration of IPv6 address for port
        '''
        info('\n##### Assign/remove IPv6 address to/from interface #######\n')
        s1 = self.net.switches[ 0 ]
        intf_cmd = "interface " + first_interface

        s1.cmdCLI("configure terminal")
        s1.cmdCLI(intf_cmd)
        s1.cmdCLI("vrf attach vrf_default")
        s1.cmdCLI("ipv6 address 2002::1/120")
        ipv6 = s1.ovscmd("/usr/bin/ovs-vsctl get port 12 ip6_address").strip()

        assert ipv6 == "2002::1/120", 'IPv6 address configuration failed'
        info('IPv6 address configured successfully\n')

        s1.cmdCLI("no ipv6 address 2002::1/120")
        ipv6 = s1.ovscmd("/usr/bin/ovs-vsctl get port 12 ip6_address").strip()

        assert ipv6 == "[]", 'IPv6 address remove failed'
        info('IPv6 address removed successfully\n')

        #Cleanup
        s1.cmdCLI("no vrf attach vrf_default")
        s1.cmdCLI("exit")
        s1.cmdCLI("exit")

class Test_vtysh_static_routes:

    # Create a test topology.
    test = vrfCLITest()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology
        Test_vtysh_static_routes.test.net.stop()

    def test_vrf(self):
        self.test.test_vrf()

    def test_interface(self):
        self.test.test_interface()

    def test_ip(self):
        self.test.test_ip()

    def test_ipv6(self):
        self.test.test_ipv6()

    def __del__(self):
        del self.test
