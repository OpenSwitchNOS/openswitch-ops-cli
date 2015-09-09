LLDP
=======
<!--Provide the title of the feature-->

### Index ###
 [Overview](#lldpover)<br>
 [Prerequisites](#lldppre)<br>
 [Configuring LLDP](#lldpconf)<br>
 &nbsp;&nbsp;&nbsp;[Basic configuration](#lldpconfbasic)<br>
 &nbsp;&nbsp;&nbsp;[Optional configuration](#lldpconfopt)<br>
 &nbsp;&nbsp;&nbsp;[Verifying the configuration](#lldpconfver)<br>
 [Troubleshooting the configuration](#lldptrouble)<br>
 [CLI](#lldpcli)<br>
 [Related features](#lldprelated)<br>


## Overview <a id="lldpover"></a> ##
 <!--Provide an overview here. This overview should give the reader an introduction of when, where and why they would use the feature. -->
The Link Layer Discovery Protocol (LLDP) is an industry-standard, vendor-neutral method to allow networked devices to advertise capabilities, identity, and other information onto a LAN. This feature is used by an network device in a data center to discover and identify other LLDP enabled devices  directly connected to it. Some of the information gathered by LLDP are:-

- System name and description
- Port name and description
- VLAN name and identifier
- IP network management address
- Capabilities of the device (for example, switch, router, or server)
- MAC address and physical layer information
- Power information
## Prerequisites <a id="lldppre"></a> ##
<!--Change heading for conceptual or reference info, such as Prerequisites. -->
All the interfaces(or atleast to which other devices are connected) of DUT must be administratively up.
## Configuring LLDP <a id="lldpconf"></a> ##

###Basic configuration <a id="lldpconfbasic"></a> ###

#####1. Configure Terminal
'configure terminal' command changes the vtysh context to config.

> ops-as5712# configure terminal<br>
ops-as5712(config)#
#####2. Enable LLDP Globally
'Feature lldp' command enables LLDP on the switch. Once LLDP is enabled, the switch begins to transmit advertisements from those ports that are configured to send LLDP packets.

> ops-as5712(config)# feature lldp<br>
ops-as5712(config)#
#####3. Enable LLDP on interface
By using the 'lldp transmission' and 'lldp reception' commands, LLDP can be enabled or disabled on individual interfaces or configured to only
send or only receive LLDP packets. Consider interface 1 which is connected to neighbor device,

> ops-as5712(config)# interface 1<br>
ops-as5712(config-if)# lldp reception<br>
ops-as5712(config-if)# <br>
ops-as5712(config-if)# lldp transmission<br>
ops-as5712(config-if)# <br>

###Optional configuration <a id="lldpconfopt"></a> ###

#####1. Setting the LLDP Timer
The 'lldp timer' command specifies the time in seconds between LLDP updates sent by the switch.

> ops-as5712(config)# lldp timer 120<br>
ops-as5712(config)#

The 'no lldp timer' commands reverts the LLDP timer to its default value of 30 seconds.
> ops-as5712(config)# no lldp timer<br>
ops-as5712(config)#
#####2. Setting the LLDP Hold Time
The lldp holdtime command sets the amount of time a receiving device should retain the information sent by the device.
> ops-as5712(config)# lldp holdtime 5<br>
ops-as5712(config)#

The 'no lldp holdtime' commands reverts the LLDP timer to its default value of 4 seconds.
> ops-as5712(config)# no lldp holdtime<br>
ops-as5712(config)#

#####3. Setting the IP Management Address to be used in the TLV
The 'lldp management-address' command specifies the IP management address in LLDP type-length-value (TLV) triplets.
> ops-as5712(config)# lldp management-address 16.93.49.1<br>
ops-as5712(config)#
#####4. Selecting the LLDP TLV
The 'lldp select-tlv' command configures the type, length, and value (TLV) to send and receive in LLDP packets. The 'no lldp select-tlv' command removes the TLV configuration.
> ops-as5712(config)# lldp select-tlv system-capabilities<br>
ops-as5712(config)# <br>
ops-as5712(config)# lldp select-tlv port-description <br>
ops-as5712(config)# <br>

#####5. Clear LLDP Counters
The 'lldp clear counters' command resets the LLDP traffic counters to zero.
> ops-as5712(config)# lldp clear counters<br>
ops-as5712(config)#

#####5. Clear LLDP neighbor information
The 'lldp clear neighbors' command clears neighbor information.
> ops-as5712(config)# lldp clear neighbors<br>
ops-as5712(config)#
###Verifying the configuration <a id="lldpconfver"></a> ###
#####1. Viewing LLDP Global Information
The 'show lldp configuration' command displays LLDP configuration information configured above.

> ops-as5712# show lldp configuration<br>
LLDP Global Configuration:


> LLDP Enabled :Yes<br>
LLDP Transmit Interval :120<br>
LLDP Hold time Multiplier :4<br>

> TLVs advertised: <br>
Management Adress <br>
Port description <br>
Port VLAN-ID <br>
Port Protocol VLAN-ID <br>
Port VLAN Name <br>
Port Protocol-ID <br>
System capabilities <br>
System description <br>
System name <br>

> LLDP Port Configuration:

> Port  Transmission-enabled     Receive-enabled
1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes
10&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Yes<br>
..................................<br>
..................................<br>
#####2. Viewing LLDP Neighbors
The 'show lldp neighbor-info' command displays information about LLDP neighbors.
> ops-as5712# show lldp neighbor-info<br>

> Total neighbor entries : 0<br>
Total neighbor entries deleted : 0<br>
Total neighbor entries dropped : 0<br>
Total neighbor entries aged-out : 0<br>

> Local Port     Neighbor Chassis-ID      Neighbor Port-ID         TTL
1
2
3
.......................<br>
.......................<br>

The 'show lldp neighbor-info <interface name>' command shows LLDP neighbors for a particular interface.
> ops-as5712# show lldp neighbor-info 1<br>
Port                           : 1<br>
Neighbor entries               : 0<br>
Neighbor entries deleted       : 0<br>
Neighbor entries dropped       : 0<br>
Neighbor entries age-out       : 0<br>
Neighbor Chassis-Name          : <br>
Neighbor Chassis-Description   : <br>
Neighbor Chassis-ID            : <br>
Chassis Capabilities Available : <br>
Chassis Capabilities Enabled   : <br>
Neighbor Port-ID               : <br>
TTL                            : <br>
ops-as5712# <br>

#####3. Viewing LLDP statistics
The 'show lldp statistics' command displays the LLDP traffic information for the switch.
> ops-as5712# show lldp statistics<br>
LLDP Global statistics:<br>

> Total Packets transmitted : 35<br>
Total Packets received : 0<br>
Total Packet received and discarded : 0<br>
Total TLVs unrecognized : 0<br>
LLDP Port Statistics:<br>
Port-ID   Tx-Packets     Rx-packets     Rx-discarded        TLVs-Unknown
1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;34&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0
10&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0
................<br>
................<br>

The 'show lldp statistics <interface name>' command shows LLDP traffic information for a particular interface.

> ops-as5712# show lldp statistics 1<br>
LLDP statistics: <br>

> Port Name: 1<br>
Packets transmitted :36<br>
Packets received :0<br>
Packets received and discarded :0<br>
Packets received and unrecognized :0<br>
ops-as5712# <br>

#####3. Viewing LLDP TLVs
The 'show lldp tlv' command displays the LLDP TLVs to be sent and received.

> ops-as5712# show lld tlv

> TLVs advertised: <br>
Management Adress <br>
Port description <br>
Port VLAN-ID <br>
Port Protocol VLAN-ID <br>
Port VLAN Name <br>
Port Protocol-ID <br>
System capabilities <br>
System description <br>
System name <br>
ops-as5712#<br>

###Troubleshooting the configuration <a id="lldptrouble"></a> ###

| Condition                                                              | Cause                                                                           | Remedy                                                                                                                                                           |
|:------------------------------------------------------------------------|:---------------------------------------------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| LLDP Neighbor information is not displayed even if neighbor is present | Interface may be down. Neighbor may not support LLDP or feature is not enabled. | make interface administratively up by using 'no shutdown' command. Refer physical interface command reference. Neighbor should support LLDP feature and enabled. |
| System description is not displayed in neighbor info.                  | system description TLV may not be selected.                                     | Select system description TLV using 'lldp select-tlv' command.                                                                                                   |
## CLI <a id="lldpcli"></a> ##
<!--Provide a link to the CLI command related to the feature. The CLI files will be generated to a CLI directory.  -->
Click [here](https://openswitch.net/cli_feat.html#cli_command_anchor) for the CLI commands related to the LLDP feature.
## Related features <a id="lldprelated"></a> ##
<!-- Enter content into this section to describe features that may need to be considered in relation to this particular feature, under what conditions and why.  Provide a hyperlink to each related feature.  Sample text is included below as a potential example or starting point.  -->
When configuring the switch for LLDP, it might also be necessary to configure [Physical Interface](https://openswitch.net./tbd/other_filefeatures/related_feature1.html#first_anchor) so that interface to which neighbor is connected will act as expected.