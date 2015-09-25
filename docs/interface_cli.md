Interface Commands
=======
## Contents

[TOC]

## Interface Configuration Commands <a id="intfmain"></a>
In vtysh every command belongs to a particular context. All interface configuration commands, except `interface`, works in interface context.
### Change to interface context <a id="intfcontext"></a>
#### Syntax
`interface <interface>`
#### Description ####
This command changes vtysh context to interface. This command works in config context.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined. |
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)#
```
### Set Interface Description <a id="intfdesc"></a>
#### Syntax
`description <description>`
#### Description ####
This command sets the description for an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *description* | Required | - | String describing the interface. |
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# description This is interface 1
```
### Remove interface description <a id="intfnodesc"></a> ###
#### Syntax ####
`no description`
#### Description ####
This command removes the description of an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no description
```
### Enable interface <a id="intfen"></a> ###
#### Syntax ####
`no shutdown`
#### Description ####
<!--Provide a description of the command. -->
This command enables an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no shutdown
```
### Disable interface <a id="intfdis"></a> ###
#### Syntax ####
`shutdown`
#### Description ####
This command disables an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# shutdown
```
### Enable routing on an interface <a id="intfroute"></a> ###
#### Syntax ####
`routing`
#### Description ####
This command enables routing on an interface.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# routing
```
### Disable routing on an interface <a id="intfnoroute"></a> ###
#### Syntax
`no routing`
#### Description
This command disables routing on an interface.
#### Authority
All users.
#### Parameters
No parameters.
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no routing
```
### Set interface speed <a id="intfspeed"></a>
#### Syntax ####
`speed (auto|1000|10000|100000|40000)`
#### Description ####
This command sets the operating speed of an interface.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters as speed.

| Parameter | Description |
|:-----------|:----------------:|:---------------------------------------|
| **auto** | Speed set to auto mode. |
| **1000** | Speed set to 1 Gbps. |
| **10000** | Speed set to 10 Gbps. |
| **100000** | Speed set to 100 Gbps. |
| **40000** | Speed set to 40 Gbps. |
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# speed 10000
```
### Set interface speed to default <a id="intfnospeed"></a> ###
#### Syntax ####
`no speed`
#### Description ####
This command sets the operating speed of an interface to default. The default setting is 'auto'.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no speed
```
### Set interface MTU <a id="intfmtu"></a> ###
#### Syntax ####
`mtu (auto|<value>)`
#### Description ####
This command sets the MTU(maximum transmission unit) of an interface.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters as MTU.

| Parameter | Syntax         | Description                           |
|:-----------|:----------------:|:---------------------------------------|
| **auto** | Literal | MTU set to auto mode. |
| *value* | 576-16360 | MTU value between 576 and 16360 bytes.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# mtu auto
ops-as5712(config-if)# mtu 580
```
### Set interface MTU to default <a id="intfnomtu"></a> ###
#### Syntax ####
`no mtu`
#### Description ####
This command sets the MTU(maximum transmission unit) of an interface to default. The default setting is 'auto'.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no mtu
```
### Set interface duplexity<a id="intfduplex"></a> ###

#### Syntax ####
`duplex (half|full)`
#### Description ####
This command sets the duplexity of an interface from among half duplex and full duplex.
#### Authority ####
All users.
#### Parameters
Choose one of the following parameters as duplexity.

| Parameter | Description|
|:-----------|:---------------------------------------|
| **half** | Choose mode as half duplex. |
| **full** | Choose mode as full duplex.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# duplex half
```
### Set interface duplexity to default <a id="intfnoduplex"></a> ###
#### Syntax ####
`no duplex`
#### Description ####
This command sets the duplexity of an interface to it's default. The default mode is 'full'.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no duplex
```
### Enable flow control <a id="intfflow"></a> ###
#### Syntax ####
`flowcontrol (receive|send) (off|on)`
#### Description ####
This command enables flow control, for sending and receiving pause frames.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters to select whether to set flow control to receive pause frames or send pause frames.

| Parameter 1| Description|
|:-----------|:---------------------------------------|
| **receive** |Select the status for receiving pause frames. |
| **send** | Select the status for sending pause frames.

Choose one of the parameters to either switch flow control on or off.

| Parameter 2| Description|
|:-----------|:----------|:----------------:|:---------------------------------------|
| **off** | Select this to switch off flow control for above parameter. |
| **on** | Select this to switch on flow control for above parameter.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# flowcontrol receive on
ops-as5712(config-if)# flowcontrol send on
```
### Set flowcontrol to default <a id="intfnoflow"></a> ###
#### Syntax ####
`no flowcontrol (receive|send)`
#### Description ####
This command sets the flow control to default. The default is flow control 'off'.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters to select whether to disable, 'receive' flow control or 'send' flow control.

