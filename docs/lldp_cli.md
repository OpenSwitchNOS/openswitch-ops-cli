LLDP Commands
=======
## Contents

[TOC]

## LLDP Configuration Commands <a id="lldpmain"></a>
All LLDP configuration commands except `lldp transmission` and `lldp reception` work in config context.
### Enable LLDP <a id="lldpfeature"></a>
#### Syntax
`feature lldp`
#### Description ####
This command enables the LLDP (Link Layer Discovery Protocol) feature in the device.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# feature lldp
```
### Disable LLDP <a id="lldpnofeature"></a> ###
#### Syntax ####
`no feature lldp`
#### Description ####
This command disables the LLDP (Link Layer Discovery Protocol) feature in the device.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# no feature lldp
```
### Clear LLDP counters <a id="lldpclearcount"></a> ###
#### Syntax ####
`lldp clear counters`
#### Description ####
This command clears LLDP neighbor counters.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp clear counters
```
### Clear LLDP neighbor details <a id="lldpcleardata"></a> ###
#### Syntax ####
`lldp clear neighbors`
#### Description ####
This command clears LLDP neighbor details.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp clear neighbors
```
### Set LLDP holdtime <a id="lldphold"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
`lldp holdtime <time>`
#### Description ####
This command sets the amount of time (in seconds), a receiving device holds the information sent before discarding it.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *time* | Required | 2-10 | Select hold time between 2 and 10 seconds.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp holdtime 5
```
### Set LLDP holdtime to default <a id="lldpnohold"></a> ###
#### Syntax
`no lldp holdtime`
#### Description
This command sets default values for the amount of time a receiving device should hold the information sent before discarding it. The default value is 4 seconds.
#### Authority
All users.
#### Parameters
No parameters.
#### Examples
```
ops-as5712# configure terminal
ops-as5712(config)# no lldp holdtime
```
### Set Management IP address <a id="lldpmgmt"></a>
#### Syntax ####
`lldp management-address ( <ipv4_address> | <ipv6_address>)`
#### Description ####
This command sets the Management IP Address to be sent using LLDP TLV.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following as parameters.

| Parameter | Syntax | Description |
|:-----------|:----------------:|:---------------------------------------|
| *ipv4_address* | A.B.C.D | Set IPV4 address as LLDP management address.
| *ipv6_address* | X:X::X:X | Set IPV6 address as LLDP management address.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp management-address 16.93.49.9
ops-as5712(config)# lldp management-address 2001:db8:85a3::8a2e:370:7334
```
### Remove Management IP address <a id="lldpnomgmt"></a> ###
#### Syntax ####
`no lldp management-address`
#### Description ####
This command removes the Management IP Address to be sent using LLDP TLV.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# no lldp management-address
```
### Select TLVs <a id="lldptlv"></a> ###
#### Syntax ####
`lldp select-tlv (management-address | port-description | port-vlan-id  | port-vlan-name | port-protocol-vlan-id | port-protocol-id | system-capabilities | system-description | system-name)`
#### Description ####
This command selects the TLVs to be sent and received in LLDP packets.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters.

| Parameter | Description |
|:-----------|:---------------------------------------|
| **management-address** | Select management-address TLV.
| **port-description**  | Select port-description TLV.
| **port-vlan-id**  | Select port-vlan-id TLV.
| **port-vlan-name** | Select port-vlan-name TLV.
| **port-protocol-vlan-id** | Select port-protocol-vlan-id TLV.
| **port-protocol-id**  | Select port-protocol-id TLV.
| **system-capabilities** | Select system-capabilities TLV.
| **system-description** | Select system-description TLV.
| **system-name** | Select system-name TLV.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp select-tlv management-address
ops-as5712(config)# lldp select-tlv port-description
ops-as5712(config)# lldp select-tlv port-vlan-id
ops-as5712(config)# lldp select-tlv port-vlan-name
ops-as5712(config)# lldp select-tlv port-protocol-vlan-id
ops-as5712(config)# lldp select-tlv port-protocol-id
ops-as5712(config)# lldp select-tlv system-capabilities
ops-as5712(config)# lldp select-tlv system-description
ops-as5712(config)# lldp select-tlv system-name
```
### Remove TLVs <a id="lldpnotlv"></a> ###
#### Syntax ####
`no lldp select-tlv (management-address | port-description | port-vlan-id  | port-vlan-name | port-protocol-vlan-id | port-protocol-id | system-capabilities | system-description | system-name)`
#### Description ####
This command removes the TLVs from being sent and received in LLDP packets.
#### Authority ####
All users.
#### Parameters ####
Choose one of the following parameters.

