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
        lines = [line.replace(' ','') for line in lines]
        return lines

    def __add_git_entry_to_source_repository(self):
        self.s1.ovscmd('ovsdb-client transact \'["OpenSwitch", \
                       {"op":"insert", "table": "Source_Repository", "row": \
                       {"name": "test-repo-1", "src_uri": "git://git.testRepo1.net", "version":"abcdef007" } } ]\'')

    def __add_other_entry_to_source_repository(self):
        self.s1.ovscmd('ovsdb-client transact \'["OpenSwitch", \
                       {"op":"insert", "table": "Source_Repository", "row": \
                       {"name": "test-repo-2", "src_uri": "http://ftp.testRepo2.com/file.tar.gz", "version":"1.0.0" } } ]\'')

    def check_show_version_detail_cli_ct(self):
        count = 0
        expected_line1 = "test-repo-1"+"git://git.testRepo1.net"+"abcdef007"
        expected_line2 = "test-repo-2"+"http://ftp.testRepo2.com/file.tar.gz"+"1.0.0"
        self.__add_git_entry_to_source_repository()
        self.__add_other_entry_to_source_repository()
        lines = self.__get_show_version_detail_cli_ct_result()
        for line in lines:
            if line == expected_line1 or line == expected_line2:
                count += 1
        assert count == 2, "Inserted records to Source_Repository were not present in show version detail output"

class ShowVersionDetailTestRunner:
    @classmethod
    def setup_class(cls):
        cls.test = ShowVersionDetailCliCtTest()

    @classmethod
    def teardown_class(cls):
        cls.test.stopNet()
        cls.test = None

    def test_show_version_detail_cli_ct(self):
        return self.test.check_show_version_detail_cli_ct()
