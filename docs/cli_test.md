# CLI Component Tests

## Contents

- [VRF Add/Delete](#vrf-adddelete)
	- [Add VRF with valid name](#add-vrf-with-valid-name)
	- [Add VRF with name greater than maximum allowed length](#add-vrf-with-name-greater-than-maximum-allowed-length)
	- [Re-add default VRF](#re-add-default-vrf)
	- [Delete default VRF](#delete-default-vrf)
- [Attach/detach interface to/from VRF](#attachdetach-interface-tofrom-vrf)
	- [Attach L3 interface to default VRF](#attach-l3-interface-to-default-vrf)
	- [Attach interface to non existent VRF](#attach-interface-to-non-existent-vrf)
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
	- [Scenarios for IPv6 Address duplication](#scenarios-for-ipv6-address-duplication)
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
	- [Internal VLAN range with start equal than end](#internal-vlan-range-with-start-equal-than-end)
	- [Descending internal VLAN range](#descending-internal-vlan-range)
	- [Ascending internal VLAN range](#ascending-internal-vlan-range)
	- [Default internal VLAN with no configuration](#default-internal-vlan-with-no-configuration)
- [IPv4 Add](#ipv4-add)
	- [Add static route without distance specified](#add-static-route-without-distance-specified)
	- [Add static route with distance specified](#add-static-route-with-distance-specified)
	- [Add static route with IPv4 Prefix and New Nexthop IP](#add-static-route-with-ipv4-prefix-and-new-nexthop-ip)
	- [Add static route with same IPv4 Prefix, new Nexthop IP and different distance](#add-static-route-with-same-ipv4-prefix-new-nexthop-ip-and-different-distance)
	- [Add static route with Nexthop as interface](#add-static-route-with-nexthop-as-interface)
	- [Add static route with Nexthop as interface](#add-static-route-with-nexthop-as-interface)
	- [Add static route with same prefix and new Nexthop as interface](#add-static-route-with-same-prefix-and-new-nexthop-as-interface)
	- [Add static route with same prefix and new Nexthop as interface with distance](#add-static-route-with-same-prefix-and-new-nexthop-as-interface-with-distance)
	- [Add static route with wrong prefix format](#add-static-route-with-wrong-prefix-format)
	- [Add static route with wrong nexthop format](#add-static-route-with-wrong-nexthop-format)
- [IPv4 Show](#ipv4-show)
	- [Show IPv4 routes](#show-ipv4-routes)
	- [Show empty IPv4 routes table](#show-empty-ipv4-routes-table)
- [IPv4 Delete](#ipv4-delete)
	- [Remove static route](#remove-static-route)
	- [Remove the last nexthop entry related](#remove-the-last-nexthop-entry-related)
	- [Remove static route with invalid distance](#remove-static-route-with-invalid-distance)
	- [Remove static route with invalid prefix](#remove-static-route-with-invalid-prefix)
	- [Remove static route with invalid Nexthop IP](#remove-static-route-with-invalid-nexthop-ip)
	- [Remove static route with invalid Nexthop interface](#remove-static-route-with-invalid-nexthop-interface)
- [IPv6 Add](#ipv6-add)
	- [Add static route with IPv6 prefix and Nexthop IPv6 without distance](#add-static-route-with-ipv6-prefix-and-nexthop-ipv6-without-distance)
	- [Add static route with IPv6 prefix, Nexthop IPv6 and distance](#add-static-route-with-ipv6-prefix-nexthop-ipv6-and-distance)
	- [Add static route with same IPv6 prefix and new Nexthop IPv6](#add-static-route-with-same-ipv6-prefix-and-new-nexthop-ipv6)
	- [Add static route with same IPv6 prefix, Nexthop IPv6 and distance](#add-static-route-with-same-ipv6-prefix-nexthop-ipv6-and-distance)
	- [Add static route with same IPv6 prefix, different Nexthop IPv6 and distance](#add-static-route-with-same-ipv6-prefix-different-nexthop-ipv6-and-distance)
	- [Add static route with Nexthop as a non-L3 interface](#add-static-route-with-nexthop-as-a-non-l3-interface)
	- [Add static route with Nexthop as interface](#add-static-route-with-nexthop-as-interface)
	- [Add static route with same prefix and new Nexthop as interface](#add-static-route-with-same-prefix-and-new-nexthop-as-interface)
	- [Add static route with same prefix and new Nexthop as L3 interface with distance](#add-static-route-with-same-prefix-and-new-nexthop-as-l3-interface-with-distance)
	- [Add static route with wrong prefix](#add-static-route-with-wrong-prefix)
	- [Add static route with wrong Nexthop](#add-static-route-with-wrong-nexthop)
	- [Add static route with trailing 0s](#add-static-route-with-trailing-0s)
- [IPv6 Show](#ipv6-show)
	- [Show IPv6 routes table](#show-ipv6-routes-table)
	- [Show empty IPv6 route table](#show-empty-ipv6-route-table)
- [IPv6 Delete](#ipv6-delete)
	- [Remove IPv6 static route](#remove-ipv6-static-route)
	- [Remove last Nexthop entry for a IPv6 static route](#remove-last-nexthop-entry-for-a-ipv6-static-route)
	- [Remove IPv6 static route with invalid distance](#remove-ipv6-static-route-with-invalid-distance)
	- [Remove IPv6 static route with invalid prefix](#remove-ipv6-static-route-with-invalid-prefix)
	- [Remove IPv6 static route with invalid Nexthop IP](#remove-ipv6-static-route-with-invalid-nexthop-ip)
	- [Remove IPv6 static route with invalid Nexthop interface](#remove-ipv6-static-route-with-invalid-nexthop-interface)
- [Show RIB for L3 Static Routes (Ipv4 and IPv6)](#show-rib-for-l3-static-routes-ipv4-and-ipv6)
	- [Display RIB entries](#display-rib-entries)

## VRF Add/Delete

###  Add VRF with valid name
#### Objective
Test case checks if non-default VRFs can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Add a new VRF with a valid name. Only default VRF is allowed.

Run command:
```
root(config)# vrf vrf0
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Non-default VRFs not supported.`
##### Test Fail Criteria

###  Add VRF with name greater than maximum allowed length
#### Objective
Test case checks adding a VRF with name longer than maximum allowed length.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Add a VRF with a name more than 32 characters long
Run command
```
root(config)# vrf thisisavrfnamewhichismorethan32characters
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Non-default VRFs not supported.`
##### Test Fail Criteria


### Re-add default VRF
#### Objective
Test case checks if default VRF can be added again.
#### Requirements
- vrf_default previously added
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Add default VRF again.

Run command:
```
root(config)# vrf vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Default VRF already exists`
##### Test Fail Criteria

###  Delete default VRF
#### Objective
Test case checks if default VRF can be deleted.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Delete the default VRF.

Run command:
```
root(config)# no vrf vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: Cannot delete default VRF.`
##### Test Fail Criteria

## Attach/detach interface to/from VRF

### Attach L3 interface to default VRF
#### Objective
Test case checks attaching an L3 interface to default VRF.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Attach an L3 interface to default VRF.

Run commands:
```
root(config)# interface 1
root(config-if)# vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Already attached to default VRF`

##### Test Fail Criteria

###  Attach interface to non existent VRF
#### Objective
Test case checks if an interface can be attached to VRF
which does not exist.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Attach an interface to a VRF that does not exist.

Run commands:
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
Test case checks if an L2 interface can be attached to default VRF.
#### Requirements
- vrf_default previously created
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Attach an L2 interface to VRF.

Run commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: Interface 1 is not L3`
##### Test Fail Criteria

###  Detach L3 interface from default VRF
#### Objective
Test case checks if L3 interface can be detached from default VRF.
#### Requirements
- vrf_default previously created
- interface 1 attached to vrf_default
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py
#### Setup
Single Switch Topology [S1]

#### Description
Detach an L3 interface from default VRF.

Run commands:
```
root(config)# interface 1
root(config-if)# no vrf attach vrf_default
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Cannot detach from default VRF`
##### Test Fail Criteria

###  Configure L3 interface into L2 interface
#### Objective
Test case checks multiple add/delete of L3 interfaces.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Add multiple L3 interfaces and try to make one of them L2.

Run commands:
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

Run command:
```
show vrf
```
The interface should not appear.
##### Test Fail Criteria

## IP address assign/remove

###  Assign IP address to non-L3 interface
#### Objective
Test case checks assigning an IP address to an L2 interface is not allowed.

#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign IP address to an interface which is not L3.

Run commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: Interface 1 is not L3`
##### Test Fail Criteria


###  Remove IP address from L3 interface
#### Objective
Test case checks removal of IP address from an interface which has no IP address.

#### Requirements
- No IP address in interface 1
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove an IP address from an L3 interface that does not have
any IP address configured on it.

Run commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: No IP Address configured on interface 1`
##### Test Fail Criteria

###  Assign IP Address to L3 interface
#### Objective
Test case checks assigning an IP address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Assign an IP address to an L3 interface.

Run commands:
```
root(config)# interface 1
root(config-if)# ip address 10.0.20.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
IP address must be added in for the port row
##### Test Fail Criteria

###  Scenarios for IP Address duplication
#### Objective
Test case checks for duplicate primary IP address on a L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Check for duplicate IP address assignment for the following cases:

Run commands to assign primary and secondary IP address:
```
root(config)# interface 1
root(config-if)#ip address 10.0.20.3/24
root(config-if)#ip address 10.0.20.4/24 secondary
```

1. Assign existing primary IP address as primary
```
root(config-if)#ip address 10.0.20.3/24
```
1. Assign existing secondary IP address as primary
```
root(config-if)#ip address 10.0.20.4/24
```
1. Assign existing primary IP address as secondary
```
root(config-if)#ip address 10.0.20.3/24 secondary
```
1. Assign existing secondary IP address as secondary
```
root(config-if)#ip address 10.0.20.4/24 secondary
```

#### Test Result Criteria
##### Test Pass Criteria
Error message will vary depending in the step on the description:
1. `Error: IP Address is already assigned to interface 1 as primary.`
2. `Error: IP Address is already assigned to interface 1 as secondary.`
3. `Error: IP Address is already assigned to interface 1 as primary.`
4. `Error: IP Address is already assigned to interface 1 as secondary.`
##### Test Fail Criteria

###  Remove primary IP address with existing secondary IP address
#### Objective
Test case checks if primary IP address can be removed without removing secondary IP addresses.
#### Requirements
- Primary IP address 10.0.20.3/24 assigned to interface 1
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove primary IP address without removing all secondary IP addresses.

Run commands:
```
root(config)# interface 1
root(config-if)# ip address 10.0.20.4/24 secondary
root(config-if)# no ip address 10.0.20.3/24
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Delete all secondary IP addresses before deleting primary.`
##### Test Fail Criteria

###  Remove non-existent IP address
#### Objective
Test case checks if primary IP address can be removed without removing secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove an IP address from an L3 interface by giving the wrong IP address.

Run commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.30.2/24
```
#### Test Result Criteria
##### Test Pass Criteria
Command will ouput:
`Error: IP Address 10.0.30.2/24 not found.`
##### Test Fail Criteria

###  Remove non-existent secondary IP address
#### Objective
Test case checks removal of secondary IP address from an L3 interface which has no secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove a secondary IP address from an L3 interface that does not have any secondary IP address configured on it.

Run commands:
```
root(config)# interface 1
root(config-if)# no ip address 10.0.30.2/24 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: No secondary IP Address configured on interface 1.`
##### Test Fail Criteria

###  Assign multiple secondary IP addresses
#### Objective
Test case checks assigning multiple secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign multiple secondary IP addresses to an L3 interface.

Run commands:
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
Test case checks removal of multiple secondary IP addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove multiple secondary IP addresses from an L3 interface.

Run commands:
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
Test case checks if IPv6 address can be assigned to an L2 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign IPv6 address to an interface which is not L3.

Run commands:
```
root(config)# interface 1
root(config-if)# no routing
root(config-if)# ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: Interface 1 is not L3.`
##### Test Fail Criteria

###  Remove IPv6 address
#### Objective
Test case checks removal of IPv6 address from an interface which has no IP address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove an IPv6 address from an L3 interface that does not have
any IPv6 address configured on it.

Run commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: No IPv6 Address configured on interface 1`
##### Test Fail Criteria

###  Assign IPv6 address
#### Objective
Test case checks assigning an IPv6 address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign an IPv6 address to an L3 interface.

Run commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2002::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
IPv6 address must be added in for the port row.
##### Test Fail Criteria

###  Update IPv6 address
#### Objective
Test case checks updating an IPv6 address to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Update IPv6 address on an L3 interface that already has an IPv6 address configured on it.

Run commands:
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
Test case checks for all duplicate scenarios between IPv6 addresses on an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Check for duplicate IPv6 address assignment for the following cases:

Run commands to assign primary and secondary IPv6 address:
```
root(config)# interface 1
root(config-if)#ipv6 address 10.0.20.3/24
root(config-if)#ipv6 address 10.0.20.4/24 secondary
```

1. Assign existing primary IPv6 address as primary
```
root(config-if)#ipv6 address 10.0.20.3/24
```
1. Assign existing secondary IP address as primary
```
root(config-if)#ipv6 address 10.0.20.4/24
```
1. Assign existing primary IP address as secondary
```
root(config-if)#ipv6 address 10.0.20.3/24 secondary
```
1. Assign existing secondary IP address as secondary
```
root(config-if)#ipv6 address 10.0.20.4/24 secondary
```

#### Test Result Criteria
##### Test Pass Criteria
Error message will vary depending in the step on the description:
1. `Error: IP Address is already assigned to interface 1 as primary.`
2. `Error: IP Address is already assigned to interface 1 as secondary.`
3. `Error: IP Address is already assigned to interface 1 as primary.`
4. `Error: IP Address is already assigned to interface 1 as secondary.`
##### Test Fail Criteria

###  Remove IPv6 address with existing secondary IPv6 address
#### Objective
Test case checks if primary IPv6 address can be removed without removing secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove primary IPv6 address without removing all secondary IPv6 addresses.

Run commands:
```
root(config)# interface 1
root(config-if)# ipv6 address 2001::2/128 secondary
root(config-if)# no ipv6 address 2001::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Delete all secondary IP addresses before deleting primary.`
##### Test Fail Criteria

###  Remove non-existent IPv6 address
#### Objective
Test case checks removal of primary IPv6 address from an L3 interface by giving wrong IPv6 address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Remove an IPv6 address from an L3 interface by giving the wrong IPv6 address.

Run commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2004::1/128
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: IPv6 Address 2004::1/128 not found.`
##### Test Fail Criteria

### Remove IPv6 address
#### Objective
Test case checks removal of primary IPv6 address from an L3 interface by giving correct IPv6 address.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]

#### Description
Remove an IPv6 address from an L3 interface by giving the correct IPv6 address.

Run commands:
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
Test case checks removal of secondary IPv6 address from an L3 interface which has no secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove a secondary IPv6 address from an L3 interface that does not have any secondary IPv6 address configured on it.

Run commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2004::1/128 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Error: No secondary IPv6 Address configured on interface 1.`
##### Test Fail Criteria

### Assign multiple secondary IPv6 addresses
#### Objective
Test case checks assigning multiple secondary IPv6 addresses.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign multiple secondary IPv6 addresses to an L3 interface.

Run commands:
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
Test case checks removal of multiple secondary IPv6 addresses.
#### Requirements
- IPv6 address 2001::2/128 and 2001::3/128 needs to be added as secondary
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Remove multiple secondary IPv6 addresses from an L3 interface.

Run commands:
```
root(config)# interface 1
root(config-if)# no ipv6 address 2001::2/128 secondary
root(config-if)# no ipv6 address 2001::3/128 secondary
```
#### Test Result Criteria
##### Test Pass Criteria
Secondary IPv6 addresses must be removed from port row.
##### Test Fail Criteria

## Toggle L2 / L3 interface

### Assign IP and IPv6 address
#### Objective
Test case checks `routing` command by configuring IP addresses to an L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Assign IP and IPv6 addresses to an L3 interface.

Run commands:
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
Test case checks if IP addresses are removed when L3 interface becomes L2.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Make an L3 interface L2 by using 'no routing' command.

Run command:
```
root(config)# interface 1
root(config-if)# no routing
```
#### Test Result Criteria
##### Test Pass Criteria
IP and IPv6 addresses previously configured must be removed from port row.
Port must be added to default bridge.
Command `show vrf` must not have interface 1.
##### Test Fail Criteria

## Show running config

### Show running configuration on multiple interfaces
#### Objective
Test case checks 'show running config' command by configuring multiple L3 interfaces.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vrf.py

#### Setup
Single Switch Topology [S1]
#### Description
Test `show running config` with different configurations

1. Run commands:
```
root(config)#interface 1
root(config-if)#no routing
root(config-if)#exit
```
Then check result with `show running configurations`
1. Run commands:
```
root(config)#interface 2
root(config-if)#ipv6 address 2002::1/128
root(config-if)#ip address 10.1.1.1/8
root(config-if)#ip address 10.1.1.3/8 secondary
root(config-if)#ipv6 address 2002::2/128 secondary
root(config-if)#exit
```
Then check result with `show running configurations`
1. Run commands:
```
root(config)#interface 3
root(config-if)#lldp transmission
root(config-if)#exit
```
Then check result with `show running configurations`
#### Test Result Criteria
##### Test Pass Criteria
1. Interface 1 with no routing.
2. Interface 2 with only the IPv4 and IPv6 addresses. It should not show routing by default
3. Interface 3 with lldp transmission.
##### Test Fail Criteria

### Internal VLAN range at system bootup
#### Objective
Test case checks the internal VLAN range on system bootup.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check internal VLAN range at system bootup.

Run command:
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
Test case checks the internal VLAN range by giving a start value greater than the end value of the range.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check internal VLAN range by giving start > end

Run command:
```
root(config)# vlan internal range 100 10 ascending
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Invalid VLAN range. End VLAN must be greater or equal to start VLAN.`
##### Test Fail Criteria

### Internal VLAN range with start equal than end
#### Objective
Test case checks if both start and end of a range can be equal.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check internal VLAN range by giving start = end.

Run command:
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
Test case checks if descending internal VLAN is configured properly.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check descending internal VLAN range.

Run command:
```
root(config)# vlan internal range 100 200 descending
```
#### Test Result Criteria
##### Test Pass Criteria
VLAN range must be set properly.
```
Internal VLAN range  : 100-200
Internal VLAN policy  : descending
```
##### Test Fail Criteria

###  Ascending internal VLAN range
#### Objective
Test case checks if ascending internal VLAN is configured properly.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check ascending internal VLAN range.

Run command:
```
root(config)# vlan internal range 10 100 ascending
```
#### Test Result Criteria
##### Test Pass Criteria
VLAN range must be set properly.
```
Internal VLAN range  : 10-100
Internal VLAN policy  : ascending
```
##### Test Fail Criteria

###  Default internal VLAN with no configuration
#### Objective
Test case checks if internal VLAN range goes back to default values on 'no' command.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_vlan_internal.py (Internal VLAN Range CLI)

#### Setup
Single Switch Topology [S1]
#### Description
Check default internal VLAN range on no command.

Run command:
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
Test case checks addition of static route with IPv4 prefix and Nexthop IP without specifying distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with IPv4 Prefix and Nexthop IP, without distance specified.

Run command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. Default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table.

Run command:
```
root(config)# ip netns exec swns ip route
```
Command will output the route in the kernel
##### Test Fail Criteria

###  Add static route with distance specified
#### Objective
Test case checks addition of static route with IPv4 prefix, Nexthop IP and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with IPv4 Prefix and Nexthop IP with distance specified.

Run command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. Distance attached will be 3.
Can verify by looking at the entries in the Route and Nexthop table.

Run command:
```
root(config)# ip netns exec swns ip route
```
Command will output the route in the kernel
##### Test Fail Criteria

###  Add static route with IPv4 Prefix and New Nexthop IP
#### Objective
Test case checks adding a static route with same IPv4 prefix and new Nexthop IP.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same IPv4 Prefix and new Nexthop IP.

Run command:
```
root(config)# ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. Default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table. There
will be multiple Nexthop entries present in the DB for same prefix.

Run command:
```
ip netns exec swns ip route
```
Command will output the route in the kernel
##### Test Fail Criteria

###  Add static route with same IPv4 Prefix, new Nexthop IP and different distance
#### Objective
Test case checks adding a static route with same IPv4 prefix but different Nexthop IP and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same IPv4 Prefix, but new Nexthop IP and different distance.

Run command:
```
ip route 1.1.1.1/8 1.1.1.3 3
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: CLI command will not get accepted for the given prefix/route, distance is set by the
first nexthop entered through CLI. Rest of the corresponding routes can accept only the
distance configured previously.
##### Test Fail Criteria

###  Add static route with Nexthop as interface
#### Objective
Test case checks adding a static route with Nexthop as a non-L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with Nexthop as an interface without configuring interface.

Run command:
```
root(config)# ip route 1.1.1.1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Error: Interface 2 not configured message`

Need to configure the interface first by assigning ip address to it
##### Test Fail Criteria

###  Add static route with Nexthop as interface
#### Objective
Test case checks adding a static route with Nexthop as a L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with Nexthop as an interface.

Run commands:
```
root(config)# interface 2
root(config-if)# ip address 1.1.1.2/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table.

Commnad `ip netns exec swns ip route ` will show the route in the kernel via dev.
##### Test Fail Criteria

### Add static route with same prefix and new Nexthop as interface
#### Objective
Test case checks adding a static route with same prefix and new Nexthop as a L3 interface. The route will not have distance
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same prefix and new Nexthop as an interface.

Run command:
```
root(config)# interface 3
root(config-if)# ip address 1.1.1.3/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. The default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table.
There will be multiple Nexthop entries present in the DB for same prefix.
Command `ip netns exec swns ip route ` will show the route in the kernel via dev 3.
##### Test Fail Criteria

### Add static route with same prefix and new Nexthop as interface with distance
#### Objective
Test case checks adding a static route with same prefix and new Nexthop as a L3 interface. The route will have distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same prefix and new Nexthop as an interface with preferred distance.

Run commands:
```
root(config)# interface 4
root(config-if)# ip address 1.1.1.4/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 4 4
```

#### Test Result Criteria
##### Test Pass Criteria
Error: CLI command will not get accepted as for a given prefix/route, distance is set by the first nexthop entered through CLI.
Rest of the corresponding routes can accept only the distance configured previously.
##### Test Fail Criteria

### Add static route with wrong prefix format
#### Objective
Test case checks adding a static route with wrong prefix format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with wrong prefix format.

Run command:
```
root(config)# ip route 1.1.1.1 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Unknown command`
##### Test Fail Criteria

### Add static route with wrong nexthop format
#### Objective
Test case checks adding a static route with wrong Nexthop format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with wrong nexthop format.

Run command:
```
root(config)# ip route 1.1.1.1 1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Interface 1.1.2 is not configured`
##### Test Fail Criteria

## IPv4 Show

### Show IPv4 routes
#### Objective
Test case checks if configured ip routes are displayed properly.
#### Requirements
- IPv4 routes added to the database
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Show when ipv4 entries are present in the database.

Run commands:
```
root(config)# do show ip route
```
#### Test Result Criteria
##### Test Pass Criteria
Run command `do show ip route`
The command will output:
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
Test case checks if proper error message is shown when there are no ip routes to display.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Show table when no ipv4 entries are present in the database

Run command:
```
root(config)# do show ip route
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`No ipv4 routes configured`
##### Test Fail Criteria

## IPv4 Delete

### Remove static route
#### Objective
Test case checks removal of static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Deleting an entry which is present from the database. You can specify the distance which is entered or leave it blank.

Run commands:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2
(OR)
root(config)# no ip route 1.1.1.1/8 1.1.1.2 2
```
#### Test Result Criteria
##### Test Pass Criteria
Entry will be deleted from the database and the kernel.
Entry related to this nexthop will be deleted from the DB but not the entry related to the Route prefix, as it might have multiple nexthops.
Command `ip netns exec swns ip route` should not show the route.

##### Test Fail Criteria

### Remove the last nexthop entry related
#### Objective
Test case checks removal of last Nexthop entry for a static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Deleting the last nexthop entry related for a route.

Run command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.3
```
#### Test Result Criteria
##### Test Pass Criteria
Entry will be deleted from the database and the kernel.
Entry related to this nexthop will be deleted from the DB and from the Route table.
Command `ip netns exec swns ip route ` should not show the route.

##### Test Fail Criteria

### Remove static route with invalid distance
#### Objective
Test case checks removal of static route with invalid distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove static route specifying wrong/different distance

Run command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2 3
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`No such ip route found`
##### Test Fail Criteria

### Remove static route with invalid prefix
#### Objective
Test case checks removal of static route with invalid prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove static route specifying wrong prefix

Run command:
```
root(config)# no ip route 1.1.1.1/8 1.1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`Unknown Command`
##### Test Fail Criteria

### Remove static route with invalid Nexthop IP
#### Objective
Test case checks removal of static route with invalid Nexthop IP.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove static route Specifying wrong nexthop IP

Run command:
```
root(config)# no ip route 1.1.1.1/8 1.1.2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`No such ip route found`
##### Test Fail Criteria

### Remove static route with invalid Nexthop interface
#### Objective
Test case checks removal of static route with invalid Nexthop interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove static route specifying wrong nexthop interface.

Run command:
```
root(config)# no ip route 1.1.1.1/8 4
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output:
`No such ip route found`
##### Test Fail Criteria

## IPv6 Add

### Add static route with IPv6 prefix and Nexthop IPv6 without distance
#### Objective
Test case checks addition of static route with IPv6 prefix and Nexthop IPv6 without specifying distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with IPv6 Prefix and Nexthop IP, without distance specified.

Run command:
```
root(config)# ipv6 route 2001::1/8 2001:: 2
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. The default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table.
Command `ip netns exec swns ip -6 route` will show the route in the kernel
##### Test Fail Criteria

### Add static route with IPv6 prefix, Nexthop IPv6 and distance
#### Objective
Test case checks addition of static route with IPv6 prefix, Nexthop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with IPv4 Prefix and Nexthop IP with distance specified.

Run command:
```
root(config)# ipv6 route 2001::1/8 2001:: 2 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. The distance attached will be 3.
Can verify by looking at the entries in the Route and Nexthop table.
Command ` ip netns exec swns ip -6 route` will show the route in the kernel
##### Test Fail Criteria

### Add static route with same IPv6 prefix and new Nexthop IPv6
#### Objective
Test case checks adding a static route with same IPv6 prefix and new Nexthop IPv6.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same IPv6 Prefix and new Nexthop IP.

Run command:
```
root(config)# ipv6 route 2001::1/8 2001:: 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. Default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table. There
will be multiple Nexthop entries present in the DB for same prefix.
##### Test Fail Criteria

### Add static route with same IPv6 prefix, Nexthop IPv6 and distance
#### Objective
Test case checks adding a static route with same IPv6 prefix but different Nexthop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same IPv6 Prefix, but new Nexthop IP and different distance.

Run command:
```
root(config)# ipv6 route 2001::1/8 2001::4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: CLI command will fail for given prefix/route, distance is set by the
first nexthop entered through CLI. Rest of the corresponding routes can accept only the distance configured previously.
Command `ip netns exec swns ip -6 route` will show the route in the kernel.
##### Test Fail Criteria

### Add static route with same IPv6 prefix, different Nexthop IPv6 and distance
#### Objective
Test case checks adding a static route with same IPv6 prefix but different Nexthop IPv6 and distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same IPv6 Prefix, but new Nexthop IP and different distance.

Run command:
```
root(config)# ipv6 route 2001::1/8 2001::4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: CLI command will not get accepted for given prefix/route, distance is set by the first nexthop entered through CLI. Rest of the corresponding routes can accept only the distance configured previously.

##### Test Fail Criteria

### Add static route with Nexthop as a non-L3 interface
#### Objective
Test case checks adding a static route with Nexthop as a non-L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with Nexthop as an interface without configuring interface.

Run command:
```
root(config)# ipv6 route 2002::1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Interface 2 not configured`
Need to configure the interface first by assigning ip address to it
##### Test Fail Criteria

### Add static route with Nexthop as interface
#### Objective
Test case checks adding a static route with Nexthop as a L3 interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with Nexthop as an interface.

Run commands:
```
root(config)# interface 2
root(config-if)# ip address 2001::2/8
root(config)# exit
root(config)# ipv6 route 2001::1/8 2
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. The default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table.
Command ` ip netns exec swns ip -6 route` will show the route in the kernel via dev 2
##### Test Fail Criteria

### Add static route with same prefix and new Nexthop as interface
#### Objective
Test case checks adding a static route with same prefix and new Nexthop as a L3 interface, without distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same prefix and new Nexthop as an interface

Run commands:
```
root(config)# interface 3
root(config-if)# ip address 2001::3/8
root(config)# exit
root(config)# ipv6 route 2001::1/8 2001:: 3
```
#### Test Result Criteria
##### Test Pass Criteria
Static route will get added to the kernel. The default distance attached will be 1.
Can verify by looking at the entries in the Route and Nexthop table. There
will be multiple Nexthop entries present in the DB for same prefix.
Command ` ip netns exec swns ip route` will show the route in the kernel via dev 3.
##### Test Fail Criteria

### Add static route with same prefix and new Nexthop as L3 interface with distance
#### Objective
Test case checks adding a static route with same prefix and new Nexthop as a L3 interface, with distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with same prefix and new Nexthop as an interface with preferred distance.

Run commands:
```
root(config)# interface 4
root(config-if)# ip address 1.1.1.4/8
root(config)# exit
root(config)# ip route 1.1.1.1/8 4 4
```
#### Test Result Criteria
##### Test Pass Criteria
**Error**: CLI command will not get accepted for given prefix/route, distance is set by the first nexthop entered through CLI. Rest of the corresponding routes can accept only the distance configured previously.
##### Test Fail Criteria

### Add static route with wrong prefix
#### Objective
Test case checks adding a static route with wrong prefix format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with wrong prefix format

Run commands:
```
root(config)# ipv6 route 2001::1 ::2/8 2001:: 2
OR
ipv6 route 2001::1 2001::2
```
Only one '::' allowed to pad the entries with "0s"
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Unknown command`
##### Test Fail Criteria

### Add static route with wrong Nexthop
#### Objective
Test case checks adding a static route with wrong Nexthop format.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Add a static route with wrong nexthop format

Run commands:
```
ipv6 route 2001::1 20::2
OR
ipv6 route 2001::1 20.1.1
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Interface 1.1.2 is not configured`
##### Test Fail Criteria

###  Add static route with trailing 0s
#### Objective
Test case checks adding a static route with trailing 0s in IPv6 prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Adding a prefix with trailing 0s

Run command:
```
ipv6 route 2001::0/8 2001::2
```
#### Test Result Criteria
##### Test Pass Criteria
The trailing 0s will be trimmed/removed and stored in the database according to
the subnet specified. The prefix will be stored as follows
`ipv6 route 2001::/8 2001::2`
##### Test Fail Criteria

## IPv6 Show

### Show IPv6 routes table
#### Objective
Test case checks if configured IPv6 routes are displayed properly.
#### Requirements
- IPv6 routes configured on the switch
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
When ipv6 entries are present in the database.

Run command:
```
root(config)# do show ipv6 route
```

#### Test Result Criteria
##### Test Pass Criteria
Will see the IPv6 routes present in the DB in the following format

Run command:
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
Test case checks if proper error message is shown when there are no IPv6 routes to display.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
When no ipv6 entries are present in the database.

Run command:
```
root(config)# do show ipv6 route
```

#### Test Result Criteria
##### Test Pass Criteria
Command will output: `No ipv6 routes configured`
##### Test Fail Criteria

## IPv6 Delete

### Remove IPv6 static route
#### Objective
Test case checks removal of IPv6 static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Deleting an entry present in the database. You can specify the distance or leave it blank.

Run command:
```
root(config)# no ipv6 route 2001::1/8 2001::2
(OR)
root(config)# no ipv6 route 2001::1/8 2001::2 2
```

#### Test Result Criteria
##### Test Pass Criteria
Entry will be deleted from the database and the kernel. Entry related to this
nexthop will be deleted from the DB but not the entry related to the Route prefix, as it might have multiple nexthops.
Command `ip netns exec swns ip -6 route` should not show the route.
##### Test Fail Criteria

### Remove last Nexthop entry for a IPv6 static route
#### Objective
Test case checks removal of last Nexthop entry for a IPv6 static route.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Deleting the last nexthop entry related for a route

Run command:
```
root(config)# no ipv6 route 2001::1/8 2001::3
```

#### Test Result Criteria
##### Test Pass Criteria
Entry will be deleted from the database and the kernel. Entry related to this
nexthop will be deleted from the DB and from the Roue table, being the last
nexthop.
Command `ip netns exec swns ip -6 route` should not show the route.
##### Test Fail Criteria

### Remove IPv6 static route with invalid distance
#### Objective
Test case checks removal of IPv6 static route with invalid distance.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Specifying wrong/different distance.

Run command:
```
root(config)# no ipv6 route 2001::1/8 2001::2 5
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `No such ipv6 route found`
##### Test Fail Criteria

### Remove IPv6 static route with invalid prefix
#### Objective
Test case checks removal of IPv6 static route with invalid prefix.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove IPv6 static route specifying wrong prefix.

Run command:
```
root(config)# no ipv6 route 2001::1 2001::2
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `Unknown Command`
##### Test Fail Criteria

### Remove IPv6 static route with invalid Nexthop IP
#### Objective
Test case checks removal of IPv6 static route with invalid Nexthop IP.

#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove IPv6 static route specifying wrong nexthop IP.

Run commands:
```
root(config)# no ipv6 route 2001::1 2001:2.3
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `No such ipv6 route found`
##### Test Fail Criteria

### Remove IPv6 static route with invalid Nexthop interface
#### Objective
Test case checks removal of IPv6 static route with invalid Nexthop interface.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Remove IPv6 static rout specifying wrong nexthop interface

Run command:
```
root(config)# no ipv6 route 2001::1 4
```
#### Test Result Criteria
##### Test Pass Criteria
Command will output: `No such ipv6 route found`
##### Test Fail Criteria

## Show RIB for L3 Static Routes (Ipv4 and IPv6)

### Display RIB entries
#### Objective
Test case checks display of RIB entries.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_l3routes.py (Static Routes)

#### Setup
[S1] -> [S2]
#### Description
Check the RIB entries.

Run command:
```
root(config): so show rib
```
#### Test Result Criteria
##### Test Pass Criteria
Shows the Routing Information Base Entries for IPv4 and Ipv6 together.
Entries selected for forwarding are marked with '*'

Command will output:
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
