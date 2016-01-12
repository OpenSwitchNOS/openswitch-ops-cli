# Source-interface Selection  CLI Component Tests

## Contents

- [Add a source-interface configuration](#add-a-source-interface-configuration)
    - [Validation of a source-interface IP address to the TFTP protocol](#validation-of-a-source-interface-ip-address-to-the-tftp-protocol)
    - [Validation of a source-interface IP address for all the specified protocols](#validation-of-a-source-interface-ip-address-for-all-the-specified-protocols)
	- [Add a source-interface IP address to the TFTP protocol](#add-a-source-interface-ip-address-to-the-tftp-protocol)
    - [Add a source-interface IP address for all the specified protocols](#add-a-source-interface-ip-address-for-all-the-specified-protocols)
    - [Add a source-interface to the TFTP protocol](#add-a-source-interface-to-the-tftp-protocol)
    - [Add a source-interface for all the specified protocols](#add-a-source-interface-for-all-the-specified-protocols)
- [Delete a Source Interface configuration](#delete-a-source-interface-configuration)
    - [Delete a source-interface to the TFTP protocol](#delete-a-source-interface-to-the-tftp-protocol)
    - [Delete a source-interface for all the specified protocols](#delete-a-source-interface-for-all-the-specified-protocols)

## Add a source-interface configuration

### Validation of a source-interface IP address to the TFTP protocol
#### Objective
This test case validates the source-interface IP address to the TFTP protocol configuration.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case validates the source-interface IP address to the TFTP protocol configuration.

Run command:
```
root(config-ter)# ip source-interface tftp address 255.255.255.255
```
#### Test result criteria
##### Test pass criteria
Configuration fails with the error message as "Broadcast, multicast and loopback addresses are not allowed".
##### Test fail criteria
TFTP source-interface address configuration is successful.

### Validation of a source-interface IP address for all the specified protocols
#### Objective
This test case validates the source-interface IP address for all the specified protocols configuration.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
validates the source-interface IP address for all the specified protocols configuration.
Run command:
```
root(config-ter)# ip source-interface all address 255.255.255.255
```
#### Test result criteria
##### Test pass criteria
Configuration fails with the error message as "Broadcast, multicast and loopback addresses are not allowed".
##### Test fail criteria
Source-interface address confiuration is successful.

### Add a source-interface IP address to the TFTP protocol
#### Objective
This test case verifies whether the source-interface IP address to the TFTP protocol configuration can be added.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case is used to add a source-interface IP address to the TFTP protocol configuration.

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
The TFTP source-interface address configuration is not present when checked using the `show ip source-interface tftp` command.

### Add a source-interface IP address for all the specified protocols
#### Objective
This test case verifies whether the source-interface IP address for all the specified protocols configuration can be added.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
this test case is used to add a source-interface IP address for all the specified protocols configuration.

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
Source-interface address configuration is not present when checked using the `show ip source-interface`command.

### Add a source-interface to the TFTP protocol
#### Objective
This test case verifies whether the source-interface to the TFTP protocol configuration can be added.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case is used to add a source-interface to the TFTP protocol configuration.

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
The TFTP source-interface configuration is not present when checked using the `show ip source-interface tftp` command.

### Add a source-interface for all the specified protocols
#### Objective
This test case verifies whether the source-interface for all the specified protocols configuration can be added.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case is used to add a source-interface for all the specified protocols configuration.

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
Source-interface configuration is not present when checked using the `show ip source-interface`command.

## Delete a Source Interface configuration
### Delete a source-interface to the TFTP protocol
#### Objective
This test case verifies whether the source-interface to the TFTP protocol configuration can be deleted.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case is used to delete a source-interface IP address to the TFTP protocol configuration.

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
CLI ouput displays a valid TFTP source-interface configuration.

### Delete a source-interface for all the specified protocols
#### Objective
This test case verifies whether the source-interface for all the specified protocols configuration can be deleted.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_source_interface_selection.py

#### Setup
Single switch topology
#### Description
DUT must be running OpenSwitch OS to execute this test, should be configured as default, and is in the login or bash-shell context.
This test case is used to delete a source-interface IP address for all the specified protocols configuration.
Run command:
```
root(config-ter)# no ip source-interface all
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
CLI ouput displays a valid source-interface configuration.