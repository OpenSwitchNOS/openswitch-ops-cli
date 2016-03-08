# -*- coding: utf-8 -*-

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can rediTestribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is diTestributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; withoutputputputputput even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, BoTeston, MA
# 02111-1307, USA.


TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


script_path = '/etc/cron.hourly/ops-gen-logrotate'
log_rotate_cnf_file = '/etc/logrotate-ops.conf'
shlog_rotate_cnf_file = 'cat /etc/logrotate-ops.conf'


def check_pattern(lines, value):
    is_in = False
    for line in lines:
        if value in line:
            is_in = True
            break
    assert is_in


def log_rotate_config(sw1, value):
    output = sw1(script_path, shell='bash')
    lines = output.splitlines()
    for line in lines:
        if 'error' in line or 'Error' in line or 'ERROR' in line:
            print(line)
    output = sw1(shlog_rotate_cnf_file, shell='bash')
    assert 'No such file' not in output
    lines = output.splitlines()
    check_pattern(lines, value)


def log_rotation(sw1):
    output = sw1('ls /var/log/messages*.gz', shell='bash')
    assert 'No such file' not in output


def conf_log_rotate_cli_get_period(sw1):
    is_in = False
    output = sw1('list system', shell='vsctl')
    lines = output.splitlines()
    for line in lines:
        if 'logrotate_config' in line and 'period=hourly' in line:
            is_in = True
            break
    assert is_in


def conf_log_rotate_cli_get_maxsize(sw1):
    is_in = False
    output = sw1('list system', shell='vsctl')
    lines = output.splitlines()
    for line in lines:
        if 'logrotate_config' in line and 'maxsize="10"' in line:
            is_in = True
            break
    assert is_in


def conf_log_rotate_cli_get_target(sw1):
    is_in = False
    output = sw1('list system', shell='vsctl')
    lines = output.splitlines()
    for line in lines:
        if 'logrotate_config' in line and 'target="tftp://1.1.1.1"' in line:
            is_in = True
            break
    assert is_in


def log_rotate_cli_period(sw1):
    sw1('logrotate period hourly')
    conf_log_rotate_cli_get_period(sw1)


def log_rotate_cli_maxsize(sw1):
    sw1('logrotate maxsize 10')
    conf_log_rotate_cli_get_maxsize(sw1)


def log_rotate_cli_target(sw1):
    sw1('logrotate target tftp://1.1.1.1')
    conf_log_rotate_cli_get_target(sw1)


def log_rotation_period(sw1):
    log_rotate_cli_period(sw1)
    now = sw1('date +"%F %T"', shell='bash')
    sw1("date --set='2015-06-26 11:21:42'", shell='bash')
    log_rotate_config('hourly')
    sw1("date --set='2015-06-26 12:21:42'", shell='bash')
    log_rotate_config('hourly')
    log_rotation()
    sw1('date --set=' + '"' + now + '"', shell='bash')


def test_vtysh_ct_logrotate(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    sw1("configure terminal")
    step("Test to verify logrotate commands")
    log_rotate_config(sw1, '10M')
    # step("Test default Config file: passed")
    log_rotate_cli_period(sw1)
    log_rotate_cli_maxsize(sw1)
    log_rotate_cli_target(sw1)
    log_rotate_config(sw1, 'hourly')
    # step("Test config file generation from DB: passed")