| Parameter | Description                           |
|:-----------|:---------------------------------------|
| **receive** |Select the status for receiving pause frames. |
| **send** |Select the status for sending pause frames.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no flowcontrol receive
ops-as5712(config-if)# no flowcontrol send
```
### Set autonegotiation state <a id="intfautonego"></a> ###
#### Syntax ####
`autonegotiation (on|off)`
#### Description ####
This command sets the autonegotiation state of the interface.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters to switch the autonegotiation state, on or off.

| Parameter| Description |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **off** |Switch off auto negotiation. |
| **on** | Switch on auto negotiation.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# autonegotiation on
```
### Set autonegotiation to default <a id="intfnoautonego"></a> ###
#### Syntax ####
`no autonegotiation`
#### Description ####
This command sets the autonegotiation state to default.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no autonegotiation
```
### Set ipv4 address for interface <a id="intfip"></a> ###
#### Syntax ####
`ip address <ipv4_address/mask> [secondary]`
#### Description ####
This command sets the ipv4 address for an interface. This command only works when interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address/mask* | Required | A.B.C.D/M | IPV4 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV4 address is secondary address.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# ip address 16.93.50.2/24
ops-as5712(config-if)# ip address 16.93.50.3/24 secondary
```
### Remove ipv4 address for interface <a id="intfnoip"></a> ###
#### Syntax ####
`no ip address <ipv4_address/mask> [secondary]`
#### Description ####
This command removes the ipv4 address associated with an interface. This command works only when the interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address/mask* | Required | A.B.C.D/M | IPV4 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV4 address is secondary address.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no ip address 16.93.50.2/24
ops-as5712(config-if)# no ip address 16.93.50.3/24 secondary
```

### Set ipv6 address for interface <a id="intfip6"></a> ###
#### Syntax ####
`ipv6 address <ipv6_address/mask> [secondary]`
#### Description ####
This command sets the ipv6 address for an interface. This command only works when the interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv6_address/mask* | Required | X:X::X:X/M | IPV6 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV6 address is secondary address.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:733/24 secondary
```
### Remove ipv6 address for interface <a id="intfnoip6"></a> ###
#### Syntax ####
`no ipv6 address <ipv6_address/mask> [secondary]`
#### Description ####
This command removes the ipv6 address associated with the interface. This command only works when the interface is configured as L3.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv6_address/mask* | Required | X:X::X:X/M | IPV6 address with mask for the interface |
| **secondary** | Optional| Literal  | Select this if IPV6 address is secondary address.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:7334/24
ops-as5712(config-if)# no ipv6 address 2001:0db8:85a3:0000:0000:8a2e:0370:733/24 secondary
```
### Spilt a QSPF interface that support splitter cables <a id="intfsplit"></a> ###
#### Syntax ####
`[no] split`
#### Description ####
The `split` command, splits a QSPF interface to work as 4 x 10Gb interfaces. Where as `no split` command combines the splitted QSPF interface to work as 1 x 40Gb interface.
The splitted interface names are append with '-1','-2','-3' and '-4'. For example, if the QSPF interface name is 54 then the splitted interface names are 54-1, 54-2, 54-3 and 54-4.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 54
ops-as5712(config)# split

ops-as5712# configure terminal
ops-as5712(config)# interface 54
ops-as5712(config)# no split
```
## Interface Show Commands <a id="showintfmain"></a> ##
### Show all interfaces <a id="showallintf"></a> ###
#### Syntax ####
`show interface [brief]`
#### Description ####
This command displays various switch interfaces with their configurations and statuses.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples ####
```
ops-as5712# show interface

Interface 1 is down (Administratively down)
Admin state is down
State information: admin_down
Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
MTU 9388
Half-duplex
Speed 0 Mb/s
Auto-Negotiation is turned on
Input flow-control is on, output flow-control is on
RX
   0 input packets 0 bytes
   0 input error   0 dropped
   0 CRC/FCS
TX
   0 output packets 0 bytes
   0 input error    4 dropped
   0 collision

Interface 10 is down (Administratively down)
Admin state is down
State information: admin_down
Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
MTU 9388
Half-duplex
Speed 0 Mb/s
Auto-Negotiation is turned on
Input flow-control is on, output flow-control is on
RX
   0 input packets 0 bytes
   0 input error   0 dropped
   0 CRC/FCS
TX
   0 output packets 0 bytes
   0 input error    4 dropped
   0 collision
.........
.........
ops-as5712# show interface brief
....................................................................................................
Ethernet      VLAN     Type    Mode    Status         Reason             Speed   Port
Interface                                                                (Mb/s)   Ch#
....................................................................................................
 1             ..       eth     ..      down     Administratively down    auto    ..
 10            ..       eth     ..      down     Administratively down    auto    ..
 11            ..       eth     ..      down     Administratively down    auto    ..
...............
...............
```
### Show interface <a id="showintf"></a> ###

