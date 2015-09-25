
BGP Commands Reference
=======================

[TOC]

## BGP Configuration Commands ##

###  router bgp ###

To use the BGP feature, you must first configure BGP router as shown below.

#### Syntax ####

```
[no] router bgp <asn>
```

#### Description ####


This command is used to configure the BGP router. To configure the BGP router, you need the Autonomous System (AS) number. The BGP protocol uses the AS number for detecting whether the BGP connection is internal or external.

#### Authority ####


admin

#### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *asn*  | Required | 1 - 4294967295 | AS number |
| **no** | Optional | Literal | Destroys a BGP router with the specified ASN |

#### Examples ####

```
s1(config)#router bgp 6001
s1(config)#no router bgp 6001
```

### bgp router-id ###


#### Syntax ####


```
[no] bgp router-id <A.B.C.D>
```

#### Description ####



This command specifies the BGP router-ID for a BGP Router.

#### Authority ####



admin

#### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | IPv4 address |
| **no** | Optional | Literal | Deletes BGP Router IP address |

#### Examples ####



```
s1(config-router)# bgp router-id 9.0.0.1
s1(config-router)# no bgp router-id 9.0.0.1
```

### network ###


#### Syntax ####


```
[no] network <A.B.C.D/M>
```

#### Description ####


This command adds the announcement network.

#### Authority ####


admin

#### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D/M*  | Required | A.B.C.D/M | IPv4 address with the prefix length.|
| **no** | Optional | Literal | Removes the announced network for this BGP router. |

#### Examples ####


This configuration example shows that network 10.0.0.0/8 is announced to all neighbors.

```
s1(config-router)# network 10.0.0.0/8
s1(config)# do sh run
Current configuration:
!
router bgp 6001
     bgp router-id 9.0.0.1
     network 10.0.0.0/8
```

### maximum-paths ###


#### Syntax ####


```
[no] maximum-paths <num>
```

#### Description ####


This command sets the maximum number of paths for a BGP route.

#### Authority ####


admin

#### Parameters ####



| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *num*  | Required | 1-255 | Maximum number of paths |
| **no** | Optional | Literal | Sets the maximum number of paths to the default value of 1 |


#### Examples ####


```
s1(config)# router bgp 6001
s1(config-router)# maximum-paths 5
```

### timers bgp ###


#### Syntax ####


```
[no] timers bgp <keepalive> <holdtime>
```

#### Description ####


This command sets the keepalive interval and hold time for a BGP router.

#### Authority ####


admin

#### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *Keepalive*  | Required | 0 - 65535 | The keepalive interval in seconds |
| *holdtime* | Required| 0 - 65535 | Hold time in seconds |
| **no** | Optional | Literal | Resets the keepalive and hold time values to their default values (60 seconds for the keepalive interval and 180 seconds for the hold time value)  |

#### Examples ####


```
s1(config)# router bgp 6001
s1(config-router)# timers bgp 60 30
```

### BGP Neighbor ###


#### neighbor remote-as ####

##### Syntax ####


```
[no] neighbor <A.B.C.D> remote-as <asn>
```

##### Description ####


This command creates a neighbor whose remote-as is *asn*, an autonomous system number. Currently only an IPv4 address is supported.

##### Authority ####


admin

##### Parameters ####



| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *asn* | Required| 1 - 4294967295 |  The autonomous system number of the peer |
| **no** | Optional | Literal | Deletes a configured BGP peer|

##### Examples ####



```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# no neighbor 9.0.0.2 remote-as 6002
```

#### neighbor description ###



##### Syntax ####


```
[no] neighbor <A.B.C.D> description <text>

```
##### Description ####


This command sets the description for the peer.

##### Authority ####


admin

##### Parameters ####



| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *text* | Required| String of maximum length 80 chars| Description of the peer |
| **no** | Optional | Literal | Deletes the peer description|

##### Examples ####


```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 description peer1
```

#### neighbor password ###


##### Syntax ####



```
[no] neighbor <A.B.C.D> password <text>
```

##### Description ####


This command enables MD5 authentication on a TCP connection between BGP peers.

##### Authority ####



admin

##### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *text* | Required| String of maximum length 80 chars| Password for peer connection |
| **no** | Optional | Literal | Disables authentication for the peer connection|

##### Examples ####



```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 password secret
```

#### neighbor timers ###



##### Syntax ####



```
[no] neighbor <A.B.C.D> timers <keepalive> <holdtimer>
```

##### Description ####


This command sets the keepalive interval and hold time for a specific BGP peer.

##### Authority ####


admin

##### Parameters ####



| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *Keepalive*  | Required | 0 - 65535 | The keepalive interval in seconds |
| *holdtime* | Required| 0 - 65535 | Hold time in seconds |
| **no** | Optional | Literal | Resets the keepalive and hold time values to their default values which is 0  |

##### Examples ####


```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 timers 20 10

```
#### neighbor allowas-in ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> allowas-in <val>
```
##### Description ####

This command specifies an allow-as-in occurrence number for an AS to be in the AS path. Issue the `no` command to clear the state.

##### Authority ####

admin
##### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *val* | Required| 1-10| Number of times BGP can allow an instance of AS to be in the AS_PATH  |
| **no** | Optional | Literal | Clears the state |

##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 allowas-in 2
```
#### neighbor remove-private-AS ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> remove-private-AS
```
##### Description ####

This command removes private AS numbers from the AS path in outbound routing updates.
##### Authority ####

admin
##### Parameters ####



| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| **no** | Optional | Literal |Resets to a cleared state (default) |
##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 remove-private-AS
```
#### neighbor soft-reconfiguration inbound ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> soft-reconfiguration inbound
```
##### Description ####

This command enables software-based reconfiguration to generate inbound updates from a neighbor without clearing the BGP session. Issue the `no` command to clear this state.
##### Authority ####

admin
##### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| **no** | Optional | Literal |Resets to a cleared state (default) |

##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 soft-reconfiguration inbound
```
#### neighbor shutdown ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> shutdown
```
##### Description ####

This command shuts down the peer. When you want to preserve the neighbor configuration, but want to drop the BGP peer state, use this syntax.

##### Authority ####

admin
##### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| **no** | Optional | Literal | Deletes the neighbor state of the peer |

##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 shutdown
```
#### neighbor peer-group ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> peer-group <name>
```
##### Description ####

This command assigns a neighbor to a peer-group.

##### Authority ####

admin
##### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *name*  | Required | String of maximum length of 80 chars | Peer-Group name |
| **no** | Optional | Literal |Removes the neighbor from the peer-group |
##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 peer-group pg1
```
#### neighbor route-map ###

##### Syntax ####

```
[no] neighbor <A.B.C.D> route-map <name> in|out
```
##### Description ####

This command applies a route-map on the neighbor for the direction given (in or out).

##### Authority ####

admin
##### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Required | A.B.C.D | Peer IPv4 address |
| *name*  | Required | String of maximum length of 80 chars | Route-map name |
| **no** | Optional | Literal |Removes the route-map for this neighbor |

##### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 route-map rm1 in
```
### neighbor peer-group ###

#### Syntax ####

```
[no] neighbor <word> peer-group
```
#### Description ####

This command creates a new peer-group.
#### Authority ####

admin
#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *name*  | Required | String of maximum length of 80 chars | Peer-group name |
| **no** | Optional | Literal |Deletes a peer-group |

#### Examples ####

```
s1(config)# router bgp 6001
s1(config-router)# neighbor pg1 peer-group
```

## Route Map Configuration Commands ##
### route-map ###

#### Syntax ####

```
[no] route-map <name> <deny | permit> <order>
```
#### Description ####

This command configures the order of the entry in the route-map name with the ‘Match Policy’ of either permit or deny.

#### Authority ####

admin
#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *name*  | Required | String of maximum length of 80 chars | Route-map name |
| *order*  | Required |1-65535| Order number of the route-map |
| **deny** | Required | Literal | Denies the order of the entry |
| **permit** | Required | Literal | Permits the order of the entry |
| **no** | Optional | Literal |Deletes the route-map |

#### Examples ####

```
s1(config)# route-map rm1 deny 1
```

### Route Map Match ###

#### Syntax ####

```
Route-map Command: match ip address prefix-list <name>
```
#### Description ####

This command configures a match clause for route map.
#### Authority ####

admin
#### Parameters ####


| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *name*  | Required | String of maximum length of 80 chars | IP prefix-list name |
| **no** | Optional | Literal |Deletes match clause for route-map |

#### Examples ####

```
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# match ip address prefix-list PLIST1
```
### Route Map Set ###

#### Syntax ####


```
Route-map Command: [no] set community <AA:NN> [additive]
Route-map Command: [no] set metric <val>
```

#### Description ####

Use the `set community` command to set the BGP community attribute. Use the `set metric` command to set the BGP attribute MED.
#### Authority ####

admin
#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *AA:NN*  | Required | AS1:AS2 where AS is 1 - 4294967295 | Sets BGP community attribute |
| *val*  | Required |0-4294967295  | Sets metric value |
| **no** | Optional | Literal |Clears community attribute |

#### Examples ####

```
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# set community 6001:7002 additive
s1(config-route-map)# set metric 100
s1(config-route-map)# no set metric 100
```
### Route Map Description ###

#### Syntax ####

```
Route-map Command: [no] description <text>
```
#### Description ####

Sets Route-map description.
#### Authority ####

admin
#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *text*  | Required | String of maximum length of 80 chars | Route-map description |
| **no** | Optional | Literal |Clears description for route-map |

#### Examples ####

```
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# description rmap-mcast
```

## IP Prefix-list Configuration Commands ##
###  IP prefix-list ###
#### Syntax ####

```
[no] ip prefix-list <name> seq <num> deny|permit <A.B.C.D/M|any>
no ip prefix-list <name>
```
#### Description ####

The `ip prefix-list` command provides a powerful prefix-based filtering mechanism. It has prefix length range  and sequential number specifications. You can add or delete prefix-based filters to arbitrary points of a prefix-list by using a sequential number specification. If `no ip prefix-list` is specified, it acts as permit. If `ip prefix-list` is defined, and no match is found, the default deny is applied.

#### Authority ####

admin

#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *name*  | Required | String of maximum length of 80 chars | IP prefix-list name |
| *num*  | Required | 1-4294967295 | Sequence number |
| *A.B.C.D/M*  | Required | A.B.C.D/M | IPv4 prefix |
| **no** | Optional | Literal |Deletes IP prefix-list |

#### Examples ####

```
s1(config)# ip prefix-list PLIST1 seq 5 deny 11.0.0.0/8
s1(config)# ip prefix-list PLIST2 seq 10 permit 10.0.0.0/8
s1(config)# no ip prefix-list PLIST1 seq 5 deny 11.0.0.0/8
s1(config)# no ip prefix-list PLIST2
```
##Display Commands ##

### show ip bgp ###



#### Syntax ####



```
show ip bgp [A.B.C.D][A.B.C.D/M]
```

#### Description ####


This command displays BGP routes from the BGP Route table. When no route is specified, all IPv4 routes are displayed.
#### Authority ####

admin
#### Parameters ####

| Parameter | Status   | Syntax |	Description          |
|-----------|----------|----------------------|
| *A.B.C.D*  | Optional | A.B.C.D | IPv4 prefix |
| *A.B.C.D/M*  | Optional | A.B.C.D/M | IPv4 prefix with prefix length |

#### Examples ####

```ditaa
s1# show ip bgp
Status codes: s suppressed, d damped, h history, * valid, > best, = multipath,
              i internal, S Stale, R Removed
Origin codes: i - IGP, e - EGP, ? - incomplete

Local router-id 9.0.0.1
   Network          Next Hop            Metric LocPrf Weight Path
*> 11.0.0.0/8       0.0.0.0                  0      0  32768  i
*> 12.0.0.0/8       10.10.10.2               0      0      0 2 5 i
*  12.0.0.0/8       20.20.20.2               0      0      0 3 5 i
*  12.0.0.0/8       30.30.30.2               0      0      0 4 5 i
Total number of entries 4
```
### show ip bgp summary ###

#### Syntax ####

```
show ip bgp summary
```
#### Description ####

The command provides a summary of the BGP neighbor status.

#### Authority ####

admin
#### Parameters ####

None
#### Examples ####

```ditaa
s1# show ip bgp summary
BGP router identifier 9.0.0.1, local AS number 1
RIB entries 2
Peers 1

Neighbor             AS MsgRcvd MsgSent Up/Down  State
9.0.0.2               2       4       5 00:00:28 Established
```
###  show bgp neighbors ###
#### Syntax ####

```
show bgp neighbors
```

#### Description ####

This command displays detailed information about BGP neighbor connections.
#### Authority ####

admin
#### Parameters ####

None
#### Examples ####

```ditaa
s1# show bgp neighbors
  name: 9.0.0.2, remote-as: 6002
    state: undefined
    shutdown: yes
    description: peer1
    capability: undefined
    local_as: undefined
    local_interface: undefined
    inbound_soft_reconfiguration: yes
    maximum_prefix_limit: undefined
    tcp_port_number: undefined
    statistics:
  name: pg1, remote-as: undefined
    state: undefined
    shutdown: undefined
    description: undefined
    capability: undefined
    local_as: undefined
    local_interface: undefined
    inbound_soft_reconfiguration: undefined
    maximum_prefix_limit: undefined
    tcp_port_number: undefined
    statistics:
```
