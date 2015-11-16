#!/usr/bin/python

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
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

# Topology definition
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}


def enterVtyshShell(dut01):
    retStruct = dut01.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    return True


def exitContext(dut01):

    retStruct = dut01.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

    return True


def setTracerouteIpTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute target ip failed"

    return True


def setTracerouteHostTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute localhost")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target host failed"

    return True


def setTracerouteIpMaxttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 maxttl 30")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip maxttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute \
target ip maxttl failed"

    return True


def setTracerouteHostMaxttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute \
localhost maxttl 30")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host maxttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host maxttl failed"

    return True


def setTracerouteIpMinttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 minttl 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip minttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute target \
ip minttl failed"

    return True


def setTracerouteHostMinttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute localhost \
minttl 1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host minttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host minttl failed"

    return True


def setTracerouteHostDstportTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute localhost \
dstport 33434")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host dstport failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host dstport failed"

    return True


def setTracerouteIpDstportTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 \
dstport 33434")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip dstport failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute \
target ip dstport failed"

    return True


def setTracerouteHostProbesTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute \
localhost probes 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host probes failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host probes failed"

    return True


def setTracerouteIpProbesTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 probes 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip probes failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute target \
ip probes failed"

    return True


def setTracerouteHostTimeoutTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute \
localhost timeout 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host timeout failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host timeout failed"

    return True


def setTracerouteIpTimeoutTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 timeout 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip timeout failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute target \
ip timeout failed"

    return True


def setTracerouteIpOptionsTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute 1.1.1.1 \
ip-option loosesourceroute 1.1.1.8")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target ip ip-options failed"
    cmdOut = devIntReturn.get('buffer')
    assert '1.1.1.1' in cmdOut, "Test to set traceroute target \
ip ip-options failed"

    return True


def setTracerouteHostIpOptionsTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute localhost \
ip-option loosesourceroute 1.1.1.8")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute target host ip-options failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute target \
host ip-options failed"

    return True


def setTraceroute6IpTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 0:0::0:1")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target ip failed"
    cmdOut = devIntReturn.get('buffer')
    assert '0:0::0:1' in cmdOut, "Test to set traceroute6 target ip failed"

    return True


def setTraceroute6HostTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 localhost")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target6 host failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute6 target host failed"

    return True


def setTraceroute6IpMaxttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 0:0::0:1 \
maxttl 30")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target ip maxttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert '0:0::0:1' in cmdOut, "Test to set traceroute6 target \
ip maxttl failed"

    return True


def setTraceroute6HostMaxttlTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 localhost \
maxttl 30")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target hostname \
maxttl failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute6 target hostname \
maxttl failed"

    return True


def setTraceroute6IpTimeoutTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 0:0::0:1 \
timeout 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target ip timeout failed"
    cmdOut = devIntReturn.get('buffer')
    assert '0:0::0:1' in cmdOut, "Test to set traceroute6 target \
ip timeout failed"

    return True


def setTraceroute6HostTimeoutTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 localhost \
timeout 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target hostname \
timeout failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute6 target \
hostname timeout failed"

    return True


def setTraceroute6IpProbesTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 0:0::0:1 \
probes 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target ip probes failed"
    cmdOut = devIntReturn.get('buffer')
    assert '0:0::0:1' in cmdOut, "Test to set traceroute6 target \
ip probes failed"

    return True


def setTraceroute6HostProbesTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 localhost \
probes 3")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target hostname probes failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute6 target \
hostname probes failed"

    return True


def setTraceroute6IpDstportTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 0:0::0:1 \
dstport 33434")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target ip dstport failed"
    cmdOut = devIntReturn.get('buffer')
    assert '0:0::0:1' in cmdOut, "Test to set traceroute6 target \
ip dstport failed"

    return True


def setTraceroute6HostDstportTest(dut01):
    if (enterVtyshShell(dut01) is False):
        return False

    devIntReturn = dut01.DeviceInteract(command="traceroute6 localhost \
dstport 33434")
    retCode = devIntReturn.get('returnCode')
    assert retCode == 0, "Test to set traceroute6 target hostname \
dstport failed"
    cmdOut = devIntReturn.get('buffer')
    assert 'localhost' in cmdOut, "Test to set traceroute6 target \
hostname dstport failed"

    return True


