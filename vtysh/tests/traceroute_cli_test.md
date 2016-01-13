# Traceroute Test Cases
====

## Contents

- [Validation of traceroute IPv4 parameters](#validation-of-traceroute-ipv4-parameters)
- [Validation of traceroute with hostname along with optional parameters](#validation-of-traceroute-with-hostname-along-with-optional-parameters)
- [Validation of traceroute IPv4 address along with multiple parameters](#validation-of-traceroute-ipv4-address-along-with-multiple-parameters)
- [Validation of traceroute6 IPv6 parameters](#validation-of-traceroute6-ipv6-parameters)
- [Validation of traceroute6 with hostname along with optional parameters](#validation-of-traceroute6-with-hostname-along-with-optional-parameters)
- [Validation of traceroute6 IPv6 address along with multiple parameters](#validation-of-traceroute6-ipv6-address-along-with-multiple-parameters)

## Validation of traceroute IPv4 parameters
### Objective
Verify that a valid IPv4 address and valid values for optional parameters are passed by the user.
Optional parameters include dstport, maxttl, minttl, probes, timeout, and
ip-options loose source route.
### Requirements
The requirements for this test case are:

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

## Validation of traceroute with hostname along with optional parameters
### Objective
Verify that a valid hostname and valid values for optional parameters are passed by the user.
Optional parameters include dstport, maxttl, minttl, probes, timeout, and
ip-options loose source route.
### Requirements
The requirements for this test case are:

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

## Validation of traceroute IPv4 along with multiple parameters
### Objective
Verify that a valid IPv4 address and valid values for multiple optional parameters are passed by the user.
### Requirements
The requirements for this test case are:

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

## Validation of traceroute IPv6 parameters
### Objective
Verify that a valid IPv6 address and valid values for optional parameters are passed by the user.
Optional parameters include dstport, maxttl, probes, and timeout.
### Requirements
The requirements for this test case are:

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
User is able to pass invalid values.

## Validation of traceroute6 with hostname along with optional parameters
### Objective
Verify that a valid hostname and valid values for optional parameters are passed by the user.
Optional parameters include dstport, maxttl, probes, and timeout.
### Requirements
The requirements for this test case are:

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

## Validation of traceroute IPv6 along with multiple parameters
### Objective
Verify that a valid IPv6 address and valid values for multiple parameters are passed by the user.
### Requirements
The requirements for this test case are:

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
