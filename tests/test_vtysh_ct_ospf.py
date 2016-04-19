#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

import pytest
import re
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *
from opsvsiutils.vtyshutils import *

# Topology definition
topoDict = {"topoExecution": 5000,
            "topoTarget": "dut01 dut02",
            "topoDevices": "dut01 dut02",
            "topoLinks": "lnk01:dut01:dut02",
            "topoFilters": "dut01:system-category:switch,\
                            dut02:system-category:switch"}


def enterConfigShell(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    return True


def enterRouterContext(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    devIntReturn = dut01.DeviceInteract(command="router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter OSPF context"

    return True


def enterInterfaceContext(dut01, interface, enable):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    retStruct = dut01.ConfigVtyShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter config terminal"

    cmd = "interface " + str(interface)
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to enter Interface context"

    if (enable is True):
        dut01.DeviceInteract(command="no shutdown")
        dut01.DeviceInteract(command="no routing")

    return True


def exitContext(dut01):
    devIntReturn = dut01.DeviceInteract(command="exit")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Failed to exit current context"

    retStruct = dut01.ConfigVtyShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit config terminal"

    retStruct = dut01.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

    return True


def createRouterInstanceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to enter OSPF context failed"

    return True


def deleteRouterInstanceTest(dut01):
    if (enterConfigShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router ospf")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to delete OSPF context failed"

    return True


def setRouterIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router-id 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set router id failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip ospf")
    assert '1.1.1.1' in cmdOut, "Test to set router id failed"

    return True


def unsetRouterIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router-id")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset router id failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show ip ospf")
    assert '1.1.1.1' not in cmdOut, "Test to unset router id failed"
    return True


def setNetworkAreaIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network area id failed"

    return True


def unsetNetworkAreaIdTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 "
                                                "area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    return True


def setMaxMetricAdminTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric admin failed"

    return True


def unsetMaxMetricAdminTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric admin failed"

    return True


def setMaxMetricStartupTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa "
                                                "on-startup 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric startup failed"

    return True


def unsetMaxMetricStartupTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa "
                                                "on-startup")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric startup failed"

    return True


def setDistanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="distance 5")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set distance cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'distance 5' in cmdOut, "Test to set distance failed"

    return True


def unsetDistanceTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no distance 5")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset distance failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'distance 5' not in cmdOut, "Test to unset distance failed"
    return True


def setCompatiblerfc1583Test(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="compatible rfc1583")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set compatible rfc1583 cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'compatible rfc1583' in cmdOut, "Test to set compatible \
                                                     rfc1583 failed"
    return True


def unsetCompatiblerfc1583Test(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no compatible rfc1583")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset compatible rfc1583 cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'compatible rfc1583' not in cmdOut, "Test to unset compatible \
                                                           rfc1583 failed"
    return True


def setDefaultmetricTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="default-metric 190")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set default-metric cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'default-metric 190' in cmdOut, "Test to set default-metric failed"
    return True


def unsetDefaultmetricTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no default-metric 190")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset default-metric cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'default-metric 190' not in cmdOut, "Test to unset default-metric \
                                                                      failed"
    return True


def setTimerslsagrouppacingTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="timers lsa-group-pacing 250")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set timers lsa-group-pacing cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'timers lsa-group-pacing 250' in cmdOut, "Test to set timers \
                                                   lsa-group-pacing failed"
    return True


def unsetTimerslsagrouppacingTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no timers lsa-group-pacing \
                                                                         250")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset timers lsa-group-pacing cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running")
    assert 'timers lsa-group-pacing 250' not in cmdOut, "Test to unset timers \
                                                      lsa-group-pacing  failed"
    return True


