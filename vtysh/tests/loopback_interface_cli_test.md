# Loopback Interface Test Cases

## Contents
- [Verify the loopback interface configurations](#verify-the-loopback-interface-configurations)
	- [Objective](#objective)
	- [Requirements](#requirements)
- [Setup](#setup)
	- [Topology diagram](#topology-diagram)
	- [Test setup](#test-setup)
- [Test case 1.01: Verify the loopback interface entry in the port table](#test-case-101-verify-the-loopback-interface-entry-in-the-port-table)
- [Test case 1.02: Verify the loopback interface entry in the interface table](#test-case-102-verify-the-loopback-interface-entry-in-the-interface-table)
- [Test case 1.03: Assign an IPv4 address to the loopback interface](#test-case-103-assign-an-ipv4-address-to-the-loopback-interface)
- [Test case 1.04: Check the invalid IPv4 address configuration](#test-case-104-check-the-invalid-ipv4-address-configuration)
- [Test case 1.05: Check an invalid IPv6 configuration](#test-case-105-check-an-invalid-ipv6-configuration)
- [Test case 1.06: Assign an IPv6 address to the loopback interface](#test-case-106-assign-an-ipv6-address-to-the-loopback-interface)
- [Test case 1.07: Check the maximum range of the loopback interfaces](#test-case-107-check-the-maximum-range-of-the-loopback-interfaces)
- [Test case 1.08: Check the minimum range of the loopback interfaces](#test-case-108-check-the-minimum-range-of-the-loopback-interfaces)
- [Test case 1.09: Delete the non-existing loopback interface](#test-case-109-delete-the-non-existing-loopback-interface)
- [Test case 1.10: Assign a duplicate IPv4 address to the other loopback interface](#test-case-110-assign-a-duplicate-ipv4-address-to-the-other-loopback-interface)
- [Test case 1.11: Multiple deletions of a loopback interface](#test-case-111-multiple-deletions-of-a-loopback-interface)


##  Verify the loopback interface configurations
### Objective
To create loopback interface entry in port table and interface table
###  Requirements
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
AS5712 switch instance.

## Test case 1.01: Verify the loopback interface entry in the port table
### Description
Verify whether loopback interface entry is created in port table.
### Test result criteria
#### Test pass criteria
Loopback Interface entry is created in port table.
#### Test fail criteria
Loopback Interface entry is not created in port table

## Test case 1.02: Verify the loopback interface entry in the interface table
### Description
Verify whether a loopback interface entry is created in the interface table.
### Test result criteria
#### Test pass criteria
The loopback interface entry is created in the interface table.
#### Test fail criteria
The loopback interface entry is not created in the interface table.

## Test case 1.03: Assign an IPv4 address to the loopback interface
### Description
Verify whether IPv4 address is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
The IPv4 address is assigned to the loopback interface.
#### Test fail criteria
The IPv4 address is not assigned to the loopback interface.

## Test case 1.04: Check the invalid IPv4 address configuration
### Description
Verify whether the invalid IPv4 address is assigned to loopback interface.
### Test result criteria
#### Test pass criteria
The IP is not assigned to the loopback interface.
#### Test fail criteria
The IP is assigned to loopback interface.

## Test case 1.05: Check an invalid IPv6 configuration
### Description
Verify whether an invalid IPv6 address is assigned to a loopback interface.
### Test result criteria
#### Test pass criteria
The IP address is not assigned to the loopback interface.
#### Test fail criteria
The IP address is assigned to the loopback interface.

## Test case 1.06: Assign an IPv6 address to the loopback interface
### Description
Verify whether the IPv6 address is assigned to the loopback interface.
### Test result criteria
#### Test pass criteria
An IPv6 address is assigned to a the loopback interface.
#### Test fail criteria
An IPv6 address is not assigned to a the loopback interface.

## Test case 1.07: Check the maximum range of the loopback interfaces
### Description
Verify whether a loopback interface is created if an interface number is above the upper limit.
### Test result criteria
#### Test pass criteria
The loopback interface is not created.
#### Test fail criteria
The loopback interface is created.

## Test case 1.08: Check the minimum range of the loopback interfaces
### Description
Verify whether the loopback interface is created if an interface number is below the lower limit.
### Test result criteria
#### Test pass criteria
The loopback interface is not created.
#### Test fail criteria
The loopback interface is created.

## Test case 1.09: Delete the non-existing loopback interface
### Description
Verify whether a non-exiting loopback interface is deleted.
### Test result criteria
#### Test pass criteria
The loopback interface is not deleted.
#### Test fail criteria
The loopback interface is deleted.

## Test case 1.10: Assign a duplicate IPv4 address to the other loopback interface
### Description
Verify that a duplicate IPv4 address is assigned.
### Test result criteria
#### Test pass criteria
An IP address is not assigned.
#### Test fail criteria
An IP address is assigned.

## Test case 1.11: Multiple deletions of a loopback interface
### Description
Delete a loopback interface multiple times.
### Test result criteria
#### Test pass criteria
An error message is displayed.
#### Test fail criteria
The loopback interface is deleted.
