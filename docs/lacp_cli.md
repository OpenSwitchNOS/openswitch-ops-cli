
#LACP commands
#Table of contents
[TOC]


## 1. LACP configuration commands <a id="lacpconfigcmds"></a> ##
###1.1 Global context commands <a id="configcxt"></a>###
#### 1.1.1 Creation of LAG interface <a id="createlagintf"></a>####
#### Syntax  :
    interface lag ID

#### Description :
This command creates a LAG interface represented by an ID.

#### Authority :
all users

#### Parameters :
This command takes an ID as a parameter which represents a LAG interface. The LAG interface ID can be in the range of 1 to 2000.

#### Examples
>switch(config)# interface lag 100
>>switch(config-lag-if)#

#### 1.1.2 Deletion of LAG interface <a id="dellagintf"></a>####
#### Syntax  :
    no interface lag ID

#### Description :
This command deletes a LAG interface represented by an ID.

#### Authority :
all users

#### Parameters :
This command takes an ID as a parameter which represents a LAG interface. The LAG interface ID can be in the range of 1 to 2000.

#### Examples
>switch(config)# no interface lag 100

#### 1.1.3 Configuring LACP system priority<a id="lacppriority"></a>####
#### Syntax  :
    lacp system-priority <0-65535>

#### Description :
This command sets an LACP system priority.

#### Authority :
all users

#### Parameters :
This command takes a system priority value in the of range 0 to 65535.

#### Examples
>switch(config)# lacp system-priority 100

#### 1.1.4 Configuring default LACP system priority<a id="deflacppriority"></a>####
#### Syntax  :
    no lacp system-priority

#### Description :
This command sets an LACP system priority to a default(65534).

#### Authority :
all users

#### Parameters :
no parameters

#### Examples
>switch(config)# lacp system-priority 100


###1.2 Interface context commands <a id="intfcxt"></a>###
#### 1.2.1 Assigning interface to LAG <a id="inttolag"></a>####
#### Syntax  :
    lag ID

#### Description :
This command adds an interface to a LAG interface specified by an ID.

#### Authority :
all users

#### Parameters :
This command takes an ID as a parameter which represents a LAG interface. The LAG interface ID can be in the range of 1 to 2000.

#### Examples
>switch(config)# interface 1
>>switch(config-if)# lag 100

#### 1.2.2 Removing interface from LAG <a id="rmintfromlag"></a>####
#### Syntax  :
    no lag ID

#### Description :
This command removes an interface from a LAG interface specified by an ID.

#### Authority :
all users

#### Parameters :
This command takes an ID as a parameter which represents a LAG interface. The LAG interface ID can be in the range of 1 to 2000.

#### Examples
>switch(config)# interface 1
>>switch(config-if)# no lag 100

#### 1.2.3 Configuring LACP port-id <a id="lacpportid"></a>####
#### Syntax  :
    lacp port-id <1-65535>

#### Description :
This command sets an LACP port-id value of the interface.

#### Authority :
all users

#### Parameters :
This command takes a port-id value in the range of 1 to 65535.

#### Examples
>switch(config-if)# lacp port-id 10

#### 1.2.4 Configuring LACP port-priority <a id="lacpportpri"></a>####
#### Syntax  :
    lacp port-priority <1-65535>

#### Description :
This command sets an LACP port-priority value for the interface.

#### Authority :
all users

#### Parameters :
This command takes a port-priority value in the range of 1 to 65535.

#### Examples
>switch(config-if)# lacp port-priority 10

###1.3 LAG context commands <a id="lagcxt"></a>###
#### 1.3.1 Entering into LAG context <a id="enterlagcxt"></a>####
#### Syntax  :
    interface lag ID

#### Description :
This command enters into the LAG context of the specified LAG ID. If the specified LAG interface is not present, this command creates a LAG interface and enters it into the LAG context.

#### Authority :
all users

#### Parameters :
This command takes an ID as a parameter which represents a LAG interface. The LAG interface ID can be in the range of 1 to 2000.

#### Examples
>switch(config)# interface 1
>>switch(config-if)# lag 100

#### 1.3.2 Configuring LACP mode <a id="lacpmode"></a>####
#### Syntax  :
    [no] lacp mode {active/passive}

#### Description :
This command sets an LACP mode to active or passive.
The **no** form of the command sets the LACP mode to **off**.

