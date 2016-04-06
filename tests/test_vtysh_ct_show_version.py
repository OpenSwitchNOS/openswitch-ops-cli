#!/usr/bin/python
#
# (c) Copyright 2015, 2016 Hewlett Packard Enterprise Development LP
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#

from mininet.node import Host
from mininet.net import Mininet
from mininet.topo import SingleSwitchTopo
from opsvsi.opsvsitest import OpsVsiTest, OpsVsiLink, VsiOpenSwitch
import pytest


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

    def teardown(self):
        self.__set_version(self.old_switch_version)
        self.__set_os_name(self.old_os_name)

    def __set_os_name(self, os_name):
        if os_name:
            self.s1.ovscmd('ovs-vsctl set system {0} '
                           'software_info:os_name="{1}"'.
                           format(self.uuid, os_name))
        else:
            self.s1.ovscmd('ovs-vsctl remove system {0} '
                           'software_info os_name'.format(self.uuid))

    def __set_version(self, version):
        self.s1.ovscmd('ovs-vsctl -- set system {0} switch_version="{1}"'.
                       format(self.uuid, version))

    def __get_show_version_result(self):
        return self.s1.cmdCLI('show version').split('\n')[1]

    def check_show_version(self, os_name="OpenSwitch", version="0.1.0"):
        self.__set_os_name(os_name)
        self.__set_version(version)
        assert os_name + " " + version == self.__get_show_version_result()


@pytest.mark.skipif(True, reason="Disabling old tests")
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
