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


def enablelldptest(dut, step):
    step('Test to verify show running-config for lldp enable')
    enable_lldp = False
    out = dut('configure terminal')
    out = dut('lldp enable')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'lldp enable' in line:
            enable_lldp = True
    assert enable_lldp is True


def setlldpholdtimetest(dut, step):
    step('Test to verify show running-config for lldp holdtime')
    set_lldp_hold_time = False
    out = dut('configure terminal')
    out = dut('lldp holdtime 5')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'lldp holdtime 5' in line:
            set_lldp_hold_time = True
    assert set_lldp_hold_time is True


def disablelldptxdirtest(dut, step):
    step('Test to verify show running-config for lldp transmission')
    lldp_txrx_disabled = False
    ovsout = dut("list interface 1", shell="vsctl")
    assert '_uuid' in ovsout
    out = dut('configure terminal')
    out = dut('interface 1')
    out = dut('no lldp transmit')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'no lldp transmit' in line:
            lldp_txrx_disabled = True
    assert lldp_txrx_disabled is True


def setlogrotateperiodtest(dut, step):
    step('Test to verify show running-config for logrotate period')
    logrotate_period_set = False
    out = dut('configure terminal')

    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate period' not in line

    dut('logrotate period none')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate period none' not in line

    dut('logrotate period hourly')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'logrotate period hourly' in line:
            logrotate_period_set = True
    assert logrotate_period_set is True


def setlogrotatemaxsizetest(dut, step):
    step('Test to verify show running-config for logrotate maxsize')
    logrotate_max_size_set = False
    out = dut('configure terminal')
    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate maxsize' not in line

    dut('logrotate maxsize 10')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate maxsize 10' not in line

    dut('logrotate maxsize 20')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'logrotate maxsize 20' in line:
            logrotate_max_size_set = True
    assert logrotate_max_size_set is True


def setlogrotatetargettest(dut, step):
    step('Test to verify show running-config for logrotate target')
    logrotate_target_set = False
    out = dut('configure terminal')
    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate target' not in line

    dut('logrotate target local')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    lines = out.splitlines()
    for line in lines:
        assert 'logrotate target local' not in line

    dut('logrotate target tftp://1.1.1.1')
    dut(' ')
    dut(' ')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'logrotate target tftp://1.1.1.1' in line:
            logrotate_target_set = True
    assert logrotate_target_set is True


def setsessiontimeouttest(dut, step):
    step('Test to verify show running-config for session timeout')
    dut('configure terminal')
    out1 = dut('session-timeout')
    out2 = dut('do show running-config')
    dut('end')
    assert (
        'session-timeout' not in out2 and
        'Command incomplete' in out1
    )

    dut('configure terminal')
    dut('session-timeout 5')
    out = dut('do show running-config')
    dut('end')
    assert 'session-timeout 5' in out


def sessiontimeoutrangetest(dut, step):
    step('Test to verify show running-config for session timeout range test')
    dut('configure terminal')
    dut('session-timeout -1')
    out = dut('do show running-config')
    assert 'session-timeout 5' in out
    dut('configure terminal')

    dut('session-timeout 43201')
    out = dut('do show running-config')
    assert 'session-timeout 5' in out

    dut('session-timeout abc')
    out = dut('do show running-config')
    dut('end')
    assert 'session-timeout 5' in out


def setdefaultsessiontimeouttest(dut, step):
    step('Test to verify show running-config for default session timeout')
    dut('configure terminal')
    dut('no session-timeout')
    out = dut('do show running-config')
    dut('end')
    assert 'session-timeout' not in out


def createlacptest(dut, step):
    step("Test to verify lacp configuration in show running-config command\n")
    success = 0
    dut('configure terminal')
    dut('interface lag 1')
    dut('lacp mode active')
    dut('interface 1')
    dut('lag 1')
    out = dut('do show running-config')
    dut('end')
    lines = out.splitlines()
    for line in lines:
        if 'interface lag 1' in line:
            success += 1
        if 'lacp mode active' in line:
            success += 1
        if line.lstrip().startswith("lag 1"):
            success += 1
    assert success == 3
    return True


def restoreconfigfromrunningconfig(dut, step):
    step("Test to verify show running-config output used to restore "
         "configuration")
    commands = [
        'vlan 900', 'no shutdown', 'interface lag 1',
        'no routing', 'vlan access 900', 'interface 1',
        'no shutdown', 'no routing', 'vlan access 900',
        'interface 2', 'no shutdown', 'lag 1'
    ]
    restorecommands = [
        'no vlan 900', 'no interface lag 1', 'interface 1',
        'shutdown', 'routing', 'no vlan access 900',
        'interface 2', 'shutdown', 'no lag 1'
    ]

    dut('configure terminal')
    # Applying configuration for the first time
    for element in commands:
        dut(element)

    # Saving first configuration
    firstout = dut('do show running-config')
    firstlines = firstout.splitlines()

    # Erasing configuration
    for element in restorecommands:
        dut(element)

    # Applying configuration with show running-config output
    for line in firstlines:
        dut(line)

    # Saving configuration for the second time
    secondout = dut('do show running-config')
    secondlines = secondout.splitlines()

    # Eliminating last element from list, this is the name
    # of the switch that the test should not validate
    del firstlines[-1]
    del secondlines[-1]

    # If list don't have the same elements mean that the
    # configuration was not properly applied
    assert sorted(firstlines) == sorted(secondlines)

    # Cleaning configuration to not affect other tests
    for element in restorecommands:
        dut(element)

    dut('end')


def test_vtysh_ct_config(topology, step):
    ops1 = topology.get('ops1')
    assert ops1 is not None

    restoreconfigfromrunningconfig(ops1, step)

    enablelldptest(ops1, step)

    setlldpholdtimetest(ops1, step)

    disablelldptxdirtest(ops1, step)

    setlogrotateperiodtest(ops1, step)

    setlogrotatemaxsizetest(ops1, step)

    setlogrotatetargettest(ops1, step)

    setsessiontimeouttest(ops1, step)

    sessiontimeoutrangetest(ops1, step)

    setdefaultsessiontimeouttest(ops1, step)

    createlacptest(ops1, step)
