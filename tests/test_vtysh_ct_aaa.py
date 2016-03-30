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

import time
import pytest
from opsvsi.docker import *
from opsvsi.opsvsitest import *

SSHD_CONFIG = '/etc/ssh/sshd_config'


def delay(sec=0.25):
    sleep(sec)


class myTopo(Topo):

    """Custom Topology Example
    H1[h1-eth0]<--->[1]S1
    """

    def build(self,
              hsts=1,
              sws=1,
              **_opts
              ):
        self.hsts = hsts
        self.sws = sws

        for h in irange(1, hsts):
            host = self.addHost('h%s' % h)

        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

        self.addLink('h1', 's1')


class AutoProvisioning(OpsVsiTest):

    def setupNet(self):

        # Create a topology with single Open switch and
        # one host.

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        aaa_topo = myTopo(hsts=1, sws=1, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(aaa_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def EnablePasskeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        info('########## Test to enable password authentication ##########\n'
             )

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('ssh password-authentication')
        assert 'Command failed' not in out, \
            'Failed to execute ssh password authentication'

        s1.cmdCLI('exit')

        out = s1.cmd('cat /etc/ssh/sshd_config')
        lines = out.split('\n')
        for line in lines:
            if 'PasswordAuthentication yes' in line:
                return True
        assert 'PasswordAuthentication yes' not in out, \
            'Failed to enable password authentication'

    def DisablePasskeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        info('########## Test to disable password authentication ##########\n'
             )

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('no ssh password-authentication')
        assert 'Command failed' not in out, \
            'Failed to execute no ssh password authentication command'

        s1.cmdCLI('exit')

        out = s1.cmd('cat /etc/ssh/sshd_config')
        lines = out.split('\n')
        for line in lines:
            if 'PasswordAuthentication no' in line:
                return True
        assert 'PasswordAuthentication no' not in out, \
            'Failed to disable password key authentication'

    def EnablePublickeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        info('########## Test to enable public key authentication ##########\n'
             )

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('ssh public-key-authentication')
        assert 'Command failed' not in out, \
            'Failed to execute ssh public-key authentication command'

        s1.cmdCLI('exit')

        out = s1.cmd('cat /etc/ssh/sshd_config')
        lines = out.split('\n')
        for line in lines:
            if 'PubkeyAuthentication yes' in line:
                return True
        assert 'PubkeyAuthentication yes' not in out, \
            'Failed to enable public key authentication'

    def DisablePublickeyAuth(self):
        ''' This function is to enable passkey authentication for
        SSH authentication method'''

        info('########## Test to disable public key authentication '
             '##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('no ssh public-key-authentication')
        assert 'Command failed' not in out, \
            'Failed to execute ssh no public-key authentication command'

        s1.cmdCLI('exit')

        out = s1.cmd('cat /etc/ssh/sshd_config')
        lines = out.split('\n')
        for line in lines:
            if 'PubkeyAuthentication no' in line:
                return True
        assert 'PubkeyAuthentication no' not in out, \
            'Failed to disable public key authentication'

    def SetRadiusServerHost(self):
        info('########## Test to configure the radius server host IP '
             '##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5')
        assert 'Command failed' not in out, \
            'Failed to configure radius server'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if 'Host IP address	: 192.168.1.5' in line:
                return True
        assert 'Host IP address : 192.168.1.5' not in out, \
            'Test to configure the radius server host IP: Failed'

    def SetRadiusServerTimeout(self):
        info('########## Test to configure radius server Timeout ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5')
        out = s1.cmdCLI('radius-server timeout 10')
        assert 'Command failed' not in out, \
            'Failed to configure radius timeout'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Timeout		: 10" in line:
                return True
        assert 'Timeout         : 10' not in out, \
            'Test to configure radius server Timeout: Failed'

    def SetRadiusServerRetries(self):
        info('########## Test to configure radius server Retries ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5')
        out = s1.cmdCLI('radius-server retries 2')
        assert 'Command failed' not in out, \
            'Failed to configure radius retries'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Retries		: 2" in line:
                return True

        assert 'Retries         : 2' not in out, \
            'Test to configure radius server Retries: Failed'

    def SetRadiusAuthPort(self):
        info('########## Test to configure radius server Authentication port '
             '##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5 auth-port 3333')
        assert 'Command failed' not in out, \
            'Failed to configure radius server authentication port'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Auth port		: 3333" in line:
                return True

        assert 'Auth port               : 3333' not in out, \
            'Test to configure radius server Authentication port: Failed'

    def SetRadiuspasskey(self):
        info('########## Test to configure radius server Passkey ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5 key myhost')
        assert 'Command failed' not in out, \
            'Failed to configure radius-server host passkey'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Shared secret		: myhost" in line:
                return True

        assert 'Shared secret           : myhost' not in out, \
            'Test to configure radius server Passkey: Failed'

    def NoRadiusPassky(self):
        info('########## Test to remove radius server Passkey and reset to '
             'default ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5 key myhost')
        out = \
            s1.cmdCLI('no radius-server host 192.168.1.5 key myhost')
        assert 'Command failed' not in out, \
            'Failed to configure no radius-server passkey'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if 'Host IP address\t: 192.168.1.5' in line:
                for line in lines:
                    return True

        assert 'Host IP address : 192.168.1.5' not in out, \
            'Test to remove radius server Passkey and reset to default: Failed'

    def NoRadiusAuthPort(self):
        info('########## Test to remove radius server Authentication port '
             'and resets to default ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5 auth-port 3333')
        out = \
            s1.cmdCLI('no radius-server host 192.168.1.5 auth-port 3333'
                      )
        assert 'Command failed' not in out, \
            'Failed to configure no radius-server auth port'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if 'Host IP address\t: 192.168.1.5' in line:
                for line in lines:
                    if "Auth port		: 1812" in line:
                        return True
        assert 'Host IP address : 192.168.1.5' not in out, \
            'Test to remove radius server Authentication port and reset \
             to default: Failed'

    def NoRadiusTimeout(self):
        info('########## Test to remove radius server timeout and resets to '
             'default ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5')
        out = s1.cmdCLI('radius-server timeout 10')
        out = s1.cmdCLI('no radius-server timeout 10')
        assert 'Command failed' not in out, \
            'Failed to configure no radius-server timeout'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Timeout		: 5" in line:
                return True
        assert 'Timeout         : 5' not in out, \
            'Test to remove radius server timeout and reset to default: Failed'

    def NoRadiusRetries(self):
        info('########## Test to remove radius server Retries and resets to '
             'default ##########\n')

        s1 = self.net.switches[0]
        out = s1.cmdCLI('configure terminal')
        assert 'Unknown command' not in out, \
            'Failed to enter configuration terminal'

        out = s1.cmdCLI('radius-server host 192.168.1.5')
        out = s1.cmdCLI('radius-server retries 2')
        out = s1.cmdCLI('no radius-server retries 2')
        assert 'Command failed' not in out, \
            'Failed to configure no radius-server retries'
        s1.cmdCLI('exit')
        out = s1.cmdCLI('show radius-server')
        lines = out.split('\n')
        for line in lines:
            if "Retries		: 1" in line:
                return True
        assert 'Retries         : 1' not in out, \
            'Test to remove radius server Retries and reset to default: Failed'


class Test_autoProvision:

    def setup_class(cls):
        Test_autoProvision.test = AutoProvisioning()

    def teardown_class(cls):

    # Stop the Docker containers, and
    # mininet topology

        Test_autoProvision.test.net.stop()

    def __del__(self):
        del self.test

    def test_EnablePasskeyAuth(self):
        if self.test.EnablePasskeyAuth():
            info('### Test to enable SSH password authentication: Passed ###\n'
                 )

    def test_DisablePasskeyAuth(self):
        if self.test.DisablePasskeyAuth():
            info('### Test to disable SSH password authentication: Passed '
                 '###\n')

    def test_EnablePublickeyAuth(self):
        if self.test.EnablePublickeyAuth():
            info('### Test to enable SSH public key authentication: Passed '
                 '###\n')

    def test_DisablePublickeyAuth(self):
        if self.test.DisablePublickeyAuth():
            info('### Test to disable SSH public key authentication: Passed '
                 '###\n')

    def test_SetRadiusServerHost(self):
        if self.test.SetRadiusServerHost():
            info('### Test to configure the radius server host IP: Passed '
                 '###\n')

    def test_SetRadiusServerTimeout(self):
        if self.test.SetRadiusServerTimeout():
            info('### Test to configure radius server Timeout: Passed ###\n')

    def test_SetRadiusServerRetries(self):
        if self.test.SetRadiusServerRetries():
            info('### Test to configure radius server Retries: Passed ###\n')

    def test_SetRadiusAuthPort(self):
        if self.test.SetRadiusAuthPort():
            info('### Test to configure radius server Authentication port: '
                 'Passed ###\n')

    def test_SetRadiuspasskey(self):
        if self.test.SetRadiuspasskey():
            info('### Test to configure radius server Passkey: Passed ###\n')

    def test_NoRadiusPassky(self):
        if self.test.NoRadiusPassky():
            info('### Test to remove radius server Passkey and reset to '
                 'default: Passed ###\n')

    def test_NoRadiusAuthPort(self):
        if self.test.NoRadiusAuthPort():
            info('### Test to remove radius server Authentication port '
                 'and reset to default: Passed ###\n')

    def test_NoRadiusTimeout(self):
        if self.test.NoRadiusTimeout():
            info('### Test to remove radius server timeout and reset to '
                 'default: Passed ###\n')

    def test_NoRadiusRetries(self):
        if self.test.NoRadiusRetries():
            info('### Test to remove radius server Retries and reset to '
                 'default: Passed ###\n')
