#!/usr/bin/env python

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import time
import pytest
from halonvsi.docker import *
from halonvsi.halon import *

SSHD_CONFIG  = "/etc/ssh/sshd_config"

def delay(sec=0.25):
    sleep(sec)

class myTopo(Topo):
    """Custom Topology Example
    H1[h1-eth0]<--->[1]S1
    """

    def build(self, hsts=1, sws=1, **_opts):
        self.hsts = hsts
        self.sws = sws

        "add list of hosts"
        for h in irange(1, hsts):
            host = self.addHost('h%s' % h)

        "add list of switches"
        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

        "Add links between nodes based on custom topo"
        self.addLink('h1', 's1')

class AutoProvisioning( HalonTest ):

    def setupNet(self):
        # Create a topology with single Halon switch and
        # one host.

        topo = myTopo(hsts = 1, sws = 1, hopts = self.getHostOpts(),
                      sopts = self.getSwitchOpts(), switch = HalonSwitch,
                      host = HalonHost, link = HalonLink, controller = None,
                      build = True)

        self.net = Mininet(topo, switch = HalonSwitch, host = HalonHost,
                           link = HalonLink, controller = None, build = True)

    def EnablePasskeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("ssh password-authentication")
        if 'Command failed' in out:
            assert 0, "Failed to enable local authentication"

        s1.cmdCLI("exit")

        out = s1.cmd("cat /etc/ssh/sshd_config")
        lines = out.split("\n")
        for line in lines:
            if "PasswordAuthentication yes" in line:
                return True
        assert 0, "Failed to enable password key authentication"

    def DisablePasskeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("no ssh password-authentication")
        if 'Command failed' in out:
            assert 0, "Failed to enable local authentication"

        s1.cmdCLI("exit")

        out = s1.cmd("cat /etc/ssh/sshd_config")
        lines = out.split("\n")
        for line in lines:
            if "PasswordAuthentication no" in line:
                return True
        assert 0, "Failed to disable password key authentication"

    def EnablePublickeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("ssh public-key-authentication")
        if 'Command failed' in out:
            assert 0, "Failed to enable local authentication"

        s1.cmdCLI("exit")

        out = s1.cmd("cat /etc/ssh/sshd_config")
        lines = out.split("\n")
        for line in lines:
            if "PubkeyAuthentication yes" in line:
                return True
        assert 0, "Failed to enable public key authentication"

    def DisablePublickeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("no ssh public-key-authentication")
        if 'Command failed' in out:
            assert 0, "Failed to enable local authentication"

        s1.cmdCLI("exit")

        out = s1.cmd("cat /etc/ssh/sshd_config")
        lines = out.split("\n")
        for line in lines:
            if "PubkeyAuthentication no" in line:
                return True
        assert 0, "Failed to disable public key authentication"

class Test_autoProvision:
    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):
        Test_autoProvision.test = AutoProvisioning()
        pass

    def teardown_class(cls):
    # Stop the Docker containers, and
    # mininet topology
       Test_autoProvision.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test

    def test_EnablePasskeyAuth(self):
        if self.test.EnablePasskeyAuth():
            print 'EnablePasskeyAuth'

    def test_DisablePasskeyAuth(self):
        if self.test.DisablePasskeyAuth():
            print 'DisablePasskeyAuth'

    def test_EnablePublickeyAuth(self):
        if self.test.EnablePublickeyAuth():
            print 'Passed EnablePublickeyAuth'

    def test_DisablePublickeyAuth(self):
        if self.test.DisablePublickeyAuth():
            print 'Passed DisablePublickeyAuth'
