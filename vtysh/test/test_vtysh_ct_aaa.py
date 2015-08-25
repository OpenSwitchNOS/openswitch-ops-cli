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

        out = s1.cmdCLI("ssh password-authentication enable")
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

        out = s1.cmdCLI("ssh password-authentication disable")
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

        out = s1.cmdCLI("ssh publickey-authentication enable")
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

        out = s1.cmdCLI("ssh publickey-authentication disable")
        if 'Command failed' in out:
            assert 0, "Failed to enable local authentication"

        s1.cmdCLI("exit")

        out = s1.cmd("cat /etc/ssh/sshd_config")
        lines = out.split("\n")
        for line in lines:
            if "PubkeyAuthentication no" in line:
                return True
        assert 0, "Failed to disable public key authentication"

    def SetRadiusServerHost(self):
        ''' This function is used to configure the radius server host IP'''
        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server host 192.168.1.5")
        if 'Command failed' in out:
             assert 0, "Failed to to configure radius server"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Host IP address	: 192.168.1.5" in line:
                return True
        assert 0, "Radius server not configured"

    def SetRadiusServerTimeout(self):
        ''' This function is used to configure radius server Timeout'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server timeout 10")
        if 'Command failed' in out:
             assert 0, "Failed to configure radius server timeout"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Timeout		: 10" in line:
                return True
        assert 0, "Radius server not configured"

    def SetRadiusServerRetries(self):
        '''This function is used to configure radius server Retries'''


        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server retries 2")
        if 'Command failed' in out:
             assert 0, "Failed to configure radius server retries"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Retries		: 2" in line:
                return True
        assert 0, "Radius server not configured"

    def SetRadiusAuthPort(self):
        '''This function is used to configure radius server authentication port'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server host 192.168.1.5 auth-port 3333")
        if 'Command failed' in out:
             assert 0, "Failed to configure radius server authentication port"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Auth port		: 3333" in line:
                return True
        assert 0, "Radius server not configured"

    def SetRadiuspasskey(self):
       ''' This function is used to configure radius server passkey'''

       s1 = self.net.switches [ 0 ]
       out = s1.cmdCLI("configure terminal")
       if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"

       out = s1.cmdCLI("radius-server host 192.168.1.5 key halonhost")
       if 'Command failed' in out:
             assert 0, "Failed to configure radius server passkey"
       s1.cmdCLI("exit")
       out = s1.cmdCLI("show radius-server")
       lines = out.split("\n")
       for line in lines:
           if "Shared secret		: halonhost" in line:
               return True
       assert 0, "Radius server not configured"

    def NoRadiusPassky(self):
       '''This function is used to remove passkey provided and resets to defaults value'''

       s1 = self.net.switches [ 0 ]
       out = s1.cmdCLI("configure terminal")
       if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"
       out =s1.cmdCLI("radius-server host 192.168.1.5 key halonhost")
       out = s1.cmdCLI("no radius-server host 192.168.1.5 key halonhost")
       if 'Command failed' in out:
             assert 0, "Failed to configure radius server passkey"
       s1.cmdCLI("exit")
       out = s1.cmdCLI("show radius-server")
       lines = out.split("\n")
       for line in lines:
           if "Host IP address	: 192.168.1.5" in line:
               for line in lines:
                   if "Shared secret		: testing123-1" in line:
                        return True
       assert 0, "Radius server not configured"

    def NoRadiusAuthPort(self):
       '''This function is used to remove authentication port provided and resets to defaults value'''

       s1 = self.net.switches [ 0 ]
       out = s1.cmdCLI("configure terminal")
       if 'Unknown command' in out:
            assert 0, "Failed to enter configuration terminal"
       out =s1.cmdCLI("radius-server host 192.168.1.5 auth-port 3333")
       out = s1.cmdCLI("no radius-server host 192.168.1.5 auth-port 3333")
       if 'Command failed' in out:
             assert 0, "Failed to configure radius server passkey"
       s1.cmdCLI("exit")
       out = s1.cmdCLI("show radius-server")
       lines = out.split("\n")
       for line in lines:
           if "Host IP address	: 192.168.1.5" in line:
               for line in lines:
                   if "Auth port		: 1812" in line:
                        return True
       assert 0, "Radius server not configured"

    def NoRadiusTimeout(self):
        ''' This function is used to remove the configured timeout and resets to default radius server Timeout'''

        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server timeout 10")
        out = s1.cmdCLI("no radius-server timeout 10")
        if 'Command failed' in out:
             assert 0, "Failed to configure radius server timeout"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Timeout		: 5" in line:
                return True
        assert 0, "Radius server not configured"


    def NoRadiusRetries(self):
        '''This function is used to remove  radius server Retries and resets to default'''


        s1 = self.net.switches [ 0 ]
        out = s1.cmdCLI("configure terminal")
        if 'Unknown command' in out:
             assert 0, "Failed to enter configuration terminal"

        out = s1.cmdCLI("radius-server retries 2")
        out = s1.cmdCLI("no radius-server retries 2")
        if 'Command failed' in out:
             assert 0, "Failed to configure radius server retries"
        s1.cmdCLI("exit")
        out = s1.cmdCLI("show radius-server")
        lines = out.split("\n")
        for line in lines:
            if "Retries		: 1" in line:
                return True
        assert 0, "Radius server not configured"

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

    def test_SetRadiusServerHost(self):
        if self.test.SetRadiusServerHost():
            print 'Passed RadiusServerHost'

    def test_SetRadiusServerTimeout(self):
        if self.test.SetRadiusServerTimeout():
            print 'Passed SetRadiusServerTimeout'

    def test_SetRadiusServerRetries(self):
        if self.test.SetRadiusServerRetries():
            print 'passed SetRadiusServerRetrie'

    def test_SetRadiusAuthPort(self):
        if self.test.SetRadiusAuthPort():
            print 'passed SetRadiusAuthPort'

    def test_SetRadiuspasskey(self):
        if self.test.SetRadiuspasskey():
            print 'passed SetRadiuspasskey'

    def test_NoRadiusPassky(self):
        if self.test.NoRadiusPassky():
            print 'passed NoRadiusPassky'

    def test_NoRadiusAuthPort(self):
        if self.test.NoRadiusAuthPort():
            print 'passed NoRadiusAuthPort'

    def test_NoRadiusTimeout(self):
        if self.test.NoRadiusTimeout():
            print 'passed NoRadiusTimeout'

    def test_NoRadiusRetries(self):
        if self.test.NoRadiusRetries():
            print 'passed NoRadiusRetries'