def configure(**kwargs):
    '''
     - Configures the IP address in SW1 and SW2
     - Creates router ospf instances
     - Configures the router id
     - Configures the network range and area
    '''

    switch1 = kwargs.get('switch1', None)
    switch2 = kwargs.get('switch2', None)

    '''
    - Enable the link.
    - Set IP for the switches.
    '''
    # Enabling interface 1 SW1.
    LogOutput('info', "Enabling interface1 on SW1")
    retStruct = InterfaceEnable(deviceObj=switch1, enable=True,
                                interface=switch1.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Unable to enable interface1 on SW1"

    # Assigning an IPv4 address on interface 1 of SW1
    LogOutput('info', "Configuring IPv4 address on link 1 SW1")
    retStruct = InterfaceIpConfig(deviceObj=switch1,
                                  interface=switch1.linkPortMapping['lnk01'],
                                  addr="10.10.10.1", mask="24",
                                  config=True)
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to configure an IPv4 address on interface 1 of SW1"

    # Enabling interface 1 SW2
    LogOutput('info', "Enabling interface1 on SW2")
    retStruct = InterfaceEnable(deviceObj=switch2, enable=True,
                                interface=switch2.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Unable to enable interface 1 on SW2"

    # Assigning an IPv4 address on interface 1 for link 1 SW2
    LogOutput('info', "Configuring IPv4 address on link 1 SW2")
    retStruct = InterfaceIpConfig(deviceObj=switch2,
                                  interface=switch2.linkPortMapping['lnk01'],
                                  addr="10.10.10.2", mask="24",
                                  config=True)
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to configure an IPv4 address on interface 1 of SW2"

    '''
    For all the switches
    - Create the instance.
    - Configure network range and area id.
    - Configure the router Id.
    '''
    result = setRouterIdTest(switch1)
    assert result is True, "OSPF router id set failed for SW1"
    LogOutput('info', "Configuring OSPF network for SW1")
    result = setNetworkAreaIdTest(switch1)
    assert result is True, "OSPF network creation failed for SW1"
    exitContext(switch1)  # In config context

    result = setRouterIdTest(switch2)
    assert result is True, "OSPF router id set failed for SW2"
    LogOutput('info', "Configuring OSPF network for SW2")
    result = setNetworkAreaIdTest(switch2)
    assert result is True, "OSPF network creation failed for SW1"
    exitContext(switch2)  # In config context

    return True


# Function to get the dead interval and hello timer value
def getTimers(dut02, timer_type):
    ospf_interface = SwitchVtyshUtils.vtysh_cmd(dut02, "show\
                                                        ip ospf interface")

    if "Dead" in timer_type:
        matchObj = re.search(r'Dead\s\d+', ospf_interface)
    elif "Hello" in timer_type:
        matchObj = re.search(r'Hello\s\d+', ospf_interface)

    if matchObj:
        timers = re.split(r'\s+', matchObj.group())
        timer = timers[1]
    else:
        timer = 0

    return timer


def setHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf hello-interval 25")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set hello interval command failed"

    if (exitContext(dut01) is False):
        return False

    return True


def unsetHelloIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf hello-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset hello interval command failed"

    if (exitContext(dut01) is False):
        return False

    setHelloIntervalTest(dut01)
    return True


def setDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="ip ospf dead-interval 50")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set dead interval command failed"

    if (exitContext(dut01) is False):
        return False

    return True


def unsetDeadIntervalTest(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no ip ospf dead-interval")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset dead interval failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setAreaAuthType(dut01, authType, area):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " authentication"
    auth_output = "simple password authentication"

    if authType == "md5":
        cmd = cmd + " message-digest"
        auth_output = "message digest authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth type %s with area %s cmd failed" % \
                         (authType, area)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetAreaAuthType(dut01, area):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " authentication"
    auth_output = "no authentication"

    print cmd
    print auth_output
    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth with area %s cmd failed"\
                         % (area)

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceAuthType(dut01, authType):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf authentication"

    if authType == "md5":
        cmd += " message-digest"

    if authType == "null":
        cmd += " null"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth type %s in interface 1 failed" % \
                         (authType)

    return True


def setInterfaceAuthKey(dut01, authKey):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf authentication-key " + authKey

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set auth key %s in interface 1 failed" % \
                         (authKey)

    return True


def unsetInterfaceAuthKey(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf authentication-key"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth key in interface 1 failed"

    return True


def setInterfaceMd5Key(dut01, md5KeyId, md5Key):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf message-digest-key " + md5KeyId + " md5 " + md5Key

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set md5 key %s with keyId %s in " \
                         "interface 1 failed" % (md5Key, md5KeyId)
    return True


def unsetInterfaceMd5Key(dut01, md5KeyId):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf message-digest-key " + md5KeyId

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset md5 key id %s in interface 1 failed" \
                         % (md5KeyId)
    return True


def unsetInterfaceAuthType(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset auth in interface 1 failed"

    return True


def setInterfaceRetransmitInterval(dut01, interval):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf retransmit-interval " + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set retransmit interval %s in " \
                         "interface 1 failed" % (interval)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfaceRetransmitInterval(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf retransmit-interval"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset retransmit interval in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceTransmitDelay(dut01, interval):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf transmit-delay " + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set transmit delay %s in " \
                         "interface 1 failed" % (interval)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfaceTransmitDelay(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf transmit-delay"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset transmit delay in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfacePriority(dut01, priority):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf priority " + priority

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set priority %s in " \
                         "interface 1 failed" % (priority)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfacePriority(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf priority"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset priority in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceMtuIgnore(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf mtu-ignore"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set mtu-ignore in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfaceMtuIgnore(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf mtu-ignore"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset mtu-ignore in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceCost(dut01, cost):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf cost " + cost

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set cost %s in " \
                         "interface 1 failed" % (cost)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfaceCost(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf cost"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset cost in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setInterfaceNetworkType(dut01, nwType):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "ip ospf network " + nwType

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network %s in " \
                         "interface 1 failed" % (nwType)

    if (exitContext(dut01) is False):
        return False

    return True


def unsetInterfaceNetworkType(dut01):
    if (enterInterfaceContext(dut01, "1", True) is False):
        return False

    cmd = "no ip ospf network"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network in " \
                         "interface 1 failed"

    if (exitContext(dut01) is False):
        return False

    return True


def setAreaNssa(dut01, area, nssaType, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " nssa " + nssaType

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set nssa type %s with area %s cmd failed" % \
                         (nssaType, area)

    output = devIntReturn.get('buffer')

    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure NSSA to backbone" in output, \
            "Should not configure backbone area as NSSA"

    return True


def unsetAreaNssa(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " nssa"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area %s from nssa failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure NSSA to backbone" in output, \
            "Should not configure backbone area as NSSA"

    return True


def setAreaStub(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " stub"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set area %s as stub failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure" and "to backbone" in output, \
            "Should not configure backbone area as Stub"

    return True


def unsetAreaStub(dut01, area, noSummary):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " stub"

    if noSummary:
        cmd += " no-summary"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area %s from stub failed" % area

    output = devIntReturn.get('buffer')
    if area == "0.0.0.0" or area == "0":
        assert "Cannot configure" and "to backbone" in output, \
            "Should not configure backbone area as Stub"

    return True


def runningConfigTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="router-id 1.2.3.4")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set router id failed"

    devIntReturn = dut01.DeviceInteract(command="network 10.0.0.0/24 area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set network area id failed"

    devIntReturn = dut01.DeviceInteract(command="max-metric router-lsa "
                                                "on-startup 200")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set max-metric startup failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' in cmdOut, "Test to show router id in " \
                                          "running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' in cmdOut, \
        "Test to show max-metric in running config failed"

    return True


def noRunningConfigTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no router-id")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset router id failed"

    devIntReturn = dut01.DeviceInteract(command="no network 10.0.0.0/24 "
                                                "area 100")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset network area id failed"

    devIntReturn = dut01.DeviceInteract(command="no max-metric router-lsa "
                                                "on-startup")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset max-metric startup failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'router-id 1.2.3.4' not in cmdOut, "Test to show router id " \
                                              "in running config failed"
    assert 'network 10.0.0.0/24 area 0.0.0.100' not in cmdOut, \
        "Test to show network in running config failed"
    assert 'max-metric router-lsa on-startup 200' not in cmdOut, \
        "Test to show max-metric in running config failed"

    return True


def setareacostTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="area 1 default-cost 1234")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set area default-cost cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'area 0.0.0.1 default-cost 1234' in cmdOut, "Test to set area \
                                                    default-cost failed"
    return True


def unsetareacostTest(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no area 0.0.0.1 default-cost \
                                                                         1234")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area default-cost cmd failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'no area 0.0.0.1 default-cost 1234' not in cmdOut, "Test to area \
                                            default-cost cmd failed failed"
    return True


def setRouteRedistribution(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="redistribute connected")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set connected route redistribution failed"

    devIntReturn = dut01.DeviceInteract(command="redistribute static")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set static route redistribution failed"

    devIntReturn = dut01.DeviceInteract(command="redistribute bgp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set bgp route redistribution failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'redistribute connected' in cmdOut, "Test to set connected route" \
                                               "redistribution failed"
    assert 'redistribute static' in cmdOut, \
        "Test to set static route route redistribution failed"
    assert 'redistribute bgp' in cmdOut, \
        "Test to set bgp route redistribution failed"

    return True


def unsetRouteRedistribution(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="no redistribute connected")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset connected route redistribution failed"

    devIntReturn = dut01.DeviceInteract(command="no redistribute static")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset static route redistribution failed"

    devIntReturn = dut01.DeviceInteract(command="no redistribute bgp")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset bgp route redistribution failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'redistribute connected' not in cmdOut, "Test to unset connected" \
           "route redistribution failed"
    assert 'redistribute static' not in cmdOut, \
        "Test to unset static route route redistribution failed"
    assert 'redistribute bgp' not in cmdOut, \
        "Test to unset bgp route redistribution failed"

    return True


def setDefaultRouteRedistribution(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(
        command="default-information originate")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set default route redistribution failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    assert 'default-information originate' in cmdOut, "Test to set default" \
           " route redistribution failed"

    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(
        command="default-information originate always")

    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set default route redistribution failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'default-information originate always' in cmdOut, \
        "Test to set default route redistribution failed"

    return True


def unsetDefaultRouteRedistribution(dut01):
    if (enterRouterContext(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(
        command="no default-information originate")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, \
        "Test to unset default route redistribution failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    assert 'default-information originate' not in cmdOut, \
        "Test to set default route redistribution failed"

    return True


def setAreaVirtualLink(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " + virtualLink

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set area %s virtual-link %s failed" % \
                         (area, virtualLink)

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                return True

    return False


def unsetAreaVirtualLink(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link " + virtualLink

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset area %s virtual-link %s failed" % \
                         (area, virtualLink)

    output = devIntReturn.get('buffer')
    assert 'Virtual link configuration is not present.' not in output, \
        "Test to unset virtual-link in area failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                return False

    return True


def setVirtualLinkHelloIntervalTest(dut01, area, virtualLink, interval):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " + virtualLink + " hello-interval "\
          + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set virtual-link hello-interval command" \
        " failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "hello-interval" in line:
                    return True

    return False


def unsetVirtualLinkHelloIntervalTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link " + virtualLink + \
          " hello-interval"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset virtual-link hello-interval command" \
        " failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "hello-interval" in line:
                    return False

    return True


def setVirtualLinkTransmitDelayTest(dut01, area, virtualLink, interval):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " + virtualLink + \
          " transmit-delay " + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set virtual-link transmit-delay command" \
        " failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "transmit-delay" in line:
                    return True

    return False


def unsetVirtualLinkTransmitDelayTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link " \
          + virtualLink + " transmit-delay"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset virtual-link transmit-delay command" \
        " failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "transmit-delay" in line:
                    return False

    return True


def setVirtualLinkRetransmitIntervalTest(dut01, area, virtualLink, interval):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " + virtualLink + \
          " retransmit-interval " + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0,\
        "Test to set virtual-link retransmit-interval command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")

    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "retransmit-interval" in line:
                    return True

    return False


def unsetVirtualLinkRetransmitIntervalTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link "\
          + virtualLink + " retransmit-interval"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, \
        "Test to unset virtual-link retransmit-interval command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "retransmit-interval" in line:
                    return False

    return True


def setVirtualLinkDeadIntervalTest(dut01, area, virtualLink, interval):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " \
          + virtualLink + " dead-interval " + interval

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, \
        "Test to set virtual-link dead-interval command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "dead-interval" in line:
                    return True

    return False


def unsetVirtualLinkDeadIntervalTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link "\
          + virtualLink + " dead-interval"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, \
        "Test to unset virtual-link dead-interval command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "dead-interval" in line:
                    return False

    return True


def setVirtualLinkAuthMsgDigestTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link "\
          + virtualLink + " authentication message-digest"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set virtual-link authentication " \
        "message-digest command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "authentication" in line:
                    if "message-digest" in line:
                        return True

    return False


def unsetVirtualLinkAuthMsgDigestTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link "\
          + virtualLink + " authentication"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset virtual-link authentication " \
        "message-digest command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "authentication" in line:
                    return False

    return True


def setVirtualLinkAuthKeyTest(dut01, area, virtualLink, auth_key):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " \
          + virtualLink + " authentication-key " + auth_key

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set virtual-link authentication-key " \
        "command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "authentication-key" in line:
                    if auth_key in line:
                        return True

    return False


def unsetVirtualLinkAuthKeyTest(dut01, area, virtualLink):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link " \
          + virtualLink + " authentication-key"

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset virtual-link authentication-key " \
        "command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "authentication-key" in line:
                    return False

    return True


def setVirtualLinkMsgDigestKeyTest(dut01, area, virtualLink, key, md5_key):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "area " + area + " virtual-link " + virtualLink + \
          " message-digest-key " + key + " md5 " + md5_key

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set virtual-link message-digest-key " \
        "command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "message-digest-key" in line:
                    if key in line:
                        if "md5" in line:
                            if md5_key in line:
                                return True

    return False


def unsetVirtualLinkMsgDigestKeyTest(dut01, area, virtualLink, key):
    if (enterRouterContext(dut01) is False):
        return False

    cmd = "no area " + area + " virtual-link " + \
          virtualLink + " message-digest-key " + key

    devIntReturn = dut01.DeviceInteract(command=cmd)
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to unset virtual-link message-digest-key " \
        "command failed"

    if (exitContext(dut01) is False):
        return False

    cmdOut = dut01.cmdVtysh(command="show running-config")
    lines = cmdOut.split('\n')
    for line in lines:
        if "virtual-link" in line:
            if virtualLink in line:
                if "message-digest-key" in line:
                    if key in line:
                        if md5 in line:
                            if md5_key in line:
                                return False

    return True


#@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_ospf_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_ospf_configuration.testObj = testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_ospf_configuration.topoObj = \
            Test_ospf_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_ospf_configuration.topoObj.terminate_nodes()

    def test_createRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = createRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Creating OSPF router instance - passed")
        else:
            LogOutput('error', "Creating OSPF router instance - failed")

    def test_deleteRouterInstanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = deleteRouterInstanceTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Deleting OSPF router instance - passed")
        else:
            LogOutput('error', "Deleting OSPF router instance - failed")

    def test_setRouterIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set router id - passed")
        else:
            LogOutput('error', "Set router id - failed")

    def test_unsetRouterIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset router id - passed")
        else:
            LogOutput('error', "Unset router id - failed")

    def test_setNetworkAreaIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setNetworkAreaIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set network area id - passed")
        else:
            LogOutput('error', "Set network area id - failed")

    def test_unsetNetworkAreaIdTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetNetworkAreaIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset network area id - passed")
        else:
            LogOutput('error', "Unset network area id - failed")

    def test_setMaxMetricAdminTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setMaxMetricAdminTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set max metric admin - passed")
        else:
            LogOutput('error', "Set max metric admin - failed")

    def test_unsetMaxMetricAdminTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetMaxMetricAdminTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset max metric admin - passed")
        else:
            LogOutput('error', "Unset max metric admin - failed")

    def test_setMaxMetricStartupTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setMaxMetricStartupTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set max metric startup - passed")
        else:
            LogOutput('error', "Set max metric startup - failed")

    def test_unsetMaxMetricStartupTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetMaxMetricStartupTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset max metric startup - passed")
        else:
            LogOutput('error', "Unset max metric startup - failed")

    def test_setDistanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set distance - passed")
        else:
            LogOutput('error', "Set distance - failed")

    def test_unsetDistanceTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetRouterIdTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset distance - passed")
        else:
            LogOutput('error', "Unset distance - failed")

    def test_runningConfigTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = runningConfigTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Running config show - passed")
        else:
            LogOutput('error', "Running config show - failed")

    def test_noRunningConfigTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = noRunningConfigTest(dut01Obj)
        if(retValue):
            LogOutput('info', "no form running config show - passed")
        else:
            LogOutput('error', "no form running config show - failed")

    def test_setCompatiblerfc1583Test(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setCompatiblerfc1583Test(dut01Obj)
        if(retValue):
            LogOutput('info', "Set compatible rfc1583 - passed")
        else:
            LogOutput('error', "Set compatible rfc1583 - failed")

    def test_unsetCompatiblerfc1583Test(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetCompatiblerfc1583Test(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Compatible rfc1583 - passed")
        else:
            LogOutput('error', "Unset Compatible rfc1583 - failed")

    def test_setDefaultmetricTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setDefaultmetricTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set default-metric - passed")
        else:
            LogOutput('error', "Set default-metric - failed")

    def test_unsetDefaultmetricTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetDefaultmetricTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset default-metric - passed")
        else:
            LogOutput('error', "Unset default-metric - failed")

    def test_setTimerslsagrouppacingTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTimerslsagrouppacingTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set timers lsa-group-pacing - passed")
        else:
            LogOutput('error', "Set timers lsa-group-pacing - failed")

    def test_unsetTimerslsagrouppacingTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetTimerslsagrouppacingTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset timers lsa-group-pacing - passed")
        else:
            LogOutput('error', "Unset timers lsa-group-pacing - failed")

    def test_setareacost(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setareacostTest(dut01Obj)
        if(retValue):
            LogOutput('info', "set area default-Cost - passed")
        else:
            LogOutput('error', "set area default-cost - failed")

    def test_unsetareacost(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetareacostTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset area default-Cost - passed")
        else:
            LogOutput('error', "Unset area default-cost - failed")

    def test_configure(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        dut02Obj = self.topoObj.deviceObjGet(device="dut02")
        configure(switch1=dut01Obj, switch2=dut02Obj)

    def test_setHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")

        retValue = setHelloIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set hello interval - passed")
        else:
            LogOutput('error', "Set hello interval - failed")

    def test_unsetHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetHelloIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset hello interval - passed")
        else:
            LogOutput('error', "Unset hello interval - failed")

    def test_setDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setDeadIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set Dead interval - passed")
        else:
            LogOutput('error', "Set Dead interval - failed")

    def test_unsetDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetDeadIntervalTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Dead interval - passed")
        else:
            LogOutput('error', "Unset Dead interval - failed")

    # Enable text auth for backbone
    def test_setAreaTextAuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "0.0.0.0")
        if(retValue):
            LogOutput('info', "Set Backbone area text auth  - passed")
        else:
            LogOutput('error', "Set Backbone area text auth - failed")

    # Enable md5 auth for backbone
    def test_setAreaMd5AuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "0")
        if(retValue):
            LogOutput('info', "Set Backbone area md5 auth  - passed")
        else:
            LogOutput('error', "Set Backbone area md5 auth - failed")

    # Enable text auth
    def test_setAreaTextAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "1.1.1.1")
        if(retValue):
            LogOutput('info', "Set area text auth  - passed")
        else:
            LogOutput('error', "Set area text auth - failed")

    # Enable md5 auth
    def test_setAreaMd5Auth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaAuthType(dut01Obj, "text", "1")
        if(retValue):
            LogOutput('info', "Set area md5 auth  - passed")
        else:
            LogOutput('error', "Set area md5 auth - failed")

    # Disable auth for backbone
    def test_unsetAreaAuthForBackbone(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaAuthType(dut01Obj, "0")
        if(retValue):
            LogOutput('info', "Unset Backbone area auth - passed")
        else:
            LogOutput('error', "Unset Backbone area auth - failed")

    def test_unsetAreaAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaAuthType(dut01Obj, "1.1.1.1")
        if(retValue):
            LogOutput('info', "Unset area auth - passed")
        else:
            LogOutput('error', "Unset area auth - failed")

    # Enable text auth in interface 1
    def test_setInterfaceTextAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "text")
        if(retValue):
            LogOutput('info', "Set Backbone interface text auth  - passed")
        else:
            LogOutput('error', "Set Backbone interface text auth - failed")

    # Enable md5 auth in interface 1
    def test_setInterfaceMd5Auth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "md5")
        if(retValue):
            LogOutput('info', "Set interface md5 auth  - passed")
        else:
            LogOutput('error', "Set interface md5 auth - failed")

    # Enable null auth in interface 1
    def test_setInterfaceNullAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthType(dut01Obj, "null")
        if(retValue):
            LogOutput('info', "Set interface null auth  - passed")
        else:
            LogOutput('error', "Set interface null auth - failed")

    def test_unsetInterfaceAuth(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceAuthType(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset interface auth - passed")
        else:
            LogOutput('error', "Unset interface auth - failed")

    def test_setAreaNssaCandidate(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "0.0.0.0", "translate-candidate",
                               False)
        if(retValue):
            LogOutput('info', "Set area NSSA candidate - passed")
        else:
            LogOutput('error', "Set area NSSA candidate - failed")

    def test_setAreaNssaNever(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "0", "translate-never", False)
        if(retValue):
            LogOutput('info', "Set area NSSA never - passed")
        else:
            LogOutput('error', "Set area NSSA never - failed")

    def test_setAreaNssaAlways(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaNssa(dut01Obj, "100", "translate-always", True)
        if(retValue):
            LogOutput('info', "Set area NSSA always - passed")
        else:
            LogOutput('error', "Set area NSSA always - failed")

    def test_unsetAreaNssa(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaNssa(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "unset area NSSA  - passed")
        else:
            LogOutput('error', "unset area NSSA - failed")

    def test_unsetAreaNssaNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaNssa(dut01Obj, "100", True)
        if(retValue):
            LogOutput('info', "unset area NSSA no summary - passed")
        else:
            LogOutput('error', "unset area NSSA no summary - failed")

    def test_setAreaStub(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaStub(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "Set area stub - passed")
        else:
            LogOutput('error', "Set area stub - failed")

    def test_setAreaStubNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaStub(dut01Obj, "0", True)
        if(retValue):
            LogOutput('info', "Set area stub no summary - passed")
        else:
            LogOutput('error', "Set area stub no summary - failed")

    def test_unsetAreaStub(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaStub(dut01Obj, "0", False)
        if(retValue):
            LogOutput('info', "unset area stub - passed")
        else:
            LogOutput('error', "unset area stub - failed")

    def test_unsetAreaStubNoSummary(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaStub(dut01Obj, "0", True)
        if(retValue):
            LogOutput('info', "Unset area stub no summary - passed")
        else:
            LogOutput('error', "Unset area stub no summary - failed")

    def test_setInterfaceAuthKey(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceAuthKey(dut01Obj, "victory")
        if(retValue):
            LogOutput('info', "Set Interface auth key - passed")
        else:
            LogOutput('error', "Set Interface auth key - failed")

    def test_unsetInterfaceAuthKey(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceAuthKey(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface auth key - passed")
        else:
            LogOutput('error', "Unset Interface auth key - failed")

    def test_setInterfaceMd5Key(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceMd5Key(dut01Obj, "1", "md5victory")
        if(retValue):
            LogOutput('info', "Set Interface md5 key - passed")
        else:
            LogOutput('error', "Set Interface md5 key - failed")

    def test_unsetInterfaceMd5Key(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceMd5Key(dut01Obj, "1")
        if(retValue):
            LogOutput('info', "Unset Interface md5 key - passed")
        else:
            LogOutput('error', "Unset Interface md5 key - failed")

    def test_setRouteRedistribution(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setRouteRedistribution(dut01Obj)
        if(retValue):
            LogOutput('info', "set Route Redistribution - passed")
        else:
            LogOutput('error', "set Route Redistribution - failed")

    def test_unsetRouteRedistribution(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetRouteRedistribution(dut01Obj)
        if(retValue):
            LogOutput('info', "unset Route Redistribution - passed")
        else:
            LogOutput('error', "unset Route Redistribution - failed")

    def test_setDefaultRouteRedistribution(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setDefaultRouteRedistribution(dut01Obj)
        if(retValue):
            LogOutput('info', "set Default Route Redistribution - passed")
        else:
            LogOutput('error', "set Default Route Redistribution - failed")

    def test_unsetDefaultRouteRedistribution(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetDefaultRouteRedistribution(dut01Obj)
        if(retValue):
            LogOutput('info', "unset Default Route Redistribution - passed")
        else:
            LogOutput('error', "unset Default Route Redistribution - failed")

    def test_setInterfaceRetransmitInterval(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceRetransmitInterval(dut01Obj, "111")
        if(retValue):
            LogOutput('info', "Set Interface RetransmitInterval - passed")
        else:
            LogOutput('error', "Set Interface RetransmitInterval - failed")

    def test_unsetInterfaceRetransmitInterval(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceRetransmitInterval(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface RetransmitInterval - passed")
        else:
            LogOutput('error', "Unset Interface RetransmitInterval - failed")

    def test_setInterfaceTransmitDelay(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceTransmitDelay(dut01Obj, "123")
        if(retValue):
            LogOutput('info', "Set Interface TransmitDelay - passed")
        else:
            LogOutput('error', "Set Interface TransmitDelay - failed")

    def test_unsetInterfaceTransmitDelay(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceTransmitDelay(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface TransmitDelay - passed")
        else:
            LogOutput('error', "Unset Interface TransmitDelay - failed")

    def test_setInterfacePriority(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfacePriority(dut01Obj, "2")
        if(retValue):
            LogOutput('info', "Set Interface Priority - passed")
        else:
            LogOutput('error', "Set Interface Priority - failed")

    def test_unsetInterfacePriority(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfacePriority(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface Priority - passed")
        else:
            LogOutput('error', "Unset Interface Priority - failed")

    def test_setInterfaceCost(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceCost(dut01Obj, "222")
        if(retValue):
            LogOutput('info', "Set Interface Cost - passed")
        else:
            LogOutput('error', "Set Interface Cost - failed")

    def test_unsetInterfaceCost(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceCost(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface Cost - passed")
        else:
            LogOutput('error', "Unset Interface Cost - failed")

    def test_setInterfaceMtuIgnore(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceMtuIgnore(dut01Obj)
        if(retValue):
            LogOutput('info', "Set Interface MtuIgnore - passed")
        else:
            LogOutput('error', "Set Interface MtuIgnore - failed")

    def test_unsetInterfaceMtuIgnore(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceMtuIgnore(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface MtuIgnore - passed")
        else:
            LogOutput('error', "Unset Interface MtuIgnore - failed")

    def test_setInterfaceNetworkType(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setInterfaceNetworkType(dut01Obj, "point-to-point")
        if(retValue):
            LogOutput('info', "Set Interface NetworkType - passed")
        else:
            LogOutput('error', "Set Interface NetworkType - failed")

    def test_unsetInterfaceNetworkType(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetInterfaceNetworkType(dut01Obj)
        if(retValue):
            LogOutput('info', "Unset Interface NetworkType - passed")
        else:
            LogOutput('error', "Unset Interface NetworkType - failed")

    def test_setAreaVirtualLink(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setAreaVirtualLink(dut01Obj, "2", "10.10.10.10")
        if(retValue):
            LogOutput('info', "set Area virtual-link - passed")
        else:
            assert False, "set Area virtual-link - failed"

    def test_unsetAreaVirtualLink(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetAreaVirtualLink(dut01Obj, "2", "10.10.10.10")
        if(retValue):
            LogOutput('info', "unset Area virtual-link - passed")
        else:
            assert False, "unset Area virtual-link - failed"

    def test_setVirtualLinkHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkHelloIntervalTest(dut01Obj,
                                                   "2", "10.10.10.1", "5")
        if(retValue):
            LogOutput('info', "set Area virtual-link hello-interval - passed")
        else:
            assert False, "set Area virtual-link hello-interval - failed"

    def test_unsetVirtualLinkHelloIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkHelloIntervalTest(dut01Obj,
                                                     "2", "10.10.10.1")
        if(retValue):
            LogOutput('info',
                      "unset Area virtual-link hello-interval - passed")
        else:
            assert False, "unset Area virtual-link hello-interval - failed"

    def test_setVirtualLinkTransmitDelayTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkTransmitDelayTest(dut01Obj,
                                                   "2", "10.10.10.2", "5")
        if(retValue):
            LogOutput('info', "set Area virtual-link transmit-delay - passed")
        else:
            assert False, "set Area virtual-link transmit-delay - failed"

    def test_unsetVirtualLinkTransmitDelayTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkTransmitDelayTest(dut01Obj,
                                                     "2", "10.10.10.2")
        if(retValue):
            LogOutput('info',
                      "unset Area virtual-link transmit-delay - passed")
        else:
            assert False, "unset Area virtual-link transmit-delay - failed"

    def test_setVirtualLinkRetransmitIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkRetransmitIntervalTest(dut01Obj,
                                                        "2", "10.10.10.3", "4")
        if(retValue):
            LogOutput('info',
                      "set Area virtual-link retransmit-interval - passed")
        else:
            assert False, "set Area virtual-link retransmit-interval - failed"

    def test_unsetVirtualLinkRetransmitIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkRetransmitIntervalTest(dut01Obj,
                                                          "2", "10.10.10.3")
        if(retValue):
            LogOutput('info',
                      "unset Area virtual-link retransmit-interval - passed")
        else:
            assert False, \
                "unset Area virtual-link retransmit-interval - failed"

    def test_setVirtualLinkDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkDeadIntervalTest(dut01Obj,
                                                  "2", "10.10.10.4", "20")
        if(retValue):
            LogOutput('info', "set Area virtual-link dead-interval - passed")
        else:
            assert False, "set Area virtual-link dead-interval - failed"

    def test_unsetVirtualLinkDeadIntervalTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkDeadIntervalTest(dut01Obj,
                                                    "2", "10.10.10.4")
        if(retValue):
            LogOutput('info',
                      "unset Area virtual-link dead-interval - passed")
        else:
            assert False, "unset Area virtual-link dead-interval - failed"

    def test_setVirtualLinkAuthMsgDigestTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkAuthMsgDigestTest(dut01Obj,
                                                   "2", "10.10.10.5")
        if(retValue):
            LogOutput('info',
                      "set virtual-link authentication MD5 - passed")
        else:
            assert False, "set virtual-link authentication MD5 - failed"

    def test_unsetVirtualLinkAuthMsgDigestTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkAuthMsgDigestTest(dut01Obj,
                                                     "2", "10.10.10.5")
        if(retValue):
            LogOutput('info',
                      "unset virtual-link authentication MD5 - passed")
        else:
            assert False, \
                "unset virtual-link authentication MD5 - failed"

    def test_setVirtualLinkAuthKeyTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setVirtualLinkAuthKeyTest(dut01Obj,
                                             "2", "10.10.10.6", "ospf")
        if(retValue):
            LogOutput('info', "set virtual-link authentication-key - passed")
        else:
            assert False, "set virtual-link authentication-key - failed"

    def test_unsetVirtualLinkAuthKeyTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkAuthKeyTest(dut01Obj, "2", "10.10.10.6")
        if(retValue):
            LogOutput('info',
                      "unset virtual-link authentication-key - passed")
        else:
            assert False, "unset virtual-link authentication-key- failed"

    def test_setVirtualLinkMsgDigestKeyTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = \
            setVirtualLinkMsgDigestKeyTest(dut01Obj,
                                           "2", "10.10.10.7", "3", "ospf")
        if(retValue):
            LogOutput('info', "set virtual-link message-digest-key - passed")
        else:
            assert False, "set virtual-link message-digest-key- failed"

    def test_unsetVirtualLinkMsgDigestKeyTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = unsetVirtualLinkMsgDigestKeyTest(dut01Obj,
                                                    "2", "10.10.10.7", "3")
        if(retValue):
            LogOutput('info',
                      "unset virtual-link message-digest-key - passed")
        else:
            assert False, "unset virtual-link message-digest-key- failed"
