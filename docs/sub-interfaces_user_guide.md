#L3 Subinterfaces
<!-- TOC depth:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [L3 Subinterfaces](#l3-subinterfaces)
	- [Overview ##](#overview-)
	- [How to use the feature ##](#how-to-use-the-feature-)
		- [Setting up the basic configuration](#setting-up-the-basic-configuration)
		- [Setting up the optional configuration](#setting-up-the-optional-configuration)
		- [Verifying the configuration](#verifying-the-configuration)
		- [Troubleshooting the configuration](#troubleshooting-the-configuration)
			- [Condition](#condition)
			- [Cause](#cause)
			- [Remedy](#remedy)
	- [CLI ##](#cli-)
	- [Related features ##](#related-features-)
<!-- /TOC -->

## Overview ##
L3 Subinterfaces are used to support router-on-a-stick configurations. Using these, one can configure separation of traffic on a L3 physical interface based on VLAN and  apply policies on them.

An example use of L3 subinterfaces in a data center deployment is shown in this diagram. An L3 interface of a TOR switch is connected to trunk port of a switch. All the outgoing traffic from the TOR switch's L3 interface is tagged with a VLAN ID to enable the switch to forward the traffic on different VLANs. This is configured by creating L3 subinterfaces on the TOR switch and configuring the routing tables to route the outgoing traffic on one of these subinterfaces while applying a different VLAN tag on each subinterface.

             +----------------------------+                   +------------------------------+
             |                            |                   |                              |
             |                            |                   |                              |
             |                            |                   |                              |
             |   OpenSwitch              +-+ VLAN1(Subintf1) +-+         L2 Switch           |
             |                    L3 I/F |||<--------------->|||Trunk                        |
             |                           |||<--------------->|||Port                         |
             |                           +-+ VLAN2(Subintf2) +-+                             |
             |                            |                   |                              |
             |                            |                   |                              |
             |                            |                   |                              |
             +----------------------------+                   +------------------------------+

Following are the restrictions applicable to subinterfaces:

* A subinterface cannot be assigned an IP address already used by any interfaces on the switch.

## How to use the feature ##

###Setting up the basic configuration

 1. Configure an interface as L3
 2. give a no shut at parent interface
 3. Create subinterface on the L3 interface
 4. Assign dot1q encapsulation
 5. Assign IP address
 6. give a no shut
 7. Enable subinterface

###Setting up the optional configuration

 1. None

###Verifying the configuration

 1. Display configured subinterfaces

###Troubleshooting the configuration
#### Condition
Unable to create subinterface.
#### Cause
The interface may be configured as an L2.
#### Remedy
Configure the interface as an L3 using the `routing` command.
## CLI ##
Click [CLI-TBL](https://openswitch.net/cli_feature_name.html#cli_command_anchor) for the CLI commands related to the L3 subinterface feature.
## Related features ##
When configuring the switch for L3 subinterface feature, it might also be necessary to configure interface (http://www.openswitch.net/documents/user/layer3_interface_cli#routing) so that parent inuterface is a L3 interface.
