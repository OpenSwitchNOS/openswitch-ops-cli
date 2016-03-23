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

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_config_domainname_through_cli(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    # Verify to configure system domainname  through CLI without input
    step("######## Test to verify configure system "
         "domainname through CLI without input ########")
    sw1('configure terminal')
    out = sw1('domain-name')
    assert '% Command incomplete.' in out

    # Verify to configure system domainname through CLI
    step("######## Test to verify configure system "
         "domainname through CLI ########")
    sw1("domain-name cli")
    out = sw1("ovs-vsctl list domain_name", shell='bash')
    assert 'domain_name cli' not in out

    # Verify to display system domainname through CLI
    step("######## Test to display configured system "
         "domainname through CLI ########")
    out = sw1("do show domain-name")
    assert 'cli' not in out

    # Verify to reset configured system domainname through CLI
    step("######## Test to verify reset configured "
         "system domainname through CLI ########")
    sw1('no domain-name')
    out = sw1("ovs-vsctl list domain_name", shell='bash')
    assert 'domain_name ' not in out
