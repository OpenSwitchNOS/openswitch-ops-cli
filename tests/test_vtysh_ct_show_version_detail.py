#!/usr/bin/python
#
# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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


class ShowVersionDetailCliCtTest(OpsVsiTest):
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

    def __get_show_version_detail_cli_ct_result(self):
        lines = self.s1.cmdCLI('show version detail').split('\n')
        lines = [line.replace(' ', '') for line in lines]
        output = ''.join(lines)
        return output

    def __add_git_entry_to_package_info(self):
        self.s1.ovscmd('ovsdb-client transact \'["OpenSwitch", \
                       {"op":"insert", "table": "Package_Info", "row": \
                       {"name": "test-repo-1", "src_type": "git", \
                       "src_url": "git.testRepo1.net", \
                       "version":"abcdef007" } } ]\'')

    def __add_other_entry_to_package_info(self):
        self.s1.ovscmd('ovsdb-client transact \'["OpenSwitch", \
                       {"op":"insert", "table": "Package_Info", "row": \
                       {"name": "test-repo-2", "src_type": "other", \
                       "src_url": "ftp.testRepo2.com/file.tar.gz",\
                       "version":"1.0.0" } } ]\'')

    def check_show_version_detail_cli_ct(self):
        expected_record1 = "PACKAGE:test-repo-1" + "VERSION:abcdef007" +\
            "SOURCETYPE:git" + "SOURCEURL:git.testRepo1.net"

        expected_record2 = "PACKAGE:test-repo-2" + "VERSION:1.0.0" +\
            "SOURCETYPE:other" + "SOURCEURL:ftp.testRepo2.com/file.tar.gz"

        self.__add_git_entry_to_package_info()
        self.__add_other_entry_to_package_info()
        output = self.__get_show_version_detail_cli_ct_result()
        assert expected_record1 in output and expected_record2 in output, \
            "Inserted records to Package_Info were not present in show version\
            detail output"

@pytest.mark.skipif(True, reason="Disabling old tests")
class TestRunner:
    @classmethod
    def setup_class(cls):
        cls.test = ShowVersionDetailCliCtTest()

    @classmethod
    def teardown_class(cls):
        cls.test.stopNet()
        cls.test = None

    def test_show_version_detail_cli_ct(self):
        return self.test.check_show_version_detail_cli_ct()
