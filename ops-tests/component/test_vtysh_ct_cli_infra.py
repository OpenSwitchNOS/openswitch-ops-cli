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

from time import sleep

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_alias_cli_command(topology, step):
    step("***           Test to verify alias clis               ***")
    sw1 = topology.get('sw1')

    assert sw1 is not None

    out = sw1('configure terminal')
    sw1('alias abc hostname MyTest')

    out = sw1('do show alias')
    sleep(1)
    if 'abc' not in out:
        print(out)
        assert 0, 'Failed to get the alias'
        return False

    out = sw1('alias 123456789012345678901234567890123 '
              'demo_cli level 2')

    if 'Max length exceeded' not in out:
        assert 0, 'Failed to check max length'
        return False

    sw1('alias llht lldp holdtime $1; hostname $2'.format(**locals()))
    sw1('llht 6 TestHName'.format(**locals()))

    sw1._shells['vtysh']._prompt = (
        '(^|\n)TestHName(\\([\\-a-zA-Z0-9]*\\))?#'
    )
    sw1(' ')

    out = sw1('do show running'.format(**locals()))

    if 'lldp holdtime 6' not in out:
        assert 0, 'Failed to check lldp hostname'
        return False

    sw1('no hostname'.format(**locals()))

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )
    sw1(' ')

    sw1('no lldp holdtime'.format(**locals()))
    out = sw1('do show running'.format(**locals()))

    if 'alias llht lldp holdtime $1; hostname $2' not in out:
        assert 0, 'Failed to check alias in show running'
        return False
    return True
