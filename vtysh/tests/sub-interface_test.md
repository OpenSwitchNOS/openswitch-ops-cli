# Sub-Interface Test Cases
<!-- TOC depth:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [Sub-Interface Test Cases](#sub-interface-test-cases)
	- [Verify sub-interface Configurations](#verify-sub-interface-configurations)
		- [Objective](#objective)
		- [Requirements](#requirements)
	- [Setup](#setup)
		- [Topology Diagram](#topology-diagram)
		- [Test Setup](#test-setup)
	- [Test case 1.01 : Verify sub interface entry in port table](#test-case-101-verify-sub-interface-entry-in-port-table)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.02 : Verify sub interface entry in Interface Table](#test-case-102-verify-sub-interface-entry-in-interface-table)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.03 : Assign IPv4 to sub interface](#test-case-103-assign-ipv4-to-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.04 : Check Invalid IPv4 configuration](#test-case-104-check-invalid-ipv4-configuration)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.05 : Check Invalid IPv6 configuration](#test-case-105-check-invalid-ipv6-configuration)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.06 : Assign IPv6 to sub interface](#test-case-106-assign-ipv6-to-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.07 : Set encapsulation type for sub interface](#test-case-107-set-encapsulation-type-for-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.08 : Set invalid encapsulation type for sub interface](#test-case-108-set-invalid-encapsulation-type-for-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.09 : No dot1Q encapuslation](#test-case-109-No-dot1Q-encapuslation)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.10 : Check maximum range of sub interfaces](#test-case-110-check-maximum-range-of-sub-interfaces)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.11 : Check minimum range of sub interfaces](#test-case-111-check-minimum-range-of-sub-interfaces)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.12 : Create sub interface with Invalid L3 interface](#test-case-112-create-sub-interface-with-invalid-l3-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.13 : Assign duplicate IPv4 address to other sub-interface](#test-case-113-assign-duplicate-ipv4-address-to-other-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.14 : Delete non existing sub-interface](#test-case-114-delete-non-existing-sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.15 : Multiple deletion of sub-interface](#test-case-115-Multiple-deletion-of- sub-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.16 : Sub interface name with special symobols](#test-case-116-Sub-interface-name-with-special-symobols)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
<!-- /TOC -->

##  Verify sub-interface Configurations
### Objective
To create sub interface entry in port table and interface table
### Requirements
The requirements for this test case are:
 - Docker version 1.7 or above.
 - Accton AS5712 switch docker instance.

## Setup
### Topology Diagram
              +------------------+
              |                  |
              |  AS5712 switch   |
              |                  |
              +------------------+

### Test Setup
AS5712 switch instance.

## Test case 1.01 : Verify sub interface entry in port table
### Description
Verify whether sub interface entry is created in port table.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface entry is created in port table.
#### Test Fail Criteria
Sub-interface entry is not created in port table

## Test case 1.02 : Verify sub interface entry in Interface Table
### Description
Verify whether sub interface entry is created in interface table.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface entry is created in interface table.
#### Test Fail Criteria
Sub-interface entry is not created in interface table

## Test case 1.03 : Assign IPv4 to sub interface
### Description
Verify whether ipv4 is assigned to sub-interface.
### Test Result Criteria
#### Test Pass Criteria
IPv4 is assigned to sub-interface.
#### Test Fail Criteria
IPv4 is not assigned to sub-interface.

## Test case 1.04 : Check Invalid IPv4 configuration
### Description
Verify whether invalid ipv4 is assigned to sub-interface.
### Test Result Criteria
#### Test Pass Criteria
IP is not assigned to sub-interface.
#### Test Fail Criteria
IP is assigned to sub-interface.

## Test case 1.05 : Check Invalid IPv6 configuration
### Description
Verify whether invalid ipv6 is assigned to sub-interface.
### Test Result Criteria
#### Test Pass Criteria
IP is not assigned to sub-interface.
#### Test Fail Criteria
IP is assigned to sub-interface.

## Test case 1.06 : Assign IPv6 to sub interface
### Description
Verify whether ipv6 is assigned to sub-interface.
### Test Result Criteria
#### Test Pass Criteria
IPv6 is assigned to sub-interface.
#### Test Fail Criteria
IPv6 is not assigned to sub-interface.

## Test case 1.07 : Set encapsulation type for sub interface
### Description
Whether the encapsulation is set to vlan-id.
### Test Result Criteria
#### Test Pass Criteria
Encapsulation is set to sub-interface.
#### Test Fail Criteria
Encapsulation is not to sub-interface.

## Test case 1.08 : Set invalid encapsulation type for sub interface
### Description
Whether invalid encapsulation is set to vlan-id.
### Test Result Criteria
#### Test Pass Criteria
Encapsulation is not set to sub-interface.
#### Test Fail Criteria
Encapsulation is to sub-interface.

## Test case 1.09: No dot1Q encapsulation
### Description
verify wether encapsulaton dot1Q is removed
### Test Result Criteria
#### Test Pass Criteria
Encapsulation dot1Q removed.
#### Test Fail Criteria
Encapsulation is not removed.

## Test case 1.10 Check maximum range of sub interfaces
### Description
verify whether sub interface is created if interface number is above upper limit.
###Test Result Criteria
#### Test Pass Criteria
Sub-interface is not created.
#### Test Fail Criteria
Sub-interface is created.

## Test case 1.11: Check minimum range of sub interfaces
### Description
verify whether sub-interface is created if interface number is below lower limit.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface is not created.
#### Test Fail Criteria
Sub-interface is created.

## Test case 1.12: Create sub interface with Invalid L3 interface
### Description
verify whether sub-interface is created if L3 interface is Invalid.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface is not created.
#### Test Fail Criteria
Sub-interface is created.

## Test case 1.13: Assign duplicate IPv4 address to other sub-interface
### Description
Verify duplicate IPv4 address is assigned.
### Test Result Criteria
#### Test Pass Criteria
IP address not assigned.
#### Test Fail Criteria
IP address assigned.

## Test case 1.14: Delete non existing sub-interface
### Description
Verify whether non exiting sub-interface is deleted.
### Test Result Criteria
#### Test Pass Criteria
sub-interface not deleted.
#### Test Fail Criteria
Sub-interface deleted.

## Test case 1.15: Multiple deletion of sub-interface
### Description
Delete a sub interface multiple time.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface not deleted.
#### Test Fail Criteria
Sub-interface deleted multiple times.

## Test case 1.16: Sub interface name with special symobols
### Description
Verify a sub-interface assigning a number with some special symbols.
### Test Result Criteria
#### Test Pass Criteria
Sub-interface not created.
#### Test Fail Criteria
Sub-interface created.
