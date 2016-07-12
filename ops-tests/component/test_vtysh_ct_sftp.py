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


@mark.gate
def test_sftp_server_configuration(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step('Enable the SFTP server and then verify the show command.')
    sw1('configure terminal')
    sw1("sftp server enable")
    cmd_out = sw1("do show sftp server")
    assert 'Enabled' in cmd_out

    step('Enable the SFTP server.')
    sw1("sftp server enable")
    cmd_out = sw1("do show sftp server")
    assert 'SFTP server : Enabled' in cmd_out

    step('Disable the SFTP server.')
    sw1("no sftp server enable")
    cmd_out = sw1("do show sftp server")
    assert 'SFTP server : Disabled' in cmd_out

    step('Enable the SFTP server and then verify the show running command.')
    sw1("sftp server enable")
    cmd_out = sw1("do show running-config")
    assert 'sftp server enable' in cmd_out

    step('Enable the SFTP server and check show running command.\n'
         'Disable the SFTP server and verify the config is removed '
         'in the show running command.')
    sw1("sftp server enable")
    cmd_out = sw1("do show running-config")
    assert 'sftp server enable' in cmd_out

    sw1("no sftp server enable")
    cmd_out = sw1("do show running-config")
    assert 'sftp server enable' not in cmd_out
