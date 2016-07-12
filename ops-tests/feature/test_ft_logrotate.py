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
# import time
# from pytest import mark
TOPOLOGY = """
#
#    +--------+
#    |  ops1  |
#    +--------+
#
# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
# Links
ops1:if01
"""
# Global Variable
script_path = '/etc/cron.hourly/ops-gen-logrotate'
logrotateCnfFile = '/etc/logrotate-ops.conf'
shLogrotateCnfFile = 'cat /etc/logrotate-ops.conf'
value = '10M'
period = "hourly"
target = "tftp://1.1.1.1"
size = "10"
"""
Author: Tamilmannan Harikrishnan - tamilmannan.h@hpe.com
testID: Logrotate FT case
test name: Logrotate
test Description:  Test the Logrotate parameters CLI
                   on Switch."""


def checkpattern(lines, value, filename):
    for line in lines:
        if value in line:
            return True
    return False


def logrotateconfig(ops1, value, step):
    step("CONFIGURE LOG ROTATE ON SWITCH")
    out = ops1(script_path, shell="bash")
    lines = out.split('\n')
    for line in lines:
        for line in lines:
            if 'error' in line or 'Error' in line or 'ERROR' in line:
                print(line)
                return False
    out = ops1(shLogrotateCnfFile, shell="bash")
    lines = out.split('\n')
    for line in lines:
        assert 'No such file' not in line,\
            logrotateCnfFile + ' not generated\n'
    assert checkpattern(lines, value, logrotateCnfFile),\
        "Configuration file check: failed"
    return True


def logrotatecliperiodtest(ops1, interval, step):
    step("CONFIGURATION OF LOGROTATE PERIOD")
    with ops1.libs.vtysh.Configure() as ctx:
        ctx.logrotate_period(interval)
    out = ops1("ovs-vsctl list system", shell="bash")
    lines = out.split('\n')
    for line in lines:
        if 'logrotate_config' in line and 'period=hourly' in line:
            return True
    return False


def logrotateclimaxsizetest(ops1, size, step):
    step("CONFIGURATION OF LOGROTATE MAXSIZE")
    with ops1.libs.vtysh.Configure() as ctx:
        ctx.logrotate_maxsize(size)
    out = ops1("ovs-vsctl list system", shell="bash")
    lines = out.split('\n')
    for line in lines:
        if 'logrotate_config' in line and 'maxsize="10"' in line:
            return True
    return False


def logrotateclitargettest(ops1, target, step):
    step("CONFIGURATION OF LOGROTATE TFTP TARGET")
    with ops1.libs.vtysh.Configure() as ctx:
        ctx.logrotate_target(target)
    out = ops1("ovs-vsctl list system", shell="bash")
    lines = out.split('\n')
    for line in lines:
        if 'logrotate_config' in line and 'target="tftp://1.1.1.1"' in line:
            return True
    return False


def test_ft_logrotate(topology, step):
    step("TEST CASE logrotate VALIDATION")
    ops1 = topology.get('ops1')
    assert ops1 is not None
    output = logrotateconfig(ops1, value, step)
    assert output is True
    output = logrotatecliperiodtest(ops1, period, step)
    assert output is True
    output = logrotateclimaxsizetest(ops1, size, step)
    assert output is True
    logrotateclitargettest(ops1, target, step)
    assert output is True
    output = logrotateconfig(ops1, period, step)
    assert output is True
