# -*- coding: utf-8 -*-

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

from time import sleep

TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def init_lldp_local_device(sw1):
    # Set IPV4 and IPv6 Mgmt address
    sw1('interface mgmt')
    sw1('ip static 192.168.1.1/24')
    sw1('ip static fd12:3456:789a:1::/64')
    sw1('exit')
    # Set hostname to openswitch
    sw1('hostname openswitch', shell='bash')
    # Set holdtime and timer, to check TTL(3*20 = 60)
    sw1('lldp holdtime 3')
    sw1('lldp  timer 20')
    sleep(10)


def init_lldp_neighborstep(sw1, uuid):
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_capability_available='
        'Bridge,Router \n '.format(uuid),
        shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_description=OpenSwitch_0.1.0'
        '_basil_Linux_3.9.11_#1_SMP_Mon_Aug_24_14:38:01'
        '_UTC_2015_x86_64'.format(uuid),
        shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_id=70:72:cf:fd:e9:26 '
        '\n'.format(uuid),
        shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_id_len= 6 '
        '\n'.format(uuid), shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_id_subtype='
        'link_local_addr \n'.format(uuid),
        shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_index=1 \n'.format(uuid),
        shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_name=as5712 '
        '\n'.format(uuid), shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_protocol=LLDP '
        '\n'.format(uuid), shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_refcount=1 '
        '\n'.format(uuid), shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:chassis_ttl=120 '
        '\n'.format(uuid), shell='vsctl')
    sw1('set interface {} '
        ' lldp_neighbor_info:mgmt_ip_list=10.10.10.10 '
        '\n'.format(uuid), shell='vsctl')
    sleep(1)


def enable_lldp_feature_test(sw1):
    sw1('conf t')
    lldp_feature_enabled = False
    sw1('lldp enable')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_enable' in line:
            lldp_feature_enabled = True
            break
    assert lldp_feature_enabled


def disable_lldp_feature_test(sw1):
    lldp_feature_enabled = True
    sw1('no lldp enable')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_enable' in line:
            lldp_feature_enabled = False
            break
    assert lldp_feature_enabled


def set_lldp_holdtime_test(sw1):
    lldp_hold_time_set = False
    sw1('lldp holdtime 7')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_hold="7"' in line:
            lldp_hold_time_set = True
            break
    assert lldp_hold_time_set


def unset_lldp_holdtime_test(sw1):
    lldp_holdtime_set = False
    sw1('no lldp holdtime')
    output = sw1('do show running')
    lines = output.split('\n')
    for line in lines:
        if 'lldp holdtime' in line:
            lldp_holdtime_set = True
            break
    assert not lldp_holdtime_set


def set_lldp_default_holdtime_test(sw1):
    lldp_default_hold_time_set = True
    sw1('lldp holdtime 4')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_hold' in line:
            lldp_default_hold_time_set = False
            break
    assert lldp_default_hold_time_set


def set_lldp_reinit_delay_test(sw1):
    lldp_reinit_delay_set = False
    sw1('lldp reinit 7')
    output = sw1('do show running')
    lines = output.split('\n')
    for line in lines:
        if 'lldp reinit 7' in line:
            lldp_reinit_delay_set = True
            break
    assert lldp_reinit_delay_set


def unset_lldp_reinit_delay_test(sw1):
    lldp_reinit_delay_set = False
    sw1('no lldp reinit')
    output = sw1('do show running')
    lines = output.split('\n')
    for line in lines:
        if 'lldp reinit' in line:
            lldp_reinit_delay_set = True
            break
    assert not lldp_reinit_delay_set


def set_lldp_default_reinit_delay_test(sw1):
    lldp_reinit_delay_set = False
    sw1('lldp reinit 2')
    output = sw1('do show running')
    lines = output.split('\n')
    for line in lines:
        if 'lldp reinit' in line:
            lldp_reinit_delay_set = True
            break
    assert not lldp_reinit_delay_set


def set_lldp_timer_test(sw1):
    lldp_timer_set = False
    sw1('lldp timer 100')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_tx_interval="100"' in line:
            lldp_timer_set = True
            break
    assert lldp_timer_set


def unset_lldp_timer_test(sw1):
    lldp_timer_set = False
    sw1('no lldp timer')
    output = sw1('do show running')
    lines = output.split('\n')
    for line in lines:
        if 'lldp timer' in line:
            lldp_timer_set = True
            break
    assert not lldp_timer_set


def set_lldp_default_timer_test(sw1):
    lldp_default_timer_set = True
    sw1('lldp timer 30')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_tx_interval' in line:
            lldp_default_timer_set = False
            break
    assert lldp_default_timer_set


def set_lldp_mgmt_address_test(sw1):
    lldp_mgmt_addr_set = False
    sw1('lldp management-address 1.1.1.1')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_mgmt_addr="1.1.1.1"' in line:
            lldp_mgmt_addr_set = True
            break
    assert lldp_mgmt_addr_set


