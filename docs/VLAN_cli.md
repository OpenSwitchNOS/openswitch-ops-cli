VLAN commands
======

[TOC]

##  VLAN configuration commands <a id="vlanconfigcmds"></a> ##
### Interface context commands <a id="vlanintfcxt"></a> ###
#### Assigning an interface to access mode VLAN <a id="vlanaccess"></a> ####
####Syntax
`vlan access <vlanid>`
#### Description
This command assigns the interface to an existing access VLAN represented by 'ID' in the command.
If the VLAN does not exist already, this command throws an error.
####Authority
All users.
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
The following commands assign interface 2 to access mode VLAN 20.
```
switch(config)# interface 2
switch(config-if)#no routing<br>
switch(config-if)#vlan access 20
```

#### Removing an interface from access mode VLAN <a id="novlanaccess"></a>####
#### Syntax
`no vlan access [<vlanid>]`
#### Description
This command will remove the interface from access VLAN represented by ID.
#### Authority
All users.
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Optional | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
```
switch(config)#interface 2
switch(config-if)# no VLAN access
```
OR
```
switch(config-if)# no VLAN access 20
```

#### Assigning a trunk native VLAN to an interface <a id="vlantrunknative"></a>####
#### Syntax
`vlan trunk native <vlanid>`
#### Description
This command assigns a trunk native VLAN represented by ID to an  interface or a LAG interface.Interface or LAG interface should have routing disabled for this command to work.
#### Authority
All users.
#### Parameters :

| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |
#### Examples
```
switch(config)# interface 2
switch(config-if)#no routing
switch(config-if)#vlan trunk native 20
```
```
switch(config)# interface lag 2
switch(config-lag-if)#no routing
switch(config-lag-if)#vlan native trunk 20
```

#### Removing a trunk native VLAN from an interface<a id="novlantrunknative"></a> ####
#### Syntax
`no vlan trunk native [<vlanid>]`
#### Description
This command removes the trunk native VLAN from an interface/LAG interface.
#### Authority
All users.
#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Optional | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |
#### Examples
```
switch(config)# interface 2
switch(config-if)#no vlan trunk native
```
```
switch(config)# interface lag 2
switch(config-lag-if)#no vlan trunk native
```

#### Assigning tagging on a native VLAN to an interface <a id="tagging"></a> ####
#### Syntax
`vlan trunk native tag`
#### Description
This command enables tagging on a native VLAN to an interface or a LAG interface.
#### Authority
All users.
#### Parameters
This command does not take a parameter.
#### Examples
```
switch(config)# interface 2
switch(config-if)#no routing
switch(config-if)#vlan trunk native tag
```
```
switch(config)# interface lag 2
switch(config-if)#no routing
switch(config-lag-if)#vlan trunk native tag
```


####Removing tagging on a native VLAN from an interface <a id="notagging"></a>####
#### Syntax
`no vlan trunk native tag`

#### Description
This command disables tagging on a native VLAN on an interface/LAG interface.

#### Authority
All users.

#### Parameters
This command does not take a parameter.

#### Examples
The following commands remove interface 2 from tagged trunk native VLAN.
switch(config)# interface 2
switch(config-if)#no vlan trunk native tag

The following commands remove interface LAG 2 to trunk native VLAN 20 which is already created.

switch(config)# interface lag 2
switch(config-lag-if)#no vlan trunk native tag <br>

#### Assigning a VLAN to a trunk on the interface <a id="vlantrunk"></a>####
#### Syntax
`vlan trunk allowed <vlanid>`

#### Description
This command assigns the VLAN represented by ID to a trunk on the interface / LAG interface. This command expects the interface / LAG interface to be already configured as part of the trunk VLAN.

#### Authority
All users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
```
switch(config)# interface 2
switch(config-if)#no routing<br>
switch(config-if)#vlan native trunk 1<br>
switch(config-if)#vlan trunk allowed  2
```
```
switch(config)# interface lag 2
switch(config-if)#no routing<br>
switch(config-lag-if)#vlan native trunk 1 <br>
switch(config-lag-if)#vlan trunk allowed 2
```

