VLANs
=======
[1. Overview ](#vlanoverview)<br>
[2. Configuring and verifying VLAN](#vlanconfig)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[2.1 Setting up basic configuration](#vlanbasic)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.1 Create a VLAN](#vlancreate)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.2 Delete a VLAN](#vlandelete)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.3 Set admin state of a VLAN](#vlanadminstate)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.4 Add access VLAN ](#vlanaddaccess)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.5 Remove access VLAN ](#vlanremaccess)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.6 Add  a trunk native VLAN ](#vlanaddtrunknative)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.7 Remove a trunk native VLAN ](#vlanremtrunknative)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.8 Add a trunk VLAN ](#vlanaddtrunk)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.9 Remove a trunk VLAN  ](#vlanremtrunk)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.10 Add tagging on native VLAN ](#vlanaddnativetag)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.1.11 Remove tagging on native VLAN ](#vlanremnativetag)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[2.2 Setting up optional configuration ](#vlanoptional)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.2.1 Set description to the VLAN ](#vlanoptdescription)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[2.3 Verifying the configuration](#verifyvlanconfig)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.3.1 Viewing VLAN summary](#viewvlansummary)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[2.3.2 Viewing VLAN detailed information](#viewvlandetail)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[3. CLI ](#vlancli)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[4. Related features](#vlanrelatedfeatures)<br>

##1. Overview <a id="vlanoverview"></a>##
 <p> A VLAN (Virtual Local Area Network) is any broadcast domain that is partitioned and isolated in a computer network at the data link layer. Packets destined within a VLAN, whether in a same switch or different will be forwarded whereas packets destined to a different VLAN, whether within a switch or in a different switch will be routed. <br>
 The main use of VLANs is to ***provide network segmentation***.VLANs also address issues such as *scalability*,*security* and *network management*.

##2. Configuring and verifying VLAN <a id="vlanconfig"></a>
###2.1 Setting up the basic configuration <a id="vlanbasic"></a>

####2.1.1 Create a VLAN <a id="vlancreate"></a>

 'configure terminal' command changes the vtysh context to config.
>switch# config terminal<br>
>switch(config)#

 'vlan ID' command creates a VLAN with the given ID and changes the vtysh context to VLAN. If the VLAN already exists then it changes the context to VLAN.
> switch(config)# vlan 12<br>
>> switch(config-vlan)#

'no shutdown' command lets the user bring the VLAN 'up'
> switch(config-vlan)#no shutdown<br>
> switch(config)#end
> switch#

####2.1.2 Delete a VLAN <a id="vlandelete"></a>

 'configure terminal' command changes the vtysh context to config.
>switch# config terminal<br>
> switch(config)#


 'no vlan ID' command deletes the VLAN.
> switch(config)# no vlan 12<br>
> switch(config)#end<br>
> switch#


####2.1.3 Set admin state of a VLAN <a id="vlanadminstate"></a>

'configure terminal' command changes the vtysh context to config.
>switch# config terminal<br>
> switch(config)#

 'vlan ID' command creates a VLAN.
> switch(config)# vlan 12<br>
> switch(config-vlan)#

'no shutdown' sets the admin state of a VLAN to 'up'.
>switch(config-vlan)# no shutdown
>switch(config-vlan)#


####2.1.4. Add access VLAN  <a id="vlanaddaccess"></a>


'configure terminal' command changes the vtysh context to config.
> switch# config terminal<>
> switch(config)#

'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no routing' command disables routing on the interface. VLAN mandates the interface to be a non-routing interface.
>switch(config-if)#no routing<br>
>switch(config-if)#


>switch(config-lag-if)#no routing<br>
>switch(config-lag-if)#

'vlan access ID' command adds the access VLAN to the interface. If the interface is already associated with one access VLAN then this command override the previous configuration. At a time there can only be one access VLAN associated with the interface.
>switch(config-if)#vlan access 1<br>
>switch(config-if)#end<br>
>switch#

>switch(config-lag-if)#vlan access 1<br>
>switch(config-lag-if)#end<br>

####2.1.5. Remove access VLAN  <a id="vlanremaccess"></a>

‘configure terminal’ command changes the context to config.
> switch# config terminal<br>
> switch(config)#

'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#

'no vlan access' command removes the access VLAN from the interface.
> switch(config-if)#no vlan access<br>
> switch(config-if)#end
> switch#

> switch(config-lag-if)#no vlan access<br>
> switch(config-lag-if)#end
> switch#

####2.1.6. Add trunk native VLAN  <a id="vlanaddtrunknative"></a>

‘configure terminal’ command changes the context to config
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no routing' command disables routing on the interface. VLAN mandates the interface to be a non-routing interface.
>switch(config-if)#no routing<br>
>switch(config-if)#


>switch(config-lag-if)#no routing<br>
>switch(config-lag-if)#

'vlan trunk native ID' command adds trunk native VLAN  to the interface. With this configuration on the interface, all the untagged packets will be allowed in native VLAN.
> switch(config-if)#vlan trunk native 1<br>
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#vlan trunk native 1<br>
> switch(config-lag-if)#end<br>
> switch#
####2.1.7. Remove trunk native VLAN  <a id="vlanremtrunknative"></a>

'configure terminal' command changes the context to config.
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#

'no vlan trunk native' command removes trunk native VLAN  from the interface.
> switch(config-if)#no vlan trunk native <br>
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#no vlan trunk native <br>
> switch(config-lag-if)#end<br>
> switch#


####2.1.8. Adding trunk VLAN  <a id="vlanaddtrunk"></a>
'configure terminal' command changes the context to config
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no routing' command disables routing on the interface. VLAN mandates the interface to be a non-routing interface.
>switch(config-if)#no routing<br>
>switch(config-if)#


>switch(config-lag-if)#no routing<br>
>switch(config-lag-if)#

'vlan trunk allowed ID' command lets the user specify the VLAN allowed in the trunk. Multiple VLANs can be allowed on a trunk.
> switch(config-if)#vlan trunk allowed 1<br>
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#vlan trunk allowed 1<br>
> switch(config-lag-if)#end<br>
> switch#


####2.1.9. Removing trunk VLAN  <a id="vlanremtrunk"></a>
'configure terminal' command changes the context to config
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no vlan trunk allowed ID' command removes the VLAN specified by ID from the trunk allowed list.
> switch(config-if)#no vlan trunk allowed 1<br>
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#no vlan trunk allowed 1<br>
> switch(config-lag-if)#end<br>
> switch#
####2.1.10. Add tagging on a native VLAN <a id="vlanaddnativetag"></a>
'configure terminal' command changes the context to config
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no routing' command disables routing on the interface. VLAN mandates the interface to be a non-routing interface.
>switch(config-if)#no routing<br>
>switch(config-if)#


>switch(config-lag-if)#no routing<br>
>switch(config-lag-if)#

'vlan trunk native tag' command enables tagging on native VLAN for the egress packets on the interface.
> switch(config-if)#vlan trunk native tag
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#vlan trunk native tag
> switch(config-lag-if)#end<br>
> switch#
####2.1.10 Remove tagging on native VLAN <a id="vlanremnativetag"></a>
'configure terminal' command changes the context to config
> switch# config terminal<br>
> switch(config)#


'interface *IFNAME*' command enters interface context, where *IFNAME* is name of the interface. <br>

> switch(config)# interface 21<br>
> switch(config-if)<br>

'interface lag *IFNAME*' command enters the interface LAG context.

>switch(config)#interface lag 21<br>
>switch(config-lag-if)#


'no vlan trunk native tag' command disables tagging on native VLAN.
> switch(config-if)#vlan trunk native tag
> switch(config-if)#end<br>
> switch#

> switch(config-lag-if)#vlan trunk native tag
> switch(config-lag-if)#end<br>
> switch#

###2.2Setting up the optional configuration <a id="vlanoptional"></a>

####2.2.1.Set description to the VLAN <a id="vlanoptdescription"></a>
'configure terminal' command changes the context to config.
> switch# config terminal<br>
> switch(config)#

The following command changes the context to VLAN and lets the user configure the VLAN.
> switch(config)# vlan 12<br>
>> switch(config-vlan)#

The optional 'description' command lets the user give a description to the VLAN.
> switch(config-vlan)#description testvlan<br>
> switch(config)#

###2.3Verifying the configuration <a id="verifyvlanconfig"></a>

####2.3.1. Viewing VLAN summary <a id="viewvlansummary"></a>
'show vlan summary' displays the summary of VLANs for the following configured VLANs.
>switch# show running-config

&nbsp;&nbsp;&nbsp;&nbsp;Current configuration:<br>
&nbsp;&nbsp;&nbsp;&nbsp;!<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 3003<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 1<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test1<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 1212<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test1212<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 33<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test33<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 2<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;   no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test2<br>
&nbsp;&nbsp;&nbsp;&nbsp;interface bridge_normal<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  no routing<br>


>switch# show vlan summary <br>
>>Number of existing VLANs: 5

####2.3.2.Viewing VLAN detailed information <a id="viewvlandetail"></a>

'show vlan' shows detailed VLAN configurations.
'show vlan ID' shows detailed configuration of a specific VLAN for the following configurations.
>switch#show running-config


&nbsp;&nbsp;&nbsp;&nbsp;Current configuration:<br>
&nbsp;&nbsp;&nbsp;&nbsp;!<br>

&nbsp;&nbsp;&nbsp;&nbsp;vlan 3003<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 1<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test1<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 1212<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test1212<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 33<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test33<br>
&nbsp;&nbsp;&nbsp;&nbsp;vlan 2<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;   no shutdown<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    description test2<br>
&nbsp;&nbsp;&nbsp;&nbsp;interface bridge_normal<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  no routing<br>
&nbsp;&nbsp;&nbsp;&nbsp;interface 2<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    no routing<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    vlan trunk native 1<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    vlan trunk allowed 33<br>
&nbsp;&nbsp;&nbsp;&nbsp;interface 1<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;   no routing<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;   vlan access 1<br>

>switch#show vlan
|-----|-------------|---------|-------------|------------|------------------------|
|VLAN | Name        | Status  |  Reason     |   Reserved |      Ports |
|-----|-------------|---------|-------------|------------|------------------------|
|3003 | vlan3003    | down    | admin_down  |   (null)   |                |
|1    | vlan1       | up      | ok          |   (null)   |      2, 1 |
|1212 | vlan1212    | up      | ok          |   (null)   |       |
|33   | vlan33      | up      | ok          |   (null)   |      2|
|2    | vlan2       | up      | ok          |   (null)   |      |


>switch#show vlan 33
|-----|-------------|---------|-------------|------------|------------------------|
|VLAN | Name        | Status  |  Reason     |   Reserved |      Ports |
|-----|-------------|---------|-------------|------------|------------------------|
|33   | vlan33      | up      | ok          |   (null)   |      2|



## 3. CLI <a id="vlancli"></a> ##
<!--Provide a link to the CLI command related to the feature. The CLI files will be generated to a CLI directory.  -->
Click [here](https://openswitch.net/cli_feature_name.html#cli_command_anchor) for the CLI commands related to the VLAN.
## 4. Related features <a id="vlanrelatedfeatures"></a> ##
(<!-- Enter content into this section to describe features that may need to be considered in relation to this particular feature, under what conditions and why.  Provide a hyperlink to each related feature.  Sample text is included below as a potential example or starting point.  -->
When configuring the switch for VLAN, it might also be necessary to configure [Physical Interface](https://openswitch.net./tbd/other_filefeatures/related_feature1.html#first_anchor) so that VLAN added to the interfaces behaves as expected. [LACP](https://openswitch.net./tbd/other_filefeatures/related_feature1.html#first_anchor)
