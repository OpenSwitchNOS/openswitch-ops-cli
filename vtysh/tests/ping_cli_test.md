Ping Test Cases
====

## Contents

- [Validation of ping IPv4 parameters](#validation-of-ping-ipv4-parameters)
- [Validation of ping with hostname along with optional parameters](#validation-of-ping-with-hostname-along-with-optional-parameters)
- [Validation of ping IPv4 address along with multiple parameters](#validation-of-ping-ipv4-address-along-with-multiple-parameters)
- [Validation of ping with hostname along with multiple parameters](#validation-of-ping-with-hostname-along-with-multiple-parameters)
- [Validation of ping6 IPv6 parameters](#validation-of-ping6-ipv6-parameters)
- [Validation of ping6 with hostname along with optional parameters](#validation-of-ping6-with-hostname-along-with-optional-parameters)
- [Validation of ping6 IPv6 address along with multiple parameters](#validation-of-ping6-ipv6-address-along-with-multiple-parameters)
- [Validation of ping6 with hostname along with multiple parameters](#validation-of-ping6-with-hostname-along-with-multiple-parameters)

## Validation of ping IPv4 parameters
### **Objective**
Verify that a valid IPv4 address and valid values for optional parameters are passed by the user.
Optional parameters include datagram-size, data-fill pattern, repetitions, interval, TOS and
ip-options (record-route, include-timestamp and include-timestamp-and address).
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test result criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.

## Validation of ping with hostname along with optional parameters
### **Objective**
Verify that a valid hostname and valid values for optional parameters are passed by the user.
Optional parameters include datagram-size, data-fill pattern, repetitions, interval, TOS and
ip-options (record-route, include-timestamp and include-timestamp-and address).
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test result criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.

## Validation of ping IPv4 address along with multiple parameters
### **Objective**
Verify that a valid IPv4 address and valid values for multiple parameters are passed by the user.
Multiple parameters are data-fill, datagram-size, interval, timeout, repetitions and ip-option include-timestamp-and-address.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test result criteria
#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test Fail Criteria
User is able to pass Invalid values.

## Validation of ping with hostname along with multiple parameters
### **Objective**
Verify that a valid hostname and valid values for multiple parameters are passed by the user.
Multiple parameters are data-fill, datagram-size, interval, timeout, repetitions and ip-option include-timestamp.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test Result Criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed
#### Test fail criteria#
User is able to pass Invalid values.

## Validation of ping6 IPv6 parameters
### **Objective**
Verify that a valid IPv6 address and valid values for optional parameters are passed by the user.
Optional parameters include datagram-size, data-fill pattern, repetitions and interval.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test result Criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.

## Validation of ping6 with hostname along with optional parameters
### **Objective**
Verify that a valid hostname and valid values for optional parameters are passed by the user.
Optional parameters include datagram-size, data-fill pattern, repetitions and interval.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test Result Criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.

## Validation of ping6 IPv6 address along with multiple parameters
### **Objective**
Verify that a valid IPv6 address and valid values for multiple parameters are passed by the user.
Multiple parameters include datagram-size, data-fill pattern, repetitions and interval.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test result criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.

## Validation of ping6 with hostname along with multiple parameters
### **Objective**
Verify that a valid IPv6 address and valid values for multiple parameters are passed by the user.
Multiple parameters include datagram-size, data-fill pattern, repetitions and interval.
### Requirements
The requirements for this test case are

 - OpenSwitch OS

### Setup

#### Topology diagram
```ditaa

    +-------+
    |       |
    |  DUT  |
    |       |
    +-------+

```
#### Test setup
- 1 DUT in standalone

### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
### Test Result Criteria

#### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
#### Test fail criteria
User is able to pass Invalid values.