#### removing a VLAN from a trunk on the interface<a id="novlantrunk"></a> ####
#### Syntax
`no vlan trunk allowed [<vlanid>]`

#### Description
This command removes the VLAN represented by ID from a trunk on the interface/LAG interface.

#### Authority
All users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
```
switch(config)# interface 2
switch(config-if)#no vlan trunk allowed 2
```
```
switch(config)# interface lag 2
switch(config-lag-if)#no vlan trunk allowed 2
```


### VLAN context commands <a id="vlancxt"></a>###
####  Description <a id="description"></a>####
#### Syntax
`description <description>`

#### Description
This command adds description to the VLAN.

#### Authority
all users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *description* | Required | Literal | Adds description to the VLAN |

#### Examples
```
switch(config)# vlan 3
switch(config-vlan)#description TrafficFromX
```

#### Removing Description <a id="nodescription"></a>####
#### Syntax
`no description [<description>]

#### Description
This command removes description of a VLAN.

#### Authority
All users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *description* | Optional | Literal | Adds description to the VLAN |


#### Examples
```
switch(config)# vlan 3
switch(config-vlan)# no description
```


#### Turn on VLAN <a id="noshutdown"></a>####
#### Syntax
`no shutdown`

#### Description
This command brings the VLAN 'up'.

#### Authority
All users.

#### Parameters
This command does not take a parameter.

#### Examples
```
switch(config)# vlan 3
switch(config-vlan)# no shutdown
```
#### Turn off VLAN <a id="shutdown"></a>####
#### Syntax
`shutdown`

#### Description
This command brings the VLAN 'down'.

#### Authority
All users.

#### Parameters
This command does not take a parameter.

#### Examples
```
switch(config)# vlan 3
switch(config-vlan)# shutdown
```
### Global context commands ###
#### Create a VLAN <a id="vlan"></a>####
#### Syntax
`vlan <vlanid>`

#### Description
This command creates a VLAN with a given ID and sets the admin state of the VLAN to default - *'down'*.

#### Authority
All users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
```
switch(config)# vlan 3
switch(config-vlan)#
```

#### Deletion a VLAN <a id="novlan"></a>####
#### Syntax
`no vlan < vlanid >`

#### Description
This command deletes the VLAN with a given ID, which takes values from 1 to 4094.

#### Authority
All users.

#### Parameters
| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |
#### Examples
```
switch(config-vlan)# no vlan 3
switch(config)#
```
## VLAN display commands <a id="vlanshowcmds"></a>##
### Display VLAN summary <a id="vlansummary"></a>###
#### Syntax
`show vlan summary`

#### Description
This command displays summary of VLAN configuration.

#### Authority
All users.

#### Parameters
This command does not take a parameter.

#### Examples
`
switch#show vlan summary
`
```
Number of existing VLANs : 2
```
### Display VLAN detail <a id="vlandetail"></a>###
#### Syntax
`show vlan [< vlanid >]`

#### Description
This command displays VLAN configuration information of all the  existing VLANs in the switch.

#### Authority
All users

#### Parameters
This command may or may not take a parameter.

| Parameter | Status   | Syntax         | Description                           |
|:-----------|:----------|:----------------:|:---------------------------------------|
| *vlanid* | Required | 1 - 4094 | Represents VLAN and takes values from 1 to 4094 |

#### Examples
`
switch#show vlan
`

```
|VLAN   |    Name    |     Status  |      Reason  |   Reserved  |  Ports  |
|-------|------------|-------------|--------------|-------------|---------|
| 1     |   default  |   active    |    ADMIN/UP  |   No        |  1,L2   |
| 33    |   asdf     |   active    |    ADMIN/UP  |   L3        |   1     |
```

`
switch#show vlan 33
`
```
|VLAN   |    Name    |     Status  |      Reason  |   Reserved  |  Ports  |
|-------|------------|-------------|--------------|-------------|---------|
| 33    |   asdf     |   active    |    ADMIN/UP  |   L3        |   1     |
```
