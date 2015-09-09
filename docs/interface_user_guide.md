Physical Interface
===================
<!--Provide the title of the feature-->

### Index ###
 [Overview](#intfover)<br>
 [Configuring physical interface](#intfconf)<br>
 &nbsp;&nbsp;&nbsp;[Basic configuration](#intfconfbasic)<br>
 &nbsp;&nbsp;&nbsp;[Optional configuration](#intfconfopt)<br>
 &nbsp;&nbsp;&nbsp;[Verifying the configuration](#intfconfver)<br>
 [Troubleshooting the configuration](#intftrouble)<br>
 [CLI](#intfcli)<br>
 [Related features](#intfrelated)<br>


## Overview <a id="intfover"></a> ##
This guide provides detail for managing and monitoring physical interface present the switch. All configurations work in interface context. There are many types of interface depending on which the speed and other configurations take effect, but that is outside the scope of this document. When interface is up then all the default configuration will take effect, follow the optional configuration for changin the default configuration.
## Configuring physical interface <a id="intfconf"></a> ##

###Basic configuration <a id="intfconfbasic"></a> ###

#####1. Change to interface context
'interface IFNAME' command changes the vtysh context to interface. IFNAME here depicts the name of the interface.

> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)#
#####2. Enable interface
'no shutdown' command enables particular interface on the switch. Once interface is enabled, all other configuration will take effect.

> ops-as5712(config-if)# no shutdown<br>
ops-as5712(config-if)#
#####3. Setting interface speed
The 'speed' command sets the interface speed. Supported speeds are 1Gbps, 10 Gbps and 40 Gbps. But depending on the interface type these configurations may and may not take effect.

> ops-as5712(config-if)# speed 1000<br>
ops-as5712(config-if)#

The 'no speed' command reverts the interface speed to auto mode.
> ops-as5712(config-if)# no speed<br>
ops-as5712(config-if)#
#####4. Setting interface duplexity
The 'duplex' command sets duplexity of the interface to either half or full duplex.
> ops-as5712(config-if)# duplex half<br>
ops-as5712(config-if)#

The 'no duplex' commands reverts the duplexity of the interface to default full.
> ops-as5712(config-if)# no duplex<br>
ops-as5712(config-if)#

#####5. Setting interface MTU
The 'mtu' command sets the MTU(maximum transmission unit) of the interface between 576 and 16360 bytes.
> ops-as5712(config)# mtu 2000<br>
ops-as5712(config)#

The 'no mtu' commands reverts the mtu of the interface to default auto mode.
> ops-as5712(config-if)# no mtu<br>
ops-as5712(config-if)#
#####6. Selecting interface autonegotiation state
The 'autonegotiation' command turns the autonegotiation state on or off. The 'no autonegotiation' command sets the autonegotiation state to default.
> ops-as5712(config-if)# autonegotiation on<br>
ops-as5712(config-if)# <br>
ops-as5712(config-if)# no autonegotiation<br>
ops-as5712(config-if)#
#####7. Setting flowcontrol
The 'flowcontrol' command enables or disables the flow control mechanism (pause frame technique). Where as the 'no flowcontrol' command disables the flow control mechanism. This command is executed for receiving and sending pause frames individually.
> ops-as5712(config-if)# flowcontrol receive on<br>
ops-as5712(config-if)# flowcontrol send on<br>

> ops-as5712(config-if)#<br>
ops-as5712(config-if)# no flowcontrol receive<br>
ops-as5712(config-if)# no flowcontrol send on<br>

###Optional configuration <a id="intfconfopt"></a> ###
#####1. Setting interface description
The 'description' command associate a description to an interface
> ops-as5712(config-if)# description This is interface 1<br>
ops-as5712(config-if)#

#####2. Configuring interface as L2/L3
By default all the interfaces are configured as L3, if any interface is not configured as L3 'routing' command can be used to set the interface to L3.
> ops-as5712(config-if)# routing<br>
ops-as5712(config-if)#

To configure the interface as L2 'no routing' command is used.
> ops-as5712(config-if)# no routing<br>
ops-as5712(config-if)#
#####2. Setting IP address of the interface
'ip address' and 'ipv6 address' commands set ip address of the interface. These two commands will work only if the interface is configured as L3.
> ops-as5712(config-if)# ip address 10.10.10.2/24 <br>
ops-as5712(config-if)#
ops-as5712(config-if)#  ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24

To set secondary ip address append 'secondary' keyword at the end as below:-
> ops-as5712(config-if)# ip address 10.10.10.2/24 secondary<br>
ops-as5712(config-if)#
ops-as5712(config-if)#  ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24 secondary

To remove ip/ipv6 address of the interface use 'no ip address' and 'no ipv6 address' commands.
> ops-as5712(config-if)# no ip address 10.10.10.2/24 <br>
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# no ip address 10.10.10.2/24 secondary<br>
ops-as5712(config-if)#  no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24 secondary
###Verifying the configuration <a id="intfconfver"></a> ###
#####1. Viewing interface information
The 'show interface' and 'show interface brief' commands display information about state and configuration of all the interfaces. Information includes speed, mtu, packet counts etc.

> ops-as5712# show interface

> Interface 45 is down (Administratively down)<br>
>  Admin state is down<br>
>  Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4<br>
>  MTU 9388 <br>
>  Half-duplex <br>
>  Speed 0 Mb/s <br>
>  Auto-Negotiation is turned on<br>
>  Input flow-control is on, output flow-control is on<br>
>  RX<br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input packets&nbsp;&nbsp;&nbsp;0 bytes  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 dropped  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 short frame&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 overrun  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 CRC/FCS  <br>
>  TX<br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 output packets&nbsp;&nbsp;&nbsp;0 bytes  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;4 dropped  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 collision  <br>
>
> Interface 36 is down (Administratively down) <br>
>  Admin state is down<br>
>  Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4 <br>
>  MTU 9388 <br>
>  Half-duplex <br>
>  Speed 0 Mb/s <br>
>  Auto-Negotiation is turned on <br>
>  Input flow-control is on, output flow-control is on<br>
>  RX<br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input packets&nbsp;&nbsp;&nbsp;0 bytes  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 dropped  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 short frame&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 overrun  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 CRC/FCS  <br>
>  TX<br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 output packets&nbsp;&nbsp;&nbsp;0 bytes  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;4 dropped  <br>
>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 collision  <br>
> ......... <br>
> ......... <br>
ops-as5712# show interface brief
..........................................................................................................................<br>
Ethernet&nbsp;&nbsp;&nbsp;&nbsp;VLAN&nbsp;&nbsp;&nbsp;Type&nbsp;&nbsp;&nbsp;Mode&nbsp;&nbsp;&nbsp;&nbsp;Status&nbsp;&nbsp;&nbsp;Reason&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Speed&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Port<br>
Interface&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Mb/s)&nbsp;&nbsp;&nbsp;&nbsp;Ch#
...........................................................................................................................<br>
 45&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;eth&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;down&nbsp;&nbsp;&nbsp;&nbsp;Administratively down&nbsp;&nbsp;&nbsp;&nbsp;auto&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..<br>
 36&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;eth&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;down&nbsp;&nbsp;&nbsp;&nbsp;Administratively down&nbsp;&nbsp;&nbsp;&nbsp;auto&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..<br>
 9&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;eth&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;down&nbsp;&nbsp;&nbsp;&nbsp;Administratively down&nbsp;&nbsp;&nbsp;&nbsp;auto&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..<br>
...............<br>
...............<br>

To view information for a particular interface use 'show interface IFNAME' and 'show interface IFNAME brief' commands, where IFNAME is the name of the interface.
> ops-as5712# show interface 1
>
Interface 1 is up <br>
 Admin state is up<br>
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4<br>
 MTU 1500 <br>
 Full-duplex <br>
 Speed 1000 Mb/s <br>
 Auto-Negotiation is turned on <br>
 Input flow-control is off, output flow-control is off<br>
 RX<br>c
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;50 input packets&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;14462 bytes  <br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;7 dropped  <br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 short frame&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 overrun  <br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 CRC/FCS  <br>
 TX<br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;213 output packets&nbsp;&nbsp;34506 bytes  <br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 input error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;4 dropped  <br>
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 collision  <br>
ops-as5712# show interface 1 brief<br>
..........................................................................................................................<br>
Ethernet&nbsp;&nbsp;&nbsp;&nbsp;VLAN&nbsp;&nbsp;&nbsp;Type&nbsp;&nbsp;&nbsp;Mode&nbsp;&nbsp;&nbsp;&nbsp;Status&nbsp;&nbsp;&nbsp;Reason&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Speed&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Port<br>
Interface&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Mb/s)&nbsp;&nbsp;&nbsp;&nbsp;Ch#
...........................................................................................................................<br>
  1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;eth&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;down&nbsp;&nbsp;&nbsp;&nbsp;Administratively down&nbsp;&nbsp;&nbsp;&nbsp;auto&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;..<br>
#####2. Viewing interface show running configuration
The 'show running-config interface' and 'show running-config interface IFNAME' commands are used to see a snapshot of running configuration for all interfaces and if used with interface name (IFNAME) displays running configuration only for a particular interface.
> ops-as5712# show running-config interface<br>
Interface 2 <br>
&nbsp;&nbsp;&nbsp;no shutdown <br>
&nbsp;&nbsp;&nbsp;speed 40000 <br>
&nbsp;&nbsp;&nbsp;autonegotiation on <br>
&nbsp;&nbsp;&nbsp;exit<br>
Interface 1 <br>
&nbsp;&nbsp;&nbsp;no shutdown <br>
&nbsp;&nbsp;&nbsp;exit<br>
.............<br>
.............<br>
> ops-as5712# show running-config interface 2<br>
Interface 2 <br>
&nbsp;&nbsp;&nbsp;no shutdown <br>
&nbsp;&nbsp;&nbsp;speed 40000 <br>
&nbsp;&nbsp;&nbsp;autonegotiation on <br>
&nbsp;&nbsp;&nbsp;exit<br>

###Troubleshooting the configuration <a id="intftrouble"></a> ###

| Condition                                                              | Cause                                                                           | Remedy                                                                                                                                                           |
|:------------------------------------------------------------------------|:---------------------------------------------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Unable to set ip/ipv6 address even after enabling interface | Interface may be configured as L2 | configure interface as L3 using 'routing' command. Please refer command reference |
## CLI <a id="intfcli"></a> ##
<!--Provide a link to the CLI command related to the feature. The CLI files will be generated to a CLI directory.  -->
Click [here](https://openswitch.net/cli_feat.html#cli_command_anchor) for the CLI commands related to the Physical interface.
## Related features <a id="intfrelated"></a> ##
<!-- Enter content into this section to describe features that may need to be considered in relation to this particular feature, under what conditions and why.  Provide a hyperlink to each related feature.  Sample text is included below as a potential example or starting point.  -->
No related features.