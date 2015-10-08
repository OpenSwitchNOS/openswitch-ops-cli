# DHCP-TFTP CLI Component Tests

## Contents
- [Add DHCP Dynamic Configuration](#add-dhcp-dynamic-configuration)
       - [Add DHCP Dynamic IPv4 configuration](#add-dhcp-dynamic-ipv4-configuration)
       - [Add DHCP Dynamic IPv6 configuration](#add-dhcp-dynamic-ipv6-configuration)
       - [DHCP Dynamic configuration validation](#dhcp-dynamic-configuration-validation)
	       - [Start IP address validation](#start-ip-address-validation)
	       - [End IP address validation](#end-ip-address-validation)
	       - [Netmask address validation](#netmask-address-validation)
	       - [IPv6 Netmask address validation](#ipv6-netmask-address-validation)
	       - [IP address range validation](#ip-address-range-validation)
	       - [Broadcast address validation](#broadcast-address-validation)
	       - [Match tags validation](#match-tags-validation)
	       - [Set tag validation](#set-tag-validation)
	       - [IP address lease duration validation](#ip-address-lease-duration-validation)
- [Add DHCP Static configuration](#add-dhcp-static-configuration)
	   - [Add DHCP Static IPv4 configuration](#add-dhcp-static-ipv4-configuration)
	   - [Add DHCP Static IPv6 configuration](#add-dhcp-static-ipv6-configuration)
	   - [DHCP Static configuration validation](#dhcp-static-configuration-validation)
		   - [Static IP address validation](#static-ip-address-validation)
		   - [MAC address validation](#mac-address-validation)
		   - [Set tags validation](#set-tags-validation)
		   - [Client ID validation](#client-id-validation)
		   - [Client hostname validation](#client-hostname-validation)
- [Add DHCP Option configuration](#add-dhcp-option-configuration)
       - [Add DHCP Option configuration using option name](#add-dhcp-option-configuration-using-option-name)
       - [Add DHCP Option configuration using option number](#add-dhcp-option-configuration-using-option-number)
       - [DHCP Option configuration validation](#dhcp-option-configuration-validation)
	       - [Option name validation](#option-name-validation)
	       - [Option number validation](#option-number-validation)
	       - [Option match tags validation](#option-match-tags-validation)
- [Add DHCP Match configuration](#add-dhcp-match-configuration)
	   -  [Add DHCP Match configuration using option number](#add-dhcp-match-configuration-using-option-number)
	   -  [Add DHCP Match configuration using option name](#add-dhcp-match-configuration-using-option-name)
	   -  [DHCP Match configuration validation](#dhcp-match-configuration-validation)
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
- [Delete DHCP Dynamic configuration](#delete-dhcp-dynamic-configuration)
	   - [Delete DHCP Dynamic IPv4 configuration](#delete-dhcp-dynamic-ipv4-configuration)
	   - [Delete DHCP Dynamic IPv6 configuration](#delete-dhcp-dynamic-ipv6-configuration)
- [Delete DHCP Static configuration](#delete-dhcp-static-configuration)
       - [Delete DHCP Static IPv4 configuration](#delete-dhcp-static-ipv4-configuration)
       - [Delete DHCP Static IPv6 configuration](#delete-dhcp-static-ipv6-configuration)
- [Delete DHCP Option configuration](#delete-dhcp-option-configuration)
	   - [Delete DHCP Option configuration using option name](#delete-dhcp-option-configuration-using-option-name)
	   - [Delete DHCP Option configuration using option number](#delete-dhcp-option-configuration-using-option-number)
- [Delete DHCP Match configuration](#delete-dhcp-match-configuration)
       - [Delete DHCP Match configuration using option number](#delete-dhcp-match-configuration-using-option-number)
       - [Delete DHCP Match configuration using option name](#delete-dhcp-match-configuration-using-option-name)
- [Delete DHCP BOOTP configuration](#delete-dhcp-bootp-configuration)
- [Disable TFTP server secure mode](#disable-tftp-server-secure-mode)
- [Disable TFTP server](#disable-tftp-server)


## Add DHCP Dynamic Configuration
### Add DHCP Dynamic IPv4 configuration
#### Objective
Test case checks if DHCP Dynamic IPv4 configuration can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Dynamic configuration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 10.0.0.1 end-ip-address 10.255.255.254 netmask 255.0.0.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.255.255.255 lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Add DHCP Dynamic IPv6 configuration
#### Objective
Test case checks if DHCP Dynamic IPv6 configuration can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Dynamic configuration.

Run command:
```
root(config-dhcp-server)# range range-ipv6 start-ip-address 2001:cdba::3257:9652 end-ip-address 2001:cdba::3257:9655 prefix-len 64 match tags v6tag1,v6tag2,v6tag3 set tag v6-stag lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### DHCP Dynamic configuration validation
#### Start IP address validation
##### Objective
Test case confirms invalid start ip address can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid start ip address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 300.300.300.300 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command.
```
###### Test Fail Criteria

#### End IP address validation
##### Objective
Test case checks if invalid end ip address can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid end ip address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 192.168.0.1 end-ip-address 300.300.300.300 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command.
```
###### Test Fail Criteria

#### Netmask address validation
##### Objective
Test case checks if invalid netmask can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid netmask.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 127.0.0.1 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
127.0.0.1 is invalid.
```
###### Test Fail Criteria

#### IPv6 Netmask address validation
##### Objective
Test case checks if netmask can be added for IPv6 configuration.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add netmask for IPv6 configuration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 2001:cdba::3257:9642 end-ip-address 2001:cdba::3257:9648  netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
Error : netmask configuration not allowed for IPv6.
```
###### Test Fail Criteria

#### IP address range validation
##### Objective
Test case checks if invalid ip address range can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid IP address range.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 10.0.0.1 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
Invalid IP address range.
```
###### Test Fail Criteria

#### Broadcast address validation
##### Objective
Test case checks if invalid broadcast address range netmask can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid broadcast address.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.0.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
10.0.0.255 is invalid.
```
###### Test Fail Criteria

#### Match tags validation
##### Objective
Test case checks if invalid match tags can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid match tags.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,this-tag-length-greater-than-15,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
this-tag-length-greater-than-15 is invalid.
```
###### Test Fail Criteria

#### Set tag validation
##### Objective
Test case checks if invalid set tag can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid set tag.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-this-tag-greater-than-15 broadcast 192.168.0.255 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
test-this-tag-greater-than-15 is invalid.
```
###### Test Fail Criteria

#### IP address lease duration validation
##### Objective
Test case checks if invalid lease duration can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid lease duration.

Run command:
```
root(config-dhcp-server)# range test-range start-ip-address start-ip-address 192.168.0.1 end-ip-address 192.168.0.254 netmask 255.255.255.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 192.168.0.255 lease-duration 120000
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command..
```
###### Test Fail Criteria

## Add DHCP Static configuration
### Add DHCP Static IPv4 configuration
#### Objective
Test case checks if DHCP Static IPv4 configuration can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# static 192.168.0.2 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Add DHCP Static IPv6 configuration
#### Objective
Test case checks if DHCP Static IPv4 configuration can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# static 2001:cdba::3257:9680 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### DHCP Static configuration validation
#### Static IP address validation
##### Objective
Test case checks if invalid start ip address can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid static ip address.

Run command:
```
root(config-dhcp-server)# static 300.300.300.300 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command.
```
P match is not configured.

#### MAC address validation
##### Objective
Test case checks if invalid mac address can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid mac address.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aabbccddeeff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
aabbccddeeff is invalid.
```
###### Test Fail Criteria


#### Set tags validation
##### Objective
Test case checks if invalid set tags can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid set tags.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags t1-tag-this-tag-length-greater-than-15,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
t1-tag-this-tag-length-greater-than-15 is invalid
```
###### Test Fail Criteria

#### Client ID validation
##### Objective
Test case checks if invalid client ID can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid client ID.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id this-client-id-length-greater-than-15  match-client-hostname testname lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria

```
this-client-id-length-greater-than-15  is invalid
```
###### Test Fail Criteria

#### Client hostname validation
##### Objective
Test case checks if invalid hostname can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid client hostname.

Run command:
```
root(config-dhcp-server)# static 150.0.0.1 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id tempid  match-client-hostname this-hostname-greater-than-15 lease-duration 60
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
this-hostname-greater-than-15 is invalid
```
###### Test Fail Criteria


## Add DHCP Option configuration
### Add DHCP Option configuration using option name
#### Objective
Test case checks if DHCP Option configuration can be added using option name.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Option configuration using option name.

Run command:
```
root(config-dhcp-server)# option set option-name Router option-value 192.168.0.1  match tags mtag1,mtag2,mtag3
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Add DHCP Option configuration using option number
#### Objective
Test case checks if DHCP Option configuration using option number can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Option configuration using option number.

Run command:
```
root(config-dhcp-server)# option set option-number 3 option-value 192.168.0.3  match tags mtag4,mtag5,mtag6
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
DHCP match is not configured.

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test Fail Criteria


### DHCP Option configuration validation
#### Option name validation
##### Objective
Test case checks if invalid option name can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid option name.

Run command:
```
root(config-dhcp-server)# option set option-name set-option-name-greater-than-15 option-value 10.0.0.1 match tags tag1,tag2,tag3
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
set-option-name-greater-than-15 is invalid
```
###### Test Fail Criteria

#### Option number validation
##### Objective
Test case checks if invalid option number can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid option number.

Run command:
```
root(config-dhcp-server)# option set option-number 300 option-value 10.0.0.1 match tags tag1,tag2,tag3
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command.
```
###### Test Fail Criteria

#### Option match tags validation
##### Objective
Test case checks if invalid match tag can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid match tag.

Run command:
```
root(config-dhcp-server)# option set option-number 3 option-value 10.0.0.1 match tags tag1,match-option-name-greater-than-15,tag3
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
match-option-name-greater-than-15 is in
```
###### Test Fail Criteria

## Add DHCP Match configuration
### Add DHCP Match configuration using option number
#### Objective
Test case checks if DHCP Match configuration can be added using option number.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Match configuration using option number.

Run command:
```
root(config-dhcp-server)# match set tag stag match-option-number 4 match-option-value 192.168.0.4
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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

DHCP BOOTP configuration
------------------------
DHCP BOOTP is not configured.
```
##### Test Fail Criteria

### Add DHCP Match configuration using option name
#### Objective
Test case checks if DHCP Match configuration can be added using option name.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP Match configuration using option name.

Run command:
```
root(config-dhcp-server)# match set tag temp-mtag match-option-name temp-mname match-option-value 192.168.0.5
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
DHCP BOOTP is not configured.
```
##### Test Fail Criteria

### DHCP Match configuration validation
#### Match set tag validation
##### Objective
Test case checks if invalid set tag can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid set tag.

Run command:
```
root(config-dhcp-server)# match set tag tag-greater-than-15 match-option-name tempname match-option-value 10.0.0.1
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
tag-greater-than-15 is invalid
```
###### Test Fail Criteria

#### Match option name validation
##### Objective
Test case checks if invalid match option name can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid match option name.

Run command:
```
root(config-dhcp-server)# match set tag test-tag match-option-name tag-name-greater-than-15 match-option-value 10.0.0.1
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
tag-name-greater-than-15 is invalid
```
###### Test Fail Criteria

#### Match option number validation
##### Objective
Test case checks if invalid match option number can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid match option number.

Run command:
```
root(config-dhcp-server)# match set tag test-tag match-option-number 300  tag-name-greater-than-15 match-option-value 10.0.0.1
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
% Unknown command.
```
###### Test Fail Criteria

## Add DHCP BOOTP
### Add DHCP BOOTP configuration
#### Objective
Test case checks if DHCP BOOTP configuration can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add DHCP BOOTP configuration.

Run command:
```
root(config-dhcp-server)# boot set file /tmp/testfile match tag boottag
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
###### Test Fail Criteria


#### DHCP BOOTP validation
##### Objective
Test case checks if invalid match tag can be added.
##### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

##### Setup
Single Switch topology
##### Description
Add invalid match tag.

Run command:
```
root(config-dhcp-server)# boot set file /tmp/tmpfile match tag boot-tag-name-greater-than-15
```
##### Test Result Criteria
###### Test Pass Criteria
Command would output:
```
boot-tag-name-greater-than-15 is invalid
```
###### Test Fail Criteria

## Verify DHCP server configuration
#### Objective
Test case verifies dhcp-server command.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Verify dhcp-server command.
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
#### Test Result Criteria
##### Test Pass Criteria
Command would output:
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
###### Test Fail Criteria

## Add TFTP server configuration
### Enable TFTP server
#### Objective
Test case checks if tftp-server can be enabled.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Enable TFTP server.
Run command:
```
root(config-tftp-server)# enable
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Disabled
TFTP server file path : Not configured
```
###### Test Fail Criteria

### Enable TFTP server secure mode
#### Objective
Test case checks if tftp-server secure mode can be enabled.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Enable TFTP server secure mode.
Run command:
```
root(config-tftp-server)# secure-mode
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : Not configured
```
###### Test Fail Criteria

### Add TFTP server root path
#### Objective
Test case check if  TFTP server root path can be added.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Add TFTP server root path.
Run command:
```
root(config-tftp-server)# path /etc/testfile
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : /etc/testfile
```
###### Test Fail Criteria


## Verify TFTP server configuration
#### Objective
Test case verifies tftp-server command.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Verify tftp-server command.
Run command:
```
root(config-dhcp-server)# do show tftp-server
```
#### Test Result Criteria
##### Test Pass Criteria
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Enabled
TFTP server file path : /etc/testfile
```
###### Test Fail Criteria

## Delete DHCP Dynamic configuration
### Delete DHCP Dynamic IPv4 configuration
#### Objective
Test case checks if DHCP Dynamic IPv4 configuration can be deleted.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Dynamic configuration.

Run command:
```
root(config-dhcp-server)# no range test-range start-ip-address 10.0.0.1 end-ip-address 10.255.255.254 netmask 255.0.0.0 match tags tag1,tag2,tag3 set tag test-tag broadcast 10.255.255.255 lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Delete DHCP Dynamic IPv6 configuration
#### Objective
Test case checks if DHCP Dynamic IPv6 configuration can be deleted.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Dynamic configuration.

Run command:
```
root(config-dhcp-server)# no range range-ipv6 start-ip-address 2001:cdba::3257:9652 end-ip-address 2001:cdba::3257:9655 prefix-len 64 match tags v6tag1,v6tag2,v6tag3 set tag v6-stag lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

## Delete DHCP Static configuration
### Delete DHCP Static IPv4 configuration
#### Objective
Test case checks if DHCP Static IPv4 configuration can be deleted.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Deleted DHCP Static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# no static 192.168.0.2 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Delete DHCP Static IPv6 configuration
#### Objective
Test case checks if DHCP Static IPv4 configuration can be deleted.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Static IPv4 configuration.

Run command:
```
root(config-dhcp-server)# no static 2001:cdba::3257:9680 match-mac-addresses aa:bb:cc:dd:ee:ff set tags tag4,tag5,tag6 match-client-id testid match-client-hostname testname lease-duration 60
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

## Delete DHCP Option configuration
### Delete DHCP Option configuration using option name
#### Objective
Test case checks if DHCP Option configuration can be deleted using option name.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Option configuration using option name.

Run command:
```
root(config-dhcp-server)# no option set option-name Router option-value 192.168.0.1  match tags mtag1,mtag2,mtag3
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

### Delete DHCP Option configuration using option number
#### Objective
Test case checks if DHCP Option configuration can be deleted using option number.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Option configuration using option number.

Run command:
```
root(config-dhcp-server)# no option set option-number 3 option-value 192.168.0.3  match tags mtag4,mtag5,mtag6
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

## Delete DHCP Match configuration
### Delete DHCP Match configuration using option number
#### Objective
Test case checks if DHCP Match configuration can be added using option number.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Match configuration using option number.

Run command:
```
root(config-dhcp-server)# no match set tag stag match-option-number 4 match-option-value 192.168.0.4
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
Option Number  Option Name       Option Value          Set tag
--------------------------------------------------------------
*              temp-mname        192.168.0.5            temp-mtag

DHCP BOOTP configuration
------------------------
Tag               File
----------------------
boottag          /tmp/testfile
```
##### Test Fail Criteria

### Delete DHCP Match configuration using option name
#### Objective
Test case checks if DHCP Match configuration can be deleted using option name.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP Match configuration using option name.

Run command:
```
root(config-dhcp-server)# no match set tag temp-mtag match-option-name temp-mname match-option-value 192.168.0.5
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria

## Delete DHCP BOOTP configuration
#### Objective
Test case checks if DHCP BOOTP configuration can be deleted.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Delete DHCP BOOTP configuration.

Run command:
```
root(config-dhcp-server)# no boot set file /tmp/testfile match tag boottag
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-dhcp-server)# do show dhcp-server
```
Command would output:
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
##### Test Fail Criteria


## Disable TFTP server secure mode
#### Objective
Test case checks if tftp-server secure mode can be enabled.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Enable TFTP server secure mode.
Run command:
```
root(config-tftp-server)# no secure-mode
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Enabled
TFTP server secure mode : Disabled
TFTP server file path : /etc/testfile
```
###### Test Fail Criteria

## Disable TFTP server
#### Objective
Test case checks if tftp-server secure mode can be disabled.
#### Requirements
- Virtual Mininet Test Setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_dhcp_tftp.py

#### Setup
Single Switch topology
#### Description
Disable TFTP server.
Run command:
```
root(config-tftp-server)# no secure-mode
```
#### Test Result Criteria
##### Test Pass Criteria
Run command:
```
root(config-tftp-server)# do show tftp-server
```
Command would output:
```
TFTP server configuration
-------------------------
TFTP server : Disabled
```
###### Test Fail Criteria
