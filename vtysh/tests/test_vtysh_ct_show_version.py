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

from mininet.node import Host
from mininet.net import Mininet
from mininet.topo import SingleSwitchTopo
from opsvsi.opsvsitest import OpsVsiTest, OpsVsiLink, VsiOpenSwitch


class ShowVersionTest(OpsVsiTest):
    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts
        # of the topology that you build or into
        # addHost/addSwitch calls
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        system_topo = SingleSwitchTopo(k=0, hopts=host_opts,
                                       sopts=switch_opts)
        self.net = Mininet(system_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)
        self.s1 = self.net.switches[0]

    def setup(self):
        self.old_switch_version = None
        self.old_os_name = None
        out = self.s1.ovscmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                self.uuid = line.split(':')[1].strip()
            elif 'switch_version' in line:
                self.old_switch_version = line.split(':')[1].strip()
            elif 'software_info' in line:
                kvs = line.split(':')[1].strip(' {}\r\n')
                if kvs:
                    for kv in kvs.split(', '):
                        key, value = kv.split('=')
                        if 'os_name':
                            self.old_os_name = value
                            print(self.old_os_name)

    def teardown(self):
        self.__set_version(self.old_switch_version)
        self.__set_os_name(self.old_os_name)

    def __set_os_name(self, os_name):
        if os_name:
            self.s1.ovscmd('ovs-vsctl set system ' + self.uuid
                           + ' software_info:os_name=' + os_name)
        else:
            self.s1.ovscmd('ovs-vsctl remove system ' + self.uuid
                           + ' software_info os_name')

    def __set_version(self, version):
        self.s1.ovscmd('ovs-vsctl -- set system ' + self.uuid
                       + ' switch_version=' + version)

    def __get_show_version_result(self):
        return self.s1.cmdCLI('show version').split('\n')[1]

    def check_show_version(self, os_name="OpenSwitch", version="0.1.0"):
        self.__set_os_name(os_name)
        self.__set_version(version)
        assert os_name + " " + version == self.__get_show_version_result()


class TestRunner:
    @classmethod
    def setup_class(cls):
        cls.test = ShowVersionTest()

    @classmethod
    def teardown_class(cls):
        cls.test.stopNet()
        cls.test = None

    def setup(self):
        self.test.setup()

    def teardown(self):
        self.test.teardown()

    def test_os_name(self):
        return self.test.check_show_version(os_name="TestOS")

    def test_os_name_with_whitespace(self):
        return self.test.check_show_version(os_name="TestVenter TestOS")

    def test_normal_version(self):
        return self.test.check_show_version(version="1.1.1")

    def test_string_version(self):
        return self.test.check_show_version(version="string_ver")