def unset_lldp_mgmt_address_test(sw1):
    lldp_mgmt_addr_set = True
    sw1('no lldp management-address')
    output = sw1('list system', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if 'lldp_mgmt_addr' in line:
            lldp_mgmt_addr_set = False
            break
    assert lldp_mgmt_addr_set


def set_lldp_clear_counters_test(sw1):
    sw1('lldp enable')
    output = sw1('list interface 1', shell='vsctl')
    lines = output.split('\n')
    uuid = None
    for line in lines:
        if '_uuid' in line:
            _id = line.split(':')
            uuid = _id[1].strip()
            init_lldp_neighborstep(sw1, uuid)
    sw1('lldp clear counters')
    sleep(10)
    output = sw1('do show lldp neighbor-info 1')
    lines = output.split('\n')
    counter = 0
    for line in lines:
        if 'Neighbor Chassis-Name' in line \
                and 'as512' not in line:
            counter += 1
        if 'Neighbor Chassis-Description' in line \
                and 'OpenSwitch_0.1.0' not in line:
            counter += 1
        if 'Neighbor Chassis-ID' in line \
                and '70:72:cf:fd:e9:26' not in line:
            counter += 1
        if 'Neighbor Management-Address' in line \
                and '10.10.10.10' not in line:
            counter += 1
        if 'Chassis Capabilities Available'in line \
                and 'Bridge,Router' not in line:
            counter += 1
        if 'TTL' in line \
                and '120' not in line:
            counter += 1
    assert counter is 6


def set_lldp_clear_neighbors_test(sw1):
    sw1('lldp enable')
    output = sw1('list interface 1', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if '_uuid' in line:
            _id = line.split(':')
            uuid = _id[1].strip()
            init_lldp_neighborstep(sw1, uuid)
            output = sw1('do show lldp neighbor-info 1')
    sw1('lldp clear neighbors')
    sleep(10)
    output = sw1('do show lldp neighbor-info 1')
    lines = output.split('\n')
    counter = 0
    for line in lines:
        if 'Neighbor Chassis-Name' in line \
                and 'as512' not in line:
            counter += 1
        if 'Neighbor Chassis-Description' in line \
                and 'OpenSwitch_0.1.0' not in line:
            counter += 1
        if 'Neighbor Chassis-ID' in line \
                and '70:72:cf:fd:e9:26' not in line:
            counter += 1
        if 'Neighbor Management-Address' in line \
                and '10.10.10.10' not in line:
            counter += 1
        if 'Chassis Capabilities Available'in line \
                and 'Bridge,Router' not in line:
            counter += 1
        if 'TTL' in line \
                and '120' not in line:
            counter += 1
    assert counter is 6


def lldp_neighbors_step_test(sw1):
    counter = 0
    sw1('lldp enable')
    output = sw1('list interface 1', shell='vsctl')
    lines = output.split('\n')
    for line in lines:
        if '_uuid' in line:
            _id = line.split(':')
            uuid = _id[1].strip()
            init_lldp_neighborstep(sw1, uuid)
            output = sw1('do show lldp neighbor-info 1')
            lines = output.split('\n')
            for line in lines:
                if 'Neighbor Chassis-Name          : as5712' in line:
                    counter += 1
                if 'Neighbor Chassis-Description   : OpenSwitch_0.1.0'\
                   '_basil_Linux_3.9.11_#1_SMP_Mon_Aug_24_14:38:01_'\
                   'UTC_2015_x86_64' in line:
                    counter += 1
                if 'Neighbor Chassis-ID            : '\
                   '70:72:cf:fd:e9:26' in line:
                    counter += 1
                if 'Chassis Capabilities Available : '\
                   'Bridge,Router' in line:
                    counter += 1
                if 'Neighbor Management-Address    : '\
                   '10.10.10.10' in line:
                    counter += 1
            assert counter is 5


def lldp_show_local_device_test(sw1):
    counter = 0
    init_lldp_local_device(sw1)
    output = sw1('do show lldp local-device')
    lines = output.split('\n')
    for line in lines:
        if 'System Name            : openswitch' in line:
            counter += 1
        if 'Management Address     : 192.168.1.1, fd12:3456:789a:1::'\
                in line:
            counter += 1
        if 'TTL                    : 60' in line:
            counter += 1
    assert counter is 3


def test_vtysh_ct_lldp(topology, step):
    sw1 = topology.get("sw1")
    step('Test to enable LLDP feature')
    enable_lldp_feature_test(sw1)
    step('Test to disable LLDP feature')
    disable_lldp_feature_test(sw1)
    step('Test setting LLDP holdtime')
    set_lldp_holdtime_test(sw1)
    step('Test unsetting LLDP holdtime')
    unset_lldp_holdtime_test(sw1)
    step('Test setting LLDP default holdtime')
    set_lldp_default_holdtime_test(sw1)
    step('Test setting LLDP reinit delay')
    set_lldp_reinit_delay_test(sw1)
    step('Test unsetting LLDP reinit delay')
    unset_lldp_reinit_delay_test(sw1)
    step('Test setting LLDP default reinit delay')
    set_lldp_default_reinit_delay_test(sw1)
    step('Test setting LLDP timer')
    set_lldp_timer_test(sw1)
    step('Test unsetting LLDP timer')
    unset_lldp_holdtime_test(sw1)
    step('Test setting default LLDP timer')
    set_lldp_default_timer_test(sw1)
    step('Test setting LLDP management address')
    set_lldp_mgmt_address_test(sw1)
    step('Test unsetting LLDP management address')
    unset_lldp_mgmt_address_test(sw1)
    step('Test LLDP clear counters')
    # set_lldp_clear_counters_test(sw1)
    step('Test LLDP clear neighbors')
    set_lldp_clear_neighbors_test(sw1)
    step('Test LLDP neighborstep command')
    lldp_neighbors_step_test(sw1)
    step('Test LLDP show local-device command')
    lldp_show_local_device_test(sw1)
