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


class sftpserver_CLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        sftpserver_topo = SingleSwitchTopo(k=0,
                                           hopts=host_opts,
                                           sopts=switch_opts)
        self.net = Mininet(sftpserver_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_sftpserver_show(self):
        info("\n########## Test to show sftp server "
             "configuration ##########\n")

        sftp_server = False

        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")

        # Test when sftp server enabled #
        s1.cmdCLI("sftp server enable")

        dump = s1.cmdCLI("do show sftp server")
        lines = dump.split('\n')
        for line in lines:
            if "SFTP server : Enabled" in line:
                sftp_server = True

        assert(sftp_server is True), \
               'Test to show Test to show sftp server \
                confguration -FAILED!'

        # Test when sftp server disabled #
        sftp_server = False
        s1.cmdCLI("no sftp server enable")

        dump = s1.cmdCLI("do show sftp server")
        lines = dump.split('\n')
        for line in lines:
            if "SFTP server : Disabled" in line:
                sftp_server = True

        assert(sftp_server is True), \
               'Test to show Test to show sftp server \
                confguration -FAILED!'

        return True

    def test_sftpserver_enable(self):
        info("\n########## Test to enable sftp server ##########\n")

        sftp_enabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("sftp server enable")

        dump = s1.cmdCLI("do show sftp server")
        lines = dump.split('\n')
        for line in lines:
            if "SFTP server : Enabled" in line:
                sftp_enabled = True

        assert(sftp_enabled is True), \
               'Test to enable SFTP server -FAILED!'

        return True

    def test_sftpserver_disable(self):
        info("\n########## Test to disable sftp server ##########\n")

        sftp_disabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("no sftp server enable")

        dump = s1.cmdCLI("do show sftp server")
        lines = dump.split('\n')
        for line in lines:
            if "SFTP server : Disabled" in line:
                sftp_disabled = True

        assert(sftp_disabled is True), \
               'Test to disable sftp server -FAILED!'

        return True


class Test_vtysh_sftpserver:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_sftpserver.test = sftpserver_CLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_sftpserver.test.net.stop()

    def test_sftpserver_show(self):
        if self.test.test_sftpserver_show():
            info("\n###  Test to show sftp server confguration"
                 " - SUCCESS! ###\n")

    def test_sftpserver_enable(self):
        if self.test.test_sftpserver_enable():
            info("\n###  Test to enable SFTP server  - SUCCESS! ###\n")

    def test_sftpserver_disable(self):
        if self.test.test_sftpserver_disable():
            info("\n###  Test to disable SFTP server  - SUCCESS! ###\n")

    def __del__(self):
        del self.test
