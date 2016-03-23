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


import pytest


TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] s1
"""


def create_db(self):
    print('\n=========================================================')
    print('***            Test method to create db                 ***')
    print('===========================================================')
    s1 = self.net.switches[0]
    s1('/usr/bin/ovs-vsctl add-br br0', shell='bash')
    for i in range(1, 4095):
        s1('/usr/bin/ovs-vsctl add-vlan br0 {}'.format(i),
           shell='bash')


def retrieve_db(s1):
    print('\n=========================================================')
    print('***          Test method to retrieve db                 ***')
    print('===========================================================')
    ovsout = s1('show running-config')
    print(ovsout)


def kill_db(s1):
    print('\n============================================================')
    print('***          Test to kill DB and verify CLI                ***')
    print('==============================================================')
    out = s1('ps -aux', shell='bash')
    lines = out.split('\n')
    pid = None
    for line in lines:
        if '/usr/sbin/ovsdb' in line:
            words = line.split(' ')
            words = filter(None, words)
            pid = words[1]
            break
    if pid is not None:
        s1('kill -9 {}'.format(pid), shell='bash')
    s1('conf t')
    out = s1('feature lldp')
    s1('end')
    assert 'Command failed' not in out


def disable_lldp_tx_dir(s1):  # noqa
    print('\n=====================================')
    print('Test to verify show running-config for lldp transmission')
    print('=======================================')
    ovsout = s1('/usr/bin/ovs-vsctl list interface 1', shell='bash')
    assert '_uuid' in ovsout
    s1('configure terminal')
    s1('interface 1')
    s1('no lldp transmission')
    out = s1('do show running-config')
    s1('exit')
    lines = out.split('\n')
    not_in = False
    for line in lines:
        if 'no lldp transmission' in line:
            not_in = True
            break
    assert not_in


@pytest.mark.skipif(True, reason="Takes too long and not needed for CIT")
def test_vtysh_ct_db(topology):
    s1 = topology.get("s1")
    assert s1 is not None
    create_db(s1)
    retrieve_db(s1)
    kill_db(s1)
