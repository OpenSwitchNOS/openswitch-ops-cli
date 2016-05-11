#!/usr/bin/python

# (c) Copyright 2015-2016 Hewlett Packard Enterprise Development LP
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
from opsvsiutils.systemutil import *
import time


class myTopo(Topo):

    '''
        Custom Topology Example
        H1[h1-eth0]<--->[1]S1[2]<--->[2]S2[1]<--->[h2-eth0]H2
    '''

    def build(self, hsts=2, sws=2, **_opts):
        self.hsts = hsts
        self.sws = sws

        # Add list of hosts

        for h in irange(1, hsts):
            host = self.addHost('h%s' % h)

        # Add list of switches

        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

        # Add links between nodes based on custom topo

        self.addLink('s1', 's2')


class staticRouteConfigTest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        static_topo = myTopo(hsts=0, sws=2, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(static_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_ipv4(self):
        info('''\n\n
########## Test to verify IPv4 static routes ##########
''')
        s1 = self.net.switches[0]
        s2 = self.net.switches[1]

        # Configure switch 1

        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('ip address 192.168.1.1/24')
        s1.cmdCLI('ipv6 address 2000::1/120')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 2')
        s1.cmdCLI('ip address 192.168.2.1/24')
        s1.cmdCLI('ipv6 address 2001::1/120')
        s1.cmdCLI('exit')
        time.sleep(1)

        # Configure switch 2

        s2.cmdCLI('configure terminal')
        s2.cmdCLI('interface 1')
        s2.cmdCLI('ip address 192.168.1.2/24')
        s1.cmdCLI('ipv6 address 2000::2/120')
        s2.cmdCLI('exit')
        s2.cmdCLI('interface 2')
        s2.cmdCLI('ip address 192.168.3.1/24')
        s1.cmdCLI('ipv6 address 2002::1/120')
        s2.cmdCLI('exit')
        time.sleep(1)

        s1.ovscmd('/usr/bin/ovs-vsctl set interface 1 user_config:admin=up')
        s1.ovscmd('/usr/bin/ovs-vsctl set interface 2 user_config:admin=up')

        s2.ovscmd('/usr/bin/ovs-vsctl set interface 1 user_config:admin=up')
        s2.ovscmd('/usr/bin/ovs-vsctl set interface 2 user_config:admin=up')

        info('### Verify ip route configuration with nexthop address ###\n')
        s1.cmdCLI('ip route 192.168.3.0/24 192.168.1.2 2')
        time.sleep(1)
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' in ret and '192.168.1.2' in ret \
            and 'static' in ret and '[2/0]' in ret, \
            'IP route configuration failed'
        info('### IP route configuration successful ###\n')

        info('''
### Verify deletion of ip route with nexthop address ###
''')
        s1.cmdCLI('no ip route 192.168.3.0/24 192.168.1.2 2')
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
            and 'static' not in ret and '[2/0]' not in ret, \
            'Deletion of ip route failed'
        info('### Deletion of ip route successful ###\n')

        info('''
### Verify prefix format ###
''')
        s1.cmdCLI('ip route 192.168.3.0 192.168.1.2 2')
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
            and 'static' not in ret and '[2/0]' not in ret, \
            'Prefix format verification failed'
        info('### Prefix format verification successful ###\n')

        info('''
### Verify ip route configuration with nexthop interface ###
''')
        s1.cmdCLI('ip route 192.168.3.0/24 2 2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' in ret and '2,' in ret and 'static' \
            in ret and '[2/0]' in ret, 'IP route configuration failed'
        info('### IP route configuration successful ###\n')

        info('''
### Verify deletion of ip route with nexthop interface ###
''')
        s1.cmdCLI('no ip route 192.168.3.0/24 2 2')
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' not in ret and 'static' not in ret \
            and '[2/0]' not in ret, 'Deletion of ip route failed'
        info('### Deletion of ip routes successful ###\n')

        info('''
### Verify setting of default distance ###
''')
        s1.cmdCLI('ip route 192.168.3.0/24 192.168.1.2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' in ret and '192.168.1.2' in ret \
            and 'static' in ret and '[1/0]' in ret, \
            'Default distance verification failed'
        info('### Default distance verification successful ###\n')

        info('''
### Verify setting of multiple nexthops for a given prefix ###
''')
        s1.cmdCLI('ip route 192.168.3.0/24 1')
        time.sleep(2)
        s1.cmdCLI('ip route 192.168.3.0/24 2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ip route')

        assert '192.168.3.0/24' in ret and '3 unicast next-hops' in ret \
            and '1,' in ret and '2,' in ret and '[1/0]' in ret \
            and 'static' in ret, 'Multiple nexthops verification failed'
        info('### Multiple nexthops verification successful ###\n')

        info('''
### Verify if nexthop is not assigned locally to an interface as a primary '''
             '''ip address ###\n''')
        s1.cmdCLI('ip route 192.168.3.0/24 192.168.2.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 192.168.3.0/24 192.168.2.1' in ret, \
            'Primary ip address check for nexthop failed'
        info('### Nexthop ip address verification successful against local '
             'primary address  ###\n')

        info('''
### Verify if nexthop is not assigned locally to an interface as a '''
             '''secondary ip address ###\n''')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('ip address 192.168.4.2/24 secondary')
        s1.cmdCLI('exit')
        s1.cmdCLI('ip route 192.168.3.0/24 192.168.4.2')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 192.168.3.0/24 192.168.4.2'in ret, \
            'Secondary ip address check for nexthop failed'
        info('### Nexthop ip address verification successful against local '
             'secondary address ###\n')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('no ip address 192.168.4.2/24 secondary')
        s1.cmdCLI('exit')

        info('''
### Verify if broadcast address cannot be assigned as a prefix'''
             ''' ###\n''')
        s1.cmdCLI('ip route 255.255.255.255/32 255.255.255.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 255.255.255.255/32 255.255.255.1' in ret, \
            'Broadcast address check for prefix failed'
        info('### Prefix verification successful for broadcast '
             'address  ###\n')

        info('''
### Verify if broadcast address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ip route 255.255.255.0/24 255.255.255.255')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 255.255.255.0/24 255.255.255.255' in ret, \
            'Broadcast address check for nexthop failed'
        info('### Nexthop as ip address verification successful for broadcast '
             'address  ###\n')

        info('''
### Verify if multicast starting address range cannot be assigned as a '''
             '''prefix ###\n''')
        s1.cmdCLI('ip route 224.10.1.0/24 223.10.1.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 224.10.1.0/24 223.10.1.1' in ret, \
            'Multicast address check for prefix failed'
        info('### Prefix verification successful for multicast '
             'starting address range) ###\n')

        info('''
### Verify if multicast starting address range cannot be assigned as a '''
             '''nexthop ###\n''')
        s1.cmdCLI('ip route 223.10.1.0/24 224.10.1.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 223.10.1.0/24 224.10.1.1' in ret, \
            'Multicast address check for nexthop failed'
        info('### Nexthop as ip address verification successful for multicast '
             'starting address range ###\n')

        info('''
### Verify if multicast ending address range cannot be assigned as a '''
             '''prefix ###\n''')
        s1.cmdCLI('ip route 239.10.1.0/24 223.10.1.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 239.10.1.0/24 223.10.1.1' in ret, \
            'Multicast address check for prefix failed'
        info('### Prefix verification successful for multicast '
             'ending address range) ###\n')

        info('''
### Verify if multicast ending address range cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ip route 223.10.1.0/24 239.10.1.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 223.10.1.0/24 239.10.1.1' in ret, \
            'Multicast address check for nexthop failed'
        info('### Nexthop as ip address verification successful for multicast '
             'ending address range ###\n')

        info('''
### Verify if loopback address cannot be assigned as a prefix'''
             ''' ###\n''')
        s1.cmdCLI('ip route 127.10.1.0/24 128.1.1.1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 127.10.1.0/24 128.1.1.1' in ret, \
            'Loopback address check for prefix failed'
        info('### Prefix verification successful for loopback '
             'address  ###\n')

        info('''
### Verify if loopback address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ip route 128.10.1.0/24 127.10.1.10')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 128.10.1.0/24 127.10.1.10' in ret, \
            'Loopback address check for nexthop failed'
        info('### Nexthop as ip address verification successful for loopback '
             'address  ###\n')

        info('''
### Verify if unspecified address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ip route 128.10.1.0/24 0.0.0.0')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ip route 128.10.1.0/24 0.0.0.0' in ret, \
            'Unspecified address check for nexthop failed'
        info('### Nexthop verification successful for '
             'unspecified address  ###\n\n\n')

    def test_ipv6(self):

        info('''
########## Test to verify IPv6 static routes ##########
''')
        s1 = self.net.switches[0]
        s2 = self.net.switches[1]

        info('### Verify ip route configuration with nexthop address ###\n'
             )
        s1.cmdCLI('ipv6 route 2002::/120 2000::2 2')
        time.sleep(1)
        ret = s1.cmdCLI('do show ipv6 route')
        assert '2002::/120' in ret and '2000::2' in ret and 'static' \
            in ret and '[2/0]' in ret, 'IPv6 route configuration failed'
        info('### IPv6 route configuration successful ###\n')

        info('''
### Verify deletion of ipv6 route ###
''')
        s1.cmdCLI('no ipv6 route 2002::/120 2000::2 2')
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' not in ret and '2000::2' not in ret \
            and 'static' not in ret and '[2/0]' not in ret, \
            'Deletion of ipv6 route failed'
        info('### Deletion of ipv6 route successful ###\n')

        info('''
### Verify prefix format ###
''')
        s1.cmdCLI('ipv6 route 2002:: 2000::2 2')
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' not in ret and '2000::2' not in ret \
            and 'static' not in ret and '[2/0]' not in ret, \
            'Prefix format verification failed'
        info('### Prefix format verification successful ###\n')

        info('''
### Verify ipv6 route configuration with nexthop interface ###
''')
        s1.cmdCLI('ipv6 route 2002::/120 2 2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' in ret and '2,' in ret and 'static' in ret \
            and '[2/0]' in ret, 'IPv6 route configuration failed'
        info('### IPv6 route configuration successful ###\n')

        info('''
### Verify deletion of ipv6 route with nexthop interface ###
''')
        s1.cmdCLI('no ipv6 route 2002::/120 2 2')
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' not in ret and 'static' not in ret \
            and '[2/0]' not in ret, 'Deletion of ipv6 routes failed'
        info('### Deletion of ipv6 routes successful ###\n')

        info('''
### Verify setting of default distance ###
''')
        s1.cmdCLI('ipv6 route 2002::/120 2000::2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' in ret and 'static' in ret and '[1/0],' \
            in ret, 'Default distance verification failed'
        info('### Default distance verification successful ###\n')

        info('''
### Verify setting of multiple nexthops for a given prefix ###
''')
        s1.cmdCLI('ipv6 route 2002::/120 1')
        time.sleep(2)
        s1.cmdCLI('ipv6 route 2002::/120 2')
        time.sleep(2)
        ret = s1.cmdCLI('do show ipv6 route')

        assert '2002::/120' in ret and '3 unicast next-hops' in ret \
            and '1,' in ret and '2,' in ret and '[1/0]' in ret, \
            'Multiple nexthops prefix verification failed'
        info('### Multiple nexthops verification successful ###\n')

        info('''
### Verify if nexthop is not assigned locally to an interface as a primary '''
             '''ipv6 address ###\n''')
        s1.cmdCLI('ipv6 route 2002::/120 2001::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 2002::/120 2001::1' in ret, \
            'Primary ipv6 address check for nexthop failed'
        info('### Nexthop ip address verification successful against local '
             'primary address  ###\n')

        info('''
### Verify if nexthop is not assigned locally to an interface as a '''
             '''secondary ipv6 address ###\n''')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('ipv6 address 2005::3/120 secondary')
        s1.cmdCLI('exit')
        s1.cmdCLI('ipv6 route 2002::/120 2005::3')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 2002::/120 2005::3' in ret, \
            'Secondary ipv6 address check for nexthop failed'
        info('### Nexthop ipv6 address verification successful against local '
             'secondary address ###\n')
        s1.cmdCLI('interface 1')
        s1.cmdCLI('no ipv6 address 2005::2/120 secondary')
        s1.cmdCLI('exit')

        info('''
### Verify if multicast address cannot be assigned as a prefix'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route ff00::/128 2001::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route ff00::/128 2001::1' in ret, \
            'Multicast address check for prefix failed'
        info('### Prefix verification successful for multicast '
             'ipv6 address ###\n')

        info('''
### Verify if multicast address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route 12ff::/128 ff00::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 12ff::/128 ff00::1' in ret, \
            'Multicast address check for nexthop failed'
        info('### Nexthop verification successful for multicast '
             'ipv6 address ###\n')

        info('''
### Verify if linklocal address cannot be assigned as a prefix'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route fe80::/10 2001::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route ::1/128 2001::1' in ret, \
            'Linklocal address check for prefix failed'
        info('### Prefix verification successful for linklocal '
             'ipv6 address ###\n')

        info('''
### Verify if linklocal address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route 12ff::/128 fe80::')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 12ff::/128 ::1' in ret, \
            'Linklocal address check for nexthop failed'
        info('### Nexthop verification successful for linklocal '
             'ipv6 address ###\n')

        info('''
### Verify if loopback address cannot be assigned as a prefix'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route ::1/128 2001::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route ::1/128 2001::1' in ret, \
            'Loopback address check for prefix failed'
        info('### Prefix verification successful for loopback '
             'ipv6 address ###\n')

        info('''
### Verify if loopback address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route 12ff::/128 ::1')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 12ff::/128 ::1' in ret, \
            'Loopback address check for nexthop failed'
        info('### Nexthop verification successful for loopback '
             'ipv6 address ###\n')

        info('''
### Verify if unspecified address cannot be assigned as a nexthop'''
             ''' ###\n''')
        s1.cmdCLI('ipv6 route 2002::/120 ::')
        ret = s1.cmdCLI('do show running-config')
        assert not 'ipv6 route 2002::/120 ::' in ret, \
            'Unspecified address check for nexthop failed'
        info('### Nexthop verification successful for '
             'unspecified ipv6 address  ###\n\n\n')

    def test_show_rib(self):
        info("""
########## Test to verify 'show rib' ##########
""")
        s1 = self.net.switches[0]
        s2 = self.net.switches[1]
        clilist = []

        info('### Verify show rib for added static routes ###\n')
        ret = s1.cmdCLI('do show rib')

        assert '*192.168.3.0/24,' in ret and '3 unicast next-hops' \
            in ret and '*via' in ret and 'ipv4' in ret and 'ipv6' \
            in ret and '*2002::/120,' in ret and '[1/0],' in ret, \
            'show rib command failure'
        info('### show rib verification successful ###\n\n\n')

        s1.cmdCLI('no ip route 192.168.3.0/24 192.168.1.2')
        s1.cmdCLI('no ip route 192.168.3.0/24 1')
        s1.cmdCLI('no ip route 192.168.3.0/24 2')
        s1.cmdCLI('no ipv6 route 2002::/120 2000::2')
        s1.cmdCLI('no ipv6 route 2002::/120 1')
        s1.cmdCLI('no ipv6 route 2002::/120 2')

    def test_show_running_config(self):
        info("""
########## Test to verify 'show running-config' ##########
""")
        s1 = self.net.switches[0]
        s2 = self.net.switches[1]
        clilist = []

        s1.cmdCLI('configure terminal')
        s1.cmdCLI('interface 3')
        s1.cmdCLI('ip address 10.0.0.5/8')
        s1.cmdCLI('ipv6 address 2003::2/120')
        s1.cmdCLI('exit')
        s1.cmdCLI('interface 4')
        s1.cmdCLI('ip address 10.0.0.7/8')
        s1.cmdCLI('ipv6 address 2004::2/120')
        s1.cmdCLI('exit')
        time.sleep(1)

        info('### Adding Ipv4 Routes ###\n')
        s1.cmdCLI('ip route 10.0.0.1/32 10.0.0.2')
        clilist.append('ip route 10.0.0.1/32 10.0.0.2')
        s1.cmdCLI('ip route 10.0.0.3/32 10.0.0.4 4')
        clilist.append('ip route 10.0.0.3/32 10.0.0.4 4')
        s1.cmdCLI('ip route 10.0.0.6/32 3')
        clilist.append('ip route 10.0.0.6/32 3')
        s1.cmdCLI('ip route 10.0.0.8/32 4 4')
        clilist.append('ip route 10.0.0.8/32 4 4')

        info('### Adding Ipv6 Routes ###\n')
        s1.cmdCLI('ipv6 route 2001::/120 2001::2')
        clilist.append('ipv6 route 2001::/120 2001::2')
        s1.cmdCLI('ipv6 route 2002::/120 2002::2 3')
        clilist.append('ipv6 route 2002::/120 2002::2 3')
        s1.cmdCLI('ipv6 route 2003::/120 3')
        clilist.append('ipv6 route 2003::/120 3')
        s1.cmdCLI('ipv6 route 2004::/120 4 4')
        clilist.append('ipv6 route 2004::/120 4 4')

        out = s1.cmdCLI('do show running-config')
        lines = out.split('\n')
        found = 0
        for line in lines:
            if line in clilist:
                found = found + 1

        info('''
### Verify show running-config for added static routes ###
''')
        assert found == 8, 'show running-config command failure'
        info('''### show running-config verification successful ###


''')


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

        # CLI(self.test.net)

    def __del__(self):
        del self.test