class Test_ospf_configuration:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_ospf_configuration.testObj = testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_ospf_configuration.topoObj = \
            Test_ospf_configuration.testObj.topoObjGet()

    def teardown_class(cls):
        Test_ospf_configuration.topoObj.terminate_nodes()

    def test_setTracerouteIpTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address - passed")
        else:
            LogOutput('error', "Set traceroute ip-address - failed")

    def test_setTracerouteHostTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host - passed")
        else:
            LogOutput('error', "Set traceroute Host - failed")

    def test_setTracerouteIpMaxttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpMaxttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address maxttl - passed")
        else:
            LogOutput('error', "Set traceroute ip-address maxttl - failed")

    def test_setTracerouteHostMaxttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostMaxttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host maxttl - passed")
        else:
            LogOutput('error', "Set traceroute Host maxttl - failed")

    def test_setTracerouteIpMinttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpMinttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address minttl - passed")
        else:
            LogOutput('error', "Set traceroute ip-address minttl - failed")

    def test_setTracerouteHostMinttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostMinttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host minttl - passed")
        else:
            LogOutput('error', "Set traceroute Host minttl - failed")

    def test_setTracerouteIpDstportTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpDstportTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address Dstport - passed")
        else:
            LogOutput('error', "Set traceroute ip-address Dstport - failed")

    def test_setTracerouteHostDstportTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostDstportTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host Dstport - passed")
        else:
            LogOutput('error', "Set traceroute Host Dstport - failed")

    def test_setTracerouteIpTimeoutTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpTimeoutTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address timeout - passed")
        else:
            LogOutput('error', "Set traceroute ip-address timeout - failed")

    def test_setTracerouteHostTimeoutTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostTimeoutTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host timeout - passed")
        else:
            LogOutput('error', "Set traceroute Host timeout - failed")

    def test_setTracerouteIpProbesTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpProbesTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address probes - passed")
        else:
            LogOutput('error', "Set traceroute ip-address probes - failed")

    def test_setTracerouteHostProbesTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostProbesTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host probes - passed")
        else:
            LogOutput('error', "Set traceroute Host probes - failed")

    def test_setTracerouteIpOptionsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteIpOptionsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute ip-address \
ip-option loosesource route - passed")
        else:
            LogOutput('error', "Set traceroute ip-address \
ip-option loosesource route - failed")

    def test_setTracerouteHostIpOptionsTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTracerouteHostIpOptionsTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute Host \
ip-option loosesource route - passed")
        else:
            LogOutput('error', "Set traceroute Host \
ip-option loosesource route - failed")

    def test_setTraceroute6IpTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6IpTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 ip-address - passed")
        else:
            LogOutput('error', "Set traceroute6 ip-address - failed")

    def test_setTraceroute6HostTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6HostTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 Host - passed")
        else:
            LogOutput('error', "Set traceroute6 Host - failed")

    def test_setTraceroute6IpMaxttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6IpMaxttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 ip-address maxttl - passed")
        else:
            LogOutput('error', "Set traceroute6 ip-address maxttl - failed")

    def test_setTraceroute6HostMaxttlTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6HostMaxttlTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 Host maxttl - passed")
        else:
            LogOutput('error', "Set traceroute6 Host maxttl - failed")

    def test_setTraceroute6IpDstportTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6IpDstportTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 ip-address Dstport - passed")
        else:
            LogOutput('error', "Set traceroute6 ip-address Dstport - failed")

    def test_setTraceroute6HostDstportTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6HostDstportTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 Host Dstport - passed")
        else:
            LogOutput('error', "Set traceroute6 Host Dstport - failed")

    def test_setTraceroute6IpTimeoutTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6IpTimeoutTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 ip-address timeout - passed")
        else:
            LogOutput('error', "Set traceroute6 ip-address timeout - failed")

    def test_setTraceroute6HostTimeoutTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6HostTimeoutTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 Host timeout - passed")
        else:
            LogOutput('error', "Set traceroute6 Host timeout - failed")

    def test_setTraceroute6IpProbesTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6IpProbesTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 ip-address probes - passed")
        else:
            LogOutput('error', "Set traceroute6 ip-address probes - failed")

    def test_setTraceroute6HostProbesTest(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = setTraceroute6HostProbesTest(dut01Obj)
        if(retValue):
            LogOutput('info', "Set traceroute6 Host probes - passed")
        else:
            LogOutput('error', "Set traceroute6 Host probes - failed")
