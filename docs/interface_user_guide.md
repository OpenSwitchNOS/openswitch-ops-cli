Physical Interface
===================
## Contents

[TOC]
## Overview <a id="intfover"></a> ##
This guide provides detail for managing and monitoring the physical interface present in the switch. All configurations work in interface context. When the interface is running, all the default configurations take effect. To change the default configuration, see [Setting up the basic configuration](#intfconfopt).
## Configuring the physical interface <a id="intfconf"></a> ##
###Setting up the basic configuration <a id="intfconfbasic"></a> ###
1. Change to the interface context.
The 'interface *interface*' command changes the vtysh context to interface. The *interface* in the command depicts the name of the interface, which is replaced with interface "1" in the following example.
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)#
```

2. Enable the interface.
The 'no shutdown' command enables a particular interface on the switch. Once the interface is enabled, all other configurations take effect.
```
ops-as5712(config-if)# no shutdown
ops-as5712(config-if)#
```

3. Set the interface speed.
The 'speed' command sets the interface speed. Supported speeds are 1Gbps, 10 Gbps and 40 Gbps. Depending upon the interface type, these configurations may or may not take effect.
```
ops-as5712(config-if)# speed 1000
ops-as5712(config-if)#
```
The 'no speed' command reverts the interface speed to auto mode.
```
ops-as5712(config-if)# no speed
ops-as5712(config-if)#
```

4. Set the interface duplexity.
The ‘duplex’ command sets the interface duplexity to either half or full duplex.
```
ops-as5712(config-if)# duplex half
ops-as5712(config-if)#
```
The ‘no duplex’ commands reverts the interface duplexity to default 'full'.
```
ops-as5712(config-if)# no duplex
ops-as5712(config-if)#
```

5. Set the interface MTU.
The ‘mtu’ command sets the interface MTU (maximum transmission unit) to between 576 bytes and 16360 bytes.
```
ops-as5712(config)# mtu 2000
ops-as5712(config)#
```
The 'no mtu' commands reverts the mtu of the interface to default auto mode.
```
ops-as5712(config-if)# no mtu
ops-as5712(config-if)#
```

6. Select the interface autonegotiation state.
The 'autonegotiation' command turns the autonegotiation state on or off. The 'no autonegotiation' command sets the autonegotiation state to default.
```
ops-as5712(config-if)# autonegotiation on
ops-as5712(config-if)#
ops-as5712(config-if)# no autonegotiation
ops-as5712(config-if)#
```

7. Set the flowcontrol.
The ‘flowcontrol’ command enables the flow control mechanism (pause frame technique). The ‘no flowcontrol’ command disables the flow control mechanism. This command is executed to receive and send pause frames individually.
```
ops-as5712(config-if)# flowcontrol receive on
ops-as5712(config-if)# flowcontrol send on
ops-as5712(config-if)#
ops-as5712(config-if)# no flowcontrol receive
ops-as5712(config-if)# no flowcontrol send on
```

###Setting up optional configurations <a id="intfconfopt"></a> ###
1. Set up interface description.
The ‘description’ command associates a description with an interface
```
ops-as5712(config-if)# description This is interface 1
ops-as5712(config-if)#
```

2. Configure the interface as L2 or L3.
By default all interfaces are configured as L3. If an interface is not configured as L3, the ‘routing’ command can be used to set the interface to L3.
```
ops-as5712(config-if)# routing
ops-as5712(config-if)#
```
To configure the interface as L2, the ‘no routing’ command is used.
```
ops-as5712(config-if)# no routing
ops-as5712(config-if)#
```

3. Set the IP address of the interface.
The ‘ip address’ and ‘ipv6 address’ commands set the ip address of the interface. These two commands work only if the interface is configured as L3.
```
ops-as5712(config-if)# ip address 10.10.10.2/24
ops-as5712(config-if)#
ops-as5712(config-if)#  ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
```
To set a secondary ip address, append the ‘secondary’ keyword at the end as shown below:
```
ops-as5712(config-if)# ip address 10.10.10.2/24 secondary
ops-as5712(config-if)#
ops-as5712(config-if)#  ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24 secondary
```
To remove the ipv4/ipv6 address of the interface, use the ‘no ip address’ and the ‘no ipv6 address’ commands.
```
ops-as5712(config-if)# no ip address 10.10.10.2/24
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# no ip address 10.10.10.2/24 secondary
ops-as5712(config-if)#  no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24 secondary
```

###Verifying the configuration <a id="intfconfver"></a> ###
#####Viewing interface information
The 'show interface' and 'show interface brief' commands display information about state and configuration of all the interfaces. The information includes details on speed, mtu, packet counts and so on.
```
ops-as5712# show interface

