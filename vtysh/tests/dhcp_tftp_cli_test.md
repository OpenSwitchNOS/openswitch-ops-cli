# DHCP-TFTP CLI Component Tests

## Contents
- [Add DHCP dynamic configuration](#add-dhcp-dynamic-configuration)
       - [Add DHCP dynamic IPv4 configuration](#add-dhcp-dynamic-ipv4-configuration)
       - [Add DHCP dynamic IPv6 configuration](#add-dhcp-dynamic-ipv6-configuration)
       - [DHCP dynamic configuration validation](#dhcp-dynamic-configuration-validation)
	       - [Start IP address validation](#start-ip-address-validation)
	       - [End IP address validation](#end-ip-address-validation)
	       - [Netmask address validation](#netmask-address-validation)
	       - [IPv6 Netmask address validation](#ipv6-netmask-address-validation)
	       - [IP address range validation](#ip-address-range-validation)
	       - [Broadcast address validation](#broadcast-address-validation)
	       - [Match tags validation](#match-tags-validation)
	       - [Set tag validation](#set-tag-validation)
	       - [IP address lease duration validation](#ip-address-lease-duration-validation)
- [Add DHCP static configuration](#add-dhcp-static-configuration)
	   - [Add DHCP static IPv4 configuration](#add-dhcp-static-ipv4-configuration)
	   - [Add DHCP static IPv6 configuration](#add-dhcp-static-ipv6-configuration)
	   - [DHCP Ssatic configuration validation](#dhcp-static-configuration-validation)
		   - [Static IP address validation](#static-ip-address-validation)
		   - [MAC address validation](#mac-address-validation)
		   - [Set tags validation](#set-tags-validation)
		   - [Client ID validation](#client-id-validation)
		   - [Client hostname validation](#client-hostname-validation)
- [Add DHCP option configuration](#add-dhcp-option-configuration)
       - [Add DHCP option configuration using option name](#add-dhcp-option-configuration-using-option-name)
       - [Add DHCP option configuration using option number](#add-dhcp-option-configuration-using-option-number)
       - [DHCP option configuration validation](#dhcp-option-configuration-validation)
	       - [Option name validation](#option-name-validation)
	       - [Option number validation](#option-number-validation)
	       - [Option match tags validation](#option-match-tags-validation)
- [Add DHCP match configuration](#add-dhcp-match-configuration)
	   -  [Add DHCP match configuration using option number](#add-dhcp-match-configuration-using-option-number)
	   -  [Add DHCP match configuration using option name](#add-dhcp-match-configuration-using-option-name)
	   -  [DHCP match configuration validation](#dhcp-match-configuration-validation)
		   - [Match set tag validation](#match-set-tag-validation)
		   - [Match option name validation](#match-option-name-validation)
		   - [Match option number validation](#match-option-number-validation)
- [Add DHCP BOOTP ](#add-dhcp-bootp)
       - [Add DHCP BOOTP configuration](#add-dhcp-bootp-configuration)
       - [DHCP BOOTP validation](#dhcp-bootp-validation)
- [Verify DHCP server configuration](#verify-dhcp-server-configuration)
- [Add TFTP server configuration](#add-tftp-server-configuration)
	   - [Enable TFTP server](#enable-tftp-server)
	   - [Enable TFTP server secure mode](#enable-tftp-server-secure-mode)
	   - [Add TFTP server root path](#add-tftp-server-root-path)
- [Verify TFTP server configuration](#verify-tftp-server-configuration)
- [Delete DHCP dynamic configuration](#delete-dhcp-dynamic-configuration)
	   - [Delete DHCP dynamic IPv4 configuration](#delete-dhcp-dynamic-ipv4-configuration)
	   - [Delete DHCP dynamic IPv6 configuration](#delete-dhcp-dynamic-ipv6-configuration)
- [Delete DHCP static configuration](#delete-dhcp-static-configuration)
       - [Delete DHCP static IPv4 configuration](#delete-dhcp-static-ipv4-configuration)
       - [Delete DHCP static IPv6 configuration](#delete-dhcp-static-ipv6-configuration)
- [Delete DHCP option configuration](#delete-dhcp-option-configuration)
	   - [Delete DHCP option configuration using option name](#delete-dhcp-option-configuration-using-option-name)
	   - [Delete DHCP option configuration using option number](#delete-dhcp-option-configuration-using-option-number)
- [Delete DHCP match configuration](#delete-dhcp-match-configuration)
       - [Delete DHCP match configuration using option number](#delete-dhcp-match-configuration-using-option-number)
       - [Delete DHCP match configuration using option name](#delete-dhcp-match-configuration-using-option-name)
- [Delete DHCP BOOTP configuration](#delete-dhcp-bootp-configuration)
- [Disable TFTP server secure mode](#disable-tftp-server-secure-mode)
- [Disable TFTP server](#disable-tftp-server)


## Add DHCP dynamic configuration
### Add DHCP dynamic IPv4 configuration
#### Objective
This test case verifies whether the DHCP dynamic IPv4 configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP dynamic configuration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 10.0.0.1 end-ip-address 10.255.255.254 netmask 255.0.0.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.255.255.255 lease-duration 60
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address    End IP Address    Netmask     Broadcast
----------------------------------------------------------------------------
test-range  10.0.0.1            10.255.255.254    255.0.0.0   10.255.255.255

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### Add DHCP dynamic IPv6 configuration
#### Objective
This test case verifies whether the DHCP dynamic IPv6 configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP dynamic configuration.

Run command:
```
root(config-dhcp-server)# range range-ipv6 start-ip-address 2001:cdba::3257:9652 end-ip-address 2001:cdba::3257:9655 prefix-len 64 match tags v6tag1,v6tag2,v6tag3 set tag v6-stag lease-duration 60
```
#### Test result criteria
##### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### DHCP dynamic configuration validation
#### Start IP address validation
##### Objective
This test case confirms the invalid start ip address can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid start ip address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 300.300.300.300 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
% Unknown command.
```
###### Test fail criteria

#### End IP address validation
##### Objective
This test case verifies whether an invalid end ip address can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid end ip address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 192.168.0.1 end-ip-address 300.300.300.300 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
% Unknown command.
```
###### Test Fail Criteria

#### Netmask address validation
##### Objective
Test case verifies whether invalid netmask can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid netmask.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 127.0.0.1 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
127.0.0.1 is invalid.
```
###### Test fail criteria

#### IPv6 Netmask address validation
##### Objective
Test case verifies whether netmask can be added for IPv6 configuration.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add netmask for IPv6 configuration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 2001:cdba::3257:9642 end-ip-address 2001:cdba::3257:9648  netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
Error : netmask configuration not allowed for IPv6.
```
###### Test Fail Criteria

#### IP address range validation
##### Objective
Test case verifies whether invalid ip address range can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid IP address range.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 10.0.0.1 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
Invalid IP address range.
```
###### Test fail criteria

#### Broadcast address validation
##### Objective
This test case verifies whether invalid broadcast address range netmask can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid broadcast address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.0.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
10.0.0.255 is invalid.
```
###### Test fail criteria

#### Match tags validation
##### Objective
Test case checks whether invalid match tags can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology.
##### Description
Add invalid match tags.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,this-tag-length-greater-than-15,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
this-tag-length-greater-than-15 is invalid.
```
###### Test fail criteria

#### Set tag validation
##### Objective
Test case checks whether invalid set tag can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single cwitch topology
##### Description
Add invalid set tag.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-this-tag-greater-than-15 broadcast 192.168.0.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
test-this-tag-greater-than-15 is invalid.
```
###### Test fail criteria

#### IP address lease duration validation
##### Objective
This test case verifies whether invalid lease duration can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid lease duration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 120000
```
##### Test result criteria
###### Test pass criteria
Command output:
```
% Unknown command..
```
###### Test fail criteria

## Add DHCP static configuration
### Add DHCP static IPv4 configuration
#### Objective
Test case verifies whether DHCP Static IPv4 configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# static 192.168.0.2 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address    Hostname    Lease time   MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2   testname     60          aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### Add DHCP static IPv6 configuration
#### Objective
This test case verifies whether DHCP Static IPv4 configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# static 2001:cdba::3257:9680 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### DHCP static configuration validation
#### Static IP address validation
##### Objective
This test case verifies whether invalid start ip address can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid static ip address.

Run command:
```
root(config-dhcp-server)# static 300.300.300.300 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
% Unknown command.
```
P match is not configured.

#### MAC address validation
##### Objective
Test case verifies whether an invalid mac address can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid mac address.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aabbccddeeff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
aabbccddeeff is invalid.
```
###### Test Fail Criteria


#### Set tags validation
##### Objective
Test case verifies whether invalid set tags can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid set tags.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags t1-tag-this-tag-length-greater-than-15,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
t1-tag-this-tag-length-greater-than-15 is invalid
```
###### Test fail criteria

#### Client ID validation
##### Objective
This test case verifies whether an invalid client ID can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid client ID.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id this-client-id-length-greater-than-15  match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria

```
this-client-id-length-greater-than-15  is invalid
```
###### Test fail criteria

#### Client hostname validation
##### Objective
Test case verifies whether an invalid hostname can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid client hostname.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id tempid  match-client-hostname this-hostname-greater-than-15 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Command output:
```
this-hostname-greater-than-15 is invalid
```
###### Test fail criteria


## Add DHCP option configuration
### Add DHCP option configuration using the option name
#### Objective
This test case verifies whether the DHCP Option configuration can be added using the option name.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP Option configuration using the option name.

Run command:
```
root(config-dhcp-server)# option set option-name Router option-value 192.168.0.1  match tags mtag1,mtag2,mtag3
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value          ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### Add DHCP option configuration using the option number
#### Objective
This test case verifis whether the DHCP Option configuration using the option number can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP option configuration using the option number.

Run command:
```
root(config-dhcp-server)# option set option-number 3 option-value 192.168.0.3  match tags mtag4,mtag5,mtag6
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria


### DHCP option configuration validation
#### Option name validation
##### Objective
This test case verifies whether an invalid option name can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid option name.

Run command:
```
root(config-dhcp-server)# option set option-name set-option-name-greater-than-15 option-value 10.0.0.1 match tags tag1,tag2,tag3
```
##### Test result criteria
###### Test pass criteria
Command output:
```
set-option-name-greater-than-15 is invalid
```
###### Test fail criteria

#### Option number validation
##### Objective
This test case verifies whether an invalid option number can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid option number.

Run command:
```
root(config-dhcp-server)# option set option-number 300 option-value 10.0.0.1 match tags tag1,tag2,tag3
```
##### Test Result Criteria
###### Test Pass Criteria
Command output:
```
% Unknown command.
```
###### Test fail criteria

#### Option match tags validation
##### Objective
This test case verifies whether an invalid match tag can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid match tag.

Run command:
```
root(config-dhcp-server)# option set option-number 3 option-value 10.0.0.1 match tags tag1,match-option-name-greater-than-15,tag3
```
##### Test result criteria
###### Test pass criteria
Command output:
```
match-option-name-greater-than-15 is in
```
###### Test fail criteria

## Add DHCP match configuration
### Add DHCP match configuration using option number
#### Objective
Test case verufies whether DHCP Match configuration can be added using the option number.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP match configuration using the option number.

Run command:
```
root(config-dhcp-server)# match set tag stag match-option-number 4 match-option-value 192.168.0.4
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test Fail Criteria

### Add DHCP match configuration using the option name
#### Objective
This test case verifies whether DHCP Match configuration can be added using the option name.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP match configuration using the option name.

Run command:
```
root(config-dhcp-server)# match set tag temp-mtag match-option-name temp-mname match-option-value 192.168.0.5
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria

### DHCP match configuration validation
#### Match set tag validation
##### Objective
This test case verifies whether an invalid set tag can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid set tag.

Run command:
```
root(config-dhcp-server)# match set tag tag-greater-than-15 match-option-name tempname match-option-value 10.0.0.1
```
##### Test result criteria
###### Test pass criteria
Command output:
```
tag-greater-than-15 is invalid
```
###### Test fail criteria

#### Match option name validation
##### Objective
This test case verifies whether an invalid match option name can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid match option name.

Run command:
```
root(config-dhcp-server)# match set tag test-tag match-option-name tag-name-greater-than-15 match-option-value 10.0.0.1
```
##### Test result criteria
###### Test pass criteria
Command output:
```
tag-name-greater-than-15 is invalid
```
###### Test fail criteria

#### Match option number validation
##### Objective
Test case verifies whether an invalid match option number can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid match option number.

Run command:
```
root(config-dhcp-server)# match set tag test-tag match-option-number 300  tag-name-greater-than-15 match-option-value 10.0.0.1
```
##### Test result criteria
###### Test pass criteria
Command output:
```
% Unknown command.
```
###### Test fail criteria

## Add DHCP BOOTP
### Add DHCP BOOTP configuration
#### Objective
Test case verifies whether DHCP BOOTP configuration can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add DHCP BOOTP configuration.

Run command:
```
root(config-dhcp-server)# boot set file /tmp/testfile match tag boottag
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
###### Test fail criteria


#### DHCP BOOTP validation
##### Objective
This test case verifies whether an invalid match tag can be added.
##### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single switch topology
##### Description
Add invalid match tag.

Run command:
```
root(config-dhcp-server)# boot set file /tmp/tmpfile match tag boot-tag-name-greater-than-15
```
##### Test result criteria
###### Test pass criteria
Command output:
```
boot-tag-name-greater-than-15 is invalid
```
###### Test fail criteria

## Verify DHCP server configuration
#### Objective
Test case verifies the dhcp-server command.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Verify the dhcp-server command.
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
##### Test result criteria
###### Test pass criteria
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
test-range  10.0.0.1             10.255.255.254       255.0.0.0 10.255.255.255
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP Match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
###### Test fail criteria

## Add TFTP server configuration
### Enable TFTP server
#### Objective
This test case verifies whether the tftp-server can be enabled.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Enable TFTP server.
Run command:
```
root(config-tftp-server)# enable
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Disabled
TFTP server file path : Not configured
```
###### Test fail criteria

### Enable TFTP server secure mode
#### Objective
Test case verifies whether the tftp-server secure mode can be enabled.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Enable the TFTP server secure mode.
Run command:
```
root(config-tftp-server)# secure-mode
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : Not configured
```
###### Test fail criteria

### Add TFTP server root path
#### Objective
This test case verufues whether the TFTP server root path can be added.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Add the TFTP server root path.
Run command:
```
root(config-tftp-server)# path /etc/testfile
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : /etc/testfile
```
###### Test fail criteria


## Verify TFTP server configuration
#### Objective
This test case verifies the tftp-server command.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Verify the tftp-server command.
Run command:
```
root(config-dhcp-server)# do show tftp-server
```
##### Test result criteria
###### Test pass criteria
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : /etc/testfile
```
###### Test fail criteria

## Delete DHCP dynamic configuration
### Delete DHCP dynamic IPv4 configuration
#### Objective
Test case verifies whether the DHCP Dynamic IPv4 configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP dynamic configuration.

Run command:
```
root(config-dhcp-server)# no range test-range start-ip-address 10.0.0.1 end-ip-address 10.255.255.254 netmask 255.0.0.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.255.255.255 lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
Name        Start IP Address     End IP Address       Netmask   Broadcast
-----------------------------------------------------------------------------
range-ipv6  2001:cdba::3257:9652 2001:cdba::3257:9655    *           *

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

### Delete DHCP dynamic IPv6 configuration
#### Objective
This test case verifies whether the DHCP Dynamic IPv6 configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP dynamic configuration.

Run command:
```
root(config-dhcp-server)# no range range-ipv6 start-ip-address 2001:cdba::3257:9652 end-ip-address 2001:cdba::3257:9655 prefix-len 64 match tags v6tag1,v6tag2,v6tag3 set tag v6-stag lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
IP Address             Hostname  Lease time MAC-Address        Set tags
----------------------------------------------------------------------------
192.168.0.2            testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3
2001:cdba::3257:9680   testname   60        aa:bb:cc:dd:ee:ff  tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

## Delete DHCP static configuration
### Delete DHCP static IPv4 configuration
#### Objective
This test case verifies whether the DHCP Static IPv4 configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# no static 192.168.0.2 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
IP Address            Hostname  Lease time MAC-Address       Set tags
--------------------------------------------------------------------------
2001:cdba::3257:9680   testname 60         aa:bb:cc:dd:ee:ff tag1,tag2,tag3

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

### Delete DHCP static IPv6 configuration
#### Objective
This test case verifies whether the DHCP Static IPv4 configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# no static 2001:cdba::3257:9680 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  *             Router         192.168.0.1          False  mtag1,mtag2,mtag3
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6


DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

## Delete the DHCP option configuration
### Delete the DHCP option configuration using the option name
#### Objective
Test case verifies whether the DHCP option configuration can be deleted using the option name.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP option configuration using the option name.

Run command:
```
root(config-dhcp-server)# no option set option-name Router option-value 192.168.0.1  match tags mtag1,mtag2,mtag3
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
Option Number  Option Name       Option Value         ipv6   Match tags
----------------------------------------------------------------------------
  3               *              10.0.0.1             False  mtag4,mtag5,mtag6

DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

### Delete the DHCP option configuration using the option number
#### Objective
This test case verifies whether the DHCP Option configuration can be deleted using the option number.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP option configuration using the option number.

Run command:
```
root(config-dhcp-server)# no option set option-number 3 option-value 192.168.0.3  match tags mtag4,mtag5,mtag6
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
4              *                 192.168.0.4            stag
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

## Delete the DHCP match configuration
### Delete the DHCP match configuration using the option number
#### Objective
This test case verifies whether the DHCP match configuration can be added using the option number.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP match configuration using the option number.

Run command:
```
root(config-dhcp-server)# no match set tag stag match-option-number 4 match-option-value 192.168.0.4
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP match configuration
------------------------
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

### Delete the DHCP match configuration using the option name
#### Objective
This test case verifies whether the DHCP match configuration can be deleted using the option name.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP match configuration using the option name.

Run command:
```
root(config-dhcp-server)# no match set tag temp-mtag match-option-name temp-mname match-option-value 192.168.0.5
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test fail criteria

## Delete the DHCP BOOTP configuration
#### Objective
This test case verifies whether the DHCP BOOTP configuration can be deleted.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Delete the DHCP BOOTP configuration.

Run command:
```
root(config-dhcp-server)# no boot set file /tmp/testfile match tag boottag
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command output:
```
DHCP dynamic IP allocation configuration
----------------------------------------
DHCP IP address range is not configured.

DHCP static IP allocation configuration
---------------------------------------
DHCP static host is not configured.

DHCP options configuration
--------------------------
DHCP options are not configured.

DHCP Match configuration
------------------------
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test fail criteria


## Disable TFTP server secure mode
#### Objective
This test case verifies whether the tftp-server secure mode can be enabled.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Enable the TFTP server secure mode.
Run command:
```
root(config-tftp-server)# no secure-mode
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Disabled
TFTP server file path : /etc/testfile
```
###### Test fail criteria

## Disable the TFTP server
#### Objective
This test case verifies whether the tftp-server secure mode can be disabled.
#### Requirements
- Virtual mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single switch topology
#### Description
Disable the TFTP server.
Run command:
```
root(config-tftp-server)# no secure-mode
```
##### Test result criteria
###### Test pass criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command output:
```
TFTP server configuration
-------------------------
TFTP server : Disabled
```
###### Test fail criteria
