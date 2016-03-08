# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import re

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_interface_commands(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step("### Test to verify interface configuration clis  ###")
    sw1('configure terminal')
    sw1('interface 2')
    sw1('mtu 2500')
    out = sw1('do show running-conf interface 2')
    assert 'mtu 2500' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('speed 1000')
    out = sw1('do show running-conf interface 2')
    assert 'speed 1000' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('duplex half')
    out = sw1('do show running-conf interface 2')
    assert 'duplex half' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('autonegotiation off')
    out = sw1('do show running-conf interface 2')
    assert 'autonegotiation off' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('flowcontrol send on')
    out = sw1('do show running-conf interface 2')
    assert 'flowcontrol send on' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('flowcontrol receive on')
    out = sw1('do show running-conf interface 2')
    assert 'flowcontrol receive on' in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('split')
    out = sw1('do show running-conf interface 2')
    assert 'split' not in out, \
           'Test to verify interface configuration clis - FAILED!'

    sw1('interface 49')

    sw1._shells['vtysh']._prompt = (
        '.*Do you want to continue [y/n]?'
    )

    sw1('split')

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )

    sw1('y')
    sw1(' ')

    out = sw1('do show running-conf interface 49')
    assert 'split' in out, \
           'Test to verify interface configuration clis - FAILED!'

    step("### Test to verify dynamic helpstr for interface speed cli  ###")
    sw1('set interface 1 hw_intf_info:speeds="1000"', shell='vsctl')
    sw1('interface 1')

    out = sw1('speed ?')
    sw1(' ')
    assert '1000   Mb/s supported' in out and \
           '10000  Mb/s not supported' in out and \
           '40000  Mb/s not supported' in out, \
           'Test to verify dyn helpstr for int speeds=1000 - FAILED!'

    sw1('set interface 1 hw_intf_info:speeds="1000,10000"', shell='vsctl')
    out = sw1('speed ?')
    sw1(' ')
    assert '1000   Mb/s supported' in out and \
           '10000  Mb/s supported' in out and \
           '40000  Mb/s not supported' in out, \
           'Test to verify dyn helpstr for int speeds="1000,10000" - FAILED!'
    sw1('set interface 1 hw_intf_info:speeds="40000"', shell='vsctl')
    sw1('interface 1')
    out = sw1('speed ?')
    assert '1000   Mb/s not supported' in out and \
           '10000  Mb/s not supported' in out and \
           '40000  Mb/s supported' in out, \
           'Test to verify dynamic helpstr for interface speed cli - FAILED!'
    out = sw1('do show interface brief')
    list_interface = out.split('\n')
    result_sortted = []
    result_orig = []
    for x in list_interface:
        test = re.match('(\d+|.+\d+)\s+', x)
        if test:
            test_number = test.group(0)
            if '-' in test_number:
                test_number = test_number.strip(" ")
                number = re.match('\d+.\d+', test_number).group(0)
                number = number.replace('-', '.')
                result_orig.append(float(number))
                result_sortted.append(float(number))
            else:
                result_orig.append(int(test_number))
                result_sortted.append(int(test_number))

    assert(result_orig == result_sortted), \
        'Test to display interface in numerical order -FAILED'

    step("### Test to verify dynamic helpstr for mtu ###")
    sw1('set Subsystem base other_info:max_transmission_unit="2500"',
        shell='vsctl')

    out = sw1('mtu ?')
    assert 'WORD  Enter MTU (in bytes) in the range <576-2500>' in out, \
           'Test to verify dyn helpstr for mtu 2500 - FAILED!'
    sw1(' ')
    step('### Test show running-config interface Port Priority ###')
    sw1('interface 2')
    sw1('lacp port-priority 1')
    out = sw1('do show running-config')
    assert "lacp port-priority 1" in out, \
        'Test show running-config lacp port priority showed - FAIL!'

    step("### Test show running-config interface with LAG ###")
    lag_interface = False
    sw1('interface lag 1')
    sw1('no routing')
    sw1('lacp mode active')
    sw1('hash l2-src-dst')
    sw1('lacp fallback')
    sw1('lacp rate fast')
    sw1('exit')
    sw1('interface 2')
    sw1('lag 1')
    sw1('lacp port-id 2')
    sw1('lacp port-priority 2')
    sw1('end')
    out = sw1('show run interface')

    if "interface lag 1" in out and "no routing" in out \
        and "lacp mode active" in out \
        and "hash l2-src-dst" in out and "lacp fallback" in out \
        and "lacp rate fast" in out and "lacp port-id 2" in out \
            and "lacp port-priority 2" in out and "lag 1" in out:
            lag_interface = True
    assert (lag_interface is True), \
        'Test show running-config interface with LAG port - FAILED!'
