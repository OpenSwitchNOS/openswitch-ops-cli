# Subinterface Test Cases

## Contents

- [Verify subinterface configurations](#verify-subinterface-configurations)
	- [Objective](#objective)
	- [Requirements](#requirements)
    - [Setup](#setup)
		- [Topology diagram](#topology-diagram)
		- [Test setup](#test-setup)
	- [Test case 1.01 : Verify subinterface entry in port table](#test-case-101-verify-subinterface-entry-in-port-table)
	- [Test case 1.02 : Verify subinterface entry in interface table](#test-case-102-verify-subinterface-entry-in-interface-table)
	- [Test case 1.03 : Assign IPv4 to subinterface](#test-case-103-assign-ipv4-to-subinterface)
	- [Test case 1.04 : Check Invalid IPv4 configuration](#test-case-104-check-invalid-ipv4-configuration)
	- [Test case 1.05 : Check Invalid IPv6 configuration](#test-case-105-check-invalid-ipv6-configuration)
	- [Test case 1.06 : Assign IPv6 to subinterface](#test-case-106-assign-ipv6-to-subinterface)
	- [Test case 1.07 : Set encapsulation type for subinterface](#test-case-107-set-encapsulation-type-for-subinterface)
	- [Test case 1.08 : Set invalid encapsulation type for subinterface](#test-case-108-set-invalid-encapsulation-type-for-subinterface)
	- [Test case 1.09: No dot1Q encapsulation](#test-case-109-no-dot1q-encapsulation)
	- [Test case 1.10 Check maximum range of subinterface](#test-case-110-check-maximum-range-of-subinterface)
	- [Test case 1.11: Check minimum range of subinterface](#test-case-111-check-minimum-range-of-subinterface)
	- [Test case 1.12: Create subinterface with Invalid L3 interface](#test-case-112-create-subinterface-with-invalid-l3-interface)
	- [Test case 1.13: Assign duplicate IPv4 address to other subinterface](#test-case-113-assign-duplicate-ipv4-address-to-other-subinterface)
	- [Test case 1.14: Delete a nonexistent subinterface](#test-case-114-delete-a-nonexistent-subinterface)
	- [Test case 1.15: Multiple deletion of subinterface](#test-case-115-multiple-deletion-of-subinterface)
	- [Test case 1.16: Sub interface name with special symbols](#test-case-116-sub-interface-name-with-special-symbols)

## Verify subinterface configurations
### Objective
To create a subinterface entry in the port and interface tables.
### Requirements
The requirements for this test case are:
 - Docker version 1.7 or above.
 - Accton AS5712 switch docker instance.

## Setup
### Topology diagram
```ditaa
              +------------------+
              |                  |
              |  AS5712 switch   |
              |                  |
              +------------------+
```
### Test setup
AS5712 switch instance

## Test case 1.01 : Verify subinterface entry in port table
### Description
Verify whether or not the subinterface entry is created in the port table.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface entry is created in the port table.
#### Test fail criteria
This test fails if the subinterface entry is not created in the port table.

## Test case 1.02 : Verify subinterface entry in interface table
### Description
Verify whether or not the subinterface entry is created in the interface table.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface entry is created in the interface table.
#### Test fail criteria
This test fails if the subinterface entry is not created in the interface table.

## Test case 1.03 : Assign IPv4 to subinterface
### Description
Verify whether or not IPv4 is assigned to the subinterface.
### Test result criteria
#### Test pass criteria
This test passes if IPv4 is assigned to the subinterface.
#### Test fail criteria
This test fails if IPv4 is not assigned to the subinterface.

## Test case 1.04 : Check Invalid IPv4 configuration
### Description
Verify whether or not an invalid IPv4 is assigned to the subinterface.
### Test result criteria
#### Test pass criteria
This test passes if the IP is not assigned to the subinterface.
#### Test fail criteria
This test fails if the IP is assigned to the subinterface.

## Test case 1.05 : Check Invalid IPv6 configuration
### Description
Verify whether or not an invalid IPv6 is assigned to the subinterface.
### Test result criteria
#### Test pass criteria
This test passes if the IP is not assigned to the subinterface.
#### Test fail criteria
This test fails if the IP is assigned to the subinterface.

## Test case 1.06 : Assign IPv6 to subinterface
### Description
Verify whether or not an IPv6 is assigned to the subinterface.
### Test result criteria
#### Test pass criteria
This test passes if an IPv6 is assigned to the subinterface.
#### Test fail criteria
This test fails if an IPv6 is not assigned to the subinterface.

## Test case 1.07 : Set encapsulation type for subinterface
### Description
Verify whether or not the encapsulation is set to vlan-id.
### Test result criteria
#### Test pass criteria
This test passes if the encapsulation is set to the subinterface.
#### Test fail criteria
This test fails if the encapsulation is not set to the subinterface.

## Test case 1.08 : Set invalid encapsulation type for subinterface
### Description
Verify whether or not the invalid encapsulation is set to the vlan-id.
### Test result criteria
#### Test pass criteria
This test passes if the encapsulation is not set to the subinterface.
#### Test fail criteria
This test fails if the encapsulation is set to the subinterface.

## Test case 1.09: No dot1Q encapsulation
### Description
Verify whether or not the encapsulation dot1Q is removed.
### Test result criteria
#### Test pass criteria
This test passes if the encapsulation dot1Q is removed.
#### Test fail criteria
This test fails if the encapsulation dot1Q is not removed.

## Test case 1.10 Check maximum range of subinterface
### Description
Verify whether or not the subinterface is created if interface number is above upper limit.
###Test Result Criteria
#### Test pass criteria
This test passes if the subinterface is not created.
#### Test fail criteria
This test fails if the subinterface is created.

## Test case 1.11: Check minimum range of subinterface
### Description
Verify whether or not the subinterface is created if interface number is below lower limit.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface is not created.
#### Test fail criteria
This test fails if the subinterface is created.

## Test case 1.12: Create subinterface with Invalid L3 interface
### Description
Verify whether or not the subinterface is created if an L3 interface is invalid.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface is not created.
#### Test fail criteria
This test fails if the subinterface is created.

## Test case 1.13: Assign duplicate IPv4 address to other subinterface
### Description
Verify whether or not a duplicate IPv4 address is assigned.
### Test result criteria
#### Test pass criteria
This test passes if the IP address is not assigned.
##### Test fail criteria
This test fails if the IP address is assigned.

## Test case 1.14: Delete a nonexistent subinterface
### Description
Verify whether or not a nonexistent subinterface is deleted.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface is not deleted.
#### Test fail criteria
This test fails if the subinterface is deleted.

## Test case 1.15: Multiple deletion of subinterface
### Description
Delete a subinterface multiple times.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface is not deleted.
#### Test fail criteria
This test fails if the subinterface is deleted multiple times.

## Test case 1.16: Sub interface name with special symbols
### Description
Verify that a subinterface is assigning a number with some special symbols.
### Test result criteria
#### Test pass criteria
This test passes if the subinterface is not created.
#### Test fail criteria
This test fails if the subinterface is created.
