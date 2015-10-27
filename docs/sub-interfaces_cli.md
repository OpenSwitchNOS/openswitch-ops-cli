# L3 Subinterfaces Commands
<!-- TOC depth:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [L3 Subinterfaces Commands](#l3-subinterfaces-commands)
	- [Configuration Commands](#configuration-commands)
		- [Create Subinterface](#create-subinterface)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Delete Subinterface](#delete-subinterface)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Set/Unset IPv4 address](#setunset-ipv4-address)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Set/Unset IPv6 address](#setunset-ipv6-address)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Set/Unset IEEE 802.1Q VLAN encapsulation](#setunset-ieee-8021q-vlan-encapsulation)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Enable interface](#enable-interface)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Disable interface](#disable-interface)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
	- [Display Commands](#display-commands)
		- [Show Running Configuration](#show-running-configuration)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Examples](#examples)
		- [Show Subinterfaces](#show-subinterfaces)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
		- [Show Subinterface](#show-subinterface)
			- [Syntax](#syntax)
			- [Description](#description)
			- [Authority](#authority)
			- [Parameters](#parameters)
			- [Examples](#examples)
	- [References](#references)
<!-- /TOC -->

## Configuration Commands
###  Create Subinterface
#### Syntax
```
interface L3_interface.subinterface
```
#### Description
This command creates subinterface on a L3 interface and enters subinterface configuration mode
#### Authority
All Users
#### Parameters
| Parameter | Status   | Syntax |	Description |
|-----------|----------|----------------------|
| **L3_interface** | Required | System defined | Name of the interface. System defined.  |
| **subinterface** | Required | Integer | Subinterface ID 1 to 4294967293 |
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)#
```
###  Delete Subinterface
#### Syntax
```
no interface L3_interface.subinterface
```
#### Description
This command deletes a subinterface on a L3 interface
#### Authority
All Users
#### Parameters
| Parameter | Status   | Syntax |	Description |
|-----------|----------|----------------------|
| **L3_interface** | Required | System defined | Name of the interface. System defined.  |
| **subinterface** | Required | Integer | Subinterface ID 1 to 2147483647 |
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# no interface 1.1
```
###  Set/Unset IPv4 address
#### Syntax
```
[no] ip address <ipv4_address/prefix-length>
```
#### Description
This command sets the ipv4 address for a subinterface.
#### Authority
All Users
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address/prefix-length* | Required | A.B.C.D/M | IPV4 address with prefix-length for the subinterface |
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)# ip address 16.93.50.2/24
```
###  Set/Unset IPv6 address
#### Syntax
```
[no] ip address <ipv6_address/prefix-length>
```
#### Description
This command sets the ipv6 address for a subinterface.
#### Authority
All Users
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv6_address/prefix-length* | Required | X:X::X:X/P | IPV6 address with prefix-length for the subinterface |
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)# ipv6 address fd00:5708::f02d:4df6/64
```
### Set/Unset IEEE 802.1Q VLAN encapsulation
#### Syntax
```
[no] encapsulation dot1Q  vlan-id
```
#### Description
Only a single VLAN can be assigned to a subinterface. Each subinterface must have a VLAN ID before it can pass traffic. To change a VLAN ID, not need to remove the old VLAN ID with the no option; but enter the vlan command with a different VLAN ID.
#### Authority
All users
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlan-id* | Required | 1 - 4094  | Represents VLAN and takes values from 1 to 4094|
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)#encapsulation dot1Q 33
```
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)#no encapsulation dot1Q 33
```
### Enable interface
#### Syntax
`no shutdown`
#### Description
This command enables a subinterface.
#### Authority
All Users
#### Parameters
No parameters.
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-subif)# no shutdown
```
### Disable interface
#### Syntax
`shutdown`
#### Description
This command disables an subinterface.
#### Authority
All Users
#### Parameters
No parameters.
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1.1
ops-as5712(config-if)# shutdown
```
##Display Commands
### Show Running Configuration
#### Syntax
`show running-config`
#### Description
The display of this command includes all configured subinterfaces
#### Authority
All users
#### Examples
```
ops-as5712# show running-config
.............................
.............................
interface 1.1
    no shutdown
    ip address 192.168.1.1/24
interface 1.2
    no shutdown
    ip address 182.168.1.1/24
    encapsulation dot1Q 44
.............................
.............................
```
### Show Subinterfaces
#### Syntax
`show interface <L3_interface> sub-interface [brief]`
#### Description
This command displays all configured subinterfaces, optionally on a particular L3 interface
#### Authority
All users
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **L3_interface** | Required | System defined | Name of the interface. System defined.  |
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples
```
ops-as5712# show interface 1 sub-interface
Interface 1.1 is down(Parent Interface Admin down)
 Admin state is down
 parent interface is 1
 encapsulation dot1Q 33
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 IPv4 address 192.168.1.1/2
 Input flow-control is off, output flow-control is off
 RX
      0 input packets     0 bytes
      0 input error       0 dropped
      0 CRC/FCS
 TX
      0 output packets  0 bytes
      0 input error     0 dropped
      0 collision
Interface 1.2 is down(Parent Interface Admin down))
 Admin state is down
 parent interface is 1
 encapsulation dot1Q 44
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 IPv4 address 182.168.1.1/2
 Input flow-control is off, output flow-control is off
 RX
      0 input packets     0 bytes
      0 input error       0 dropped
      0 CRC/FCS
 TX
      0 output packets  0 bytes
      0 input error     0 dropped
      0 collision

ops-as5712# show interface 1 sub-interface brief
....................................................................................................
Sub         VLAN   Type   Mode    Status   Reason                    Speed     Port
Interface                                                            (Mb/s)    Ch#
...................................................................................................
  1.1        33    eth     ..     down    Administratively down       auto     ..
  1.2        44    eth     ..     down    Administratively down       auto     ..
```
### Show Subinterface
#### Syntax
`show interface <L3_interface.subinterface> [brief]`
#### Description
This command displays the configuration and status of a subinterface.
#### Authority
All users
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **L3_interface** | Required | System defined | Name of the interface. System defined.  |
| **subinterface** | Required | Integer | Subinterface ID 1 to 1024 |
| **brief** | Optional| Literal  | Select this for format the output in tabular format.
#### Examples
```
ops-as5712# show interface 1.1
Interface 1.1 is down(Parent Interface Admin down)
 Admin state is down
 parent interface is 1
 encapsulation dot1Q 33
 Hardware: Ethernet, MAC Address: 70:72:cf:fd:e7:b4
 IPv4 address 182.168.1.1/2
 Input flow-control is off, output flow-control is off
 RX
      0 input packets     0 bytes
      0 input error       0 dropped
      0 CRC/FCS
 TX
      0 output packets  0 bytes
      0 input error     0 dropped
      0 collision
ops-as5712# show interface 1.1 brief
....................................................................................................
Sub         VLAN   Type   Mode    Status   Reason                    Speed     Port
Interface                                                            (Mb/s)    Ch#
...................................................................................................
  1.1        33    eth     ..     down    Administratively down       auto     ..
```
##References
* [Reference 1]`interface_cli.md`
* [Reference 2]`sub-interface_design.md`
