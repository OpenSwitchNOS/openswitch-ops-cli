# -*- coding: utf-8 -*-
# (C) Copyright 2015 Hewlett Packard Enterprise Development LP
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
#
##########################################################################

"""
OpenSwitch Test for switchd related configurations.
"""


TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


def enablepasskeyauth(dut):
    ''' This function is to enable passkey authentication for
    SSH authentication method'''

    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('ssh password-authentication')
    assert 'Command failed' not in out

    dut('end')

    out = dut('cat /etc/ssh/sshd_config', shell='bash')
    lines = out.splitlines()
    for line in lines:
        if 'PasswordAuthentication yes' in line:
            return True
    assert 'PasswordAuthentication yes' not in out


def disablepasskeyauth(dut):
    ''' This function is to enable passkey authentication for
    SSH authentication method'''

    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('no ssh password-authentication')
    assert 'Command failed' not in out

    dut('end')

    out = dut('cat /etc/ssh/sshd_config', shell='bash')
    lines = out.splitlines()
    for line in lines:
        if 'PasswordAuthentication no' in line:
            return True
    assert 'PasswordAuthentication no' not in out


def enablepublickeyauth(dut):
    ''' This function is to enable passkey authentication for
    SSH authentication method'''

    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('ssh public-key-authentication')
    assert 'Command failed' not in out

    dut('end')

    out = dut('cat /etc/ssh/sshd_config', shell='bash')
    lines = out.splitlines()
    for line in lines:
        if 'PubkeyAuthentication yes' in line:
            return True
    assert 'PubkeyAuthentication yes' not in out


def disablepublickeyauth(dut):
    ''' This function is to enable passkey authentication for
    SSH authentication method'''

    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('no ssh public-key-authentication')
    assert 'Command failed' not in out

    dut('end')

    out = dut('cat /etc/ssh/sshd_config', shell='bash')
    lines = out.splitlines()
    for line in lines:
        if 'PubkeyAuthentication no' in line:
            return True
    assert 'PubkeyAuthentication no' not in out


def setradiusserverhost(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5')
    assert 'Command failed' not in out, \
        'Failed to configure radius server'
    dut('end')

    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if 'Host IP address    : 192.168.1.5' in line:
            return True
    assert 'Host IP address : 192.168.1.5' not in out


def setradiusservertimeout(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5')
    out = dut('radius-server timeout 10')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Timeout        : 10" in line:
            return True
    assert 'Timeout         : 10' not in out


def setradiusserverretries(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5')
    out = dut('radius-server retries 2')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Retries        : 2" in line:
            return True

    assert 'Retries         : 2' not in out


def setradiusauthport(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5 auth-port 3333')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Auth port        : 3333" in line:
            return True

    assert 'Auth port               : 3333' not in out


def setradiuspasskey(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5 key myhost')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Shared secret        : myhost" in line:
            return True

    assert 'Shared secret           : myhost' not in out


def noradiuspassky(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5 key myhost')
    out = dut('no radius-server host 192.168.1.5 key myhost')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if 'Host IP address\t: 192.168.1.5' in line:
            for line in lines:
                return True

    assert 'Host IP address : 192.168.1.5' not in out


def noradiusauthport(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5 auth-port 3333')
    out = dut('no radius-server host 192.168.1.5 auth-port 3333')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if 'Host IP address\t: 192.168.1.5' in line:
            for line in lines:
                if "Auth port        : 1812" in line:
                    return True
    assert 'Host IP address : 192.168.1.5' not in out


def noradiustimeout(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5')
    out = dut('radius-server timeout 10')
    out = dut('no radius-server timeout 10')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Timeout        : 5" in line:
            return True
    assert 'Timeout         : 5' not in out


def noradiusretries(dut):
    out = dut('configure terminal')
    assert 'Unknown command' not in out

    out = dut('radius-server host 192.168.1.5')
    out = dut('radius-server retries 2')
    out = dut('no radius-server retries 2')
    assert 'Command failed' not in out
    dut('end')
    out = dut('show radius-server')
    lines = out.splitlines()
    for line in lines:
        if "Retries        : 1" in line:
            return True
    assert 'Retries         : 1' not in out


def test_vtysh_ct_aaa(topology, step):
    ops1 = topology.get('ops1')
    assert ops1 is not None

    step('Test to enable SSH password authentication')
    enablepasskeyauth(ops1)

    step('Test to disable SSH password authentication')
    disablepasskeyauth(ops1)

    step('Test to enable SSH public key authentication')
    enablepublickeyauth(ops1)

    step('Test to disable SSH public key authentication')
    disablepublickeyauth(ops1)

    step('Test to configure the radius server host IP')
    setradiusserverhost(ops1)

    step('Test to configure radius server Timeout')
    setradiusservertimeout(ops1)

    step('Test to configure radius server Retries')
    setradiusserverretries(ops1)

    step('Test to configure radius server Authentication port')
    setradiusauthport(ops1)

    step('Test to configure radius server Passkey')
    setradiuspasskey(ops1)

    step('Test to remove radius server Passkey and reset to default')
    noradiuspassky(ops1)

    step('Test to remove radius server Authentication port and reset to'
         ' default')
    noradiusauthport(ops1)

    step('Test to remove radius server timeout and reset to default')
    noradiustimeout(ops1)

    step('Test to remove radius server Retries and reset to default')
    noradiusretries(ops1)
