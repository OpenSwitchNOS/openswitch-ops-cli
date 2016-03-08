#!/usr/bin/env python
# (c) Copyright [2015-2016] Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.


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


# Reboot switch
def switch_reboot(dut01):
    LogOutput('info', "Reboot switch")
    dut01.Reboot()
    rebootRetStruct = returnStruct(returnCode=0)
    return rebootRetStruct


def config_router_id_through_cli(**kwargs):
    device1 = kwargs.get('device1', None)

# Verify to configure system router-id through CLI without input
    LogOutput('info', "\n\n######## Test to verify configure system "
              "router-id through CLI without input ########")
    returnStructure = device1.DeviceInteract(command="router-id")
    bufferout = returnStructure.get('buffer')
    assert '% Command incomplete.' not in bufferout, "Test to configure \
           router-id through CLI without input has failed"
    retCode = returnStructure.get('returnCode')
    LogOutput('info', "### Successfully verified configuring "
              "router-id through CLI without input ###")

# Verify to configure system router-id through CLI
    LogOutput('info', "\n\n######## Test to verify configure system "
              "router-id through CLI ########")
    returnStructure = device1.DeviceInteract(command="router-id 1.1.1.1")
    returnStructure = device1.DeviceInteract(command="ovs-vsctl list system")
    bufferout = returnStructure.get('buffer')
    retCode = returnStructure.get('returnCode')
    assert 'router-id 1.1.1.1' not in bufferout, "Test to configure router-id \
           through CLI has failed"
    LogOutput('info', "### Successfully verified configuring "
              "router-id through CLI ###")

# Verify to display system router-id through CLI
    LogOutput('info', "\n\n######## Test to display configured system "
              "router-id through CLI ########")
    returnStructure = device1.DeviceInteract(command="show running-config")
    bufferout = returnStructure.get('buffer')
    retCode = returnStructure.get('returnCode')
    assert '1.1.1.1' not in bufferout, "Test to display \
           configured router-id through CLI without input has failed"
    LogOutput('info', "### Successfully verified displaying "
              "router-id through CLI ###")

# Verify to set system router-id through CLI which is not valid
    LogOutput('info', "\n\n######## Test to verify set invalid "
              "system router-id through CLI ########")
    returnStructure = device1.DeviceInteract(command="router-id 0.0.0.0")
    bufferout = returnStructure.get('buffer')
    assert 'Invalid router-id 0.0.0.0' not in bufferout, "Test to \
           set invalid router-id through CLI has failed"
    retCode = returnStructure.get('returnCode')
    LogOutput('info', "### Successfully verified set invalid "
              "router-id through CLI ###")

# Verify to reset configured system router-id through CLI

    LogOutput('info', "\n\n######## Test to verify reset configured "
              "system router-id through CLI ########")
    returnStructure = device1.DeviceInteract(command="no router-id")
    returnStructure = device1.DeviceInteract(command="ovs-vsctl list router-id")
    bufferout = returnStructure.get('buffer')
    retCode = returnStructure.get('returnCode')
    assert 'router-id ' not in bufferout, "Test to reset configured router-id \
           through CLI has failed"
    LogOutput('info', "### Successfully verified reset configured "
              "router-id through CLI ###")


class Test_config_router_id_through_cli:

    def setup_class(cls):
       # Test object will parse command line and formulate the env
        Test_config_router_id_through_cli.testObj = \
            testEnviron(topoDict=topoDict)
       #    Get topology object
        Test_config_router_id_through_cli.topoObj = \
            Test_config_router_id_through_cli.testObj.topoObjGet()

    def teardown_class(cls):
        Test_config_router_id_through_cli.topoObj.terminate_nodes()

    def test_config_router_id_through_cli(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = config_router_id_through_cli(device1=dut01Obj)
