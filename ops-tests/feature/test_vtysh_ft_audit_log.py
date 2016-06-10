# -*- coding: utf-8 -*-

#
# Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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

from pytest import mark

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


@mark.platform_incompatible(['docker'])
def test_vtysh_ct_audit_log(topology, step):
    sw1 = topology.get('sw1')
    assert sw1 is not None

    step("****Test to verfiy config clis in audit.log file ****")

    sw1('show version', shell='vtysh')
    output = sw1("ausearch -i -x vtysh", shell='bash')
    assert "show version" not in output

    sw1('configure terminal', shell='vtysh')
    sw1('hostname audit', shell='vtysh')
    sw1('router ospf', shell='vtysh')
    output = sw1('do show running-config', shell='vtysh')
    assert "hostname audit" in output

    output = sw1("ausearch -i -x vtysh", shell='bash')
    assert "hostname audit" in output and \
           "router ospf" in output
