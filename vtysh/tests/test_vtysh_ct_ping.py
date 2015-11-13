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

class pingCLITest(OpsVsiTest):

    def setupNet(self):
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=VsiOpenSwitch,
                           host=OpsVsiHost,
                           link=OpsVsiLink,
                           controller=None,
                           build=True)

    def pingValidationIpv4Test(self):
        info('\n########## ping IP validations ##########\n')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping 300.300.300.300")
        assert 'Invalid ipv4 address/Hostname' in ret, \
            'Ping IP address validation failed'
        info('\n### Ping IP address validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 data-size 65469")
        assert '% command incomplete.'not in ret, \
            'Ping IP :data-size validation failed'
        info('\n### Ping IP: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 data-fill pa")
        assert 'pattern for datafill should be in hex only' in ret, \
            'Ping IP :data-fill validation failed'
        info('\n### Ping IP: value of data-fill validation passed ###\n)

        ret = s1.cmdCLI("ping 1.1.1.1 repetitions 0")
        assert '% command incomplete.'not in ret, \
            'Ping IP :repetition validation failed'
        info('\n### Ping IP: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 repetitions 100000")
        assert '% command incomplete.'not in ret, \
            'Ping IP :repetition validation failed'
        info('\n### Ping IP: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 timeout 61")
        assert '% command incomplete.'not in ret, \
            'Ping IP :timeout validation failed'
        info('\n### Ping IP: value of timeout validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 interval 61")
        assert '% command incomplete.'not in ret, \
            'Ping IP :interval validation failed'
        info('\n### Ping IP: value of interval validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 tos 256")
        assert '% command incomplete.'not in ret, \
            'Ping IP :tos validation failed'
        info('\n### Ping IP: value of tos validation passed ###\n')

    def pingValidationIpv4HostTest(self):
        s1 = self.net.switches[0]
        info('\n########## ping Host validations ##########\n')
        ret = s1.cmdCLI("ping google")
        assert 'ping: unknown host'not in ret, \
            'Ping Host :Host validation failed'
        info('\n### Ping Host: Host validation passed ###\n')

        ret = s1.cmdCLI("ping testname data-size 65469")
        assert '% command incomplete.'not in ret, \
            'Ping Host :data-size validation failed'
        info('\n### Ping Host: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping testname data-fill pa")
        assert 'pattern for datafill should be in hex only' in ret, \
            'Ping Host :data-fill validation failed'
        info('\n### Ping Host: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping testname repetitions 0")
        assert '% command incomplete.'not in ret, \
            'Ping Host :repetition validation failed'
        info('\n### Ping Host: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping testname repetitions 100000")
        assert '% command incomplete.'not in ret, \
            'Ping Host :repetition validation failed'
        info('\n### Ping Host: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping testname timeout 61")
        assert '% command incomplete.'not in ret, \
            'Ping Host :timeout validation failed'
        info('\n### Ping Host: value of timeout validation passed ###\n')

        ret = s1.cmdCLI("ping testname interval 61")
        assert '% command incomplete.'not in ret, \
            'Ping Host :interval validation failed'
        info('\n### Ping Host: value of interval validation passed ###\n')

        ret = s1.cmdCLI("ping testname tos 256")
        assert '% command incomplete.'not in ret, \
            'Ping Host :tos validation failed'
        info('\n### Ping Host: value of tos validation passed ###\n')


    def pingValidationIpv6Test(self):
        info('\n########## ping6 IP validations ##########\n')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping6 1:1::1:1")
        assert 'Invalid ipv6 address/Hostname' not in ret, \
            'Ping IPv6 address validation failed'
        info('\n### Ping IPv6 address validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 data-size 65469")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :data-size validation failed'
        info('\n### Ping IPv6: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 data-fill pa")
        assert 'pattern for datafill should be in hex only' in ret, \
            'Ping IPv6 :data-fill validation failed'
        info('\n### Ping IPv6: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 repetitions 0")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 repetitions 100000")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 interval 61")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :interval validation failed'
        info('\n### Ping IPv6: value of interval validation passed ###\n')

    def pingValidationIpv6HostTest(self):
        info('\n########## ping6 Host validations ##########\n')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping6 google")
        assert 'ping6: unknown host google' not in ret, \
            'Ping IPv6 Host validation failed'
        info('\n### Ping IPv6 Host validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname data-size 65469")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :data-size validation failed'
        info('\n### Ping IPv6: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname data-fill pa")
        assert 'pattern for datafill should be in hex only' in ret, \
            'Ping IPv6 :data-fill validation failed'
        info('\n### Ping IPv6: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname repetitions 0")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname repetitions 100000")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname interval 61")
        assert '% command incomplete.'not in ret, \
            'Ping IPv6 :interval validation failed'
        info('\n### Ping IPv6: value of interval validation passed ###\n')


class Test_vtysh_ping:

    def setup_class(cls):
        # Create a test topology
        Test_vtysh_ping.test = pingCLITest()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology
        Test_vtysh_ping.test.net.stop()

    def test_pingValidationIpv4Test(self):
        self.test.pingValidationIpv4Test()

    def test_pingValidationIpv4HostTest(self):
        self.test.pingValidationIpv4HostTest()

    def test_pingValidationIpv6Test(self):
        self.test.pingValidationIpv6Test()

    def test_pingValidationIpv6HostTest(self):
        self.test.pingValidationIpv6HostTest()

    def __del__(self):
        del self.test
