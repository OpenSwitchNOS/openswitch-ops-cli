


> Written with [StackEdit](https://stackedit.io/).
<!--  See the https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet for additional information about markdown text.
Here are a few suggestions in regards to style and grammar:
* Use active voice. With active voice, the subject is the doer of the action. Tell the reader what
to do by using the imperative mood, for example, Press Enter to view the next screen. See https://en.wikipedia.org/wiki/Active_voice for more information about the active voice.
* Use present tense. See https://en.wikipedia.org/wiki/Present_tense for more information about using the present tense.
* Avoid the use of I or third person. Address your instructions to the user. In text, refer to the reader as you (second person) rather than as the user (third person). The exception to not using the third-person is when the documentation is for an administrator. In that case, *the user* is someone the reader interacts with, for example, teach your users how to back up their laptop.
* See https://en.wikipedia.org/wiki/Wikipedia%3aManual_of_Style for an online style guide.
Note regarding anchors:
--StackEdit automatically creates an anchor tag based off of each heading.  Spaces and other nonconforming characters are substituted by other characters in the anchor when the file is converted to HTML.
 -->
BGP Commands Reference
=======================
<!--Provide the name of the grouping of commands, for example, LLDP commands-->

 [TOC]

## BGP Configuration Commands ##
<!-- Change LLDP -->
###  BGP Router ###
To use BGP feature, you must first configure BGP router as shown below.
#### Syntax ####
router bgp *asn*
no router bgp *asn*
<!--For example,    myprogramstart [option] <process_name> -->
#### Description ####
<!--Provide a description of the command. -->
To configure BGP router, you need Autonomous System (AS) number. BGP protocol uses the AS number for detecting whether the BGP connection is internal one or external one.
Issue the `no` command to destroy a BGP router with the specified asn.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
AS number in the range of 1 - 4294967295.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)#router bgp 6001
s1(config)#no router bgp 6001
### BGP Router-id ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
bgp router-id *A.B.C.D*
no bgp router-id *A.B.C.D*
#### Description ####
<!--Provide a description of the command. -->
This command specifies the BGP router-ID for a BGP Router.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
IPv4 address
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config-router)# bgp router-id 9.0.0.1
s1(config-router)# no bgp router-id 9.0.0.1
### BGP Network ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
network *A.B.C.D/M*
no network *A.B.C.D/M*
#### Description ####
<!--Provide a description of the command. -->
This command adds the announcement network. This configuration example says that network 10.0.0.0/8 will be announced to all neighbors.
Issue the `no` command to remove the announced network for this BGP router.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
IPv4 network address with prefix length.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config-router)# network 10.0.0.0/8
s1(config)# do sh run
Current configuration:
!
router bgp 6001
     bgp router-id 9.0.0.1
     network 10.0.0.0/8
### BGP Maximum paths ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
maximum-paths *num*
#### Description ####
<!--Provide a description of the command. -->
Sets the maximum number of paths for a BGP route.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Integer in the range 1-255.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# maximum-paths 5
###  BGP Timers ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
timers bgp *keepalive* *holdtime*

#### Description ####
<!--Provide a description of the command. -->
Sets the keepalive interval and hold time for a BGP router.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Keepalive interval in the range of 0-65535
Hold time in the range of 0-65535
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# timers bgp 60 30

