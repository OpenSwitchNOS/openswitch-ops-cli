# Loopback Interface Test Cases

## Contents

- [Verify loopback interface configurations](#verify-loopback-interface-configurations)
	- [Objective](#objective)
	- [Requirements](#requirements)
	- [Setup](#setup)
		- [Topology diagram](#topology-diagram)
		- [Test setup](#test-setup)
- [Test case 1.01 : Verify loopback interface entry in port table](#test-case-101-verify-loopback-interface-entry-in-port-table)
- [Test case 1.02 : Verify loopback interface entry in Interface Table](#test-case-102-verify-loopback-interface-entry-in-interface-table)
- [Test case 1.03 : Assign IPv4 to loopback interface](#test-case-103-assign-ipv4-to-loopback-interface)
- [Test case 1.04 : Check Invalid IPv4 configuration](#test-case-104-check-invalid-ipv4-configuration)
- [Test case 1.05 : Check Invalid IPv6 configuration](#test-case-105-check-invalid-ipv6-configuration)
- [Test case 1.06 : Assign IPv6 to loopback interface](#test-case-106-assign-ipv6-to-loopback-interface)
- [Test case 1.07 : Check maximum range of loopback interfaces](#test-case-107-check-maximum-range-of-loopback-interfaces)
- [Test case 1.08 : Check minimum range of loopback interfaces](#test-case-108-check-minimum-range-of-loopback-interfaces)
- [Test case 1.09 : Delete non existing loopback-interface](#test-case-109-delete-non-existing-loopback-interface)
- [Test case 1.10 : Assign duplicate IPv4 address to other loopback-interface](#test-case-110-assign-duplicate-ipv4-address-to-other-loopback-interface)

## Verify loopback interface configurations
### Objective
To create a loopback interface entry in the port and interface tables.
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

## Test case 1.01 : Verify loopback interface entry in port table
### Description
Verify whether or not the loopback interface entry is created in the port table.
### Test result criteria
#### Test pass criteria
This test passes if the loopback interface entry is created in the port table.
#### Test fail criteria
This test fails if the loopback interface entry is not created in the port table.

## Test case 1.02 : Verify loopback interface entry in Interface Table
### Description
Verify whether or not the loopback interface entry is created in the interface table.
### Test result criteria
#### Test pass criteria
This test passes if the loopback interface entry is created in the interface table.
#### Test fail criteria
This test fails if the loopback Interface entry is not created in the interface table.

## Test case 1.03 : Assign IPv4 to loopback interface
### Description
Verify whether or not the IPv4 is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
This test passes if IPv4 is assigned to the loopback interface.
#### Test fail criteria
This test fails if IPv4 is not assigned to the loopback interface.

## Test case 1.04 : Check Invalid IPv4 configuration
### Description
Verify whether or not an invalid IPv4 is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
This test passes if the IP is not assigned to the loopback interface.
#### Test fail criteria
This test fails if the IP is assigned to the loopback interface.

## Test case 1.05 : Check Invalid IPv6 configuration
### Description
Verify whether or not an invalid IPv6 is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
This test passes if the IP is not assigned to the loopback interface.
#### Test fail criteria
This test fails if the IP is assigned to the loopback interface.

## Test case 1.06 : Assign IPv6 to loopback interface
### Description
Verify whether or not an invalid IPv6 is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
This test passes if the IPv6 is assigned to the loopback interface.
#### Test fail criteria
This test fails if the IPv6 is not assigned to the loopback-interface.

## Test case 1.07 : Check maximum range of loopback interfaces
### Description

Verify whether or not the loopback interface is created if the interface number is above the upper limit.
### Test result criteria
#### Test pass criteria
This test passes if the loopback interface is not created.
#### Test fail criteria
This test fails if the loopback interface is created.

## Test case 1.08 : Check minimum range of loopback interfaces
### Description
Verify whether or not the loopback interface is created if the interface number is below the lower limit.
### Test result criteria
#### Test pass criteria
This test passes if the loopback interface is not created.
#### Test fail criteria
This test fails if the loopback interface is created.

## Test case 1.09 : Delete non-existing loopback-interface
### Description
Verify whether or not a non-existing loopback interface is deleted.
### Test result criteria
#### Test pass criteria
his test passes if the loopback interface is not deleted.
#### Test fail criteria
This test fails if the loopback interface is deleted.

## Test case 1.10 : Assign duplicate IPv4 address to other loopback-interface
### Description
Verify whether or not the duplicate IPv4 address is assigned.
### Test result criteria
#### Test Pass Criteria
This test passes if the IP address is not assigned.
#### Test fail criteria
This test fails if the IP address is assigned.
