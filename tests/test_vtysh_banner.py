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
OpenSwitch Test for custom login banners
"""

from pytest import mark


TOPOLOGY = """
# +-------+
# |       |
# |ops1   |  for netop login
# |       |
# +-------+
# +-------+
# |       |
# |ops2   |  for user root
# |       |
# +-------+
# Since each topology gets only one bash shell and that same shell session is
# returned by get_shell(), we use 2 containers so that the current user is
# unambiguous. There is no link between the containers.
# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
[type=openswitch name="OpenSwitch 1"] ops2
# Links
"""

# terminal commands
config_command = "configure terminal"
exit_command = "exit"
pre_cmd = "banner"
post_cmd = "banner exec"
disable_command = "no banner"
show_banner_command = "show banner"
ssh_command = "ssh -o StrictHostKeyChecking=no netop@localhost"
cat_issue = "cat /etc/issue.net"
cat_motd = "cat /etc/motd"

# sample custom login banner
line1 = "The use of COBOL cripples the mind;"
line2 = "its teaching should, therefore,"
line3 = "be regarded as a criminal offense"
line4 = "Edsgar Djikstra"
terminator = "\%"

# another sample custom login banner
line1b = "Software is like entropy:"
line2b = "It is diffuclt to grasp, weighs nothing,"
line3b = "and obeys the Second Law of Thermodyanmics;"
line4b = "i.e., it always increases"
line5b = "Norman Augustine"

# default banners
pre_default = "Welcome to OpenSwitch"
post_default = "Please be responsible"

# banner update responses
success = "Banner updated successfully!"
invalid_user = "Only network operators may change login banners."

# terminal prompts
vty_prompt = ".*\#"
bash_prompt = vty_prompt  # for readability
banner_readline = ">>"
vty_config = ".*\(config\)#"
conn_closed = "Connection to localhost closed"

# passwords
netop_pw = "netop"


@mark.platform_incompatible(['ostl'])
def test_custom_pre_login_valid_user(topology):
    """
    Update the banner as a user in the netop group.
    The result should be reflected by the show banner command (OVSDB), and in
    the contents of the file /etc/issue.net which are displayed before the
    password prompt in an SSH session.

    Begin an interactive bash shell as root
    1. su to 'netop' inheriting environment

    Using vtysh shell from the switch:
    1. set banner to an empty string
    2. set banner to a known value, checking for success indicator

    SSH to switch
    1. make sure that known value is displayed between before password prompt
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user 'netop' with default shell (vtysh)")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to empty string")
    shell.send_command(disable_command, [success, vty_config])
    print("Set banner to a known value")
    shell.send_command(" ".join([pre_cmd, terminator]), [banner_readline],
                       timeout=1)
    shell.send_command(line1, [banner_readline])
    shell.send_command(line2, [banner_readline])
    shell.send_command(line3, [banner_readline])
    shell.send_command(line4, [banner_readline])
    shell.send_command(terminator, [success])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Return to bash prompt")
    shell.send_command(exit_command, [bash_prompt])
    print("SSH to localhost as netop")
    shell.send_command(ssh_command, [line1])
    shell.send_command(netop_pw, [bash_prompt])
    print("Return to bash shell")
    shell.send_command(exit_command, [conn_closed])

    print("Banner set succesfully")
    print("Test custom_pre_login_valid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_custom_post_login_valid_user(topology):
    """
    Update the banner as a user in the netop group.
    The result should be reflected by the show banner command (OVSDB), and in
    the contents of the file /etc/motd,

    Begin an interactive bash shell as root
    1. su to 'netop' inheriting environment

    Using vtysh shell from the switch:
    1. set banner to an empty string
    2. set banner to known value, checking for success indicator

    SSH to switch
    1. make sure that known value is displayed after password is provided
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user 'netop' with default shell (vtysh)")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to empty string")
    shell.send_command(" ".join([disable_command, "exec"]),
                       [success, vty_config])
    print("Set banner to a known value")
    shell.send_command(" ".join([post_cmd, terminator]), [banner_readline],
                       timeout=1)
    shell.send_command(line1b, [banner_readline])
    shell.send_command(line2b, [banner_readline])
    shell.send_command(line3b, [banner_readline])
    shell.send_command(line4b, [banner_readline])
    shell.send_command(line5b, [banner_readline])
    shell.send_command(terminator, [success])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Return to bash prompt")
    shell.send_command(exit_command, [bash_prompt])
    print("SSH to localhost as netop")
    shell.send_command(ssh_command, ["password"])
    shell.send_command(netop_pw, [line3b])
    shell.send_command(exit_command, [conn_closed])

    print("Banner set succesfully")
    print("Test custom_post_login_valid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_custom_pre_login_invalid_user(topology):
    """
    Update the banner as a user that is not in the netop group.
    The requested update should be refused.

    Begin an interactive bash shell as root
    1. run vtysh

    Using vtysh shell from the switch:
    1. issue command to change banner
    2. check for failure message
    """

    ops2 = topology.get('ops2')

    assert ops2 is not None

    print("Get bash shell")
    shell = ops2.get_shell('bash')
    print("Run vtysh as root")
    shell.send_command("vtysh", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Attempt to set banner to custom value")
    shell.send_command(" ".join([pre_cmd, terminator]), [banner_readline],
                       timeout=1)
    shell.send_command("hello", [banner_readline])
    shell.send_command(terminator, [invalid_user])
    print("Exit to bash shell")
    shell.send_command(exit_command, [bash_prompt])

    print("Banner unchanged")
    print("Test custom_pre_login_invalid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_custom_post_login_invalid_user(topology):
    """
    Update the banner as a user that is not in the netop group.
    The requested update should be refused.

    Begin an interactive bash shell as root

    Using vtysh shell from the switch:
    1. issue command to change banner
    2. check for failure message
    """

    ops2 = topology.get('ops2')

    assert ops2 is not None

    print("Get bash shell")
    shell = ops2.get_shell('bash')
    print("Run vtysh as root")
    shell.send_command("vtysh", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Attempt to set banner to custom value")
    shell.send_command(" ".join([post_cmd, terminator]), [banner_readline],
                       timeout=1)
    shell.send_command("hello", [banner_readline])
    shell.send_command(terminator, [invalid_user])
    print("Exit to bash shell")
    shell.send_command(exit_command, [bash_prompt])

    print("Banner unchanged")
    print("Test custom_post_login_invalid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_default_pre_login_valid_user(topology):
    """
    Restore defualt pre-login banner as a user in the netop group.
    The result should be reflected by the show banner command (OVSDB), and in
    the contents of the file /etc/issue.net

    Begin an interactive bash shell as root
    1. su to 'netop' inheriting environment

    Using vtysh shell from the switch:
    1. restore default banner

    SSH to switch
    1. make sure that known value is displayed between before password prompt
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user 'netop' with default shell (vtysh)")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to empty string")
    shell.send_command(disable_command, [success, vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([pre_cmd, "default"]), [success])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Return to bash prompt")
    shell.send_command(exit_command, [bash_prompt])
    print("SSH to localhost as netop")
    shell.send_command(ssh_command, [pre_default])
    shell.send_command(netop_pw, [bash_prompt])
    print("Exit to bash shell")
    shell.send_command(exit_command, [conn_closed])

    print("Banner set succesfully")
    print("Test default_pre_login_valid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_default_post_login_valid_user(topology):
    """
    Restore defualt post-login banner as a user in the netop group.
    The result should be reflected by the show banner command (OVSDB), and in
    the contents of the file /etc/issue.net

    Begin an interactive bash shell as root
    1. su to 'netop' inheriting environment

    Using vtysh shell from the switch:
    1. restore default banner

    SSH to switch
    1. make sure that known value is displayed between before password prompt
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user 'netop' with default shell (vtysh)")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to empty string")
    shell.send_command(" ".join([disable_command, "exec"]),
                       [success, vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([post_cmd, "default"]), [success])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Return to bash prompt")
    shell.send_command(exit_command, [bash_prompt])
    print("SSH to localhost as netop")
    shell.send_command(ssh_command, ["password"])
    shell.send_command(netop_pw, [post_default])
    print("Exit to bash shell")
    shell.send_command(exit_command, [conn_closed])

    print("Banner set succesfully")
    print("Test default_post_login_valid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_default_pre_login_invalid_user(topology):
    """
    Restore defualt pre-login banner as a user not in the netop group.
    The attempt should be rejected.

    Begin an interactive bash shell as root

    Using vtysh shell from the switch:
    1. issue restore default banner command, check for failure message
    """

    ops2 = topology.get('ops2')

    assert ops2 is not None

    print("Get bash shell")
    shell = ops2.get_shell('bash')
    print("Enter vtysh as root")
    shell.send_command("vtysh", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([pre_cmd, "default"]), [invalid_user],
                       timeout=1)

    print("Banner set succesfully")
    print("Test default_pre_login_invalid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_default_post_login_invalid_user(topology):
    """
    Restore defualt pre-login banner as a user in the netop group.
    The result should be reflected by the show banner command (OVSDB), and in
    the contents of the file /etc/issue.net

    Begin an interactive bash shell as root

    Using vtysh shell from the switch:
    1. issue restore default banner command, check for failure message
    """

    ops2 = topology.get('ops2')

    assert ops2 is not None

    print("Get bash shell")
    shell = ops2.get_shell('bash')
    print("Enter vtysh as root")
    shell.send_command("vtysh", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([pre_cmd, "default"]), [invalid_user],
                       timeout=1)

    print("Banner set succesfully")
    print("Test default_pre_login_invalid_user PASSED")


@mark.platform_incompatible(['ostl'])
def test_disable_pre_login_valid_user(topology):
    """
    Disable the pre-login banner. If the file /etc/issue.net contains only a
    single new line, then OVSDB and the SSH banner have been changed
    appropriately.

    Begin an interactive bash shell as root
    1. su to user netop, inheriting environment

    Using vtysh shell from the switch:
    1. restore the default banner
    2. disable the banner, check for success
    3. exit vtysh

    Using bash, once again
    1. issue command 'cat /etc/issue.net', storing output
    2. confirm that the content of file is a single newline character and
    nothing else.
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user netop, inheriting shell")
    shell.send_command("su - netop", [vty_prompt])
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([pre_cmd, "default"]), [success, vty_config],
                       timeout=1)
    print("Disable banner, checking for success")
    shell.send_command(" ".join(["no", pre_cmd]), [success])
    print("Exit to bash")
    shell.send_command(exit_command, [vty_prompt])
    shell.send_command(exit_command, [bash_prompt])
    issue_output = ops1(cat_issue, shell='bash')
    issue_output = ops1(cat_issue, shell='bash')
    assert issue_output is ''


@mark.platform_incompatible(['ostl'])
def test_disable_post_login_valid_user(topology):
    """
    Disable the post-login banner. If the file /etc/motd contains only a single
    new line, then OVSDB and the SSH banner have been changed appropriately.

    Begin an interactive bash shell as root
    1. su to user netop, inheriting environment

    Using vtysh shell from the switch:
    1. restore the default banner
    2. disable the banner, check for success
    3. exit vtysh

    Using bash, once again
    1. issue command 'cat /etc/motd', storing output
    2. confirm that the content of file is a single newline character and
    nothing else.
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Switch to user netop, inheriting shell")
    shell.send_command("su - netop", [vty_prompt])
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([post_cmd, "default"]), [success, vty_config],
                       timeout=1)
    print("Disable banner, checking for success")
    shell.send_command(" ".join(["no", post_cmd]), [success])
    print("Exit to bash")
    shell.send_command(exit_command, [vty_prompt])
    shell.send_command(exit_command, [bash_prompt])
    issue_output = ops1(cat_motd, shell='bash')
    issue_output = ops1(cat_motd, shell='bash')
    assert issue_output is ''


@mark.platform_incompatible(['ostl'])
def test_disable_pre_login_invalid_user(topology):
    """
    Attempt to disable the login banner. The attempt should be refused.

    Begin an interactive bash shell as root
    1. enter vtysh interactive shell

    Using vtysh shell from the switch:
    1. enter configuration context
    2. attempt to change banner, checking for failure
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Enter vtysh shell")
    shell.send_command("vtysh", [vty_prompt])
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Disable banner, checking for failure")
    shell.send_command(" ".join(["no", pre_cmd]), [invalid_user])
    print("Exit to bash")
    shell.send_command(exit_command, [vty_prompt])
    shell.send_command(exit_command, [bash_prompt])


@mark.platform_incompatible(['ostl'])
def test_disable_post_login_invalid_user(topology):
    """
    Attempt to disable the login banner. The attempt should be refused.

    Begin an interactive bash shell as root
    1. enter vtysh interactive shell

    Using vtysh shell from the switch:
    1. enter configuration context
    2. attempt to change banner, checking for failure
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("Enter vtysh shell")
    shell.send_command("vtysh", [vty_prompt])
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Disable banner, checking for failure")
    shell.send_command(" ".join(["no", post_cmd]), [invalid_user])
    print("Exit to bash")
    shell.send_command(exit_command, [vty_prompt])
    shell.send_command(exit_command, [bash_prompt])


@mark.platform_incompatible(['ostl'])
def test_display_pre_login(topology):
    """
    Attempt to display the login banner. It should match the expected value.

    Begin an interactive bash shell as root
    1. su to user netop, inheriting environment

    Using vtysh shell from the switch:
    1. enter configuration context
    2. restore default banner
    3. exit configuration context
    4. issue command to show banner, confirm it matches default
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("su to user netop")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([pre_cmd, "default"]), [success, ""])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Display the configured banner")
    shell.send_command(" ".join(["show", pre_cmd]), [pre_default])
    print("Exit to bash shell")
    shell.send_command(exit_command, [bash_prompt])

    print("Banner displayed succesfully")
    print("Test display_pre_login PASSED")


@mark.platform_incompatible(['ostl'])
def test_display_post_login(topology):
    """
    Attempt to display the login banner. It should match the expected value.

    Begin an interactive bash shell as root
    1. su to user netop, inheriting environment

    Using vtysh shell from the switch:
    1. enter configuration context
    2. restore default banner
    3. exit configuration context
    4. issue command to show banner, confirm it matches default
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    ops1 = topology.get('ops1')

    assert ops1 is not None

    print("Get bash shell")
    shell = ops1.get_shell('bash')
    print("su to user netop")
    shell.send_command("su - netop", vty_prompt)
    print("Enter configuration context")
    shell.send_command(config_command, [vty_config])
    print("Set banner to default value")
    shell.send_command(" ".join([post_cmd, "default"]), [success, ""])
    print("Exit configuration context")
    shell.send_command(exit_command, [vty_prompt])
    print("Display the configured banner")
    shell.send_command(" ".join(["show", post_cmd]), [post_default])
    print("Exit to bash shell")
    shell.send_command(exit_command, [bash_prompt])

    print("Banner displayed succesfully")
    print("Test display_post_login PASSED")
