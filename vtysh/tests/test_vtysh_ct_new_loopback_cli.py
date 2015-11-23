import time
import pytest
import re
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *

# Topology definition
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01 wrkston01",
            "topoLinks": "lnk01:dut01:wrkston01",
            "topoFilters": "dut01:system-category:switch,\
                            wrkston01:system-category:workstation"}


def loopback_cli(**kwargs):
    device1 = kwargs.get('device1', None)
    device2 = kwargs.get('device2', None)

# creating a loopback interface and verifying in interface and porttable

    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", config=True)
    LogOutput('info', "### Verify the Loopback  interface is created with'\
             ' same name for L3 port ###")

    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get interface lo1 name")
    retCode = devIntReturn.get('buffer')
    assert "lo1"in retCode, "Failed to retrieve ovs-vsctl command"
    LogOutput('info', "### lo1 created successfully in intf row  ###")

    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port lo1 name")
    retCode = devIntReturn.get('buffer')
    assert "lo1"in retCode, "Failed to retrieve ovs-vsctl command"
    LogOutput('info', "### lo1 created successfully in port row ###")

# assigning ipv4 and verifying whether ip assigned
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", addr="192.168.1.5",
                                        mask=24, config=True)
    devIntReturn = device1.DeviceInteract(command="ovs-vsctl get\
                                                   port lo1 ip4_address ")
    retCode = devIntReturn.get('buffer')
    assert '192.168.1.5/24' in retCode, \
           'Test to verify Loopback-interface configuration clis - FAILED!'

# verifying dupplicate ip
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="2", addr="192.168.1.5",
                                        mask=24, config=True)
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port lo1 ip4_address")
    retCode = devIntReturn.get('buffer')
    print retCode
    if 'Duplicate IP Address.' in retCode:
        LogOutput('info', "IPV4 already assigned !")

# assigning and verifying ipv6
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", addr="10:10::10:10",
                                        ipv6flag=True, mask=24, config=True)
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get port lo1 ip6_address")
    retCode = devIntReturn.get('buffer')
    assert '10:10::10:10/24' in retCode, \
           'Test to verify Loopback-interface configuration clis - FAILED!'

# unassigning the ipv4 and verifying whether it is unassinged
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", addr="192.168.1.5",
                                        mask=24, config=False)
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get interface\
                                           lo1 ip4_address")
    retCode = devIntReturn.get('buffer').splitlines()
    if len(retCode) == 2:
        retCode = retCode[1]
    if " " in retCode:
        LogOutput('info', "### IPV4 unassigend for lo1! ###")

# Deleting the loopback interface
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", config=False)
    devIntReturn = device1.DeviceInteract(command=
                                          "ovs-vsctl get interface lo1 name")
    retCode = devIntReturn.get('buffer').splitlines()
    if len(retCode) == 3:
        retCode = retCode[1]
    if "ovs-vsctl: no row \"lo1\" in table Interface" in retCode:
        LogOutput('info', "### interface lo1 deleted ! ###")

# assigning invalid ip
    retStruct = LoopbackInterfaceEnable(deviceObj=device1,
                                        loopback="1", addr="255.255.255.255",
                                        mask=24, config=True)
    retCode = devIntReturn.get('buffer')
    if '% Unknown command.' in retCode:
        LogOutput('info', "Cannot create the loopback interface - FAILED!")

    devIntReturn = device1.DeviceInteract(command="vtysh")
    devIntReturn = device1.DeviceInteract(command="conf t")
    devIntReturn = device1.DeviceInteract(command="int loopback 21474836501")
    retCode = devIntReturn.get('buffer')
    if '% Unknown command.' in retCode:
        LogOutput('info', "Cannot create the loopback interface - FAILED!")

    devIntReturn = device1.DeviceInteract(command="no int loopback 1")
    devIntReturn = device1.DeviceInteract(command="no int loopback 1")
    retCode = devIntReturn.get('buffer')
    if 'Loopback interface does not exist.' in retCode:
        LogOutput('info', "Cannot create the loopback interface - FAILED!")

    devIntReturn = device1.DeviceInteract(command="int loopback 0")
    retCode = devIntReturn.get('buffer')
    if '% Unknown command.' in retCode:
        LogOutput('info', "Cannot create the loopback interface - FAILED!")

    devIntReturn = device1.DeviceInteract(command="no int loopback 6")
    retCode = devIntReturn.get('buffer')
    if 'Interface does not exist' in retCode:
        LogOutput('info',
                  "trying to delete a non existing loopback intf - FAILED!")
    devIntReturn = device1.DeviceInteract(command="exit")
    devIntReturn = device1.DeviceInteract(command="exit")


class Test_loopback_cli:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_loopback_cli.testObj = testEnviron(topoDict=topoDict)
        # Get topology object
        Test_loopback_cli.topoObj = Test_loopback_cli.testObj.topoObjGet()

    def teardown_class(cls):
        Test_loopback_cli.topoObj.terminate_nodes()

    def test_subintf_cli(self):
        LogOutput('info', "**configuring**")
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        wrkston1Obj = self.topoObj.deviceObjGet(device="wrkston01")
        retValue = loopback_cli(device1=dut01Obj, device2=wrkston1Obj)
