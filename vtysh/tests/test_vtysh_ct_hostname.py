#!/usr/bin/env python
# (c) Copyright [2015] Hewlett Packard Enterprise Development LP
# All Rights Reserved.
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
import os
import sys
import time
import re
from mininet.net import *
from mininet.topo import *
from mininet.node import *
from mininet.link import *
from mininet.cli import *
from mininet.log import *
from mininet.util import *
from subprocess import *
from opsvsi.docker import *
from opsvsi.opsvsitest import *
import select
import pytest


class hostnameTests(OpsVsiTest):

    def setupNet(self):
        # If you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls.
        hostname_topo = SingleSwitchTopo(k=0,
                                         hopts=self.getHostOpts(),
                                         sopts=self.getSwitchOpts())
        self.net = Mininet(topo=hostname_topo,
                           switch=VsiOpenSwitch,
                           host=Host,
                           link=OpsVsiLink, controller=None,
                           build=True)
        # Disabling dhclient profile on VM.
        if os.path.isfile("/etc/apparmor.d/sbin.dhclient") is True:
            os.system("sudo ln -s /etc/apparmor.d/sbin.dhclient "
                      "  /etc/apparmor.d/disable/")
            os.system('sudo apparmor_parser -R /etc/apparmor.d/sbin.dhclient')

    # Verify to configure system hostname through CLI without input
    def config_set_hostname_from_cli_without_input(self):
        s1 = self.net.switches[0]
        s1.cmdCLI("end")
        s1.cmdCLI("config terminal")
        output = s1.cmdCLI("hostname")
        assert 'Command incomplete.' in output,\
               "Test to set hostname through CLI"\
               " has failed"
        info("### Successfully verified configuring"
             " hostname using CLI without input ###\n")

    # Verify to configure system hostname through CLI
    def config_set_hostname_from_cli(self):
        s1 = self.net.switches[0]
        sleep(5)
        s1.cmdCLI("end")
        s1.cmdCLI("config terminal")
        s1.cmdCLI("hostname cli")
        cnt = 15
        while cnt:
            cmd_output = s1.ovscmd("ovs-vsctl list system")
            hostname = s1.ovscmd("ovs-vsctl get system . "
                                 "hostname").rstrip('\r\n')
            output = s1.cmd("uname -n")
            if "hostname=cli" in cmd_output and \
               hostname == "cli" and \
               "cli" in output:
                break
            else:
                cnt -= 1
                sleep(1)
        assert 'hostname=cli' in cmd_output and \
               hostname == 'cli' and \
               'cli' in output,\
               "Test to set hostname through CLI"\
               " has failed"
        info("### Successfully verified configuring"
             " hostname using CLI ###\n")

    # Verify to display system hostname through CLI
    def display_system_hostname_through_cli(self):
        s1 = self.net.switches[0]
        sleep(5)
        s1.cmdCLI("end")
        output_hostname = s1.cmdCLI("show hostname")
        cnt = 15
        while cnt:
            cmd_output = s1.ovscmd("ovs-vsctl list system")
            hostname = s1.ovscmd("ovs-vsctl get system . "
                                 "hostname").rstrip('\r\n')
            output = s1.cmd("uname -n")
            if "hostname=cli" in cmd_output and \
               hostname == "cli" and \
                           "cli" in output_hostname and \
               "cli" in output:
                break
            else:
                cnt -= 1
                sleep(1)
        assert 'hostname=cli' in cmd_output and \
               hostname == 'cli' and \
                           "cli" in output_hostname and \
               'cli' in output,\
               "Test to set hostname through CLI"\
               " has failed"
        info("### Successfully verified displaying"
             " hostname using CLI ###\n")

    # Verify to reset system hostname through CLI which is not configured
    def config_no_hostname_from_cli_not_configured(self):
        s1 = self.net.switches[0]
        s1.cmdCLI("end")
        s1.cmdCLI("config terminal")
        output = s1.cmdCLI("no hostname cli_not")
        assert('Hostname cli not configured.' in output,
               "Test to set hostname through CLI"
               " has failed")
        info("### Successfully verified configuring "
             " hostname using CLI which is not configured ###\n")

    # Verify to reset configured system hostname through CLI
    def config_no_hostname_from_cli_configured(self):
        s1 = self.net.switches[0]
        s1.cmdCLI("end")
        s1.cmdCLI("config terminal")
        s1.cmdCLI("no hostname cli")
        cnt = 15
        while cnt:
            cmd_output = s1.ovscmd("ovs-vsctl list system")
            hostname = s1.ovscmd("ovs-vsctl get system . "
                                 "hostname").rstrip('\r\n')
            output = s1.cmd("uname -n")
            if "hostname=switch" in cmd_output and \
               hostname == "" and \
               "switch" in output:
                break
            else:
                cnt -= 1
                sleep(1)
        assert 'hostname=switch' in cmd_output and \
               hostname == '' and \
               'switch' in output,\
               "Test to unset hostname through CLI"\
               " has failed"
        info("### Successfully verified setting hostname"
             " to default value with input using CLI ###\n")


class Test_hostname:

    def setup_class(cls):
        # Create the Mininet topology based on mininet.
        Test_hostname.test = hostnameTests()

    def teardown_class(cls):
        # Stop the Docker containers, and
        # mininet topology.
        Test_hostname.test.net.stop()
        # Enabling dhclient.profile on VM.
        if os.path.isfile("/etc/apparmor.d/sbin.dhclient") is True:
            os.system('sudo rm /etc/apparmor.d/disable/sbin.dhclient')
            os.system('sudo apparmor_parser -r /etc/apparmor.d/sbin.dhclient')

    def __del__(self):
        del self.test

    # hostname tests
    def test_config_set_hostname_from_cli_without_input(self):
        info("\n########## Test to configure System Hostname "
             " ##########\n")
        self.test.config_set_hostname_from_cli_without_input()

    def test_config_set_hostname_from_cli(self):
        self.test.config_set_hostname_from_cli()

    def test_display_system_hostname_through_cli(self):
        self.test.display_system_hostname_through_cli()

    def test_config_no_hostname_from_cli_not_configured(self):
        self.test.config_no_hostname_from_cli_not_configured()

    def test_config_no_hostname_from_cli_configured(self):
        self.test.config_no_hostname_from_cli_configured()