| Parameter | Description |
|:-----------|:---------------------------------------|
| **management-address** | Select management-address TLV.
| **port-description**  | Select port-description TLV.
| **port-vlan-id**  | Select port-vlan-id TLV.
| **port-vlan-name** | Select port-vlan-name TLV.
| **port-protocol-vlan-id** | Select port-protocol-vlan-id TLV.
| **port-protocol-id**  | Select port-protocol-id TLV.
| **system-capabilities** | Select system-capabilities TLV.
| **system-description** | Select system-description TLV.
| **system-name** | Select system-name TLV.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# no lldp select-tlv management-address
ops-as5712(config)# no lldp select-tlv port-description
ops-as5712(config)# no lldp select-tlv port-vlan-id
ops-as5712(config)# no lldp select-tlv port-vlan-name
ops-as5712(config)# no lldp select-tlv port-protocol-vlan-id
ops-as5712(config)# no lldp select-tlv port-protocol-id
ops-as5712(config)# no lldp select-tlv system-capabilities
ops-as5712(config)# no lldp select-tlv system-description
ops-as5712(config)# no lldp select-tlv system-name
```
### Set LLDP timer <a id="lldptimer"></a> ###
#### Syntax ####
`lldp timer <time>`
#### Description ####
This command sets the LLDP status update interval in seconds which are transmitted to neighbors.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *time* | Required | 5-32768| Select timer between 5 and 32768 seconds
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# lldp timer 7
```
### Set LLDP timer to default <a id="lldpnotimer"></a> ###
#### Syntax ####
`no lldp timer`
#### Description ####
This command sets the default time interval for transmitting LLDP status updates to neighbors. The default value is 30 seconds.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# no lldp timer
```
### Enable LLDP transmission <a id="lldptrans"></a> ###
#### Syntax ####
`lldp transmission`
#### Description ####
This command enables LLDP transmission (TX) for a particular interface. This command only works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# lldp transmission
```
### Disable LLDP transmission <a id="lldpnotrans"></a> ###
#### Syntax ####
`no lldp transmission`
#### Description ####
This command disables LLDP transmission (TX) for a particular interface. This command only works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no lldp transmission
```
### Enable LLDP reception <a id="lldprecep"></a> ###
#### Syntax ####
`lldp reception`
#### Description ####
This command enables LLDP reception (RX) for a particular interface. This command only works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# lldp reception
```
### Disable LLDP reception <a id="lldpnorecep"></a> ###
#### Syntax ####
`no lldp reception`
#### Description ####
This command disables LLDP reception (RX) for a particular interface. This command only works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# configure terminal
ops-as5712(config)# interface 1
ops-as5712(config-if)# no lldp reception
```

## LLDP Show Commands <a id="showlldpmain"></a> ##
### Show LLDP Configuration <a id="showlldpconf"></a> ###
#### Syntax ####
`show lldp configuration`
#### Description ####
This command displays various switch LLDP configurations. The configuration includes the LLDP timer, transmission status, reception status, selected TLVs and so on.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# show lldp configuration
LLDP Global Configuration:

LLDP Enabled :No
LLDP Transmit Interval :30
LLDP Hold time Multiplier :4

TLVs advertised:
Management Address
Port description
Port VLAN-ID
Port Protocol VLAN-ID
Port VLAN Name
Port Protocol-ID
System capabilities
System description
System name

LLDP Port Configuration:

Port  Transmission-enabled     Receive-enabled
1      Yes                               Yes
10     Yes                               Yes
11     Yes                               Yes
12     Yes                               Yes
13     Yes                               Yes
...........
...........
```
### Show LLDP TLV <a id="showlldptlv"></a> ###
#### Syntax ####
`show lldp tlv`
#### Description ####
This command displays TLVs that will be sent and received in LLDP packets.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# show lldp tlv

TLVs advertised:
Management Adress
Port description
Port VLAN-ID
Port Protocol VLAN-ID
Port VLAN Name
Port Protocol-ID
System capabilities
System description
System name
```
### Show LLDP Neighbor information <a id="showlldpneighbor"></a> ###
#### Syntax ####
`show lldp neighbor-info`
#### Description ####
This command displays information about the switch's neighbors.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# show lldp neighbor-info

Total neighbor entries : 1
Total neighbor entries deleted : 0
Total neighbor entries dropped : 0
Total neighbor entries aged-out : 0

Local Port     Neighbor Chassis-ID      Neighbor Port-ID         TTL
1                 10:60:4b:39:3e:80      1                      120
2
3
-----------
-----------
```
### Show LLDP Neighbor information (Interface) <a id="showlldpneighborifname"></a> ###
#### Syntax ####
`show lldp neighbor-info <interface>`
#### Description ####
This command displays detailed information about a particular neighbor connected to a particular interface.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined. |
#### Examples ####
```
ops-as5712# show lldp neighbor-info 1
Port                           : 1
Neighbor entries               : 1
Neighbor entries deleted       : 0
Neighbor entries dropped       : 0
Neighbor entries age-out       : 0
Neighbor Chassis-Name          : HP-3800-24G-PoEP-2XG
Neighbor Chassis-Description   : HP J9587A 3800-24G-PoE+-2XG Switch, revision KA.15.15.0006, ROM KA.15.09 (/ws/swbuildm/rel_nashville_qaoff/code/build/tam(swbuildm_rel_nashville_qaoff_rel_nashville))
Neighbor Chassis-ID            : 10:60:4b:39:3e:80
Chassis Capabilities Available : Bridge, Router
Chassis Capabilities Enabled   : Bridge
Neighbor Port-ID               : 1
TTL                            : 120
```
### Show LLDP Statistics <a id="showlldpstats"></a> ###
#### Syntax ####
`show lldp statistics`
#### Description ####
This command displays global LLDP statistics such as packet counts, unknown TLV received and so on.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
```
ops-as5712# show lldp statistics
LLDP Global statistics:

Total Packets transmitted : 9
Total Packets received : 12
Total Packet received and discarded : 0
Total TLVs unrecognized : 0
LLDP Port Statistics:
Port-ID   Tx-Packets     Rx-packets     Rx-discarded        TLVs-Unknown
1          9                12                  0                  0
10         0                0                   0                  0
...........
...........
```
### Show LLDP Statistics (Interface) <a id="showlldpstatsifname"></a> ###
#### Syntax ####
`show lldp statistics <interface>`
#### Description ####
This command displays LLDP statistics for a particular interface such as packet counts, unknown TLV received and so on.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined. |
#### Examples ####
```
ops-as5712# show lldp statistics 1
LLDP statistics:

Port Name: 1
Packets transmitted :20
Packets received :23
Packets received and discarded :0
Packets received and unrecognized :0
```
