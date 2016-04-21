# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

from time import sleep
from pytest import mark

TOPOLOGY = """
#
# +-------+                                 +-------+
# |       |     +-------+     +-------+     |       |
# |  hs1  <----->  sw1  <----->  sw2  <----->  hs2  |
# |       |     +-------+     +-------+     |       |
# +-------+                                 +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
[type=openswitch name="Switch 2"] sw2
[type=host name="host 1"] h1
[type=host name="host 2"] h2

# Links
sw1:if02 -- h1:if01
sw2:if02 -- h2:if01
sw1:if01 -- sw2:if01
sw1:if03
sw1:if04

"""

@mark.skipif(True, reason="Disabling due to gate job failures")
def test_static_route_config(topology, step):
    sw1 = topology.get('sw1')
    sw2 = topology.get('sw2')
    h1 = topology.get('h1')
    h2 = topology.get('h2')

    assert sw1 is not None
    assert sw2 is not None
    assert h1 is not None
    assert h2 is not None

    sw1p1 = sw1.ports['if01']
    sw1p2 = sw1.ports['if02']
    sw1p3 = sw1.ports['if03']
    sw1p4 = sw1.ports['if04']
    sw2p1 = sw2.ports['if01']
    sw2p2 = sw2.ports['if02']

    step('### Test to verify IPv4 static routes ###')
    # Configure switch 1
    sw1('configure terminal')
    sw1('interface {sw1p1}'.format(**locals()))
    sw1('ip address 192.168.1.1/24')
    sw1('ipv6 address 2000::1/120')
    sw1('interface {sw1p2}'.format(**locals()))
    sw1('ip address 192.168.2.1/24')
    sw1('ipv6 address 2001::1/120')
    sleep(1)

    # Configure switch 2
    sw2('configure terminal')
    sw2('interface {sw2p1}'.format(**locals()))
    sw2('ip address 192.168.1.2/24')
    sw2('ipv6 address 2000::2/120')
    sw2('interface {sw2p2}'.format(**locals()))
    sw2('ip address 192.168.3.1/24')
    sw2('ipv6 address 2002::1/120')
    sleep(1)

    sw1('set interface {sw1p1} user_config:admin=up'.format(**locals()),
        shell='vsctl')
    sw1('set interface {sw1p2} user_config:admin=up'.format(**locals()),
        shell='vsctl')

    sw2('set interface {sw2p1} user_config:admin=up'.format(**locals()),
        shell='vsctl')
    sw2('set interface {sw2p2} user_config:admin=up'.format(**locals()),
        shell='vsctl')

    # Configuring hosts
    h1.libs.ip.interface("if01", addr="192.168.2.2/24", up=True)
    h1.libs.ip.interface("if01", addr="2001::2/120", up=True)
    h1("ip -6 route add default via 2001::1")
    h1("ip -4 route add default via 192.168.2.1")

    h2.libs.ip.interface("if01", addr="192.168.3.2/24", up=True)
    h2.libs.ip.interface("if01", addr="2002::2/120", up=True)
    h2("ip -6 route add default via 2002::1")
    h2("ip -4 route add default via 192.168.3.1")

    step('### Verify ip route configuration with nexthop address ###')
    sw1('ip route 192.168.3.0/24 192.168.1.2 2')
    sleep(1)
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' in ret and '192.168.1.2' in ret \
           and 'static' in ret and '[2/0]' in ret, \
           'IP route configuration failed'

    step('### Verify deletion of ip route with nexthop address ###')
    sw1('no ip route 192.168.3.0/24 192.168.1.2 2')
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
           and 'static' not in ret and '[2/0]' not in ret, \
           'Deletion of ip route failed'

    step('### Verify prefix format ###')
    sw1('ip route 192.168.3.0 192.168.1.2 2')
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' not in ret and '192.168.1.2' not in ret \
           and 'static' not in ret and '[2/0]' not in ret, \
           'Prefix format verification failed'

    step('### Verify ip route configuration with nexthop interface ###')
    sw1('ip route 192.168.3.0/24 2 2')
    sleep(2)
    ret = sw1('do show ip route')
    assert '192.168.3.0/24' in ret and '2,' in ret and 'static' \
           in ret and '[2/0]' in ret, 'IP route configuration failed'

    step('### Verify deletion of ip route with nexthop interface ###')
    sw1('no ip route 192.168.3.0/24 2 2')
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' not in ret and 'static' not in ret \
           and '[2/0]' not in ret, 'Deletion of ip route failed'

    step('### Verify setting of default distance ###')
    sw1('ip route 192.168.3.0/24 192.168.1.2')
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' in ret and '192.168.1.2' in ret \
           and 'static' in ret and '[1/0]' in ret, \
           'Default distance verification failed'

    step('### Verify setting of multiple nexthops for a given prefix ###')
    sw1('ip route 192.168.3.0/24 1')
    sleep(2)
    sw1('ip route 192.168.3.0/24 2')
    sleep(2)
    ret = sw1('do show ip route')

    assert '192.168.3.0/24' in ret and '3 unicast next-hops' in ret \
           and '1,' in ret and '2,' in ret and '[1/0]' in ret \
           and 'static' in ret, 'Multiple nexthops verification failed'

    step('### Test to verify IPv6 static routes ###')
    step('### Verify ip route configuration with nexthop address ###')
    sw1('ipv6 route 2002::/120 2000::2 2')
    sleep(1)
    ret = sw1('do show ipv6 route')

    assert '2002::/120' in ret and '2000::2' in ret and 'static' \
           in ret and '[2/0]' in ret, 'IPv6 route configuration failed'

    step('### Verify deletion of ipv6 route ###')
    sw1('no ipv6 route 2002::/120 2000::2 2')
    ret = sw1('do show ipv6 route')

    assert '2002::/120' not in ret and '2000::2' not in ret \
           and 'static' not in ret and '[2/0]' not in ret, \
           'Deletion of ipv6 route failed'

    step('### Verify prefix format ###')
    sw1('ipv6 route 2002:: 2000::2 2')
    ret = sw1('do show ipv6 route')

    assert '2002::/120' not in ret and '2000::2' not in ret \
           and 'static' not in ret and '[2/0]' not in ret, \
           'Prefix format verification failed'

    step('### Verify ipv6 route configuration with nexthop interface ###')
    sw1('ipv6 route 2002::/120 2 2')
    sleep(2)
    ret = sw1('do show ipv6 route')

    assert '2002::/120' in ret and '2,' in ret and 'static' in ret \
           and '[2/0]' in ret, 'IPv6 route configuration failed'

    step('### Verify deletion of ipv6 route with nexthop interface ###')
    sw1('no ipv6 route 2002::/120 2 2')
    ret = sw1('do show ipv6 route')

    assert '2002::/120' not in ret and 'static' not in ret \
           and '[2/0]' not in ret, 'Deletion of ipv6 routes failed'

    step('### Verify setting of default distance ###')
    sw1('ipv6 route 2002::/120 2000::2')
    sleep(2)
    ret = sw1('do show ipv6 route')

    assert '2002::/120' in ret and 'static' in ret and '[1/0],' \
           in ret, 'Default distance verification failed'

    step('### Verify setting of multiple nexthops for a given prefix ###')
    sw1('ipv6 route 2002::/120 1')
    sleep(2)
    sw1('ipv6 route 2002::/120 2')
    sleep(2)
    ret = sw1('do show ipv6 route')

    assert '2002::/120' in ret and '3 unicast next-hops' in ret \
           and '1,' in ret and '2,' in ret and '[1/0]' in ret, \
           'Multiple nexthops prefix verification failed'

    step('### Test to verify "show rib" ###')
    step('### Verify show rib for added static routes ###')
    ret = sw1('do show rib')

    assert '*192.168.3.0/24,' in ret and '3 unicast next-hops' \
           in ret and '*via' in ret and 'ipv4' in ret and 'ipv6' \
           in ret and '*2002::/120,' in ret and '[1/0],' in ret, \
           'show rib command failure'

    sw1('no ip route 192.168.3.0/24 192.168.1.2')
    sw1('no ip route 192.168.3.0/24 1')
    sw1('no ip route 192.168.3.0/24 2')
    sw1('no ipv6 route 2002::/120 2000::2')
    sw1('no ipv6 route 2002::/120 1')
    sw1('no ipv6 route 2002::/120 2')

    step('### Test to verify "show running-config" ###')
    sw1('interface {sw1p3}'.format(**locals()))
    sw1('ip address 10.0.0.5/8')
    sw1('ipv6 address 2003::2/120')
    sw1('interface {sw1p4}'.format(**locals()))
    sw1('ip address 10.0.0.7/8')
    sw1('ipv6 address 2004::2/120')
    sleep(1)

    step('### Adding Ipv4 Routes ###')
    cli_list = []
    sw1('ip route 10.0.0.1/32 10.0.0.2')
    cli_list.append('ip route 10.0.0.1/32 10.0.0.2')
    sw1('ip route 10.0.0.3/32 10.0.0.4 4')
    cli_list.append('ip route 10.0.0.3/32 10.0.0.4 4')
    sw1('ip route 10.0.0.6/32 3')
    cli_list.append('ip route 10.0.0.6/32 3')
    sw1('ip route 10.0.0.8/32 4 4')
    cli_list.append('ip route 10.0.0.8/32 4 4')

    step('### Adding Ipv6 Routes ###')
    sw1('ipv6 route 2001::/120 2001::2')
    cli_list.append('ipv6 route 2001::/120 2001::2')
    sw1('ipv6 route 2002::/120 2002::2 3')
    cli_list.append('ipv6 route 2002::/120 2002::2 3')
    sw1('ipv6 route 2003::/120 3')
    cli_list.append('ipv6 route 2003::/120 3')
    sw1('ipv6 route 2004::/120 4 4')
    cli_list.append('ipv6 route 2004::/120 4 4')

    out = sw1('do show running-config')
    lines = out.split('\n')
    found = 0
    for line in lines:
        if line in cli_list:
            found = found + 1

    step('### Verify show running-config for added static routes ###')
    assert found == 8, 'show running-config command failure'

    # The following is to verify that the test have connection
    step('### Deleting Ipv4 Routes ###')
    sw1('no ip route 10.0.0.1/32 10.0.0.2')
    sw1('no ip route 10.0.0.3/32 10.0.0.4 4')
    sw1('no ip route 10.0.0.6/32 3')
    sw1('no ip route 10.0.0.8/32 4 4')

    step('### Deleting Ipv6 Routes ###')
    sw1('no ipv6 route 2001::/120 2001::2')
    sw1('no ipv6 route 2002::/120 2002::2 3')
    sw1('no ipv6 route 2003::/120 3')
    sw1('no ipv6 route 2004::/120 4 4')

    sw1("ip route 192.168.3.0/24 192.168.1.2")
    sw1("ipv6 route 2002::/120 2000::2 2")
    sw2("ip route 192.168.2.0/24 192.168.1.1")
    sw2("ipv6 route 2001::/120 2000::1 2")

    sleep(1)

    out = sw1('do sh ip route')
    assert "192.168.3.0/24" and "[1/0]" and "static" in out

    out = sw1('do sh ipv6 route')
    assert "2002::/120" and "2000::2" and "[2/0]" and "static" in out

    out = sw2('do sh ip route')
    assert "192.168.2.0/24" and "192.168.1.1" and "[1/0]" and "static" in out

    out = sw2('do sh ipv6 route')
    assert "2001::/120" and "2000::1" and "[2/0]" and "static" in out

    out = h1('ping6 -c 5 2002::2')
    assert "5 packets transmitted" and "5 received" and " 0% packet loss" in \
        out

    out = h1('ping -c 5 192.168.3.2')
    assert "5 packets transmitted" and "5 received" and " 0% packet loss" in \
        out
