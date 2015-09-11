Interface Commands
=======
<!--Provide the name of the grouping of commands, for example, LLDP commands-->

### Index ###
 [Interface Configuration Commands](#intfmain)<br>
 &nbsp;&nbsp;&nbsp;[Change to interface context](#intfcontext)<br>
 &nbsp;&nbsp;&nbsp;[Set Interface Description](#intfdesc)<br>
 &nbsp;&nbsp;&nbsp;[Removes interface description](#intfnodesc)<br>
 &nbsp;&nbsp;&nbsp;[Enables an interface](#intfen)<br>
 &nbsp;&nbsp;&nbsp;[Disable interface](#intfdis)<br>
 &nbsp;&nbsp;&nbsp;[Enable routing for an interface](#intfroute)<br>
 &nbsp;&nbsp;&nbsp;[Disable routing for interface](#intfnoroute)<br>
 &nbsp;&nbsp;&nbsp;[Set interface speed](#intfspeed)<br>
 &nbsp;&nbsp;&nbsp;[Set interface speed to default ](#intfnospeed)<br>
 &nbsp;&nbsp;&nbsp;[Set interface MTU](#intfmtu)<br>
 &nbsp;&nbsp;&nbsp;[Set interface MTU to default](#intfnomtu)<br>
 &nbsp;&nbsp;&nbsp;[Set duplexity of interface](#intfduplex)<br>
 &nbsp;&nbsp;&nbsp;[Set duplexity of interface to default](#intfnoduplex)<br>
 &nbsp;&nbsp;&nbsp;[Enable flow control](#intfflow)<br>
 &nbsp;&nbsp;&nbsp;[Set flowcontrol to default](#intfnoflow)<br>
 &nbsp;&nbsp;&nbsp;[Set autonegotiation state](#intfautonego)<br>
 &nbsp;&nbsp;&nbsp;[Set autonegotiation to default](#intfnoautonego)<br>
 &nbsp;&nbsp;&nbsp;[Set ipv4 address for interface](#intfip)<br>
 &nbsp;&nbsp;&nbsp;[Remove ipv4 address for interface](#intfnoip)<br>
 &nbsp;&nbsp;&nbsp;[Set ipv6 address for interface](#intfip6)<br>
 &nbsp;&nbsp;&nbsp;[Remove ipv6 address for interface](#intfnoip6)<br>
 [Interface Show Commands](#showintfmain)<br>
 &nbsp;&nbsp;&nbsp;[Show all interfaces](#showallintf)<br>
 &nbsp;&nbsp;&nbsp;[Show interface](#showintf)<br>
 &nbsp;&nbsp;&nbsp;[Show all interfaces running configuration](#showallintfrun)<br>
 &nbsp;&nbsp;&nbsp;[Show interface running configuration](#showintfrun)<br>

## Interface Configuration Commands <a id="intfmain"></a>
All interface configuration commands except "interface <interface>" works in interface context.
### 1. Change to interface context <a id="intfcontext"></a>
#### Syntax
	interface <interface>
#### Description ####
This command changes vtysh context to interface. Works in config context.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined |
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)#<br>
### 2. Set Interface Description <a id="intfdesc"></a>
#### Syntax
	description <description>
#### Description ####
This command sets the description for an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *description* | Required | - | string describing the interface |
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# description This is interface 1
### 3. Removes interface description <a id="intfnodesc"></a> ###
#### Syntax ####
    no description
#### Description ####
<!--Provide a description of the command. -->
This command removes description of an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no description
### 4. Enable interface <a id="intfen"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no shutdown
#### Description ####
<!--Provide a description of the command. -->
This command enables an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no shutdown
### 5. Disable interface <a id="intfdis"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    shutdown
#### Description ####
<!--Provide a description of the command. -->
This command enables an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# shutdown
### 6. Enable routing for interface <a id="intfroute"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    routing
#### Description ####
<!--Provide a description of the command. -->
This command enables routing for an interface making in L3 interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# routing
### 7. Disable routing for interface <a id="intfnoroute"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax
    no routing
#### Description
<!--Provide a description of the command. -->
This command disables routing for an interface.
#### Authority
All users.
#### Parameters
No parameters.
#### Examples
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no routing
### 8. Set interface speed <a id="intfspeed"></a>
#### Syntax ####
    speed (auto|1000|10000|100000|40000)
#### Description ####
<!--Provide a description of the command. -->
This command sets operating speed of an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **auto** | choose one| Literal | speed set to auto mode |
| **1000** | choose one| Literal | speed set to 1 Gbps |
| **10000** | choose one| Literal | speed set to 10 Gbps |
| **100000** | choose one| Literal | speed set to 100 Gbps |
| **40000** | choose one| Literal | speed set to 40 Gbps |
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# speed 10000
### 9. Set interface speed to default <a id="intfnospeed"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no speed
#### Description ####
<!--Provide a description of the command. -->
This command sets operating speed of an interface to default. The default is setting is 'auto'
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no speed
### 10. Set interface MTU <a id="intfmtu"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    mtu (auto|<value>)
#### Description ####
<!--Provide a description of the command. -->
This command sets MTU(maximum transmission unit) for an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **auto** | choose one| Literal | mtu set to auto mode |
| *value* | choose one| 576-16360 | mtu value between 576 to 16360 bytes
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# mtu auto<br>
ops-as5712(config-if)# mtu 580
### 11. Set interface MTU to default <a id="intfnomtu"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no mtu
#### Description ####
<!--Provide a description of the command. -->
This command sets MTU(maximum transmission unit) for an interface to default. The default setting is 'auto'.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no mtu
### 12. Set duplexity of interface <a id="intfduplex"></a> ###

#### Syntax ####
    duplex (half|full)
#### Description ####
<!--Provide a description of the command. -->
This command sets the duplexity of the interface from among half duplex and full duplex.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **half** | choose one| Literal |choose mode as half duplex |
| **full** | choose one| Literal  | choose mode as full duplex
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# duplex half
### 13. Set duplexity of interface to default <a id="intfnoduplex"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no duplex
#### Description ####
<!--Provide a description of the command. -->
This command sets the duplexity of the interface to it's default. The default mode is 'full'.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no duplex
### 14. Enable flow control <a id="intfflow"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    flowcontrol (receive|send) (off|on)
#### Description ####
<!--Provide a description of the command. -->
This command enables flow control, for sending and receiving pause frames.
#### Authority ####
All users.
#### Parameters ####
| Parameter 1| Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **receive** | choose one| Literal |select to status for receiving pause frames |
| **send** | choose one| Literal  | select to status for sending pause frames

| Parameter 2| Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **off** | choose one| Literal |Select this to switch off flow control for above parameter |
| **on** | choose one| Literal  | Select this to switch on flow control for above parameter
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# flowcontrol receive on
ops-as5712(config-if)# flowcontrol send on
### 15. Set flowcontrol to default <a id="intfnoflow"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no flowcontrol (receive|send)
#### Description ####
<!--Provide a description of the command. -->
This command sets flow control to default. The default is flow control 'off'.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **receive** | choose one| Literal |select to status for receiving pause frames |
| **send** | choose one| Literal  | select to status for sending pause frames
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no flowcontrol receive
ops-as5712(config-if)# no flowcontrol send
### 16. Set autonegotiation state <a id="intfautonego"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    autonegotiation (on|off)
#### Description ####
<!--Provide a description of the command. -->
This command sets autonegotiation state for the interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter| Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **off** | choose one| Literal | switch off auto negotiation |
| **on** | choose one| Literal  | switch on auto negotiation
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# autonegotiation on
### 17. Set autonegotiation to default <a id="intfnoautonego"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no autonegotiation
#### Description ####
<!--Provide a description of the command. -->
This command sets autonegotiation to state to default.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no autonegotiation
### 18. Set ipv4 address for interface <a id="intfip"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    ip address <ipv4_address/mask> [secondary]
#### Description ####
<!--Provide a description of the command. -->
This command sets ipv4 for the interface. This command works when interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address/mask* | Required | A.B.C.D/M | IPV4 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV4 address is secondary address
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# ip address 16.93.50.2/24
ops-as5712(config-if)# ip address 16.93.50.3/24 secondary

### 19. Remove ipv4 address for interface <a id="intfnoip"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no ip address <ipv4_address/mask> [secondary]
#### Description ####
<!--Provide a description of the command. -->
This command removes ipv4 address associated for the interface. This command works when interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address/mask* | Required | A.B.C.D/M | IPV4 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV4 address is secondary address
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no ip address 16.93.50.2/24
ops-as5712(config-if)# no ip address 16.93.50.3/24 secondary

### 20. Set ipv6 address for interface <a id="intfip6"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    ipv6 address <ipv6_address/mask> [secondary]
#### Description ####
<!--Provide a description of the command. -->
This command sets ipv6 for the interface. This command works when interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv6_address/mask* | Required | X:X::X:X/M | IPV6 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV6 address is secondary address
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:733/24 secondary

### 21. Remove ipv6 address for interface <a id="intfnoip6"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no ipv6 address <ipv6_address/mask> [secondary]
#### Description ####
<!--Provide a description of the command. -->
This command removes ipv6 address associated for the interface. This command works when interface is configured as L3.
#### Authority ####
All users.
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv6_address/mask* | Required | X:X::X:X/M | IPV6 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV6 address is secondary address
#### Examples ####
> ops-as5712# configure terminal<br>
ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:733/24 secondary


## Interface Show Commands <a id="showintfmain"></a> ##
### 1. Show all interfaces <a id="showallintf"></a> ###
#### Syntax ####
    show interface [brief]
#### Description ####
This commands displays various interfaces of the switch with their configuration and status.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **brief** | Optional| Literal  | Select this for format the output in tabular format
#### Examples ####
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
### 2. Show interface <a id="showintf"></a> ###

#### Syntax ####
    show interface <interface> [brief]
#### Description ####
This commands displays configuration and status of an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined |
| **brief** | Optional| Literal  | Select this for format the output in tabular format
#### Examples ####
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
### 3. Show all interfaces running configuration <a id="showallintfrun"></a> ###
#### Syntax ####
    show running-config interface
#### Description ####
This commands displays running configuration of various interfaces of the switch.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
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
### 4. Show interface running configuration <a id="showintfrun"></a> ###
#### Syntax ####
    show running-config interface <interface>
#### Description ####
This commands displays running configuration of a particular interface of the switch.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined
#### Examples ####
> ops-as5712# show running-config interface 2<br>
Interface 2 <br>
&nbsp;&nbsp;&nbsp;no shutdown <br>
&nbsp;&nbsp;&nbsp;speed 40000 <br>
&nbsp;&nbsp;&nbsp;autonegotiation on <br>
&nbsp;&nbsp;&nbsp;exit<br>
