LLDP Commands
=======

### Index ###
 [LLDP Configuration Commands](#lldpmain)<br>
 &nbsp;&nbsp;&nbsp;[Enable LLDP](#lldpfeature)<br>
 &nbsp;&nbsp;&nbsp;[Disable LLDP](#lldpnofeature)<br>
 &nbsp;&nbsp;&nbsp;[Clear LLDP counters](#lldpclearcount)<br>
 &nbsp;&nbsp;&nbsp;[Clear LLDP neighbor details](#lldpcleardata)<br>
 &nbsp;&nbsp;&nbsp;[Set LLDP holdtime](#lldphold)<br>
 &nbsp;&nbsp;&nbsp;[Set LLDP holdtime to default](#lldpnohold)<br>
 &nbsp;&nbsp;&nbsp;[Set Management IP address](#lldpmgmt)<br>
 &nbsp;&nbsp;&nbsp;[Remove Management IP address](#lldpnomgmt)<br>
 &nbsp;&nbsp;&nbsp;[Select TLVs](#lldptlv)<br>
 &nbsp;&nbsp;&nbsp;[Remove TLVs](#lldpnotlv)<br>
 &nbsp;&nbsp;&nbsp;[Set LLDP timer](#lldptimer)<br>
 &nbsp;&nbsp;&nbsp;[Set LLDP timer to default](#lldpnotimer)<br>
 &nbsp;&nbsp;&nbsp;[Enable LLDP transmission](#lldptrans)<br>
 &nbsp;&nbsp;&nbsp;[Disable LLDP transmission](#lldpnotrans)<br>
 &nbsp;&nbsp;&nbsp;[Enable LLDP reception](#lldprecep)<br>
 &nbsp;&nbsp;&nbsp;[Disable LLDP reception](#lldpnorecep)<br>
 [LLDP Show Commands](#showlldpmain)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP Configuration](#showlldpconf)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP TLVs](#showlldptlv)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP Neighbor information](#showlldpneighbor)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP Neighbor information (Interface)](#showlldpneighborifname)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP Statistics](#showlldpstats)<br>
 &nbsp;&nbsp;&nbsp;[Show LLDP Statistics (Interface)](#showlldpstatsifname)<br>

## LLDP Configuration Commands <a id="lldpmain"></a>
All LLDP configuration commands except "lldp transmission" and "lldp reception" works in config context.
<!-- Change LLDP -->
### 1. Enable LLDP <a id="lldpfeature"></a>
#### Syntax
	feature lldp
#### Description ####
This command enables LLDP(Link Layer Discovery Protocol) feature in the device.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# feature lldp
### 2. Disable LLDP <a id="lldpnofeature"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no feature lldp
#### Description ####
<!--Provide a description of the command. -->
This command disables LLDP(Link Layer Discovery Protocol) feature in the device.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# no feature lldp
### 3. Clear LLDP counters <a id="lldpclearcount"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp clear counters
#### Description ####
<!--Provide a description of the command. -->
This command clears LLDP neighbor counters.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp clear counters
### 4. Clear LLDP neighbor details <a id="lldpcleardata"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp clear neighbors
#### Description ####
<!--Provide a description of the command. -->
This command clears LLDP neighbor details.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp clear neighbors
### 5. Set LLDP holdtime <a id="lldphold"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp holdtime <time>
#### Description ####
<!--Provide a description of the command. -->
This command sets the amount of time in seconds, a receiving device should hold the information sent by sending device before discarding it.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *time* | Required | 2-10 | Select hold time between 2 and 10 seconds
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp holdtime 5
### 6. Set LLDP holdtime to default <a id="lldpnohold"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax
    no lldp holdtime
#### Description
<!--Provide a description of the command. -->
This command sets default value for the amount of time a receiving device should hold the information sent by sending device before discarding it. The default value is 4 seconds.
#### Authority
All users.
#### Parameters
No parameters.
#### Examples
> ops-as5712# configure terminal<br>
> ops-as5712(config)# no lldp holdtime
### 7. Set Management IP address <a id="lldpmgmt"></a>
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp management-address ( <ipv4_address> | <ipv6_address>)
#### Description ####
<!--Provide a description of the command. -->
This command sets the Management IP Address to be sent in LLDP TLV.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *ipv4_address* | choose one | A.B.C.D | set IPV4 address as LLDP management address
| *ipv6_address* | choose one | X:X::X:X | set IPV6 address as LLDP management address
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp management-address 16.93.49.9 <br>
> ops-as5712(config)# lldp management-address 2001:db8:85a3::8a2e:370:7334
### 8. Remove Management IP address <a id="lldpnomgmt"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no lldp management-address
#### Description ####
<!--Provide a description of the command. -->
This command removes Management IP Address to be sent in LLDP TLV.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# no lldp management-address
### 9. Select TLVs <a id="lldptlv"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp select-tlv (management-address | port-description | port-vlan-id  | port-vlan-name | port-protocol-vlan-id | port-protocol-id | system-capabilities | system-description | system-name)
#### Description ####
<!--Provide a description of the command. -->
This command selects the TLVs to be sent and received in LLDP packets.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **management-address** | choose one | Literal | Select management-address TLV
| **port-description**  | choose one | Literal | Select port-description TLV
| **port-vlan-id**  | choose one | Literal | Select port-vlan-id TLV
| **port-vlan-name** | choose one | Literal | Select port-vlan-name TLV
| **port-protocol-vlan-id** | choose one | Literal | Select port-protocol-vlan-id TLV
| **port-protocol-id**  | choose one | Literal | Select port-protocol-id TLV
| **system-capabilities** | choose one | Literal | Select system-capabilities TLV
| **system-description** | choose one | Literal | Select system-description TLV
| **system-name** | choose one | Literal | Select system-nameess TLV
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp select-tlv management-address<br>
> ops-as5712(config)# lldp select-tlv port-description<br>
> ops-as5712(config)# lldp select-tlv port-vlan-id<br>
> ops-as5712(config)# lldp select-tlv port-vlan-name<br>
> ops-as5712(config)# lldp select-tlv port-protocol-vlan-id<br>
> ops-as5712(config)# lldp select-tlv port-protocol-id<br>
> ops-as5712(config)# lldp select-tlv system-capabilities<br>
> ops-as5712(config)# lldp select-tlv system-description<br>
> ops-as5712(config)# lldp select-tlv system-name
### 10. Remove TLVs <a id="lldpnotlv"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no lldp select-tlv (management-address | port-description | port-vlan-id  | port-vlan-name | port-protocol-vlan-id | port-protocol-id | system-capabilities | system-description | system-name)
#### Description ####
<!--Provide a description of the command. -->
This command removes the TLVs from being sent and received in LLDP packets.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| **management-address** | choose one | Literal | Select management-address TLV
| **port-description**  | choose one | Literal | Select port-description TLV
| **port-vlan-id**  | choose one | Literal | Select port-vlan-id TLV
| **port-vlan-name** | choose one | Literal | Select port-vlan-name TLV
| **port-protocol-vlan-id** | choose one | Literal | Select port-protocol-vlan-id TLV
| **port-protocol-id**  | choose one | Literal | Select port-protocol-id TLV
| **system-capabilities** | choose one | Literal | Select system-capabilities TLV
| **system-description** | choose one | Literal | Select system-description TLV
| **system-name** | choose one | Literal | Select system-nameess TLV
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# no lldp select-tlv management-address<br>
> ops-as5712(config)# no lldp select-tlv port-description<br>
> ops-as5712(config)# no lldp select-tlv port-vlan-id<br>
> ops-as5712(config)# no lldp select-tlv port-vlan-name<br>
> ops-as5712(config)# no lldp select-tlv port-protocol-vlan-id<br>
> ops-as5712(config)# no lldp select-tlv port-protocol-id<br>
> ops-as5712(config)# no lldp select-tlv system-capabilities<br>
> ops-as5712(config)# no lldp select-tlv system-description<br>
> ops-as5712(config)# no lldp select-tlv system-name
### 11. Set LLDP timer <a id="lldptimer"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp timer <time>
#### Description ####
<!--Provide a description of the command. -->
This command sets the interval in seconds, at which LLDP status updates are transmitted to neighbors.
#### Authority ####
All users.
#### Parameters ####
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *time* | Required | 5-32768| Select timer between 5 and 32768 seconds
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# lldp timer 7
### 12. Set LLDP timer to default <a id="lldpnotimer"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no lldp timer
#### Description ####
<!--Provide a description of the command. -->
This command sets the interval at which LLDP status updates are transmitted to neighbors in seconds to it's default value. The default value is 30 seconds.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# no lldp timer
### 13. Enable LLDP transmission <a id="lldptrans"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp transmission
#### Description ####
<!--Provide a description of the command. -->
This command enables LLDP transmission(TX) for a particular interface. Works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# interface 1<br>
> ops-as5712(config-if)# lldp transmission<br>
### 14. Disable LLDP transmission <a id="lldpnotrans"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no lldp transmission
#### Description ####
<!--Provide a description of the command. -->
This command disables LLDP transmission(TX) for a particular interface. Works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# interface 1<br>
> ops-as5712(config-if)# no lldp transmission<br>
### 15. Enable LLDP reception <a id="lldprecep"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    lldp reception
#### Description ####
<!--Provide a description of the command. -->
This command enables LLDP reception(RX) for a particular interface. Works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# interface 1<br>
> ops-as5712(config-if)# lldp reception<br>
### 16. Disables LLDP reception <a id="lldpnorecep"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    no lldp reception
#### Description ####
<!--Provide a description of the command. -->
This command disables LLDP reception(RX) for a particular interface. Works in interface context.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# configure terminal<br>
> ops-as5712(config)# interface 1<br>
> ops-as5712(config-if)# no lldp reception<br>


## LLDP Show Commands <a id="showlldpmain"></a> ##
### 1. Show LLDP Configuration <a id="showlldpconf"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
    show lldp configuration
#### Description ####
<!--Provide a description of the command. -->
This commands displays various LLDP configuration of the switch. Configuration includes LLDP timer, transmission status, reception status, TLVs selected etc.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
<!--Provide for the parameters for the command.-->
No parameters.
#### Examples ####
> ops-as5712# show lldp configuration<br>
> LLDP Global Configuration:
>
LLDP Enabled :No<br>
LLDP Transmit Interval :30<br>
LLDP Hold time Multiplier :4<br>
>
TLVs advertised:<br>
Management Adress<br
Port description<br>
Port VLAN-ID<br>
Port Protocol VLAN-ID<br
Port VLAN Name<br>
Port Protocol-ID<br>
System capabilities<br>
System description<br>
System name<br>

>
LLDP Port Configuration:

>
Port  Transmission-enabled     Receive-enabled
1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
10&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
11&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
12&nbsp;&nbsp;&nbsp;&nbsp; Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
13&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
...........<br>
...........
### 2. Show LLDP TLVs <a id="showlldptlv"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
    show lldp tlv
#### Description ####
<!--Provide a description of the command. -->
This commands displays TLVs that will be sent and received in LLDP packets.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
<!--Provide for the parameters for the command.-->
No parameters.
#### Examples ####
> ops-as5712# show lldp tlv
>
> TLVs advertised:<br>
> Management Adress<br>
> Port description<br>
> Port VLAN-ID<br>
> Port Protocol VLAN-ID<br>
> Port VLAN Name<br>
> Port Protocol-ID<br>
> System capabilities<br>
> System description<br>
> System name<br>
### 3. Show LLDP Neighbor information <a id="showlldpneighbor"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    show lldp neighbor-info
#### Description ####
This commands displays information about neighbors of the switch.
#### Authority ####
All users.
#### Parameters ####
No parameters.
#### Examples ####
> ops-as5712# show lldp neighbor-info
>
> Total neighbor entries : 1<br>
> Total neighbor entries deleted : 0<br>
> Total neighbor entries dropped : 0<br>
> Total neighbor entries aged-out : 0<br>
>
> Local Port     Neighbor Chassis-ID      Neighbor Port-ID         TTL
> 1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;10:60:4b:39:3e:80&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;120
> 2
> 3
### 4. Show LLDP Neighbor information (Interface) <a id="showlldpneighborifname"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
    show lldp neighbor-info <interface>
#### Description ####
<!--Provide a description of the command. -->
This commands displays detailed information about a particular neighbor connected to a particular interface.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
<!--Provide for the parameters for the command.-->
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined |
#### Examples ####

> ops-as5712# show lldp neighbor-info 1<br>
Port                           : 1<br>
Neighbor entries               : 1<br>
Neighbor entries deleted       : 0<br>
Neighbor entries dropped       : 0<br>
Neighbor entries age-out       : 0<br>
Neighbor Chassis-Name          : HP-3800-24G-PoEP-2XG<br>
Neighbor Chassis-Description   : HP J9587A 3800-24G-PoE+-2XG Switch, revision KA.15.15.0006, ROM KA.15.09 (/ws/swbuildm/rel_nashville_qaoff/code/build/tam(swbuildm_rel_nashville_qaoff_rel_nashville))<br>
Neighbor Chassis-ID            : 10:60:4b:39:3e:80<br>
Chassis Capabilities Available : Bridge, Router<br>
Chassis Capabilities Enabled   : Bridge<br>
Neighbor Port-ID               : 1<br>
TTL                            : 120<br>

### 5. Show LLDP Statistics <a id="showlldpstats"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
    show lldp statistics
#### Description ####
<!--Provide a description of the command. -->
This commands displays global LLDP statistics like packet counts, unknown TLV received etc.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
<!--Provide for the parameters for the command.-->
No parameters.
#### Examples ####

> ops-as5712# show lldp statistics<br>
LLDP Global statistics:

> Total Packets transmitted : 9<br>
Total Packets received : 12<br>
Total Packet received and discarded : 0<br>
Total TLVs unrecognized : 0<br>
LLDP Port Statistics:<br>
Port-ID   Tx-Packets     Rx-packets     Rx-discarded        TLVs-Unknown
1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;9&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;12&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0
10&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0<br>
...........<br>
...........
### 6. Show LLDP Statistics (Interface) <a id="showlldpstatsifname"></a> ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
    show lldp statistics <interface>
#### Description ####
<!--Provide a description of the command. -->
This commands displays LLDP statistics like packet counts, unknown TLV received etc for a particular interface.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
All users.
#### Parameters ####
<!--Provide for the parameters for the command.-->
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *interface* | Required | System defined | Name of the interface. System defined |
#### Examples ####

> ops-as5712# show lldp statistics 1<br>
LLDP statistics:<br>

>
Port Name: 1<br>
Packets transmitted :20<br>
Packets received :23<br>
Packets received and discarded :0<br>
Packets received and unrecognized :0<br>
