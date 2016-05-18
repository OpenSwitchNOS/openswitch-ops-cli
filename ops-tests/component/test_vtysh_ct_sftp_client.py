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


def test_sftp_client_configuration(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step('Test SFTP client username max limit')
    max_username_len = 256
    username = 'u'
    srcpath = ' /etc/ssh/sshd_config'
    dstpath = ' /home/admin/'

    for x in range(1, max_username_len+1):
        username = username+'u'
    print(len(username))
    copy = "copy sftp "
    hostip = ' 127.0.0.1'
    cmd = copy+username+hostip+srcpath+dstpath

    sw1._shells['vtysh']._prompt = (
        'switch#'
    )
    out = sw1(cmd)
    assert 'Username should be less than 256 characters' in out

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )
    sw1(' ')

    step('Test SFTP client hostname max limit')
    max_hostname_len = 256
    hostname = ' h'
    username = "root"
    srcpath = ' /etc/ssh/sshd_config'
    dstpath = ' /home/admin/'
    copy = "copy sftp "

    for y in range(1, max_hostname_len+1):
        hostname = hostname+'h'

    cmd = copy+username+hostname+srcpath+dstpath

    sw1._shells['vtysh']._prompt = (
        'switch#'
    )
    out = sw1(cmd)
    assert 'Hostname should be less than 256 characters' in out

    sw1._shells['vtysh']._prompt = (
        '(^|\n)switch(\\([\\-a-zA-Z0-9]*\\))?#'
    )
    sw1(' ')
