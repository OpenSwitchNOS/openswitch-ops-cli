# CLI Component Tests

## Contents

- [VRF Add/Delete](#vrf-adddelete)
	- [Add VRF with valid name](#add-vrf-with-valid-name)
	- [Add VRF with name greater than maximum allowed length](#add-vrf-with-name-greater-than-maximum-allowed-length)
	- [Re-add default VRF](#re-add-default-vrf)
	- [Delete default VRF](#delete-default-vrf)
- [Attach/detach interface to/from VRF](#attachdetach-interface-tofrom-vrf)
	- [Attach L3 interface to default VRF](#attach-l3-interface-to-default-vrf)
	- [Attach interface to nonexistent VRF](#attach-interface-to-nonexistent-vrf)
	- [Attach L2 interface to default VRF](#attach-l2-interface-to-default-vrf)
	- [Detach L3 interface from default VRF](#detach-l3-interface-from-default-vrf)
	- [Configure L3 interface into L2 interface](#configure-l3-interface-into-l2-interface)
- [IP address assign/remove](#ip-address-assignremove)
	- [Assign IP address to non-L3 interface](#assign-ip-address-to-non-l3-interface)
	- [Remove IP address from L3 interface](#remove-ip-address-from-l3-interface)
	- [Assign IP Address to L3 interface](#assign-ip-address-to-l3-interface)
	- [Scenarios for IP Address duplication](#scenarios-for-ip-address-duplication)
	- [Remove primary IP address with existing secondary IP address](#remove-primary-ip-address-with-existing-secondary-ip-address)
	- [Remove non-existent IP address](#remove-non-existent-ip-address)
	- [Remove non-existent secondary IP address](#remove-non-existent-secondary-ip-address)
	- [Assign multiple secondary IP addresses](#assign-multiple-secondary-ip-addresses)
	- [Remove multiple secondary IP addresses](#remove-multiple-secondary-ip-addresses)
- [IPv6 address assign/remove](#ipv6-address-assignremove)
	- [Assign IPv6 address to a non-L3 interface](#assign-ipv6-address-to-a-non-l3-interface)
	- [Remove IPv6 address](#remove-ipv6-address)
	- [Assign IPv6 address](#assign-ipv6-address)
	- [Update IPv6 address](#update-ipv6-address)
	- [Scenarios for IPv6 address duplication](#scenarios-for-ipv6-address-duplication)
	- [Remove IPv6 address with existing secondary IPv6 address](#remove-ipv6-address-with-existing-secondary-ipv6-address)
	- [Remove non-existent IPv6 address](#remove-non-existent-ipv6-address)
	- [Remove IPv6 address](#remove-ipv6-address)
	- [Remove non-existent secondary IPv6 address](#remove-non-existent-secondary-ipv6-address)
	- [Assign multiple secondary IPv6 addresses](#assign-multiple-secondary-ipv6-addresses)
	- [Remove multiple secondary IPv6 addresses](#remove-multiple-secondary-ipv6-addresses)
- [Toggle L2 / L3 interface](#toggle-l2-l3-interface)
	- [Assign IP and IPv6 address](#assign-ip-and-ipv6-address)
	- [Convert L3 interface to L2](#convert-l3-interface-to-l2)
- [Show running config](#show-running-config)
	- [Show running configuration on multiple interfaces](#show-running-configuration-on-multiple-interfaces)
	- [Internal VLAN range at system bootup](#internal-vlan-range-at-system-bootup)
	- [Internal VLAN range with start greater than end](#internal-vlan-range-with-start-greater-than-end)
	- [Internal VLAN range with start equal to end](#internal-vlan-range-with-start-equal-to-end)
	- [Descending internal VLAN range](#descending-internal-vlan-range)
	- [Ascending internal VLAN range](#ascending-internal-vlan-range)
	- [Default internal VLAN with no configuration](#default-internal-vlan-with-no-configuration)
- [IPv4 Add](#ipv4-add)
	- [Add static route without distance specified](#add-static-route-without-distance-specified)
	- [Add static route with distance specified](#add-static-route-with-distance-specified)
	- [Add static route with IPv4 prefix and new next hop IP](#add-static-route-with-ipv4-prefix-and-new-next-hop-ip)
	- [Add static route with same IPv4 prefix, new next hop IP and different distance](#add-static-route-with-same-ipv4-prefix-new-next-hop-ip-and-different-distance)
	- [Add static route with next hop as interface](#add-static-route-with-next-hop-as-interface)
	- [Add static route with same prefix and new next hop as interface](#add-static-route-with-same-prefix-and-new-next-hop-as-interface)
	- [Add static route with same prefix and new next hop as interface with distance](#add-static-route-with-same-prefix-and-new-next-hop-as-interface-with-distance)
	- [Add static route with wrong prefix format](#add-static-route-with-wrong-prefix-format)
	- [Add static route with wrong next hop format](#add-static-route-with-wrong-next-hop-format)
- [IPv4 Show](#ipv4-show)
	- [Show IPv4 routes](#show-ipv4-routes)
	- [Show empty IPv4 routes table](#show-empty-ipv4-routes-table)
- [IPv4 Delete](#ipv4-delete)
	- [Remove static route](#remove-static-route)
	- [Remove the last next hop entry related](#remove-the-last-next-hop-entry-related)
	- [Remove static route with invalid distance](#remove-static-route-with-invalid-distance)
	- [Remove static route with invalid prefix](#remove-static-route-with-invalid-prefix)
	- [Remove static route with invalid next hop IP](#remove-static-route-with-invalid-next-hop-ip)
	- [Remove static route with invalid next hop interface](#remove-static-route-with-invalid-next-hop-interface)
- [IPv6 Add](#ipv6-add)
	- [Add static route with IPv6 prefix and next hop IPv6 without distance](#add-static-route-with-ipv6-prefix-and-next-hop-ipv6-without-distance)
	- [Add static route with IPv6 prefix, next hop IPv6 and distance](#add-static-route-with-ipv6-prefix-next-hop-ipv6-and-distance)
	- [Add static route with same IPv6 prefix and new next hop IPv6](#add-static-route-with-same-ipv6-prefix-and-new-next-hop-ipv6)
	- [Add static route with same IPv6 prefix, next hop IPv6 and distance](#add-static-route-with-same-ipv6-prefix-next-hop-ipv6-and-distance)
	- [Add static route with same IPv6 prefix, different next hop IPv6 and distance](#add-static-route-with-same-ipv6-prefix-different-next-hop-ipv6-and-distance)
	- [Add static route with next hop as a non-L3 interface](#add-static-route-with-next-hop-as-a-non-l3-interface)
	- [Add static route with next hop as interface](#add-static-route-with-next-hop-as-interface)
	- [Add static route with same prefix and new next hop as interface](#add-static-route-with-same-prefix-and-new-next-hop-as-interface)
	- [Add static route with same prefix and new next hop as L3 interface with distance](#add-static-route-with-same-prefix-and-new-next-hop-as-l3-interface-with-distance)
	- [Add static route with wrong prefix](#add-static-route-with-wrong-prefix)
	- [Add static route with wrong next hop](#add-static-route-with-wrong-next-hop)
	- [Add static route with trailing 0s](#add-static-route-with-trailing-0s)
- [IPv6 Show](#ipv6-show)
	- [Show IPv6 routes table](#show-ipv6-routes-table)
	- [Show empty IPv6 route table](#show-empty-ipv6-route-table)
- [IPv6 Delete](#ipv6-delete)
	- [Remove IPv6 static route](#remove-ipv6-static-route)
	- [Remove last next hop entry for an IPv6 static route](#remove-last-next-hop-entry-for-an-ipv6-static-route)
	- [Remove IPv6 static route with invalid distance](#remove-ipv6-static-route-with-invalid-distance)
	- [Remove IPv6 static route with invalid prefix](#remove-ipv6-static-route-with-invalid-prefix)
	- [Remove IPv6 static route with invalid next hop IP](#remove-ipv6-static-route-with-invalid-next-hop-ip)
	- [Remove IPv6 static route with invalid next hop interface](#remove-ipv6-static-route-with-invalid-next-hop-interface)
- [Show RIB for L3 Static Routes (Ipv4 and IPv6)](#show-rib-for-l3-static-routes-ipv4-and-ipv6)
	- [Display RIB entries](#display-rib-entries)
- [VLAN Interface](#vlan-interface)
	- [Invalid VLAN input parameters](#invalid-vlan-input-parameters)
	- [Add/Delete VLAN interface](#adddelete-vlan-interface)
	- [Modify VLAN interface](#modify-vlan-interface)

## VRF Add/Delete

###  Add VRF with valid name
#### Objective
The test case checks if non-default VRFs can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Add a new VRF with a valid name. Only the default VRF is allowed.

Run the command:
```
root(config)# vrf vrf0
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Non-default VRFs not supported.`
##### Test Fail Criteria

###  Add VRF with name greater than maximum allowed length
#### Objective
The test case checks adding a VRF with name longer than maximum allowed length.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Add a VRF with a name more than 32 characters long.
Run the command:
```
root(config)# vrf thisisavrfnamewhichismorethan32characters
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Non-default VRFs not supported.`
##### Test Fail Criteria


### Re-add default VRF
#### Objective
The test case checks if the default VRF can be added again.
#### Requirements
- vrf_default previously added
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Add the default VRF again.

Run the command:
```
root(config)# vrf vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Default VRF already exists`
##### Test Fail Criteria

###  Delete default VRF
#### Objective
The test case checks if the default VRF can be deleted.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Delete the default VRF.

Run the command:
```
root(config)# no vrf vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: Cannot delete default VRF.`
##### Test Fail Criteria

## Attach/detach interface to/from VRF

### Attach L3 interface to default VRF
#### Objective
The test case checks the attachment of an L3 interface to default VRF.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Attach an L3 interface to default VRF.

Run the commands:
```
root(config)# interface 1
root(config-if)# vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Already attached to default VRF`

##### Test Fail Criteria

###  Attach interface to nonexistent VRF
#### Objective
The test case checks if an interface can be attached to a VRF that does not exist.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Attach an interface to a VRF that does not exist.

Run the commands:
```
root(config)# interface 1
root(config-if)# vrf attach abcd
```
#### Test Result Criteria
##### Test Pass Criteria
Code will output:
`Non-default VRFs not supported`
##### Test Fail Criteria

###  Attach L2 interface to default VRF
#### Objective
The test case checks if an L2 interface can be attached to default VRF.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Attach an L2 interface to a VRF.

Run the commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: Interface 1 is not L3`
##### Test Fail Criteria

###  Detach L3 interface from default VRF
#### Objective
The test case checks if the L3 interface can be detached from the default VRF.
#### Requirements
- vrf_default previously created
- interface 1 attached to vrf_default
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py
#### Setup
Single Switch Topology

#### Description
Detach an L3 interface from the default VRF.

Run the commands:
```
root(config)# interface 1
root(config-if)# no vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Cannot detach from default VRF`
##### Test Fail Criteria

###  Configure L3 interface into L2 interface
#### Objective
The test case checks the multiple additions and deletions of L3 interfaces.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Add multiple L3 interfaces and try to make one of them L2.

Run the commands:
```
root(config)# interface 1
root(config-if)# routing
root(config-if)# ip address 10.1.1.1/8
root(config-if)# exit
root(config)# interface 2
root(config-if)# routing
root(config-if)# ip address 10.1.1.2/8
root(config-if)# exit
root(config)# interface 3
root(config-if)# routing
root(config-if)# ip address 10.1.1.3/8
root(config-if)# exit
root(config)# interface 2
root(config-if)# no routing
root(config-if)# exit
```
#### Test Result Criteria
##### Test Pass Criteria
Interface 2 must become L2 as part of a bridge.

Run the command:
```
show vrf
```
The interface should not appear.
##### Test Fail Criteria

## IP address assign/remove

###  Assign IP address to non-L3 interface
#### Objective
The test case checks the assignment of an IP address to an L2 interface is not allowed.

#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign an IP address to an interface that is not L3.

Run the commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: Interface 1 is not L3`
##### Test Fail Criteria


###  Remove IP address from L3 interface
#### Objective
The test case checks the removal of an IP address from an interface that has no IP address.

#### Requirements
- No IP address in interface 1
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove an IP address from an L3 interface that does not have an IP address configured on it.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: No IP Address configured on interface 1`
##### Test Fail Criteria

###  Assign IP Address to L3 interface
#### Objective
The test case checks the assignment of an IP address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Assign an IP address to an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
IP address must be added for the port row.
##### Test Fail Criteria

###  Scenarios for IP address duplication
#### Objective
The test case checks for a duplicate primary IP address on an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Check for a duplicate IP address assignment for the following cases:

Run the commands to assign primary and secondary IP addresses:
```
root(config)# interface 1
root(config-if)#ip address 10.0.20.3/24
root(config-if)#ip address 10.0.20.4/24 secondary
```

1. Assign the existing primary IP address as primary.
```
root(config-if)#ip address 10.0.20.3/24
```
1. Assign the existing secondary IP address as primary.
```
root(config-if)#ip address 10.0.20.4/24
```
1. Assign the existing primary IP address as secondary.
```
root(config-if)#ip address 10.0.20.3/24 secondary
```
1. Assign the existing secondary IP address as secondary.
```
root(config-if)#ip address 10.0.20.4/24 secondary
```

#### Test Result Criteria
##### Test Pass Criteria
Error messages vary depending on the step mentioned in the description:
1. `Error: IP Address is already assigned to interface 1 as primary.`
2. `Error: IP Address is already assigned to interface 1 as secondary.`
3. `Error: IP Address is already assigned to interface 1 as primary.`
4. `Error: IP Address is already assigned to interface 1 as secondary.`

##### Test Fail Criteria

###  Remove primary IP address with existing secondary IP address
#### Objective
The test case checks if a primary IP address can be removed without removing secondary IP addresses.
#### Requirements
- Primary IP address 10.0.20.3/24 assigned to interface 1
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove primary IP address without removing all secondary IP addresses.

Run the commands:
```
root(config)# interface 1
root(config-if)# ip address 10.0.20.4/24 secondary
root(config-if)# no ip address 10.0.20.3/24
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Delete all secondary IP addresses before deleting primary.`

##### Test Fail Criteria

###  Remove non-existent IP address
#### Objective
The test case checks if a primary IP address can be removed without removing secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove an IP address from an L3 interface by giving the wrong IP address.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.30.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: IP Address 10.0.30.2/24 not found.`
##### Test Fail Criteria

###  Remove non-existent secondary IP address
#### Objective
The test case checks the removal of secondary IP address from an L3 interface that has no secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove a secondary IP address from an L3 interface that does not have any secondary IP address configured on it.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.30.2/24 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: No secondary IP Address configured on interface 1.`
##### Test Fail Criteria

###  Assign multiple secondary IP addresses
#### Objective
The test case checks the assignment of multiple secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign multiple secondary IP addresses to an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# ip address 10.0.20.3/24 secondary
root(config-if)# ip address 10.0.20.4/24 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Secondary IP addresses must be added to the port row.
##### Test Fail Criteria

###  Remove multiple secondary IP addresses
#### Objective
The test case checks the removal of multiple secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove multiple secondary IP addresses from an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.20.3/24 secondary
root(config-if)# no ip address 10.0.20.4/24 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Secondary IP addresses must be removed from port row.
##### Test Fail Criteria

## IPv6 address assign/remove

###  Assign IPv6 address to a non-L3 interface
#### Objective
The test case checks if an IPv6 address can be assigned to an L2 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign an IP v6 address to an interface that is not L3.

Run the commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: Interface 1 is not L3.`
##### Test Fail Criteria

###  Remove IPv6 address
#### Objective
The test case checks the removal of an IPv6 address from an interface that has no IP address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove an IPv6 address from an L3 interface that does not have
an IPv6 address configured on it.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: No IPv6 Address configured on interface 1`
##### Test Fail Criteria

###  Assign IPv6 address
#### Objective
The test case checks the assignment of an IPv6 address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign an IPv6 address to an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
IPv6 address must be added for the port row.
##### Test Fail Criteria

###  Update IPv6 address
#### Objective
The test case checks updating an IPv6 address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Update IPv6 address on an L3 interface that already has an IPv6 address configured on it.

Run the commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2001::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
IPv6 address must be updated with the new value.
##### Test Fail Criteria

### Scenarios for IPv6 Address duplication
#### Objective
The test case checks for all duplicate scenarios between IPv6 addresses on an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Check for a duplicate IPv6 address assignment for the following cases:

Run the commands to assign primary and secondary IPv6 address:
```
root(config)# interface 1
root(config-if)#ipv6 address 10.0.20.3/24
root(config-if)#ipv6 address 10.0.20.4/24 secondary
```

1. Assign existing primary IPv6 address as primary.
```
root(config-if)#ipv6 address 10.0.20.3/24
```
1. Assign the existing secondary IP address as primary.
```
root(config-if)#ipv6 address 10.0.20.4/24
```
1. Assign the existing primary IP address as secondary.
```
root(config-if)#ipv6 address 10.0.20.3/24 secondary
```
1. Assign the existing secondary IP address as secondary.
```
root(config-if)#ipv6 address 10.0.20.4/24 secondary
```

#### Test Result Criteria
##### Test Pass Criteria
Error messages vary depending on the step mentioned in the description:
1. `Error: IP Address is already assigned to interface 1 as primary.`
2. `Error: IP Address is already assigned to interface 1 as secondary.`
3. `Error: IP Address is already assigned to interface 1 as primary.`
4. `Error: IP Address is already assigned to interface 1 as secondary.`

##### Test Fail Criteria

###  Remove IPv6 address with existing secondary IPv6 address
#### Objective
The test case checks if a primary IPv6 address can be removed without removing secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove a primary IPv6 address without removing all secondary IPv6 addresses.

Run the commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2001::2/128 secondary
root(config-if)# no ipv6 address 2001::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Delete all secondary IP addresses before deleting primary.`
##### Test Fail Criteria

###  Remove non-existent IPv6 address
#### Objective
The test case checks the removal of primary IPv6 address from an L3 interface by giving a wrong IPv6 address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Remove an IPv6 address from an L3 interface by giving a wrong IPv6 address.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2004::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: IPv6 Address 2004::1/128 not found.`
##### Test Fail Criteria

### Remove IPv6 address
#### Objective
The test case checks the removal of primary IPv6 address from an L3 interface by giving a correct IPv6 address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology

#### Description
Remove an IPv6 address from an L3 interface by giving the a correct IPv6 address.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2001::1/128
```

#### Test Result Criteria
##### Test Pass Criteria
IPv6 address must be removed from the port row.
##### Test Fail Criteria

### Remove non-existent secondary IPv6 address
#### Objective
The test case checks the removal of a secondary IPv6 address from an L3 interface that has no secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove a secondary IPv6 address from an L3 interface that does not have any secondary IPv6 address configured on it.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2004::1/128 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Error: No secondary IPv6 Address configured on interface 1.`
##### Test Fail Criteria

### Assign multiple secondary IPv6 addresses
#### Objective
The test case checks the assignment of multiple secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign multiple secondary IPv6 addresses to an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2001::2/128 secondary
root(config-if)# ipv6 address 2001::3/128 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Secondary IPv6 addresses must be added to the port row.
##### Test Fail Criteria

### Remove multiple secondary IPv6 addresses
#### Objective
The test case checks the removal of multiple secondary IPv6 addresses.
#### Requirements
- IPv6 address 2001::2/128 and 2001::3/128 needs to be added as secondary
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Remove multiple secondary IPv6 addresses from an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2001::2/128 secondary
root(config-if)# no ipv6 address 2001::3/128 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Secondary IPv6 addresses must be removed from the port row.
##### Test Fail Criteria

## Toggle L2 / L3 interface

### Assign IP and IPv6 address
#### Objective
The test case checks `routing` command by configuring IP addresses to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Assign an IP and IPv6 addresses to an L3 interface.

Run the commands:
```
root(config)# interface 1
root(config-if)# routing
root(config-if)# ip address 10.1.1.1/8
root(config-if)# ipv6 address 2001::2/128
```
#### Test Result Criteria
##### Test Pass Criteria
IP and IPv6 addresses must be updated for the interface.
Interface must be added to default VRF.

Command `show vrf` must show interface 1 under default VRF
##### Test Fail Criteria

### Convert L3 interface to L2
#### Objective
The test case checks if IP addresses are removed when L3 interface becomes L2.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Make an L3 interface L2 by using 'no routing' command.

Run the command:
```
root(config)# interface 1
root(config-if)# no routing
```
#### Test Result Criteria
##### Test Pass Criteria
IP and IPv6 addresses previously configured must be removed from the port row.
The port must be added to default bridge.
Command `show vrf` must not have interface 1.
##### Test Fail Criteria

## Show running config

### Show running configuration on multiple interfaces
#### Objective
The test case checks the 'show running config' command by configuring multiple L3 interfaces.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology
#### Description
Test the `show running config` command with different configurations.

1. Run the commands:
```
root(config)#interface 1
root(config-if)#no routing
root(config-if)#exit
```
Then, check the result with the `show running configurations` command.
1. Run the commands:
```
root(config)#interface 2
root(config-if)#ipv6 address 2002::1/128
root(config-if)#ip address 10.1.1.1/8
root(config-if)#ip address 10.1.1.3/8 secondary
root(config-if)#ipv6 address 2002::2/128 secondary
root(config-if)#exit
```
Then, check the result with the `show running configurations` command.
1. Run the commands:
```
root(config)#interface 3
root(config-if)#lldp transmission
root(config-if)#exit
```
Then, check the result with the `show running configurations` command.

#### Test Result Criteria

##### Test Pass Criteria
1. Interface 1 with no routing.
2. Interface 2 with only the IPv4 and IPv6 addresses. It should not show routing by default.
3. Interface 3 with LLDP transmission.

##### Test Fail Criteria

### Internal VLAN range at system bootup
#### Objective
The test case checks the internal VLAN range on system bootup.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check the internal VLAN range at system bootup.

Run the command:
```
root# show vlan internal
```
#### Test Result Criteria
##### Test Pass Criteria
The default values should be displayed.
```
Internal VLAN range  : 1024-4094
Internal VLAN policy  : ascending
```
##### Test Fail Criteria

### Internal VLAN range with start greater than end
#### Objective
The test case checks the internal VLAN range by giving a start value greater than the end value of the range.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check the internal VLAN range by giving a start number that is larger than the ending number for the range. 

Run the command:
```
root(config)# vlan internal range 100 10 ascending
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Invalid VLAN range. End VLAN must be greater or equal to start VLAN.`
##### Test Fail Criteria

### Internal VLAN range with start equal to end
#### Objective
The test case checks if both start and end of a range can be equal.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check internal VLAN range by giving the same number for both the start and end of the range. 

Run the command:
```
root(config)# vlan internal range 10 10 ascending
```
#### Test Result Criteria
##### Test Pass Criteria
The VLAN range must be set with a range of 1.
```
Internal VLAN range  : 10-10
Internal VLAN policy  : ascending
```
##### Test Fail Criteria

###  Descending internal VLAN range
#### Objective
The test case checks if a descending internal VLAN is configured properly.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check the descending internal VLAN range.

Run the command:
```
root(config)# vlan internal range 100 200 descending
```
#### Test Result Criteria
##### Test Pass Criteria
The VLAN range must be set properly.
```
Internal VLAN range  : 100-200
Internal VLAN policy  : descending
```
##### Test Fail Criteria

###  Ascending internal VLAN range
#### Objective
The test case checks if an ascending internal VLAN is configured properly.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check the ascending internal VLAN range.

Run the command:
```
root(config)# vlan internal range 10 100 ascending
```
#### Test Result Criteria
##### Test Pass Criteria
The VLAN range must be set properly.
```
Internal VLAN range  : 10-100
Internal VLAN policy  : ascending
```
##### Test Fail Criteria

###  Default internal VLAN with no configuration
#### Objective
The test case checks if an internal VLAN range goes back to the default values on the 'no' command.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology
#### Description
Check the default internal VLAN range on `no` command.

Run the command:
```
root(config)# no vlan internal range
```
#### Test Result Criteria
##### Test Pass Criteria
The default values should be displayed.
```
Internal VLAN range  : 1024-4094
Internal VLAN policy  : ascending
```
##### Test Fail Criteria

## IPv4 Add

###  Add static route without distance specified
#### Objective
The test case checks the addition of static route with IPv4 prefix and next hop IP without specifying the distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```
#### Description
Add a static route with IPv4 prefix and next hop IP without the distance specified.

Run the command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the routing and next hop table.

Run the command:
```
root(config)# ip netns exec swns ip route
```
The command outputs the route in the kernel.
##### Test Fail Criteria

###  Add static route with distance specified
#### Objective
The test case checks addition of static route with an IPv4 prefix, next hop IP and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```
#### Description
Add a static route with IPv4 prefix and next hop IP with distance specified.

Run the command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The distance attached is 3.
Verify by looking at the entries in the route and next hop table.

Run the command:
```
root(config)# ip netns exec swns ip route
```
The command outputs the route in the kernel.
##### Test Fail Criteria

###  Add static route with IPv4 prefix and new next hop IP
#### Objective
The test case checks adding a static route with the same IPv4 prefix and new next hop IP.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```
#### Description
Add a static route with the same IPv4 prefix and new next hop IP.

Run the command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table. Multiple next hop entries are present in the database with the same prefix.

Run the command:
```
ip netns exec swns ip route
```
The command outputs the route in the kernel.
##### Test Fail Criteria

###  Add static route with same IPv4 prefix, new next hop IP and different distance
#### Objective
The test case checks adding a static route with the same IPv4 prefix, but with a different next hop IP and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same IPv4 prefix, but with a new next hop IP and different distance.

Run the command:
```
ip route 1.1.1.1/8 1.1.1.3 3
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: The CLI command is accepted for the given prefix/route, distance is set by the
first next hop entered through the CLI. The rest of the corresponding routes can accept only the
distance configured previously.
##### Test Fail Criteria

###  Add static route with next hop as interface
#### Objective
The test case checks adding a static route with next hop as a non-L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with next hop as an interface without configuring the interface.

Run the command:
```
root(config)# ip route 1.1.1.1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Error: Interface 2 not configured message`

You must configure the interface first by assigning an IP address to it.
##### Test Fail Criteria

###  Add static route with next hop as interface
#### Objective
The test case checks adding a static route with next hop as an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with next hop as an interface.

Run the commands:
```
root(config)# interface 2
root(config-if)# ip address 1.1.1.2/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table.

The command `ip netns exec swns ip route ` shows the route in the kernel via dev.
##### Test Fail Criteria

### Add static route with same prefix and new next hop as interface
#### Objective
The test case checks adding a static route with the same prefix and the new next hop as an L3 interface. The route will not have distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram
```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same prefix and new next hop as an interface.

Run the command:
```
root(config)# interface 3
root(config-if)# ip address 1.1.1.3/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route are added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table.
Multiple next hop entries are present in the database for the same prefix.
The command `ip netns exec swns ip route ` shows the route in the kernel via dev 3.
##### Test Fail Criteria

### Add static route with same prefix and new next hop as interface with distance
#### Objective
The test case checks adding a static route with same prefix and a new next hop as an L3 interface. The route will have distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same prefix and new next hop as an interface with a preferred distance.

Run the commands:
```
root(config)# interface 4
root(config-if)# ip address 1.1.1.4/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 4 4
```

#### Test Result Criteria
##### Test Pass Criteria
Error: CLI command is not accepted as for a given prefix/route, because the distance is set by the first next hop entered through the CLI.
The rest of the corresponding routes can accept only the distance configured previously.
##### Test Fail Criteria

### Add static route with wrong prefix format
#### Objective
The test case checks adding a static route with the wrong prefix format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the wrong prefix format.

Run the command:
```
root(config)# ip route 1.1.1.1 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Unknown command`
##### Test Fail Criteria

### Add static route with wrong next hop format
#### Objective
The test case checks adding a static route with the wrong next hop format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the wrong next hop format.

Run the command:
```
root(config)# ip route 1.1.1.1 1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Interface 1.1.2 is not configured`
##### Test Fail Criteria

## IPv4 Show

### Show IPv4 routes
#### Objective
The test case checks if the configured IP routes are displayed properly.
#### Requirements
- IPv4 routes added to the database
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Show when IPv4 entries are present in the database.

Run the commands:
```
root(config)# do show ip route
```
#### Test Result Criteria
##### Test Pass Criteria
Run the command `do show ip route`
The command outputs:
```
Displaying IP routes
'[x/y]' denotes [distance/metric]
10.0.30.0/24, 2 unicast next-hops
via 10.0.20.2, [1/0], static
via 10.0.20.2, [1/0], static
1.1.1.1/8, 3 unicast next-hops
via 2, [3/0], static
via 1.1.1.3, [3/0], static
via 1, [3/0], static
```
##### Test Fail Criteria

### Show empty IPv4 routes table
#### Objective
The test case checks if the proper error message is shown when there are no IP routes to display.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Show the empty IPv4 routes table when no IPv4 entries are present in the database.

Run the command:
```
root(config)# do show ip route
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`No ipv4 routes configured`
##### Test Fail Criteria

## IPv4 Delete

### Remove static route
#### Objective
The test case checks the removal of a static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Delete an entry that is present from the database. You can specify the distance, which is entered, or leave it blank.

Run the commands:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2
```
**OR**
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2 2
```
#### Test Result Criteria
##### Test Pass Criteria
The entry is deleted from the database and the kernel.
The entry related to this next hop is also deleted from the database, but not the entry related to the route prefix, as it might have multiple next hops.
The command `ip netns exec swns ip route` should not show the route.

##### Test Fail Criteria

### Remove the last next hop entry related
#### Objective
The test case checks the removal of the last next hop entry for a static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Delete the last next hop entry related for a route.

Run the command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.3
```
#### Test Result Criteria
##### Test Pass Criteria
The entry is deleted from the database and the kernel.
The entry related to this next hop is also deleted from the database and from the route table.
The command `ip netns exec swns ip route ` should not show the route.

##### Test Fail Criteria

### Remove static route with invalid distance
#### Objective
The test case checks the removal of a static route with an invalid distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove a static route by specifying the wrong or different distance.

Run the command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`No such ip route found`
##### Test Fail Criteria

### Remove static route with invalid prefix
#### Objective
The test case checks the removal of a static route with invalid prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove a static route specifying the wrong prefix.

Run the command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`Unknown Command`
##### Test Fail Criteria

### Remove static route with invalid next hop IP
#### Objective
The test case checks the removal of a static route with an invalid next hop IP.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove the static route, specifying the wrong next hop IP.

Run the command:
```
root(config)# no ip route 1.1.1.1/8 1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`No such ip route found`
##### Test Fail Criteria

### Remove static route with invalid next hop interface
#### Objective
The test case checks the removal of a static route with an invalid next hop interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove the static route specifying the wrong next hop interface.

Run the command:
```
root(config)# no ip route 1.1.1.1/8 4
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs:
`No such ip route found`
##### Test Fail Criteria

## IPv6 Add

### Add static route with IPv6 prefix and next hop IPv6 without distance
#### Objective
The test case checks the addition of a static route with an IPv6 prefix and next hop IPv6 without specifying the distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with an IPv6 prefix and a next hop IP, without the distance specified.

Run the command:
```
root(config)# ipv6 route 2001::1/8 2001:: 2
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table.
The command `ip netns exec swns ip -6 route` shows the route in the kernel.
##### Test Fail Criteria

### Add static route with IPv6 prefix, next hop IPv6 and distance
#### Objective
The test case checks the addition of a static route with the IPv6 prefix, next hop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the IPv4 prefix and next hop IP with the distance specified.

Run the command:
```
root(config)# ipv6 route 2001::1/8 2001:: 2 3
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The distance attached is 3.
Verify by looking at the entries in the route and next hop table.
The command ` ip netns exec swns ip -6 route` shows the route in the kernel.
##### Test Fail Criteria

### Add static route with same IPv6 prefix and new next hop IPv6
#### Objective
The test case checks adding a static route with same the IPv6 prefix and new next hop IPv6.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same IPv6 prefix and the new next hop IP.

Run the command:
```
root(config)# ipv6 route 2001::1/8 2001:: 3
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance is 1.
Verify by looking at the entries in the route and next hop table. Multiple next hop entries are present in the database for same prefix.
##### Test Fail Criteria

### Add static route with same IPv6 prefix, next hop IPv6 and distance
#### Objective
The test case checks adding a static route with same IPv6 prefix but different next hop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same IPv6 prefix, but with a new next hop IP and different distance.

Run the command:
```
root(config)# ipv6 route 2001::1/8 2001::4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: The CLI command fails for a given prefix/route, because the distance is set by the
first next hop entered through the CLI. The rest of the corresponding routes can accept only the distance configured previously.
The command `ip netns exec swns ip -6 route` shows the route in the kernel.
##### Test Fail Criteria

### Add static route with same IPv6 prefix, different next hop IPv6 and distance
#### Objective
The test case checks adding a static route with the same IPv6 prefix but with a different next hop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same IPv6 Prefix, but with a new next hop IP and different distance.

Run the command:
```
root(config)# ipv6 route 2001::1/8 2001::4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: The CLI command is not accepted for a given prefix/route, because the distance is set by the first next hop entered through the CLI. The rest of the corresponding routes can accept only the distance configured previously.

##### Test Fail Criteria

### Add static route with next hop as a non-L3 interface
#### Objective
The test case checks adding a static route with next hop as a non-L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with next hop as an interface without configuring the interface.

Run the command:
```
root(config)# ipv6 route 2002::1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Interface 2 not configured`
You must configure the interface first by assigning an IP address to it.
##### Test Fail Criteria

### Add static route with next hop as interface
#### Objective
The test case checks adding a static route with the next hop as an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with next hop as an interface.

Run the commands:
```
root(config)# interface 2
root(config-if)# ip address 2001::2/8
root(config)# exit
root(config)# ipv6 route 2001::1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table.
The command ` ip netns exec swns ip -6 route` shows the route in the kernel via dev 2.
##### Test Fail Criteria

### Add static route with same prefix and new next hop as interface
#### Objective
The test case checks adding a static route with the same prefix and the new next hop as an L3 interface, without the distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same prefix and the new next hop as an interface.

Run the commands:
```
root(config)# interface 3
root(config-if)# ip address 2001::3/8
root(config)# exit
root(config)# ipv6 route 2001::1/8 2001:: 3
```
#### Test Result Criteria
##### Test Pass Criteria
The static route is added to the kernel. The default distance attached is 1.
Verify by looking at the entries in the route and next hop table. Multiple next hop entries are present in the database for the same prefix.
The command ` ip netns exec swns ip route` shows the route in the kernel via dev 3.
##### Test Fail Criteria

### Add static route with same prefix and new next hop as L3 interface with distance
#### Objective
The test case checks adding a static route with the same prefix and new next hop as an L3 interface, with distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the same prefix and a new next hop as an interface with the preferred distance.

Run the commands:
```
root(config)# interface 4
root(config-if)# ip address 1.1.1.4/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: The CLI command is not accepted for a given prefix/route, because the distance is set by the first next hop entered through the CLI. The rest of the corresponding routes can accept only the distance configured previously.
##### Test Fail Criteria

### Add static route with wrong prefix
#### Objective
The test case checks adding a static route with the wrong prefix format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the wrong prefix format.

Run the commands:
```
root(config)# ipv6 route 2001::1 ::2/8 2001:: 2
```
**OR**
```
root(config)# ipv6 route 2001::1 2001::2
```
Only one '::' allowed to pad the entries with "0s"
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Unknown command`
##### Test Fail Criteria

### Add static route with wrong next hop
#### Objective
The test case checks adding a static route with wrong next hop format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a static route with the wrong next hop format.

Run the commands:
```
ipv6 route 2001::1 20::2
```
**OR**
```
ipv6 route 2001::1 20.1.1
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Interface 1.1.2 is not configured`
##### Test Fail Criteria

###  Add static route with trailing 0s
#### Objective
The test case checks adding a static route with trailing 0s in IPv6 prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Add a prefix with trailing 0s.

Run the command:
```
ipv6 route 2001::0/8 2001::2
```
#### Test Result Criteria
##### Test Pass Criteria
The trailing 0s are trimmed/removed and stored in the database according to
the subnet specified. The prefix is stored as follows:
`ipv6 route 2001::/8 2001::2`
##### Test Fail Criteria

## IPv6 Show

### Show IPv6 routes table
#### Objective
The test case checks if the configured IPv6 routes are displayed properly.
#### Requirements
- IPv6 routes configured on the switch
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Show IPv6 entries in the database.

Run the command:
```
root(config)# do show ipv6 route
```

#### Test Result Criteria
##### Test Pass Criteria
The IPv6 routes appear in the database in the following format:

Run the command:
```
root(config)# do show ip route
```
The command should show:
```
Displaying IPv6 routes
'[x/y]' denotes [distance/metric]
2011::/120, 2 unicast next-hops
via 2013::, [1/0], static
via 2012::, [1/0], static
2010::/120, 3 unicast next-hops
via 2, [1/0], static
via 3, [1/0], static
via 4, [1/0], static
```
##### Test Fail Criteria

### Show empty IPv6 route table
#### Objective
The test case checks if a proper error message is shown when there are no IPv6 routes to display.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Verify that no IPv6 entries are present in the database.

Run the command:
```
root(config)# do show ipv6 route
```

#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `No ipv6 routes configured`
##### Test Fail Criteria

## IPv6 Delete

### Remove IPv6 static route
#### Objective
The test case checks the removal of IPv6 static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Delete an entry present in the database. You can specify the distance or leave it blank.

Run the command:
```
root(config)# no ipv6 route 2001::1/8 2001::2
```
**OR**
```
root(config)# no ipv6 route 2001::1/8 2001::2 2
```

#### Test Result Criteria
##### Test Pass Criteria
The entry was deleted from the database and the kernel. The entry related to this
next hop is deleted from the database but not the entry related to the route prefix, as it might have multiple next hops.
The command `ip netns exec swns ip -6 route` should not show the route.
##### Test Fail Criteria

### Remove last next hop entry for an IPv6 static route
#### Objective
The test case checks the removal of the last next hop entry for an IPv6 static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Delete the last next hop entry related for a route.

Run the command:
```
root(config)# no ipv6 route 2001::1/8 2001::3
```

#### Test Result Criteria
##### Test Pass Criteria
The entry is deleted from the database and the kernel. The entry related to this
next hop is deleted from the database and from the route table, being the last
next hop.
The command `ip netns exec swns ip -6 route` should not show the route.
##### Test Fail Criteria

### Remove IPv6 static route with invalid distance
#### Objective
The test case checks the removal of an IPv6 static route with an invalid distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Specify the wrong/different distance.

Run the command:
```
root(config)# no ipv6 route 2001::1/8 2001::2 5
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `No such ipv6 route found`
##### Test Fail Criteria

### Remove IPv6 static route with invalid prefix
#### Objective
The test case checks the removal of an IPv6 static route with an invalid prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove an IPv6 static route by specifying a wrong prefix.

Run the command:
```
root(config)# no ipv6 route 2001::1 2001::2
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `Unknown Command`
##### Test Fail Criteria

### Remove IPv6 static route with invalid next hop IP
#### Objective
The test case checks the removal of an IPv6 static route with an invalid next hop IP.

#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove an IPv6 static route by specifying the wrong next hop IP.

Run the commands:
```
root(config)# no ipv6 route 2001::1 2001:2.3
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `No such ipv6 route found`
##### Test Fail Criteria

### Remove IPv6 static route with invalid next hop interface
#### Objective
The test case checks the removal of an IPv6 static route with an invalid next hop interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Remove the IPv6 static route by specifying the wrong next hop interface.

Run the command:
```
root(config)# no ipv6 route 2001::1 4
```
#### Test Result Criteria
##### Test Pass Criteria
The command outputs: `No such ipv6 route found`
##### Test Fail Criteria

## Show RIB for L3 Static Routes (Ipv4 and IPv6)

### Display RIB entries
#### Objective
The test case checks the display of the Routing Information Base (RIB) entries.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
##### Topology Diagram

```ditaa
+------+              +------+
|      |              |      |
|  S1  +------------> |  S2  |
|      |              |      |
+------+              +------+
```

#### Description
Check the RIB entries.

Run the command:
```
root(config): so show rib
```
#### Test Result Criteria
##### Test Pass Criteria
Shows the RIB entries for both IPv4 and Ipv6.
Entries selected for forwarding are marked with '*'

The command outputs:
```
Displaying ipv4 rib entries
'*' denotes selected
'[x/y]' denotes [distance/metric]
*1.1.1.1/8, 5 unicast next-hops
*via 2, [3/0], static
*via 1.1.1.3, [3/0], static
*via 1, [3/0], static
*via 1.1.1.2, [3/0], static
*via 3, [3/0], static
1.1.1.4/8, 2 unicast next-hops
via 1.1.1.6, [1/0], static
via 1.1.1.5, [1/0], static

-----------------------------------------------------

Displaying ipv6 rib entries
'*' denotes selected
'[x/y]' denotes [distance/metric]
*2011::/120, 2 unicast next-hops
*via 2013::, [1/0], static
*via 2012::, [1/0], static
2010::/120, 6 unicast next-hops
via 2, [1/0], static
via 3, [1/0], static
via 4, [1/0], static
via 1, [1/0], static
via 2016::, [1/0], static
via 2015::, [1/0], static
```
##### Test Fail Criteria

## VLAN Interface

### Invalid VLAN input parameters
#### Objective
Test invalid interface VLAN input parameters.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_intervlan.py

#### Setup
Single Switch Topology
#### Description
1. Verify if we can add out of range VLANs <1-4094>.
Run the commands:
```
root(config)interface vlan 4095
root(config)interface vlan4095
root(config)interface vlan 0
root(config)interface vlan0
```
1. Check the invalid interface VLAN input parameters (VLAN ID: 2abc & abc2abc & abc#$)
Run the commands:
```
root(config)interface vlan 2abc
root(config)interface vlan2abc
root(config)interface vlan abc2abc
root(config)interface vlanabc2abc
root(config)interface vlan abc#$
root(config)interface vlanabc#$
```
1. Delete the interface VLAN outside range <1-4094>.
Run the commands:
```
root(config)no interface vlan 4095
root(config)no interface vlan4095
root(config)no interface vlan 0
root(config)no interface vlan0
```
#### Test Result Criteria
##### Test Pass Criteria
1. The CLI displays a message specifying that the VLAN is out of range.
2. Successfully verified the invalid VLAN ID (VLAN ID: 2abc & abc2abc & abc#$)

##### Test Fail Criteria

### Add/Delete VLAN interface
#### Objective
Test adding and deleting the VLAN interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_intervlan.py

#### Setup
Single Switch Topology
#### Description
1. Add the VLAN interface and verify if the VRF can see it.
Run the commands:
```
root(config)interface vlan 1
root(config)do show vrf
```
1. Verify you can see the interface and port created in OVSDB. Also, verify they are attached to a bridge with the same name as the interface name and with an internal interface of type internal.
	* Verify the interface exists in OVSDB.
	```
	/usr/bin/ovs-vsctl get interface vlan%s name
	```
	* Verify the interface type.
	```
	/usr/bin/ovs-vsctl get interface vlan%s type
	```
	* Verify the port exists in OVSDB.
	```
	/usr/bin/ovs-vsctl get port vlan%s name
	```
	* Verify the interface UUID in port row.
	```
	/usr/bin/ovs-vsctl get interface vlan%s _uuid
	/usr/bin/ovs-vsctl get port vlan%s interfaces
	```
	* Verify the port is in the bridge.
	```
	/usr/bin/ovs-vsctl get port vlan%s _uuid
	/usr/bin/ovs-vsctl get bridge bridge_normal ports
	```
	* Verify the port is in VRF.
	```
	/usr/bin/ovs-vsctl get port vlan%s _uuid
	/usr/bin/ovs-vsctl get vrf vrf_default ports
	```
1. Delete the interface VLAN, and verify if VRF can see it.
Run the commands:
```
root(config)no interface vlan 1
root(config)do show vrf
```
1. Delete the VLAN interface from OVSDB with the same name as the interface name.
	* Check for the interface name in OVSDB.
	```
	/usr/bin/ovs-vsctl get interface vlan%s name
	```
	* Check for the port name in OVSDB.
	```
	/usr/bin/ovs-vsctl get port vlan%s name
	```
1. Check the addition and deletion of multiple interfaces.

#### Test Result Criteria

##### Test Pass Criteria
1. Add a VLAN interface and attach it to a VRF.
1. On the successful configuration, OVSDB should have a port row and a corresponding interface of type internal.
1. Delete a VLAN interface and detach it from VRF.
1. On the VLAN interface delete, the port and interface should not be present in OVSDB.
1. Successfully add and delete these interfaces.

##### Test Fail Criteria

### Modify VLAN interface
#### Objective
Modify the VLAN interface configuration to test the `do show running-config` command.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_intervlan.py

#### Setup
Single Switch Topology
#### Description
Check the `do show running-config` command.
Run the commands:
```
root(config)interface vlan2
root(config-if-vlan)ipv6 address 2002::1/120
root(config-if-vlan)ip address 10.1.1.1/8
root(config-if-vlan)ip address 10.1.1.3/8 secondary
root(config-if-vlan)ipv6 address 2002::2/120 secondary
root(config-if-vlan)do show running-config
```
#### Test Result Criteria
##### Test Pass Criteria
The IP address should be displayed for the `do show running-config` command.
##### Test Fail Criteria
