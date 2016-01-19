#Loopback Interface Test Cases

The following test cases verify loopback interface configuration is :
<!-- TOC depth:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [Loopback Interface Test Cases](#loopback-interface-test-cases)
	- [Verify loopback interface Configurations](#verify-loopback-interface-configurations)
		- [Objective](#objective)
		- [Requirements](#requirements)
	- [Setup](#setup)
		- [Topology Diagram](#topology-diagram)
		- [Test Setup](#test-setup)
	- [Test case 1.01 : Verify loopback interface entry in port table](#test-case-101-verify-loopback-interface-entry-in-port-table)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.02 : Verify loopback interface entry in Interface Table](#test-case-102-verify-loopback-interface-entry-in-interface-table)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.03 : Assign IPv4 to loopback interface](#test-case-103-assign-ipv4-to-loopback-interface)
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
	- [Test case 1.06 : Assign IPv6 to loopback interface](#test-case-106-assign-ipv6-to-loopback-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.07 : Check maximum range of loopback interfaces](#test-case-107-check-maximum-range-of-loopback-interfaces)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.08 : Check minimum range of loopback interfaces](#test-case-108-check-minimum-range-of-loopback-interfaces)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.09 : Delete non existing loopback-interface](#test-case-109-delete-non-existing-loopback-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.10 : Assign duplicate IPv4 address to other loopback-interface](#test-case-110-assign-duplicate-ipv4-address-to-other-loopback-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)
	- [Test case 1.11 : multi[le delete of a loopback-interface](#test-case-111-multiple-delete-of-loopback-interface)
		- [Description](#description)
		- [Test Result Criteria](#test-result-criteria)
			- [Test Pass Criteria](#test-pass-criteria)
			- [Test Fail Criteria](#test-fail-criteria)

<!-- /TOC -->

##  Verify loopback interface Configurations
### Objective
To create loopback interface entry in port table and interface table
###  Requirements
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

## Test case 1.01 : Verify loopback interface entry in port table
### Description
Verify whether loopback interface entry is created in port table.
### Test Result Criteria
#### Test Pass Criteria
Loopback Interface entry is created in port table.
#### Test Fail Criteria
Loopback Interface entry is not created in port table

## Test case 1.02 : Verify loopback interface entry in Interface Table
### Description
Verify whether loopback interface entry is created in interface table.
### Test Result Criteria
#### Test Pass Criteria
loopback Interface entry is created in interface table.
#### Test Fail Criteria
loopback Interface entry is not created in interface table

## Test case 1.03 : Assign IPv4 to loopback interface
### Description
Verify whether ipv4 is assigned to loopback-interface.
### Test Result Criteria
#### Test Pass Criteria
IPv4 is assigned to loopback-interface.
#### Test Fail Criteria
IPv4 is not assigned to loopback-interface.

## Test case 1.04 : Check Invalid IPv4 configuration
### Description
Verify whether invalid ipv4 is assigned to loopback-interface.
### Test Result Criteria
#### Test Pass Criteria
IP is not assigned to loopback-interface.
#### Test Fail Criteria
IP is assigned to loopback-interface.

## Test case 1.05 : Check Invalid IPv6 configuration
### Description
Verify whether invalid ipv6 is assigned to loopback-interface.
### Test Result Criteria
#### Test Pass Criteria
IP is not assigned to loopback-interface.
#### Test Fail Criteria
IP is assigned to loopback-interface.

## Test case 1.06 : Assign IPv6 to loopback interface
### Description
Verify whether ipv6 is assigned to loopback-interface.
### Test Result Criteria
#### Test Pass Criteria
IPv6 is assigned to loopback-interface.
#### Test Fail Criteria
IPv6 is not assigned to loopback-interface.

## Test case 1.07 : Check maximum range of loopback interfaces
### Description
verify whether loopback interface is created if interface number is above upper limit.
### Test Result Criteria
#### Test Pass Criteria
Loopback interface is not created.
#### Test Fail Criteria
Loopback interface is created.

## Test case 1.08 : Check minimum range of loopback interfaces
### Description
verify whether loopback interface is created if interface number is below lower limit.
### Test Result Criteria
#### Test Pass Criteria
Loopback interface is not created.
#### Test Fail Criteria
Loopback interface is created.

## Test case 1.09 : Delete non existing loopback-interface
### Description
Verify whether non exiting loopback-interface is deleted.
### Test Result Criteria
#### Test Pass Criteria
loopback-interface not deleted.
#### Test Fail Criteria
loopback-interface deleted.

## Test case 1.10 : Assign duplicate IPv4 address to other loopback-interface
### Description
Verify duplicate IPv4 address is assigned.
### Test Result Criteria
#### Test Pass Criteria
IP address not assigned.
#### Test Fail Criteria
IP address assigned.

## Test case 1.11 : multiple delete of a loopback-interface
### Description
Delete loopback interface multiple time.
### Test Result Criteria
#### Test Pass Criteria
Throug error .
#### Test Fail Criteria.
Loopback Interface deleted.
