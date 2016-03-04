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

"""
OpenSwitch Test for vlan related configurations.
"""

from __future__ import unicode_literals, absolute_import
from __future__ import print_function, division

from pytest import mark

# from lacp_lib import turn_on_interface

TOPOLOGY = """
# +-------+
# |       |
# |ops1   |
# |       |
# +-------+

# Nodes
# [image="genericx86-64:latest" \
# type=openswitch name="OpenSwitch 1"] ops1
[type=openswitch name="OpenSwitch 1"] ops1
# Links
"""


@mark.platform_incompatible(['ostl'])
def test_change_user_password(topology):
    """
    Test that user logged-on via CLI can change own password using 'password'
    CLI command.

    Using bash shell from the switch
    1. ssh to switch as a user netop
    2. run password command
    3. change current password to '1111'
    4. exit from CLI
    5. re-login with a new password
    """
    ops1 = topology.get('ops1')
    conn = "ssh -o StrictHostKeyChecking=no netop@localhost"
    old_password = "netop"
    new_password = "1111"
    matches = ['fingerprint', 'continue', 'yes', 'no']

    assert ops1 is not None

    # get bash shell instance
    bash_shell = ops1.get_shell('bash')

    # connect to vtysh as netop via @localhost
    print("SSH to localhost as netop")
    # assert bash_shell.send_command(conn, matches) is 0
    assert bash_shell.send_command(conn, matches=['password:']) is 0
    assert bash_shell.send_command(old_password, matches=['switch#']) is 0

    # change password
    matches = ['old password:']
    print("Execute password command")
    assert bash_shell.send_command("password", matches) is 0

    # enter old password
    matches = ['new password:']
    print("Enter old password")
    assert bash_shell.send_command(old_password, matches) is 0

    # enter new password
    print("Enter new password")
    assert bash_shell.send_command(new_password, matches) is 0

    # confirm new password
    matches = ['executed successfully.']
    print("Re-enter new password")
    assert bash_shell.send_command(new_password, matches) is 0

    print("Password updated for netop")

    # repeat above process with a new password
    # exit from the ssh
    matches = ['Connection to localhost closed.']
    assert bash_shell.send_command('exit', matches) is 0

    # connect to vtysh as netop via @localhost'
    matches = ['password:']
    print("Re-connect to SSH session as netop using new password")
    assert bash_shell.send_command(conn, matches) is 0
    assert bash_shell.send_command(new_password, matches=['switch#']) is 0

    print("Change the password back to default")

    # change password
    matches = ['old password:']
    print("Execute password command")
    assert bash_shell.send_command("password", matches) is 0

    # enter old password
    matches = ['new password:']
    print("Enter old password")
    assert bash_shell.send_command(new_password, matches) is 0

    # enter new password
    print("Enter new password")
    assert bash_shell.send_command(old_password, matches) is 0

    # confirm new password
    matches = ['executed successfully.']
    print("Re-enter new password")
    assert bash_shell.send_command(old_password, matches) is 0

    # exit from the ssh
    matches = ['Connection to localhost closed.']
    assert bash_shell.send_command('exit', matches) is 0

    print("Test test_change_user_password PASSED")


@mark.platform_incompatible(['ostl'])
def test_invalid_old_password(topology):
    """
    Test that user logged-on via CLI can change own password using 'password'
    CLI command and provide the wrong password.  The password server must
    reject the password change request and send error status back to CLI.

    Using bash shell from the switch
    1. ssh to switch as a user netop
    2. run password command
    3. change current password as 'FFFF'
    4. verify that error "Old password did not match." has occurred
    4. exit from CLI
    5. re-login with a new password
    """

    ops1 = topology.get('ops1')
    conn = "ssh -o StrictHostKeyChecking=no netop@localhost"
    new_password = "1111"
    wrong_password = "FFFF"
    old_password = "netop"
    matches = ['fingerprint', 'continue', 'yes', 'no']

    assert ops1 is not None

    # get bash shell instance
    bash_shell = ops1.get_shell('bash')

    # connect to vtysh as netop via @localhost
    print("SSH to localhost as netop")
    assert bash_shell.send_command(conn, matches=['password:']) is 0
    assert bash_shell.send_command(old_password, matches=['switch#']) is 0

    # change password
    matches = ['old password:']
    print("Execute password command")
    assert bash_shell.send_command("password", matches) is 0

    # enter old password
    matches = ['new password:']
    print("Enter invalid password")
    assert bash_shell.send_command(wrong_password, matches) is 0

    # enter new password
    print("Enter new password")
    assert bash_shell.send_command(new_password, matches) is 0

    # confirm new password
    matches = ['did not match']
    print("Re-enter new password")
    assert bash_shell.send_command(new_password, matches) is 0

    # exit from the ssh
    matches = ['Connection to localhost closed.']
    assert bash_shell.send_command('exit', matches) is 0
    print("Test test_invalid_old_password PASSED")
