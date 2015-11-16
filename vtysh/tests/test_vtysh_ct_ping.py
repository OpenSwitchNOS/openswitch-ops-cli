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
        assert 'Invalid IPv4 Address' in ret, \
            'Ping IP address validation failed'
        info('\n### Ping IP address validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 data-size 65469")
        assert '% Unknown command.' in ret, \
            'Ping IP :data-size validation failed'
        info('\n### Ping IP: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 data-fill pa")
        assert 'datafill pattern should be in hexadecimal only' in ret,\
            'Ping IP :data-fill validation failed'
        info('\n### Ping IP: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 repetitions 0")
        assert '% Unknown command.' in ret, \
            'Ping IP :repetition validation failed'
        info('\n### Ping IP: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 repetitions 100000")
        assert '% Unknown command.' in ret, \
            'Ping IP :repetition validation failed'
        info('\n### Ping IP: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 timeout 61")
        assert '% Unknown command.' in ret, \
            'Ping IP :timeout validation failed'
        info('\n### Ping IP: value of timeout validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 interval 61")
        assert '% Unknown command.' in ret, \
            'Ping IP :interval validation failed'
        info('\n### Ping IP: value of interval validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 tos 256")
        assert '% Unknown command.' in ret, \
            'Ping IP :tos validation failed'
        info('\n### Ping IP: value of tos validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 ip-option include-timestamp 3")
        assert '% Unknown command.' in ret, \
            'Ping IP :ip-option include-timestamp validation failed'
        info('\n### Ping IP: ip-option include-timestamp validation'
             ' passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 ip-option \
            include-timestamp-and-address 3")
        assert '% Unknown command.' in ret, \
            'Ping IP :ip-option include-timestamp-address validation failed'
        info('\n### Ping IP: ip-option include-timestamp-address'
             ' validation passed ###\n')

        ret = s1.cmdCLI("ping 1.1.1.1 ip-option record-route 3")
        assert '% Unknown command.' in ret, \
            'Ping IP :ip-option record-route validation failed'
        info('\n### Ping IP: ip-option record-route validation passed ###\n')

    def pingValidationIpv4HostTest(self):
        s1 = self.net.switches[0]
        info('\n########## ping Host validations ##########\n')

        ret = s1.cmdCLI("ping testname data-size 65469")
        assert '% Unknown command.' in ret, \
            'Ping Host :data-size validation failed'
        info('\n### Ping Host: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping testname data-fill pa")
        assert 'datafill pattern should be in hexadecimal only' in ret, \
            'Ping Host :data-fill validation failed'
        info('\n### Ping Host: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping testname repetitions 0")
        assert '% Unknown command.' in ret, \
            'Ping Host :repetition validation failed'
        info('\n### Ping Host: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping testname repetitions 100000")
        assert '% Unknown command.' in ret, \
            'Ping Host :repetition validation failed'
        info('\n### Ping Host: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping testname timeout 61")
        assert '% Unknown command.' in ret, \
            'Ping Host :timeout validation failed'
        info('\n### Ping Host: value of timeout validation passed ###\n')

        ret = s1.cmdCLI("ping testname interval 61")
        assert '% Unknown command.' in ret, \
            'Ping Host :interval validation failed'
        info('\n### Ping Host: value of interval validation passed ###\n')

        ret = s1.cmdCLI("ping testname tos 256")
        assert '% Unknown command.' in ret, \
            'Ping Host :tos validation failed'
        info('\n### Ping Host: value of tos validation passed ###\n')

        ret = s1.cmdCLI("ping testname ip-option include-timestamp 3")
        assert '% Unknown command.' in ret, \
            'Ping Host :ip-option include-timestamp validation failed'
        info('\n### Ping Host: ip-option include-timestamp validation'
             ' passed ###\n')

        ret = s1.cmdCLI("ping testname ip-option \
            include-timestamp-and-address 3")
        assert '% Unknown command.' in ret, \
            'Ping Host :ip-option include-timestamp-address validation failed'
        info('\n### Ping Host: ip-option include-timestamp-address'
             ' validation passed ###\n')

        ret = s1.cmdCLI("ping testname ip-option record-route 3")
        assert '% Unknown command.' in ret, \
            'Ping Host :ip-option record-route validation failed'
        info('\n### Ping Host: ip-option record-route validation passed ###\n')

    def pingValidationIpv6Test(self):
        info('\n########## ping6 IP validations ##########\n')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping6 1.1::1.1")
        assert 'Invalid IPv6 Address' in ret, \
            'Ping IPv6 address validation failed'
        info('\n### Ping IPv6 address validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 data-size 65469")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :data-size validation failed'
        info('\n### Ping IPv6: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 data-fill pa")
        assert 'datafill pattern should be in hexadecimal only' in ret, \
            'Ping IPv6 :data-fill validation failed'
        info('\n### Ping IPv6: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 repetitions 0")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 repetitions 100000")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 1:1::1:1 interval 61")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :interval validation failed'
        info('\n### Ping IPv6: value of interval validation passed ###\n')

    def pingValidationIpv6HostTest(self):
        info('\n########## ping6 Host validations ##########\n')
        s1 = self.net.switches[0]

        ret = s1.cmdCLI("ping6 testname data-size 65469")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :data-size validation failed'
        info('\n### Ping IPv6: value of data-size validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname data-fill pa")
        assert 'datafill pattern should be in hexadecimal only' in ret, \
            'Ping IPv6 :data-fill validation failed'
        info('\n### Ping IPv6: value of data-fill validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname repetitions 0")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname repetitions 100000")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :repetition validation failed'
        info('\n### Ping IPv6: value of repetition validation passed ###\n')

        ret = s1.cmdCLI("ping6 testname interval 61")
        assert '% Unknown command.' in ret, \
            'Ping IPv6 :interval validation failed'
        info('\n### Ping IPv6: value of interval validation passed ###\n')

    def pingTargetIpv4Ip(self):
        ping_target_ip_set = False
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping 127.0.0.1")
        lines = ret.split('\n')
        for line in lines:
            if '127.0.0.1' in line:
                ping_target_ip_set = True
        assert (ping_target_ip_set is True), \
            'Ping IPv4 with IP address test- FAILED!'
        info('\n### Ping IPv4 with IP address test passed\n')

        ping_target_ip_set = False
        ret = s1.cmdCLI("ping localhost data-fill dee datagram-size 200"
                        " interval 2 repetitions 1 timeout 2 tos 0 ip-option"
                        " include-timestamp-and-address")
        lines = ret.split('\n')
        for line in lines:
            if '127.0.0.1' in line and '200 data bytes' in line:
                ping_target_ip_set = True
        assert (ping_target_ip_set is True), \
            'Ping IPv4 with IP address and multiple parameters test- FAILED!'
        info('\n### Ping IPv4 with IP address and multiple parameters'
             ' test passed\n')

    def pingTargetIpv4Host(self):
        ping_target_host_set = False
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping localhost")
        lines = ret.split('\n')
        for line in lines:
            if 'PING localhost' in line:
                ping_target_host_set = True
        assert (ping_target_host_set is True), \
            'Ping IPv4 with Hostname test- FAILED!'
        info('\n### Ping IPv4 with Hostname test passed\n')

        ping_target_host_set = False
        ret = s1.cmdCLI("ping localhost data-fill dee datagram-size 200"
                        " interval 2 repetitions 1 timeout 2 tos 0"
                        " ip-option include-timestamp")
        lines = ret.split('\n')
        for line in lines:
            if 'PING localhost' in line and '200 data bytes' in line:
                ping_target_host_set = True
        assert (ping_target_host_set is True), \
            'Ping IPv4 with Hostname and multiple parameters test- FAILED!'
        info('\n### Ping IPv4 with Hostname and multiple parameters'
             ' test passed\n')

    def pingTargetIpv6Ip(self):
        ping_target_ip_set = False
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping6 ::1")
        lines = ret.split('\n')
        for line in lines:
            if 'PING ::1 (::1)' in line:
                ping_target_ip_set = True
        assert (ping_target_ip_set is True), \
            'Ping6 with IPv6 address test- FAILED!'
        info('\n### Ping6 with IPv6 address test passed\n')

        ping_target_ip_set = False
        ret = s1.cmdCLI("ping6 ::1")
        lines = ret.split('\n')
        for line in lines:
            if 'PING ::1 (::1)' in line:
                ping_target_ip_set = True
        assert (ping_target_ip_set is True), \
            'Ping6 with IPv6 address test- FAILED!'
        info('\n### Ping6 with IPv6 address test passed\n')

        ping_target_ip_set = False
        ret = s1.cmdCLI("ping6 ::1 data-fill dee datagram-size 200"
                        " interval 2 repetitions 1")
        lines = ret.split('\n')
        for line in lines:
            if 'PING ::1 (::1)' in line and '200 data bytes' in line:
                ping_target_ip_set = True
        assert (ping_target_ip_set is True), \
            'Ping6 with IPv6 address with multiple parameters test- FAILED!'
        info('\n### Ping6 with IPv6 address with multiple parameters'
             ' test passed\n')

    def pingTargetIpv6Host(self):
        ping_target_host_set = False
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("ping6 localhost")
        lines = ret.split('\n')
        for line in lines:
            if 'PING localhost (::1)' in line:
                ping_target_host_set = True
        assert (ping_target_host_set is True), \
            'Ping6 with Hostname test- FAILED!'
        info('\n### Ping6 with Hostname test passed\n')

        ping_target_host_set = False
        ret = s1.cmdCLI("ping6 localhost data-fill dee datagram-size 200"
                        " interval 2 repetitions 1")
        lines = ret.split('\n')
        for line in lines:
            if 'PING localhost (::1)' in line and '200 data bytes' in line:
                ping_target_host_set = True
        assert (ping_target_host_set is True), \
            'Ping6 with Hostname with multiple parameters test- FAILED!'
        info('\n### Ping6 with Hostname with multiple parameters'
             ' test passed\n')


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

    def test_pingTargetIpv4Ip(self):
        self.test.pingTargetIpv4Ip()

    def test_pingTargetIpv4Host(self):
        self.test.pingTargetIpv4Host()

    def test_pingTargetIpv6Ip(self):
        self.test.pingTargetIpv6Ip()

    def test_pingTargetIpv6Host(self):
        self.test.pingTargetIpv6Host()

    def __del__(self):
        del self.test
