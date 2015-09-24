LLDP
=======
## Contents

[TOC]

## Overview <a id="lldpover"></a> ##
The Link Layer Discovery Protocol (LLDP) is an industry-standard, vendor-neutral method to allow networked devices to advertise capabilities, discover and identify other LLDP enabled devices and gather information in a LAN. The following bullet list contains some of the information gathered by LLDP:

- System name and description
- Port name and description
- VLAN name and identifier
- IP network management address
- Device Capabilities (for example, switch, router, or server)
- MAC address and physical layer information
- Power information

## Prerequisites <a id="lldppre"></a> ##
All the DUT interfaces (at least the interfaces that are connected to other devices) must be administratively up.
## Configuring LLDP <a id="lldpconf"></a> ##
###Setting up the basic configuration <a id="lldpconfbasic"></a> ###
1. Configure the terminal to change the vtysh context to config context with the following commands:
```
ops-as5712# configure terminal
ops-as5712(config)#
```

2. Enable LLDP globally on the switch with the following command:
```
ops-as5712(config)# feature lldp
ops-as5712(config)#
```
Once LLDP is enabled, the switch begins to transmit advertisements from those ports that are configured to send LLDP packets.

3. Enable LLDP on interface.
By using the `lldp transmission` and `lldp reception` commands, LLDP can be enabled or disabled on individual interfaces or configured to only
send or only receive LLDP packets. Consider interface 1 which is connected to neighbor device,
```
ops-as5712(config)# interface 1
ops-as5712(config-if)# lldp reception
ops-as5712(config-if)#
ops-as5712(config-if)# lldp transmission
ops-as5712(config-if)#
```

###Setting up optional configurations <a id="lldpconfopt"></a> ###

1. Setting the LLDP Timer.
The `lldp timer` command specifies the time in seconds between LLDP updates sent by the switch.
```
ops-as5712(config)# lldp timer 120
ops-as5712(config)#
```
The `no lldp timer` commands reverts the LLDP timer to its default value of 30 seconds.
```
ops-as5712(config)# no lldp timer
ops-as5712(config)#
```

2. Setting the LLDP Hold Time.
The `lldp holdtime` command sets the amount of time a receiving device should retain the information sent by the device.
```
ops-as5712(config)# lldp holdtime 5
ops-as5712(config)#
```
The `no lldp holdtime` commands reverts the LLDP timer to its default value of four seconds.
```
ops-as5712(config)# no lldp holdtime
ops-as5712(config)#
```

3. Setting the ip to be used in the Management Address TLV.
The `lldp management-address` command specifies the ip to be used in  management address LLDP type-length-value (TLV) triplets.
```
ops-as5712(config)# lldp management-address 16.93.49.1
ops-as5712(config)#
```

4. Select LLDP TLV.
The `lldp select-tlv` command configures the type, length, and value (TLV) to send and receive in LLDP packets. The `no lldp select-tlv` command removes the TLV configuration.
```
ops-as5712(config)# lldp select-tlv system-capabilities
ops-as5712(config)#
ops-as5712(config)# lldp select-tlv port-description
ops-as5712(config)#
```

5. Clearing the LLDP Counters.
The `lldp clear counters` command resets the LLDP traffic counters to zero.
```
ops-as5712(config)# lldp clear counters
ops-as5712(config)#
```

6. Clearing the LLDP neighbor information.
The `lldp clear neighbors` command clears neighbor information.
```
ops-as5712(config)# lldp clear neighbors
ops-as5712(config)#
```

###Verifying the configuration <a id="lldpconfver"></a> ###
#####Viewing LLDP Global Information
The `show lldp configuration` command displays LLDP configuration information configured above.
```
ops-as5712# show lldp configuration
LLDP Global Configuration:


LLDP Enabled :Yes
LLDP Transmit Interval :120
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
1            Yes                    Yes
10           Yes                    Yes
..................................
..................................
```

#####Viewing LLDP Neighbors
The `show lldp neighbor-info` command displays information about LLDP neighbors.
```
ops-as5712# show lldp neighbor-info

Total neighbor entries : 0
Total neighbor entries deleted : 0
Total neighbor entries dropped : 0
Total neighbor entries aged-out : 0

Local Port     Neighbor Chassis-ID      Neighbor Port-ID         TTL
1
2
3
.......................
.......................
```
The `show lldp neighbor-info <interface>` command shows LLDP neighbors for a particular interface.
```
ops-as5712# show lldp neighbor-info 1
Port                           : 1
Neighbor entries               : 0
Neighbor entries deleted       : 0
Neighbor entries dropped       : 0
Neighbor entries age-out       : 0
Neighbor Chassis-Name          :
Neighbor Chassis-Description   :
Neighbor Chassis-ID            :
Chassis Capabilities Available :
Chassis Capabilities Enabled   :
Neighbor Port-ID               :
TTL                            :
ops-as5712#
```

#####Viewing LLDP statistics
The `show lldp statistics` command displays the LLDP traffic information for the switch.
```
ops-as5712# show lldp statistics
LLDP Global statistics:

Total Packets transmitted : 35
Total Packets received : 0
Total Packet received and discarded : 0
Total TLVs unrecognized : 0
LLDP Port Statistics:
Port-ID   Tx-Packets     Rx-packets     Rx-discarded        TLVs-Unknown
1              34            0                 0                    0
10              0            0                 0                    0
................
................
```

The `show lldp statistics <interface>` command shows LLDP traffic information for a particular interface.

```
ops-as5712# show lldp statistics 1
LLDP statistics:

Port Name: 1
Packets transmitted :36
Packets received :0
Packets received and discarded :0
Packets received and unrecognized :0
ops-as5712#
```

#####Viewing LLDP TLVs
The 'show lldp tlv' command displays the LLDP TLVs to be sent and received.
```
ops-as5712# show lld tlv

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
ops-as5712#
```

###Troubleshooting the configuration <a id="lldptrouble"></a> ###

####Condition
- LLDP Neighbor information is not displayed even if neighbor is present.
- System description is not displayed in neighbor info.

####Cause
- Interface may be down.
- Neighbor may not support LLDP or feature is not enabled.
- system description TLV may not be selected.

####Remedy
- Make interface administratively up by using 'no shutdown' command. Refer physical interface command reference. Neighbor should support LLDP feature and enabled.
- Select system description TLV using 'lldp select-tlv' command.

## CLI <a id="lldpcli"></a> ##
<!--Provide a link to the CLI command related to the feature. The CLI files will be generated to a CLI directory.  -->
Click [here](https://openswitch.net/cli_feat.html#cli_command_anchor) for the CLI commands related to the LLDP feature.
## Related features <a id="lldprelated"></a> ##
When configuring the switch for LLDP, it might also be necessary to configure [Physical Interface](https://openswitch.net./tbd/other_filefeatures/related_feature1.html#first_anchor) so that interface to which neighbor is connected will act as expected.