#### Authority :
all users

#### Parameters :
This command takes an **active** or **passive** keyword as an argument to set an LACP mode.

#### Examples
>switch(config)# interface lag 1
>>switch(config-lag-if)# lacp mode active
>>switch(config-lag-if)# no lacp mode active

#### 1.3.3 Configuring hash type <a id="lacphashtype"></a>####
#### Syntax  :
    hash l2-src-dst

#### Description :
This command sets an LACP hash type to l2-src-dst. The default is l3-src-dst.
The **no** form of the command sets an LACP hash type to l3-src-dst.

#### Authority :
all users

#### Parameters :
no parameters.

#### Examples
>switch(config)# interface lag 1
>>switch(config-lag-if)# hash l2-src-dst
>>switch(config-lag-if)# no hash l2-src-dst

#### 1.3.4 Configuring LACP fallback mode <a id="lacpfallback"></a>####
#### Syntax  :
    lacp fallback

#### Description :
This command enables an LACP fallback mode.
The **no** form of the command disables the an LACP fallback mode.

#### Authority :
all users

#### Parameters :
no parameters.

#### Examples
>switch(config)# interface lag 1
>>switch(config-lag-if)# lacp fallback
>>switch(config-lag-if)# no lacp fallback

#### 1.3.5 Configuring LACP rate <a id="lacprate"></a>####
#### Syntax  :
    lacp rate fast

#### Description :
This command sets an LACP heartbeat request time to **fast**. The default is **slow**, which is once every 30 seconds. The **no** form of the command sets an LACP rate to **slow**.

#### Authority :
all users

#### Parameters :
no parameters.

#### Examples
>switch(config)# interface lag 1
>>switch(config-lag-if)# lacp rate fast

###2 LAG display commands <a id="configcxt"></a>###
#### 2.1 Display global LACP configuration <a id="displaylacpconfig"></a>####
#### Syntax  :
    show lacp configuration

#### Description :
This command displays global a LACP configuration.

#### Authority :
all users

#### Parameters :
no parameters

#### Examples
>switch# show lacp configuration
System-id       : 70:72:cf:ef:fc:d9
System-priority : 65534

#### 2.2 Display LACP aggregates <a id="displaylacpagg"></a>####
#### Syntax  :
    show lacp aggregates [lag-name]

#### Description :
This command displays all LACP aggregate information if no parameter is passed. If a LAG name is passed as an argument, it shows information of the specified LAG

#### Authority :
all users

#### Parameters :
This command takes a LAG name as an optional parameter.

#### Examples
>switch# show lacp aggregates lag100
Aggregate-name          : lag100
Aggregated-interfaces :
Heartbeat rate              : slow
Fallback                        : false
Hash                             : l3-src-dst
Aggregate mode           : off

>switch# show lacp aggregates
Aggregate-name          : lag100
Aggregated-interfaces :
Heartbeat rate             : slow
Fallback                       : false
Hash                            : l3-src-dst
Aggregate mode         : off

>Aggregate-name        : lag200
Aggregated-interfaces :
Heartbeat rate             : slow
Fallback                       : false
Hash                             : l3-src-dst
Aggregate mode           : off

#### 2.3 Display LACP interface configuration <a id="displaylacpint"></a>####
#### Syntax  :
    show lacp interfaces [IFNAME]

#### Description :
This command displays an LACP configuration of the physical interfaces. If an interface name is passed as argument, it only displays an LACP configuration of a specified interface.

#### Authority :
all users

#### Parameters :
This command takes an interface name as an optional parameter.

#### Examples
>switch# show lacp interfaces
State abbreviations :
A - Active        P - Passive      F - Aggregable I - Individual
S - Short-timeout L - Long-timeout N - InSync     O - OutofSync
C - Collecting    D - Distributing
X - State m/c expired              E - Default neighbor state
.
Actor details of all interfaces:
\-------------------------------------------
Intf-name    Key    Priority   State
\-------------------------------------------
Aggregate-name : lag100
1 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;500
Aggregate-name : lag200
3
4
2
.
Partner details of all interfaces:
\-------------------------------------------------
Intf-name    Partner  Key    Priority   State
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;port-id
\-------------------------------------------------
Aggregate-name : lag100
1 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;500
Aggregate-name : lag200
3
4
2
