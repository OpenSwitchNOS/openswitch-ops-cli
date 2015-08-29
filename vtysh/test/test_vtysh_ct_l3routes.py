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
import time

class myTopo( Topo ):
    '''
        Custom Topology Example
        H1[h1-eth0]<--->[1]S1[2]<--->[2]S2[1]<--->[h2-eth0]H2
    '''

    def build(self, hsts=2, sws=2, **_opts):
        self.hsts = hsts
        self.sws = sws

        # Add list of hosts
        for h in irange( 1, hsts):
            host = self.addHost( 'h%s' % h)

        # Add list of switches
        for s in irange(1, sws):
            switch = self.addSwitch( 's%s' %s)

        # Add links between nodes based on custom topo
        self.addLink('s1', 's2')

class staticRouteConfigTest( HalonTest ):

    def setupNet(self):
        self.net = Mininet(topo=myTopo(hsts=0, sws=2,
                                       hopts=self.getHostOpts(),
                                       sopts=self.getSwitchOpts()),
                                       switch=HalonSwitch,
                                       host=HalonHost,
                                       link=HalonLink, controller=None,
                                       build=True)

    def test_ipv4(self):
        info('\n\n########## Test to verify IPv4 static routes ##########\n')
        s1 = self.net.switches[ 0 ]
        s2 = self.net.switches[ 1 ]

        # Configure switch 1
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("interface 1")
        s1.cmdCLI("ip address 192.168.1.1/24")
        s1.cmdCLI("ipv6 address 2000::1/120")
        s1.cmdCLI("exit")
        s1.cmdCLI("interface 2")
        s1.cmdCLI("ip address 192.168.2.1/24")
        s1.cmdCLI("ipv6 address 2001::1/120")
        s1.cmdCLI("exit")
        time.sleep(1)

        # Configure switch 2
        s2.cmdCLI("configure terminal")
        s2.cmdCLI("interface 1")
        s2.cmdCLI("ip address 192.168.1.2/24")
        s1.cmdCLI("ipv6 address 2000::2/120")
        s2.cmdCLI("exit")
        s2.cmdCLI("interface 2")
        s2.cmdCLI("ip address 192.168.3.1/24")
        s1.cmdCLI("ipv6 address 2002::1/120")
        s2.cmdCLI("exit")
        time.sleep(1)

        s1.cmd("/usr/bin/ovs-vsctl set interface 1 user_config:admin=up")
        s1.cmd("/usr/bin/ovs-vsctl set interface 2 user_config:admin=up")

        s2.cmd("/usr/bin/ovs-vsctl set interface 1 user_config:admin=up")
        s2.cmd("/usr/bin/ovs-vsctl set interface 2 user_config:admin=up")

        info('### Verify ip route configuration with nexthop address ###\n')
        s1.cmdCLI("ip route 192.168.3.0/24 192.168.1.2 2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ip route")

        assert ('192.168.3.0/24' in ret and '192.168.1.2' in ret and \
                'static' in ret and '[2/0]' in ret), 'IP route configuration failed'
        info('### IP route configuration successful ###\n')

        info('\n### Verify deletion of ip route with nexthop address ###\n')
        s1.cmdCLI("no ip route 192.168.3.0/24 192.168.1.2 2")
        ret = s1.cmdCLI("do show ip route")

        assert('192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
                and 'static' not in ret and '[2/0]' not in ret), 'Deletion of ip route failed'
        info('### Deletion of ip route successful ###\n')

        info('\n### Verify prefix format ###\n')
        s1.cmdCLI("ip route 192.168.3.0 192.168.1.2 2")
        ret = s1.cmdCLI("do show ip route")

        assert('192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
                and 'static' not in ret and '[2/0]' not in ret), 'Prefix format verification failed'
        info('### Prefix format verification successful ###\n')

        info('\n### Verify ip route configuration with nexthop interface ###\n')
        s1.cmdCLI("ip route 192.168.3.0/24 2 2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ip route")

        assert ('192.168.3.0/24' in ret and '2,' in ret and \
                'static' in ret and '[2/0]' in ret), 'IP route configuration failed'
        info('### IP route configuration successful ###\n')

        info('\n### Verify deletion of ip route with nexthop interface ###\n')
        s1.cmdCLI("no ip route 192.168.3.0/24 2 2")
        ret = s1.cmdCLI("do show ip route")

        assert('192.168.3.0/24' not in ret and 'static' not in ret \
                and '[2/0]' not in ret), 'Deletion of ip route failed'
        info('### Deletion of ip routes successful ###\n')

        info('\n### Verify setting of default distance ###\n')
        s1.cmdCLI("ip route 192.168.3.0/24 192.168.1.2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ip route")

        assert ('192.168.3.0/24' in ret and '192.168.1.2' in ret and \
                'static' in ret and '[1/0]' in ret), 'Default distance verification failed'
        info('### Default distance verification successful ###\n')

        info('\n### Verify setting of multiple nexthops for a given prefix ###\n')
        s1.cmdCLI("ip route 192.168.3.0/24 1")
        s1.cmdCLI("ip route 192.168.3.0/24 2")
        ret = s1.cmdCLI("do show ip route")

        assert('192.168.3.0/24' in ret and '3 unicast next-hops' in ret and '1,' in ret \
                and '2,' in ret and '[1/0]' in ret and 'static' in ret), 'Multiple nexthops verification failed'
        info('### Multiple nexthops verification successful ###\n')


    def test_ipv6(self):

        info('\n\n\n########## Test to verify IPv6 static routes ##########\n')
        s1 = self.net.switches[ 0 ]
        s2 = self.net.switches[ 1 ]

        info('### Verify ip route configuration with nexthop address ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 2000::2 2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ipv6 route")
        assert('2002::/120' in ret and '2000::2' in ret and 'static' in ret \
                and '[2/0]' in ret), 'IPv6 route configuration failed'
        info('### IPv6 route configuration successful ###\n')

        info('\n### Verify deletion of ipv6 route ###\n')
        s1.cmdCLI("no ipv6 route 2002::/120 2000::2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' not in ret and '2000::2' not in ret and 'static' not in \
                ret and '[2/0]' not in ret), 'Deletion of ipv6 route failed'
        info('### Deletion of ipv6 route successful ###\n')

        info('\n### Verify prefix format ###\n')
        s1.cmdCLI("ipv6 route 2002:: 2000::2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' not in ret and '2000::2' not in ret and 'static' not in \
                ret and '[2/0]' not in ret), 'Prefix format verification failed'
        info('### Prefix format verification successful ###\n')

        info('\n### Verify ipv6 route configuration with nexthop interface ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 2 2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' in ret and '2,' in ret and 'static' in ret \
                and '[2/0]' in ret), 'IPv6 route configuration failed'
        info('### IPv6 route configuration successful ###\n')

        info('\n### Verify deletion of ipv6 route with nexthop interface ###\n')
        s1.cmdCLI("no ipv6 route 2002::/120 2 2")
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' not in ret and 'static' not in ret \
                and '[2/0]' not in ret), 'Deletion of ipv6 routes failed'
        info('### Deletion of ipv6 routes successful ###\n')

        info('\n### Verify setting of default distance ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 2000::2")
        time.sleep(1)
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' in ret and 'static' in ret \
                and '[1/0]' in ret), 'Default distance verification failed'
        info('### Default distance verification successful ###\n')

        info('\n### Verify setting of multiple nexthops for a given prefix ###\n')
        s1.cmdCLI("ipv6 route 2002::/120 1")
        s1.cmdCLI("ipv6 route 2002::/120 2")
        ret = s1.cmdCLI("do show ipv6 route")

        assert('2002::/120' in ret and '3 unicast next-hops' in ret and '1,' in ret \
                and '2,' in ret and '[1/0]' in ret), 'Multiple nexthops prefix verification failed'
        info('### Multiple nexthops verification successful ###\n')


    def test_show_rib(self):
        info("\n\n\n########## Test to verify 'show rib' ##########\n")
        s1 = self.net.switches[ 0 ]
        s2 = self.net.switches[ 1 ]
        clilist = []

        info('### Verify show rib for added static routes ###\n')
        ret = s1.cmdCLI("do show rib")

        assert('*192.168.3.0/24,' in ret and '3 unicast next-hops' in ret and '*via' in ret and 'ipv4' in ret \
                and 'ipv6' in ret and '*2002::/120,' in ret and '[1/0],' in ret), 'show rib command failure'
        info('### show rib verification successful ###\n\n')

        s1.cmdCLI("no ip route 192.168.3.0/24 192.168.1.2")
        s1.cmdCLI("no ip route 192.168.3.0/24 1")
        s1.cmdCLI("no ip route 192.168.3.0/24 2")
        s1.cmdCLI("no ipv6 route 2002::/120 2000::2")
        s1.cmdCLI("no ipv6 route 2002::/120 1")
        s1.cmdCLI("no ipv6 route 2002::/120 2")


    def test_show_running_config(self):
        info("\n\n\n########## Test to verify 'show running-config' ##########\n")
        s1 = self.net.switches[ 0 ]
        s2 = self.net.switches[ 1 ]
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
        time.sleep(1)

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
        assert(found == 8), 'show running-config command failure'
        info('### show running-config verification successful ###\n\n\n')


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
