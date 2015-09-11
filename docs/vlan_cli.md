 VLAN commands
=======


[1. VLAN configuration commands](#vlanconfigcmds)<br>

&nbsp; [1.1 Interface context commands](#vlanintfcxt)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.1 Assigning interface to an access mode VLAN](#vlanaccess)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.2 Removing interface from an access mode VLAN](#novlanaccess)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.3 Assigning a trunk native VLAN to an interface](#vlantrunknative)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.4 Removing a trunk native VLAN from an interface](#novlantrunknative)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.5 Assigning tagging on a native VLAN to an interface](#tagging)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.6 Removing tagging on a native VLAN from an interface](#notagging)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.7 Assigning a VLAN to a trunk on the interface](#vlantrunk)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.1.8 Removing a VLAN from a trunk on the interface](#novlantrunk)<br>
&nbsp;[1.2 VLAN context commands](#vlancxt)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.2.1 Description](#description)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.2.2 Delete description](#nodescription)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.2.3 Turn off VLAN](#noshutdown)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.2.4 Turn on VLAN](#shutdown)<br>
&nbsp;[1.3 Global context commands](#configcxt)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.3.1 creation of VLAN](#vlan)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[1.3.2 deletion of VLAN](#novlan)<br>

[2. VLAN display commands](#vlanshowcmds)<br>

&nbsp;[2.1 Display VLAN summary](#vlansummary)<br>
&nbsp;[2.2 Display VLAN detail information](#vlandetail)<br>



## 1. VLAN configuration commands <a id="vlanconfigcmds"></a> ##
### 1.1 Interface context commands <a id="vlanintfcxt"></a> ###
#### 1.1.1 Assigning an interface to access mode VLAN <a id="vlanaccess"></a> ####
####Syntax :
    vlan access ID
#### Description :
This command assigns the interface to an existing access VLAN represented by 'ID' in the command.
If the VLAN does not exist already, this command throws an error.
####Authority :
All users
#### Parameters :
This command takes ID as a parameter. ID represents VLAN and it takes value from 1 to 4094.
#### Examples
The following commands assign interface 2 to access mode VLAN 20.
>      switch(config)# interface 2
>>    switch(config-if)#no routing<br>
>>    switch(config-if)#vlan access 20

#### 1.1.2 Removing an interface from access mode VLAN <a id="novlanaccess"></a>####
#### Syntax :
    no vlan access
#### Description :
This command will remove the interface from access VLAN represented by ID.
#### Authority :
All users
#### Parameters :
This command does not take a parameter.
#### Examples
>    switch(config)#interface 2
>>    switch(config-if)# no VLAN access

#### 1.1.3 Assigning a trunk native VLAN to an interface <a id="vlantrunknative"></a>####
#### Syntax  :
    vlan trunk native ID
#### Description :
This command assigns a trunk native VLAN represented by ID to an  interface or a LAG interface.
The interface or LAG interface should have routing disabled for this command to work.
#### Authority :
All users
#### Parameters :
This command takes ID as a parameter. ID represents VLAN and takes value from 1 to 4094.
#### Examples
The following commands assign existing trunk native VLAN 20 to interface 2.
>    switch(config)# interface 2
>>    switch(config-if)#no routing<br>
>>    switch(config-if)#vlan trunk native 20

Following commands assign LAG interface 2 to existing trunk native VLAN 20.
>    switch(config)# interface lag 2
>>  switch(config-lag-if)#no routing<br>
>>    switch(config-lag-if)#vlan native trunk 20 <br>

#### 1.1.4 Removing a trunk native VLAN from an interface<a id="novlantrunknative"></a> ####
#### Syntax  :
    no vlan trunk native

#### Description :
This command removes the trunk native VLAN from an interface/LAG interface.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
The following commands remove interface 2 from a trunk native VLAN.
>    switch(config)# interface 2
>>    switch(config-if)#no vlan trunk native

The following commands remove LAG interface 2 from a trunk native VLAN.

>    switch(config)# interface lag 2
>>    switch(config-lag-if)#no vlan trunk native <br>

#### 1.1.5 Assigning tagging on a native VLAN to an interface <a id="tagging"></a> ####
#### Syntax  :
    vlan trunk native tag

#### Description :
This command enables tagging on a native VLAN to an interface or a LAG interface.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
The following commands assign interface 2 to tagged trunk native VLAN .
>    switch(config)# interface 2
>>    switch(config-if)#no routing<br>
>>    switch(config-if)#vlan trunk native tag

The following commands enable tagging on trunk native VLAN on LAG interface 2.

>    switch(config)# interface lag 2
>>    switch(config-if)#no routing<br>
>>    switch(config-lag-if)#vlan trunk native tag <br>


#### 1.1.6 Removing tagging on a native VLAN from an interface <a id="notagging"></a>####
#### Syntax  :
    no vlan trunk native tag

#### Description :
This command disables tagging on a native VLAN on an interface/LAG interface.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
The following commands remove interface 2 from tagged trunk native VLAN.
>    switch(config)# interface 2
>>    switch(config-if)#no vlan trunk native tag

The following commands remove interface LAG 2 to trunk native VLAN 20 which is already created.

>    switch(config)# interface lag 2
>>    switch(config-lag-if)#no vlan trunk native tag <br>

#### 1.1.7 Assigning a VLAN to a trunk on the interface <a id="vlantrunk"></a>####
#### Syntax  :
    vlan trunk allowed ID

#### Description :
This command assigns the VLAN represented by ID to a trunk on the interface / LAG interface. This command expects the interface / LAG interface to be already configured as part of the trunk VLAN.

#### Authority :
All users

#### Parameters :
This command takes ID as a parameter which represents a VLAN and takes the value from 1 to 4094.

#### Examples
The following commands allow VLAN 2 to a trunk, on the interface 2 which is configured as a part of native trunk VLAN 1.

>    switch(config)# interface 2
>>    switch(config-if)#no routing<br>
>>    switch(config-if)#vlan native trunk 1<br>
>>  switch(config-if)#vlan trunk allowed  2

The following commands allow VLAN 2 to a trunk, on the interface LAG 2 which is configured as a part of trunk VLAN 1.

>    switch(config)# interface lag 2
>>    switch(config-if)#no routing<br>
>>    switch(config-lag-if)#vlan native trunk 1 <br>
>>  switch(config-lag-if)#vlan trunk allowed 2



#### 1.1.8 removing a VLAN from a trunk on the interface<a id="novlantrunk"></a> ####
#### Syntax  :
    no vlan trunk allowed ID

#### Description :
This command removes the VLAN represented by ID from a trunk on the interface/LAG interface.

#### Authority :
All users

#### Parameters :
This command takes ID as a parameter which represents a VLAN and takes the value from 1 to 4094.

#### Examples
The following commands remove VLAN 2 from a trunk, on the interface 2 which is configured as a part of native trunk VLAN 1.

>    switch(config)# interface 2
>>  switch(config-if)#no vlan trunk allowed 2

The following commands remove VLAN 2 from a trunk, on the LAG interface 2 which is configured as a part of native trunk VLAN 1.

>    switch(config)# interface lag 2
>>    switch(config-lag-if)#no vlan trunk allowed 2<br>


### 1.3 VLAN context commands <a id="vlancxt"></a>###
#### 1.3.1 Description <a id="description"></a>####
#### Syntax  :
    description WORD

#### Description :
This command adds description to the VLAN.

#### Authority :
all users

#### Parameters :
This command takes *string* parameter which indicates the description of the VLAN.

#### Examples
>   switch(config)# vlan 3
>>  switch(config-vlan)#description TrafficFromX


#### 1.3.2 Removing Description <a id="nodescription"></a>####
#### Syntax  :
    no description

#### Description :
This command removes description of a VLAN.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
>   switch(config)# vlan 3
>>  switch(config-vlan)# no description


#### 1.3.2 Turn on VLAN <a id="noshutdown"></a>####
#### Syntax  :
    no shutdown

#### Description :
This command brings the VLAN 'up'.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
>   switch(config)# vlan 3
>>  switch(config-vlan)# no shutdown



#### 1.3.2 Turn off VLAN <a id="shutdown"></a>####
#### Syntax  :
    shutdown

#### Description :
This command brings the VLAN 'down'.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
>   switch(config)# vlan 3
>>  switch(config-vlan)# shutdown


###1.4 Global context commands ###
#### 1.4.1 creation of VLAN <a id="vlan"></a>####
#### Syntax  :
    vlan ID

#### Description :
This command creates a VLAN with a given ID and sets the admin state of the VLAN to default - *'down'*.

#### Authority :
All users

#### Parameters :
This command takes ID as a parameter which represents the VLAN, which takes values from 1 to 4094.

#### Examples
>   switch(config)# vlan 3
>>  switch(config-vlan)#

#### 1.4.2 deletion of VLAN <a id="novlan"></a>####
#### Syntax  :
    no vlan ID

#### Description :
This command deletes the VLAN with a given ID, which takes values from 1 to 4094.

#### Authority :
All users

#### Parameters :
This command takes ID as a parameter.

#### Examples

>  switch(config-vlan)# no vlan 3
>>  switch(config)#


##2. VLAN display commands <a id="vlanshowcmds"></a>##
### 2.1 Display VLAN summary <a id="vlansummary"></a>###
#### Syntax  :
    show vlan summary

#### Description :
This command displays summary of VLAN configuration.

#### Authority :
All users

#### Parameters :
This command does not take a parameter.

#### Examples
>   switch#show vlan summary<br>
>
    Number of existing VLANs : 2

### 2.2 Display VLAN detail <a id="vlandetail"></a>###
#### Syntax  :
    show vlan {ID}

#### Description :
This command displays VLAN configuration information of all the  existing VLANs in the switch.

#### Authority :
All users

#### Parameters :
This command may or may not take a parameter. By taking ID as a parameter, this command displays configuration details of that specific VLAN.
#### Examples
>   switch#show vlan <br>


|VLAN   |    Name    |     Status  |      Reason  |   Reserved  |  Ports  |
|-------|------------|-------------|--------------|-------------|---------|
| 1     |   default  |   active    |    ADMIN/UP  |   No        |  1,L2   |
| 33    |   asdf     |   active    |    ADMIN/UP  |   L3        |   1     |

>   switch#show vlan 33 <br>

|VLAN   |    Name    |     Status  |      Reason  |   Reserved  |  Ports  |
|-------|------------|-------------|--------------|-------------|---------|
| 33    |   asdf     |   active    |    ADMIN/UP  |   L3        |   1     |
