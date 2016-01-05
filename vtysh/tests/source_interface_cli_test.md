# Source-interface Selection  CLI Component Tests

## Contents

- [Add source-interface configuration](#add-source-interface-configuration)
    - [Validation of a source-interface IP address to TFTP protocol](#validation-of-a-source-interface-ip-address-to-tftp-protocol)
    - [Validation of a source-interface IP address to all the specified protocols](#validation-of-a-source-interface-ip-address-to-all-the-specified-protocols)
	- [Add a source-interface IP address to TFTP protocol](#add-a-source-interface-ip-address-to-tftp-protocol)
    - [Add a source-interface IP address to all the specified protocols](#add-a-source-interface-ip-address-to-all-the-specified-protocols)
    - [Add a source-interface to TFTP protocol](#add-a-source-interface-to-tftp-protocol)
    - [Add a source-interface to all the specified protocols](#add-a-source-interface-to-all-the-specified-protocols)
- [Delete a Source Interface configuration](#delete-a-source-interface-configuration)
    - [Delete a source-interface to TFTP protocol](#delete-a-source-interface-to-tftp-protocol)
    - [Delete a source-interface to all the specified protocols](#delete-a-source-interface-to-all-the-specified-protocols)

## Add source-interface configuration
### Validation of a source-interface IP address to TFTP protocol
#### Objective
This test case validates the source-interface IP address to TFTP protocol configuration.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
validates the source-interface IP address to TFTP protocol configuration.

Run command:
```
root(config-ter)# ip source-interface tftp address 10.10.10.1
```
#### Test result criteria
##### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
##### Test fail criteria
User is able to pass Invalid values like unconfigured IP address or broadcast address or multicast address.

### Validation of a source-interface IP address to all the specified protocols
#### Objective
This test case validates the source-interface IP address to all the specified protocol configuration.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
validates the source-interface IP address to all the specified protocol configuration.
Run command:
```
root(config-ter)# ip source-interface all address 10.10.10.1
```
#### Test result criteria
##### Test pass criteria
An appropriate error message is displayed when an Invalid value is passed.
##### Test fail criteria
User is able to pass Invalid values like unconfigured IP address or broadcast address or multicast address.
### Add a source-interface IP address to TFTP protocol
#### Objective
This test case verifies whether the source-interface IP address to TFTP protocol configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
 Add source-interface configuration

Run command:
```
root(config-ter)# ip source-interface tftp address 1.1.1.1
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface tftp
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            1.1.1.1
```
##### Test fail criteria

### Add a source-interface IP address to all the specified protocols
#### Objective
This test case verifies whether the source-interface IP address to all the specified protocol configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
 Add a source-interface IP address to all the specified protocols

Run command:
```
root(config-ter)# ip source-interface all address 1.1.1.1
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            1.1.1.1
```
##### Test fail criteria

### Add a source-interface to TFTP protocol
#### Objective
This test case verifies whether the source-interface to TFTP protocol configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
Add a source-interface to TFTP protocol

Run command:
```
root(config-ter)# ip source-interface tftp interface 1
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface tftp
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            1
```
##### Test fail criteria

### Add a source-interface to all the specified protocols
#### Objective
This test case verifies whether the source-interface to all the specified protocol configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
Add a source-interface to all the specified protocols

Run command:
```
root(config-ter)# ip source-interface all interface 1
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            1
```
##### Test fail criteria
##Delete a Source Interface configuration
### Delete a source-interface to TFTP protocol
#### Objective
This test case verifies whether the source-interface to TFTP protocol configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
Delete a source-interface to TFTP protocol

Run command:
```
root(config-ter)# no ip source-interface tftp
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface tftp
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            (null)
```
##### Test fail criteria

### Delete a source-interface to all the specified protocols
#### Objective
This test case verifies whether the source-interface to all the specified protocol configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
Delete a source-interface to all the specified protocols

Run command:
```
root(config-ter)# no ip source-interface al
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-ter)# do show ip source-interface
```
Command output:
```
Source-interface Information

Protocol        Source Interface
--------        ----------------
tftp            (null)
```
##### Test fail criteria