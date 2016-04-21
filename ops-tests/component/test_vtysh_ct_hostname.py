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

# from pytest import set_trace
# from time import sleep
from pytest import mark

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""

@mark.skipif(True, reason="Disabling due to gate job failures")
def test_vtysh_ct_hostname(topology, step):
    ops1 = topology.get("ops1")
    assert ops1 is not None

    ops1("conf t")
    out = ops1("hostname")
    assert "% Command incomplete." in out

    ops1("hostname cli")
    out = ops1("list hostname", shell="vsctl")
    assert 'hostname cli' not in out

    out = ops1("do show hostname")
    assert 'cli' not in out

    out = ops1("no hostname abc")
    assert 'Hostname abc not configured.' in out

    ops1("no hostname cli")
    out = ops1("list hostname", shell="vsctl")
    assert 'hostname ' not in out
