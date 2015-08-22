#!/usr/bin/python

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
from halonutils.halonutil import *

class staticRouteConfigTest( HalonTest ):

    def setupNet(self):
        self.net = Mininet(topo=SingleSwitchTopo(k=0,
                                                 hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                                                 switch=HalonSwitch,
                                                 host=HalonHost,
                                                 link=HalonLink, controller=None,
                                                 build=True)

    def test_ipv4(self):
        info('\n########## Test  to verify IPv4 static routes ##########\n')
        s1 = self.net.switches[ 0 ]
        intf_check = 0

        s1.cmdCLI("configure terminal")
        s1.cmdCLI("interface 1")
        s1.cmdCLI("ip address 192.168.1.1/24")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 2")
        s1.cmdCLI("ip address 192.168.2.1/24")
        s1.cmdCLI("exit")

        info('### Verify ip route configuration with nexthop address ###\n')
        s1.cmdCLI("ip route 192.168.3.1/24 192.168.2.2 2")
        ret = s1.cmdCLI("do show ip route")

        if '192.168.3.1/24' in ret and '192.168.2.2' in ret and 'static' in ret and '[2/0]' in ret:
            info('### IP route configuration successful ###\n')
        else:
            assert 0, ('IP route configuration failed')

        info('\n### Verify deletion of ip route with nexthop address ###\n')
        s1.cmdCLI("no ip route 192.168.3.1/24 192.168.2.2 2")
        ret = s1.cmdCLI("do show ip route")

        if 'No ipv4 routes configured' in ret:
            info('### Deletion of ip route successful ###\n')
        else:
            assert 0, ('Deletion of ip route failed')

        info('\n### Verify prefix format ###\n')
        s1.cmdCLI("ip route 192.168.3.1 192.168.2.2 2")
        ret = s1.cmdCLI("do show ip route")

        if 'No ipv4 routes configured' in ret:
            info('### Prefix format verification successful ###\n\n')
        else:
            assert 0, ('Prefix format verification failed')

        info('\n### Verify ip route configuration with nexthop interface ###\n')
        s1.cmdCLI("ip route 192.168.3.1/24 2 2")
        ret = s1.cmdCLI("do show ip route")

        for index, word in enumerate(ret.split(" ")):
            if index == 17 and word == '2,':
                intf_check = 1

        if intf_check == 1:
            info('### IP route configuration successful ###\n\n')
        else:
            assert 0, ('IP route configuration failed')

        info('\n### Verify deletion of ip route with nexthop interface ###\n')
        s1.cmdCLI("no ip route 192.168.3.1/24 2 2")
        ret = s1.cmdCLI("do show ip route")

        if 'No ipv4 routes configured' in ret:
            info('### Deletion of ip routes successful ###\n\n')
        else:
            assert 0, ('Deletion of ip routes failed')

        info('\n### Verify setting of default distance ###\n')
        s1.cmdCLI("ip route 192.168.4.1/24 192.168.4.2")
        ret = s1.cmdCLI("do show ip route")

        intf_check = 0
        for index, word in enumerate(ret.split(" ")):
            if index == 19 and word == '[1/0],':
                intf_check = 1

        if intf_check == 1:
            info('### Default distance verification successful ###\n\n')
        else:
            assert 0, ('Default distance verification failed')

        s1.cmdCLI("no ip route 192.168.4.1/24 192.168.4.2")

        info('\n### Verify setting of multiple nexthops for a given prefix ###\n')
        s1.cmdCLI("ip route 192.168.10.0/24 192.168.10.10")
        s1.cmdCLI("ip route 192.168.10.0/24 192.168.10.20")
        s1.cmdCLI("ip route 192.168.10.0/24 1")
        s1.cmdCLI("ip route 192.168.10.0/24 2")
        ret = s1.cmdCLI("do show ip route")

        if '192.168.10.10' in ret and '192.168.10.20' in ret and '1,' in ret \
                and '2,' in ret and '[1/0]' in ret and '4' in ret:
            info('### Multiple nexthops verification successful ###\n\n')
        else:
            assert 0, ('Multiple nexthops verification failed')


    def test_ipv6(self):

        info('\n########## Test  to verify IPv6 static routes ##########\n')
        s1 = self.net.switches[ 0 ]
        intf_check = 0

        s1.cmdCLI("configure terminal")
        s1.cmdCLI("interface 1")
        s1.cmdCLI("ipv6 address 2000::1/120")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 2")
        s1.cmdCLI("ipv6 address 2001::1/120")
        s1.cmdCLI("exit")

        info('### Verify ip route configuration with nexthop address ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 2001::2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        if '2002::/120' in ret and '2001::2' in ret and 'static' in ret and '[2/0]' in ret:
            info('### IPv6 route configuration successful ###\n')
        else:
            assert 0, ('IPv6 route configuration failed')

        info('\n### Verify deletion of ipv6 route ###\n')
        s1.cmdCLI("no ipv6 route 2002::/120 2001::2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        if 'No ipv6 routes configured' in ret:
            info('### Deletion of ipv6 route successful ###\n')
        else:
            assert 0, ('Deletion of ipv6 route failed')

        info('\n### Verify prefix format ###\n')
        s1.cmdCLI("ipv6 route 2002:: 2001::2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        if 'No ipv6 routes configured' in ret:
            info('### Prefix format verification successful ###\n\n')
        else:
            assert 0, ('Prefix format verification failed')

        info('\n### Verify ipv6 route configuration with nexthop interface ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        for index, word in enumerate(ret.split(" ")):
            if index == 17 and word == '2,':
                intf_check = 1

        if intf_check == 1:
            info('### IPv6 route configuration successful ###\n\n')
        else:
            assert 0, ('IPv6 route configuration failed')

        info('\n### Verify deletion of ipv6 route with nexthop interface ###\n')
        s1.cmdCLI("no ipv6 route 2002::/120 2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        if 'No ipv6 routes configured' in ret:
            info('### Deletion of ipv6 routes successful ###\n\n')
        else:
            assert 0, ('Deletion of ipv6 routes failed')

        info('\n### Verify setting of default distance ###\n')
        s1.cmdCLI("ipv6 route 1001:2001::/64 1001:3001::3")
        ret = s1.cmdCLI("do show ipv6 route")

        for index, word in enumerate(ret.split(" ")):
            if index == 19 and word == '[1/0]':
                intf_check = 1

        if intf_check == 1:
            info('### Default distance verification successful ###\n\n')
        else:
            assert 0, ('Default distance verification failed')

        s1.cmdCLI("no ipv6 route 1001:2001::/64 1001:3001::3")

        info('\n### Verify setting of multiple nexthops for a given prefix ###\n')
        s1.cmdCLI("ipv6 route 2000::/120 2000::10")
        s1.cmdCLI("ipv6 route 2000::/120 2000::20")
        s1.cmdCLI("ipv6 route 2000::/120 1")
        s1.cmdCLI("ipv6 route 2000::/120 2")
        ret = s1.cmdCLI("do show ipv6 route")

        if '2000::10' in ret and '2000::20' in ret and '1,' in ret \
                and '2,' in ret and '[1/0]' in ret:
            info('### Multiple nexthops verification successful ###\n\n')
        else:
            assert 0, ('Multiple nexthops prefix verification failed')


    def test_show_rib(self):
        info("\n########## Test  to verify 'show rib' ##########\n")
        s1 = self.net.switches[ 0 ]
        intf_check = 0
        clilist = []

        info('### Verify show rib for added static routes ###\n')
        ret = s1.cmdCLI("do show rib")
        if '*192.168.10.0/24,' in ret and '4' in ret and '*via' in ret and 'ipv4' in ret \
                and 'ipv6' in ret and '*2000::/120,' in ret and '[1/0],' in ret:
            info('### show rib verification successful ###\n\n')
        else:
            assert 0, ('show rib command failure')

        s1.cmdCLI("no ip route 192.168.10.0/24 192.168.10.10")
        s1.cmdCLI("no ip route 192.168.10.0/24 192.168.10.20")
        s1.cmdCLI("no ip route 192.168.10.0/24 1")
        s1.cmdCLI("no ip route 192.168.10.0/24 2")
        s1.cmdCLI("no ipv6 route 2000::/120 2000::10")
        s1.cmdCLI("no ipv6 route 2000::/120 2000::20")
        s1.cmdCLI("no ipv6 route 2000::/120 1")
        s1.cmdCLI("no ipv6 route 2000::/120 2")


    def test_show_running_config(self):
        info("\n########## Test  to verify 'show running-config' ##########\n")
        s1 = self.net.switches[ 0 ]
        intf_check = 0
        clilist = []

        s1.cmdCLI("configure terminal")
        s1.cmdCLI("interface 3")
        s1.cmdCLI("ip address 10.0.0.5/8")
        s1.cmdCLI("ipv6 address 2003::2/120")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 4")
        s1.cmdCLI("ip address 10.0.0.7/8")
        s1.cmdCLI("ipv6 address 2004::2/120")
        s1.cmdCLI("exit")

        info('### Adding Ipv4 Routes ###\n')
        s1.cmdCLI("ip route 10.0.0.1/8 10.0.0.2")
        clilist.append('ip route 10.0.0.1/8 10.0.0.2')
        s1.cmdCLI("ip route 10.0.0.3/8 10.0.0.4 4")
        clilist.append('ip route 10.0.0.3/8 10.0.0.4 4')
        s1.cmdCLI("ip route 10.0.0.6/8 3")
        clilist.append('ip route 10.0.0.6/8 3')
        s1.cmdCLI("ip route 10.0.0.8/8 4 4")
        clilist.append('ip route 10.0.0.8/8 4 4')

        info('### Adding Ipv6 Routes ###\n')
        s1.cmdCLI("ipv6 route 2001::/120 2001::2")
        clilist.append('ipv6 route 2001::/120 2001::2')
        s1.cmdCLI("ipv6 route 2002::/120 2002::2 3")
        clilist.append('ipv6 route 2002::/120 2002::2 3')
        s1.cmdCLI("ipv6 route 2003::/120 3")
        clilist.append('ipv6 route 2003::/120 3')
        s1.cmdCLI("ipv6 route 2004::/120 4 4")
        clilist.append('ipv6 route 2004::/120 4 4')

        out = s1.cmdCLI("do show running-config")
        lines = out.split('\n')
        found = 0
        for line in lines:
            if line in clilist:
                found = found + 1

        info('\n### Verify show running-config for added static routes ###\n')
        if found == 8:
            info('### show running-config verification successful ###\n\n\n')
        else:
            assert 0, ('show running-config command failure')


class Test_vtysh_static_routes_ct:

    def setup_class(cls):
        Test_vtysh_static_routes_ct.test = staticRouteConfigTest()

    def teardown_class(cls):
        Test_vtysh_static_routes_ct.test.net.stop()

    def test_ipv4(self):
        self.test.test_ipv4()

    def test_ipv6(self):
        self.test.test_ipv6()

    def test_show_rib(self):
        self.test.test_show_rib()

    def test_show_running_config(self):
        self.test.test_show_running_config()
        #CLI(self.test.net)

    def __del__(self):
        del self.test