Interface 45 is down (Administratively down)
 Admin state is down
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 MTU 9388
 Half-duplex
 Speed 0 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is on, output flow-control is on
 RX
      0 input packets   0 bytes
      0 input error     0 dropped
      0 short frame     0 overrun
      0 CRC/FCS
 TX
      0 output packets   0 bytes
      0 input error      4 dropped
      0 collision

Interface 36 is down (Administratively down)
 Admin state is down
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 MTU 9388
 Half-duplex
 Speed 0 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is on, output flow-control is on
 RX
      0 input packets   0 bytes
      0 input error     0 dropped
      0 short frame     0 overrun
      0 CRC/FCS
 TX
      0 output packets   0 bytes
      0 input error      4 dropped
      0 collision
.........
.........
ops-as5712# show interface brief
....................................................................................................
Ethernet    VLAN   Type   Mode    Status   Reason                   Speed     Port
Interface                                                           (Mb/s)    Ch#
....................................................................................................
 45          ..   eth     ..       down    Administratively down    auto      ..
 36          ..   eth     ..       down    Administratively down    auto      ..
 9           ..   eth     ..       down    Administratively down    auto      ..
...............
...............
```

To view information for a particular interface use the ‘show interface *interface*’ or ‘show interface *interface* brief’ commands, where *interface* is the name of the interface, which is replaced with interface "1" in the following example.
```
ops-as5712# show interface 1

Interface 1 is up
 Admin state is up
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 MTU 1500
 Full-duplex
 Speed 1000 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is off, output flow-control is off
 RXc
      50 input packets     14462 bytes
      0 input error        7 dropped
      0 short frame        0 overrun
      0 CRC/FCS
 TX
      213 output packets  34506 bytes
      0 input error       4 dropped
      0 collision
ops-as5712# show interface 1 brief
....................................................................................................
Ethernet    VLAN   Type   Mode    Status     Reason                Speed     Port
Interface                                                          (Mb/s)    Ch#
....................................................................................................
  1          ..    eth     ..      down    Administratively down    auto     ..
```
##### Viewing snapshot of active configurations.
The ‘show running-config interface’ and ‘show running-config interface *interface*’ commands are used to see a snapshot of active configurations for all interfaces and if used with the interface name (*interface*), active configurations for a particular interface is displayed.
```
ops-as5712# show running-config interface
Interface 2
   no shutdown
   speed 40000
   autonegotiation on
   exit
Interface 1
   no shutdown
   exit
.............
.............
ops-as5712# show running-config interface 2
Interface 2
   no shutdown
   speed 40000
   autonegotiation on
   exit
```

###Troubleshooting the configuration <a id="intftrouble"></a> ###
#### Condition
Unable to set the ipv4/ipv6 address even after enabling the interface.
#### Cause
Interface may be configured as L2
#### Remedy
Configure interface as L3 using 'routing' command. Please refer command reference.
## CLI <a id="intfcli"></a> ##
Click [here](https://openswitch.net/cli_feat.html#cli_command_anchor) for the CLI commands related to the Physical interface.
## Related features <a id="intfrelated"></a> ##
No related features.
