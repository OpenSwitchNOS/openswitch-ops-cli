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
import time


class TracerouteTest(OpsVsiTest):

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=VsiOpenSwitch,
                           host=OpsVsiHost,
                           link=OpsVsiLink,
                           controller=None,
                           build=True)

    def traceroute_ip(self):
        info('''\n########## traceroute ip validations ##########\n''')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("traceroute 300.300.300.300")
        assert '% Invalid IPv4 address.' not in ret, \
            'Traceroute IP address validation failed'
        info('\n### Traceroute  ip validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 dstport 48000")
        assert '% Unknown command.' in ret, \
            'Traceroute dstport validation failed'
        info('\n### Traceroute ip dstport validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 maxttl 296")
        assert '% Unknown command.' in ret, \
            'Traceroute maxttl validation failed'
        info('\n### Traceroute ip maxttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 minttl 296")
        assert '% Unknown command.' in ret, \
            'Traceroute minttl validation failed'
        info('\n### Traceroute ip  minttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 timeout 155")
        assert '% Unknown command.' in ret, \
            'Traceroute timeout validation failed'
        info('\n### Traceroute ip timeout validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 probes 60")
        assert '% Unknown command.' in ret, \
            'Traceroute probes validation failed'
        info('\n### Traceroute ip  probes validation passed ###\n')

        ret = s1.cmdCLI("traceroute 1.1.1.1 ip-option \
        loose-source-route 300.300.300.300")
        assert '% invalid IP address.' not in ret, \
            'Traceroute ip-option loose-source-route validation failed'
        info('\n### Traceroute ip ip-option \
        loose-source-route validation passed ###\n')

    def traceroute_hostname(self):
        info('''\n########## traceroute hostname validations ##########\n''')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("traceroute test_name")
        assert 'traceroute: unknown host' not in ret, \
            'Traceroute hostname validation failed'
        info('\n### Traceroute hostname validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost dstport 48000")
        assert '% Unknown command.' in ret, \
            'Traceroute dstport validation failed'
        info('\n### Traceroute hostname dstport validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost maxttl 296")
        assert '% Unknown command.' in ret, \
            'Traceroute maxttl validation failed'
        info('\n### Traceroute hostname maxttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost minttl 260")
        assert '% Unknown command.' in ret, \
            'Traceroute minttl validation failed'
        info('\n### Traceroute hostname minttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost timeout 155")
        assert '% Unknown command.' in ret, \
            'Traceroute timeout validation failed'
        info('\n### Traceroute hostname timeout validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost probes 60")
        assert '% Unknown command.' in ret, \
            'Traceroute probes validation failed'
        info('\n### Traceroute hostname probes validation passed ###\n')

        ret = s1.cmdCLI("traceroute localhost ip-option \
        loose-source-route 300.300.300.300")
        assert '% Invalid IPv4 address.' not in ret, \
            'Traceroute ip-option loose-source-route validation failed'
        info('\n### Traceroute hostname ip-option \
        loose-source-route validation passed ###\n')

    def traceroute6_ip(self):
        info('''\n########## traceroute6 ip validations ##########\n''')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("traceroute6 1.1:.1.1")
        assert '% Invalid IPv6 address.' not in ret, \
            'Traceroute6 IP address validation failed'
        info('\n### Traceroute6  ip validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 1:1::1:1 dstport 48000")
        assert '% Unknown command.' in ret, \
            'Traceroute6 dstport validation failed'
        info('\n### Traceroute6 ip dstport validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 1:1::1:1 maxttl 296")
        assert '% Unknown command.' in ret, \
            'Traceroute6 maxttl validation failed'
        info('\n### Traceroute6 ip maxttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 1:1::1:1 timeout 155")
        assert '% Unknown command.' in ret, \
            'Traceroute6 timeout validation failed'
        info('\n### Traceroute6 ip timeout validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 1:1::1:1 probes 60")
        assert '% Unknown command.' in ret, \
            'Traceroute6 probes validation failed'
        info('\n### Traceroute6 ip  probes validation passed ###\n')

    def traceroute6_hostname(self):
        info('''\n########## traceroute6 hostname validations ##########\n''')
        s1 = self.net.switches[0]
        ret = s1.cmdCLI("traceroute6 test_name")
        assert 'traceroute: unknown host' not in ret, \
            'Traceroute6 hostname validation failed'
        info('\n### Traceroute6 hostname validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 localhost dstport 48000")
        assert '% Unknown command.' in ret, \
            'Traceroute6 dstport validation failed'
        info('\n### Traceroute6 hostname dstport validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 localhost maxttl 296")
        assert '% Unknown command.' in ret, \
            'Traceroute6 maxttl validation failed'
        info('\n### Traceroute6 hostname maxttl validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 localhost timeout 155")
        assert '% Unknown command.' in ret, \
            'Traceroute6 timeout validation failed'
        info('\n### Traceroute6 hostname timeout validation passed ###\n')

        ret = s1.cmdCLI("traceroute6 localhost probes 60")
        assert '% Unknown command.' in ret, \
            'Traceroute6 probes validation failed'
        info('\n### Traceroute6 hostname probes validation passed ###\n')

    def traceroute_target_ipv4(self):
        info('''\n########## Verification of valid cases ##########\n''')
        info('''\n########## Test traceroute target ip ##########\n''')
        traceroute_target_ip_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_ip_set = True
        assert(traceroute_target_ip_set is True), \
            'Test setting traceroute target Ip -FAILED!'
        info('\n### traceroute target ip verification successfull ###\n')

    def traceroute_target_maxttl(self):
        info('''\n########## test traceroute target maxttl ##########\n''')
        traceroute_target_maxttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 maxttl 30")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if ' 30 hops max' in line:
                traceroute_target_maxttl_set = True
        assert(traceroute_target_maxttl_set is True), \
            'Test setting traceroute target maxttl -FAILED!'
        info('\n### traceroute target maxttl verification successfull ###\n')

    def traceroute_target_dstport(self):
        info('''\n########## Test traceroute target dstport ##########\n''')
        traceroute_target_dstport_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 dstport 33434")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_dstport_set = True
        assert(traceroute_target_dstport_set is True), \
            'Test setting traceroute target portport -FAILED!'
        info('\n### traceroute target ip dstport \
        verification successfull ###\n')

    def traceroute_target_minttl(self):
        info('''\n########## test traceroute target minttl ##########\n''')
        traceroute_target_minttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 minttl 1")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_minttl_set = True
        assert(traceroute_target_minttl_set is True), \
            'Test setting traceroute target minttl -FAILED!'
        info('\n### traceroute target minttl verification successfull ###\n')

    def traceroute_target_probes(self):
        info('''\n########## Test traceroute target probes ##########\n''')
        traceroute_target_probes_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 probes 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_probes_set = True
        assert(traceroute_target_probes_set is True), \
            'Test setting traceroute target probes -FAILED!'
        info('\n### traceroute target ip probes \
        verification successfull ###\n')

    def traceroute_target_timeout(self):
        info('''\n########## test traceroute target timeout ##########\n''')
        traceroute_target_timeout_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 timeout 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_timeout_set = True
        assert(traceroute_target_timeout_set is True), \
            'Test setting traceroute target timeout -FAILED!'
        info('\n### traceroute target timeout verification successfull ###\n')

    def traceroute_target_ipoption(self):
        info('''\n########## test traceroute target ip-option ##########\n''')
        traceroute_target_ipoption_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute 1.1.1.1 ip-option \
        loosesourceroute 1.1.1.5")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 1.1.1.1' in line:
                traceroute_target_ipoption_set = True
        assert(traceroute_target_ipoption_set is True), \
            'Test setting traceroute target ipoption -FAILED!'
        info('\n### traceroute target ipoption verification successfull ###\n')

    def traceroute_target_host(self):
        info('''\n########## Test traceroute target host ##########\n''')
        traceroute_target_host_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_target_host_set = True
        assert(traceroute_target_host_set is True), \
            'Test setting traceroute target host -FAILED!'
        info('\n### traceroute target host verification successfull ###\n')

    def traceroute_targethost_maxttl(self):
        info('''\n########## test traceroute targethost maxttl ##########\n''')
        traceroute_targethost_maxttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost maxttl 30")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if ' 30 hops max' in line:
                traceroute_targethost_maxttl_set = True
        assert(traceroute_targethost_maxttl_set is True), \
            'Test setting traceroute targethost maxttl -FAILED!'
        info('\n### traceroute targethost maxttl \
        verification successfull ###\n')

    def traceroute_targethost_dstport(self):
        info('''\n########## Test traceroute \
        targethhost dstport ##########\n''')
        traceroute_targethost_dstport_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost dstport 33434")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_targethost_dstport_set = True
        assert(traceroute_targethost_dstport_set is True), \
            'Test setting traceroute targethost maxport -FAILED!'
        info('\n### traceroute targethost dstport \
        verification successfull ###\n')

    def traceroute_targethost_minttl(self):
        info('''\n########## test traceroute targethost minttl ##########\n''')
        traceroute_targethost_minttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost minttl 1")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_targethost_minttl_set = True
        assert(traceroute_targethost_minttl_set is True), \
            'Test setting traceroutehost target minttl -FAILED!'
        info('\n### traceroute targethost minttl \
        verification successfull ###\n')

    def traceroute_targethost_probes(self):
        info('''\n########## Test traceroute targethost probes ##########\n''')
        traceroute_targethost_probes_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost probes 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_targethost_probes_set = True
        assert(traceroute_targethost_probes_set is True), \
            'Test setting traceroute targethost probes -FAILED!'
        info('\n### traceroute targethost probes \
        verification successfull ###\n')

    def traceroute_targethost_timeout(self):
        info('''\n########## test traceroute \
        targethost timeout ##########\n''')
        traceroute_targethost_timeout_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost timeout 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_targethost_timeout_set = True
        assert(traceroute_targethost_timeout_set is True), \
            'Test setting traceroutehost target timeout -FAILED!'
        info('\n### traceroute targethost \
        timeout verification successfull ###\n')

    def traceroute_targethost_ipoption(self):
        info('''\n########## test traceroute \
        targethost ip-option ##########\n''')
        traceroute_targethost_ipoption_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost \
        ip-option loosesourceroute 1.1.1.5")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute_targethost_ipoption_set = True
        assert(traceroute_targethost_ipoption_set is True), \
            'Test setting traceroute targethost ipoption -FAILED!'
        info('\n### traceroute targethost \
        ipoption verification successfull ###\n')

    def traceroute6_target_ipv6(self):
        info('''\n########## Test traceroute6 target ip ##########\n''')
        traceroute6_target_ip_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 0:0::0:1")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 0:0::0:1' in line:
                traceroute6_target_ip_set = True
        assert(traceroute6_target_ip_set is True), \
            'Test setting traceroute6 target Ip -FAILED!'
        info('\n### traceroute6 target ip verification successfull ###\n')

    def traceroute6_target_maxttl(self):
        info('''\n########## test traceroute6 target maxttl ##########\n''')
        traceroute6_target_maxttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 0:0::0:1 maxttl 30")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if ' 30 hops max' in line:
                traceroute6_target_maxttl_set = True
        assert(traceroute6_target_maxttl_set is True), \
            'Test setting traceroute6 target maxttl -FAILED!'
        info('\n### traceroute6 target maxttl verification successfull ###\n')

    def traceroute6_target_dstport(self):
        info('''\n########## Test traceroute6 target dstport ##########\n''')
        traceroute6_target_dstport_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 0:0::0:1 dstport 33434")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 0:0::0:1' in line:
                traceroute6_target_dstport_set = True
        assert(traceroute6_target_dstport_set is True), \
            'Test setting traceroute target dstport -FAILED!'
        info('\n### traceroute6 target \
        ip maxport verification successfull ###\n')

    def traceroute6_target_probes(self):
        info('''\n########## Test traceroute6 target probes ##########\n''')
        traceroute6_target_probes_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 0:0::0:1 probes 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 0:0::0:1' in line:
                traceroute6_target_probes_set = True
        assert(traceroute6_target_probes_set is True), \
            'Test setting traceroute6 target probes -FAILED!'
        info('\n### traceroute6 target \
        ip probes verification successfull ###\n')

    def traceroute6_target_timeout(self):
        info('''\n########## test traceroute6 target timeout ##########\n''')
        traceroute6_target_timeout_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 0:0::0:1 timeout 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to 0:0::0:1' in line:
                traceroute6_target_timeout_set = True
        assert(traceroute6_target_timeout_set is True), \
            'Test setting traceroute6 target timeout -FAILED!'
        info('\n### traceroute6 target timeout verification successfull ###\n')

    def traceroute6_target_host(self):
        info('''\n########## Test traceroute6 target host ##########\n''')
        traceroute6_target_host_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 localhost")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute6_target_host_set = True
        assert(traceroute6_target_host_set is True), \
            'Test setting traceroute6 target host -FAILED!'
        info('\n### traceroute6 target host verification successfull ###\n')

    def traceroute6_targethost_maxttl(self):
        info('''\n########## test traceroute6 \
        targethost maxttl ##########\n''')
        traceroute6_targethost_maxttl_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 localhost maxttl 30")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if ' 30 hops max' in line:
                traceroute6_targethost_maxttl_set = True
        assert(traceroute6_targethost_maxttl_set is True), \
            'Test setting traceroute6 targethost maxttl -FAILED!'
        info('\n### traceroute6 targethost \
        maxttl verification successfull ###\n')

    def traceroute6_targethost_dstport(self):
        info('''\n########## Test traceroute6 \
         targethhost dstport ##########\n''')
        traceroute6_targethost_maxport_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 localhost dstport 33434")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute6_targethost_maxport_set = True
        assert(traceroute6_targethost_maxport_set is True), \
            'Test setting traceroute6 targethost maxport -FAILED!'
        info('\n### traceroute6 targethost \
        maxport verification successfull ###\n')

    def traceroute6_targethost_probes(self):
        info('''\n########## Test traceroute6host \
        target probes ##########\n''')
        traceroute6_target_probes_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute localhost probes 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute6_target_probes_set = True
        assert(traceroute6_target_probes_set is True), \
            'Test setting traceroute6 target probes -FAILED!'
        info('\n### traceroute6 target ip probes \
        verification successfull ###\n')

    def traceroute6_targethost_timeout(self):
        info('''\n########## test traceroutehost \
        target timeout ##########\n''')
        traceroute6_targethost_timeout_set = False
        s1 = self.net.switches[0]
        dump = s1.cmdCLI("traceroute6 localhost timeout 3")
        time.sleep(2)
        lines = dump.split('\n')
        for line in lines:
            if 'traceroute to localhost' in line:
                traceroute6_targethost_timeout_set = True
        assert(traceroute6_targethost_timeout_set is True), \
            'Test setting traceroute6host target timeout -FAILED!'
        info('\n### traceroute6 targethost \
        timeout verification successfull ###\n''')


class Test_traceroute_cli:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_traceroute_cli.test = TracerouteTest()

    def test_traceroute_ip(self):
        self.test.traceroute_ip()

    def test_traceroute_hostname(self):
        self.test.traceroute_hostname()

    def test_traceroute6_ip(self):
        self.test.traceroute6_ip()

    def test_traceroute6_hostname(self):
        self.test.traceroute6_hostname()

    def test_traceroute_target_ipv4(self):
        self.test.traceroute_target_ipv4()

    def test_traceroute_target_maxttl(self):
        self.test.traceroute_target_maxttl()

    def test_traceroute_target_dstport(self):
        self.test.traceroute_target_dstport()

    def test_traceroute_target_minttl(self):
        self.test.traceroute_target_minttl()

    def test_traceroute_target_probes(self):
        self.test.traceroute_target_probes()

    def test_traceroute_target_timeout(self):
        self.test.traceroute_target_timeout()

    def test_traceroute_target_ipoption(self):
        self.test.traceroute_target_ipoption()

    def test_traceroute_target_host(self):
        self.test.traceroute_target_host()

    def test_traceroute_targethost_maxttl(self):
        self.test.traceroute_targethost_maxttl()

    def test_traceroute_targethost_dstport(self):
        self.test.traceroute_targethost_dstport()

    def test_traceroute_targethost_minttl(self):
        self.test.traceroute_targethost_minttl()

    def test_traceroute_targethost_probes(self):
        self.test.traceroute_targethost_probes()

    def test_traceroute_targethost_timeout(self):
        self.test.traceroute_targethost_timeout()

    def test_traceroute_targethost_ipoption(self):
        self.test.traceroute_targethost_ipoption()

    def test_traceroute6_target_ipv6(self):
        self.test.traceroute6_target_ipv6()

    def test_traceroute6_target_maxttl(self):
        self.test.traceroute6_target_maxttl()

    def test_traceroute6_target_dstport(self):
        self.test.traceroute6_target_dstport()

    def test_traceroute6_target_probes(self):
        self.test.traceroute6_target_probes()

    def test_traceroute6_target_timeout(self):
        self.test.traceroute6_target_timeout()

    def test_traceroute6_target_host(self):
        self.test.traceroute6_target_host()

    def test_traceroute6_targethost_maxttl(self):
        self.test.traceroute6_targethost_maxttl()

    def test_traceroute6_targethost_dstport(self):
        self.test.traceroute6_targethost_dstport()

    def test_traceroute6_targethost_timeout(self):
        self.test.traceroute6_targethost_timeout()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology
        Test_traceroute_cli.test.net.stop()

    def __del__(self):
        del self.test