### BGP Neighbor ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Create Neighbor ####
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* remote-as *asn*
no neighbor *A.B.C.D* remote-as *asn*
##### Description ####
<!--Provide a description of the command. -->
Creates a new neighbor whose remote-as is *asn*, Autonomous System Number. Currently only IPv4 address is supported.
Issue the `no` command to delete a configured BGP peer.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
Peer Autonomous System (AS) Number in range of 1 - 4294967295.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# no neighbor 9.0.0.2 remote-as 6002
#### Neighbor Description ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* description *text*
no neighbor *A.B.C.D* description *text*
##### Description ####
<!--Provide a description of the command. -->
Sets description for the peer.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
String of maximum length 80 characters.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 description peer1
#### Neighbor Password ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor  *A.B.C.D* password *text*
no neighbor  *A.B.C.D* password *text*
##### Description ####
<!--Provide a description of the command. -->
Enables MD5 authentication on TCP connection between BGP peers. Issue the `no` command to disable authentication.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
Password is a string of maximum length of 80 characters.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 password secret
#### Neighbor Timers ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* timers *keepalive* *holdtimer*
no neighbor *A.B.C.D* timers *keepalive* *holdtimer*
##### Description ####
<!--Provide a description of the command. -->
Sets the keepalive interval and hold time for a specific BGP peer. To clear the timers use the `no` command.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Keepalive interval in the range of 0-65535.
Hold time in the range of 0-65535.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 timers 20 10
#### Neighbor Allow-as-in ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* allowas-in <0-10>
##### Description ####
<!--Provide a description of the command. -->
Allow-as-in is the number of times BGP can allow an instance of AS to be in the AS_PATH. Issue the `no` command to clear the state.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
Integer in the range 1-10.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 allowas-in 2
#### Neighbor Remove-private-AS ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* remove-private-AS
##### Description ####
<!--Provide a description of the command. -->
Removes private AS numbers from the AS path in outbound routing updates. Issue the `no` command to clear this state.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 remove-private-AS
#### Neighbor Soft-reconfiguration ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* soft-reconfiguration inbound
no neighbor *A.B.C.D* soft-reconfiguration inbound
##### Description ####
<!--Provide a description of the command. -->
Enables software-based reconfiguration to generate inbound updates from a neighbor without clearing the BGP session. Issue the `no` command to clear this state.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 soft-reconfiguration inbound
#### Neighbor Shutdown ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* shutdown
no neighbor *A.B.C.D* shutdown
##### Description ####
<!--Provide a description of the command. -->
Shuts down the peer. When you want to preserve the neighbor configuration, but want to drop the BGP peer state, use this syntax. Using `no neighbor peer remote-as as-number` will delete all neighbor configuration state but the configuration will also be deleted.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 shutdown
#### Neighbor Peer-group ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* peer-group *name*
no neighbor *A.B.C.D* peer-group *name*
##### Description ####
<!--Provide a description of the command. -->
Assigns a neighbor to a peer-group. Issue the `no` command to remove the neighbor from the peer-group.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
Peer group name: string of maximum 80 characters.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 peer-group pg1
#### Neighbor Route-map ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
##### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *A.B.C.D* route-map *name* in|out
no neighbor *A.B.C.D* route-map *name* in|out
##### Description ####
<!--Provide a description of the command. -->
Apply a route-map on the neighbor for the direction given (in or out). Issue the `no` command to remove route-map from this neighbor.
##### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
##### Parameters ####
<!--Provide for the parameters for the command.-->
Peer IP address in *A.B.C.D* format.
Route-map name: string of maximum 80 characters.
##### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor 9.0.0.2 remote-as 6002
s1(config-router)# neighbor 9.0.0.2 route-map rm1 in
### BGP Peer Group ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
neighbor *word* peer-group
no neighbor *word* peer-group
#### Description ####
<!--Provide a description of the command. -->
Creates a new peer-group. Issue the `no` command to delete a peer-group.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
String of maximum 80 characters.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# router bgp 6001
s1(config-router)# neighbor pg1 peer-group
## Route Map Configuration Commands ##
### Create Route Map ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
route-map *name* (deny|permit) *order*
no route-map *name* (deny|permit) *order*
#### Description ####
<!--Provide a description of the command. -->
Configure the order’th entry in route-map name with ‘Match Policy’ of either permit or deny.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Route-map name: string of maximum 80 characters.
Order number: Integer in the range of 1-65535.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# route-map rm1 deny 1
### Route Map Match ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
Route-map Command: match ip address prefix-list *word*
#### Description ####
<!--Provide a description of the command. -->
Configures a match clause for route map.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Prefix list name: string of maximum 80 chars.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# match ip address prefix-list PLIST1
### Route Map  Set ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
Route-map Command: set community *AA:NN* additive
Route-map Command: no set community *AA:NN* additive
Route-map Command: set metric *metric*
Route-map Command: no set metric *metric*
#### Description ####
<!--Provide a description of the command. -->
Use the `set community` command to set the BGP community attribute. Use the `set metric` command to set the BGP attribute MED.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
AA, NN: AS number
Metric: Integer in the range of 0-4294967295
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# set community 6001:7002 additive
s1(config-route-map)# set metric 100
s1(config-route-map)# no set metric 100
### Route Map Description ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
Route-map Command: description *text*
Route-map Command: no description *text*
#### Description ####
<!--Provide a description of the command. -->
Sets Route-map description.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Route-map description: string of maximum 80 chars.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# route-map RMAP1 deny 1
s1(config-route-map)# description rmap-mcast
## IP Prefix-list Configuration Commands ##
###  Create IP Prefix-list ###
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
ip prefix-list *name* seq *num* (deny|permit) (*A.B.C.D/M*|any)
no ip prefix-list *name* seq *num* (deny|permit) (*A.B.C.D/M*|any)
no ip prefix-list *name*
#### Description ####
<!--Provide a description of the command. -->
ip prefix-list provides the most powerful prefix based filtering mechanism. It has prefix length range specification and sequential number specification. You can add or delete prefix based filters to arbitrary points of prefix-list using sequential number specification. If no ip prefix-list is specified, it acts as permit. If ip prefix-list is defined, and no match is found, default deny is applied.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
Prefix-list name: string of maximum 80 chars.
Seq num: Integer in the range of 1-4294967295.
IP Prefix: IPv4 network/prefix length.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
s1(config)# ip prefix-list PLIST1 seq 5 deny 11.0.0.0/8
s1(config)# ip prefix-list PLIST2 seq 10 permit 10.0.0.0/8
s1(config)# no ip prefix-list PLIST1 seq 5 deny 11.0.0.0/8
s1(config)# no ip prefix-list PLIST2
##Display Commands ##
### show ip bgp ###
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
show ip bgp
show ip bgp *A.B.C.D*
show ip bgp *A.B.C.D/M*
#### Description ####
<!--Provide a description of the command. -->
Displays BGP routes from BGP Route table. When no route is specified all IPv4 routes are displayed.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
IPv4 network address or IPv4 prefix.
#### Examples ####
<!--    myprogramstart -s process_xyz-->
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
<!--Change the value of the anchor tag above, so this command can be directly linked. -->
#### Syntax ####
<!--For example,    myprogramstart [option] <process_name> -->
show ip bgp summary
#### Description ####
<!--Provide a description of the command. -->
Displays summary of BGP neighbor status.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
None
#### Examples ####
<!--    myprogramstart -s process_xyz-->
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
<!--For example,    myprogramstart [option] <process_name> -->
show bgp neighbors
#### Description ####
<!--Provide a description of the command. -->
Displays detailed information about BGP neighbor connections.
#### Authority ####
<!--Provide who is authorized to use this command, such as Super Admin or all users.-->
admin
#### Parameters ####
<!--Provide for the parameters for the command.-->
None
#### Examples ####
<!--    myprogramstart -s process_xyz-->
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