#### Syntax ####
`show interface <interface> [brief]`
#### Description ####
This command displays the configuration and status of an interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined. |
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples ####
```
ops-as5712# show interface 1

Interface 1 is up
 Admin state is up
 State information: admin_down
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 MTU 1500
 Full-duplex
 Speed 1000 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is off, output flow-control is off
 RX
      0 input packets     0 bytes
      0 input error       0 dropped
      0 CRC/FCS
 TX
      0 output packets  0 bytes
      0 input error     0 dropped
      0 collision
ops-as5712# show interface 1 brief
....................................................................................................
Ethernet    VLAN   Type   Mode    Status   Reason                    Speed     Port
Interface                                                            (Mb/s)    Ch#
....................................................................................................
  1          ..    eth     ..     down    Administratively down       auto     ..
```
### Show transceiver information of all interfaces <a id="showalltransintf"></a> ###
#### Syntax ####
`show interface transceiver [brief]`
#### Description ####
This command displays information about various pluggable modules (or fixed interfaces) present in the switch.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples ####
```
ops-as5712# show interface transceiver

Interface 1:
 Connector: SFP+
 Transceiver module: SFP_RJ45
 Connector status: supported
 Vendor name: AVAGO
 Part number: ABCU-5710RZ-HP8
 Part revision:
 Serial number: MY36G2C52D
 Supported speeds: 1000

Interface 10:
 Connector: SFP+
 Transceiver module: not present

Interface 11:
 Connector: SFP+
 Transceiver module: not present
 -------
 -------
ops-as5712# show interface transceiver brief

-----------------------------------------------
Ethernet      Connector    Module     Module
Interface                  Type       Status
-----------------------------------------------
 1              SFP+       SFP_RJ45   supported
 10             SFP+       --         --
 11             SFP+       --         --
 12             SFP+       --         --
 13             SFP+       --         --
 -------
 -------
 ```
### Show transceiver information of an interface <a id="showtransintf"></a> ###
#### Syntax ####
`show interface <interface> transceiver [brief]`
#### Description ####
This command displays transceiver information about a particular interface of the switch.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined. |
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples ####
```
ops-as5712# show interface 1 transceiver

Interface 1:
 Connector: SFP+
 Transceiver module: SFP_RJ45
 Connector status: supported
 Vendor name: AVAGO
 Part number: ABCU-5710RZ-HP8
 Part revision:
 Serial number: MY36G2C52D
 Supported speeds: 1000
 
ops-as5712# show interface 1 transceiver brief

-----------------------------------------------
Ethernet      Connector    Module     Module
Interface                  Type       Status
-----------------------------------------------
 1              SFP+       SFP_RJ45   supported
```
### Show all interfaces running configuration <a id="showallintfrun"></a> ###
#### Syntax ####
`show running-config interface`
#### Description ####
This command displays active configurations of various switch interfaces.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
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
```
### Show interface running configuration <a id="showintfrun"></a> ###
#### Syntax ####
`show running-config interface <interface>`
#### Description ####
This command displays active configurations of a particular switch interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined.
#### Examples ####
```
ops-as5712# show running-config interface 2
Interface 2
   no shutdown
   speed 40000
   autonegotiation on
   exit
```
