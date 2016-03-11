VLAN CLI Component Tests
===

## Contents

- [Create VLAN](#create-VLAN)
- [Show VLAN summary](#show-VLAN-summary)
- [Delete VLAN](#delete-VLAN)
- [Add interface to access VLAN](#add-interface-to-access-VLAN)
- [Add interface to trunk VLAN](#add-interface-to-trunk-VLAN)
- [Add interface to trunk native VLAN](#add-interface-to-trunk-native-VLAN)
- [Add interface to trunk native tag VLAN](#add-interface-to-trunk-native-tag-VLAN)
- [Add interface to access VLAN](#add-interface-to-access-VLAN)
- [Add interface to trunk VLAN](#add-interface-to-trunk-VLAN)
- [Add interface to trunk native VLAN](#add-interface-to-trunk-native-VLAN)
- [Add interface to trunk native tag VLAN](#add-interface-to-trunk-native-tag-VLAN)
- [VLAN commands](#VLAN-commands)

### Create VLAN  ###
#### Description ####
Verify the creation of a new VLAN.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show running-config` command output displays the VLAN.
#### Test fail criteria ####
This test fails if the VLAN is not created and is not displayed in the `do show running-config` command output.

### Show VLAN summary  ###
#### Description ####
Confirm that the `show vlan summary` command output displays the right number of existing VLANs.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `show vlan summary` command output displays the right number of existing VLANs.
#### Test fail criteria ####
This test fails if the `show vlan summary` command output does not display the right number of existing VLANs.

### Delete VLAN  ###
#### Description ####
Verify the removal of a new VLAN.

### Test result criteria ###
#### Test pass criteria ####
The test is successful if the `show vlan summary` command output does not display the removed VLAN.
#### Test fail criteria ####
This test fails if the VLAN is not removed.

### Add interface to access VLAN ###
#### Description ####
Verify the addition of an interface to access VLANs. Corner cases need to be covered such as checking for routing
status on the interface.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show running-config` command output has VLAN access under an interface.
#### Test fail criteria ####
This test fails with any other `do show running-config` command output.

### Add interface to trunk VLAN  ###
#### Description ####
Confirm the addition of an interface to a trunk VLAN. Corner cases need to be covered such as checking for routing
status on the interface and that the interface is not part of a LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and `do show running-config` has an interface in the VLAN.
#### Test fail criteria ####
This test fails if theres is no interface in the VLAN.

### Add interface to trunk native VLAN  ###
#### Description ####
Verify the addition of an interface to the trunk native VLAN. Corner cases need to be covered such as checks for
routing status on the interface and that the interface is not part of LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and the `do show running-config` command outputs show an interface with
a native VLAN.
#### Test fail criteria ####
This test fails if there is no interface with a native VLAN or if there is any other output.

### Add interface to trunk native tag VLAN  ###
#### Description ####
Confirm the addition of an interface to the trunk native tagging VLAN. Corner cases need to be covered, such as checks
for routing status on the interface and that the interface is not part of LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and the `do show running-config` command output show an interface with
VLAN tagging.
#### Test fail criteria ####
This test fails if there is no interface with VLAN tagging.

### Add LAG to access VLAN ###
#### Description ####
Verify the addition of LAG to an access VLAN. Corner cases need to be covered such as checks for routing status on the LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show running-config` command output shows VLAN access under the LAG.
#### Test fail criteria ####
This test fails if the `do show running-config` command output does not show VLAN access under the LAG.

### Add LAG to trunk VLAN  ###
#### Description ####
Confirm the addition of a LAG to the trunk VLAN. Corner cases need to be covered such as checks for routing 
status on the LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and the `do show running-config` command output show a LAG in the VLAN.
#### Test fail criteria ####
This test fails if the `do show vlan` and the `do show running-config` commands output does not display a LAG in the VLAN.

### Add LAG to trunk native VLAN ###
#### Description ####
Verify the addition of a LAG to the trunk native VLAN. Corner cases need to be covered such as checks for routing
status on the LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and `do show running-config` command output displays a LAG
with native VLAN.
#### Test fail criteria ####
This test fails if the `do show vlan` and `do show running-config` command output does not display a LAG
with a native VLAN.

### Add LAG to trunk native tag VLAN  ###
#### Description ####
Confirm the addition of a LAG to the trunk native tagging VLAN. Corner cases need to be covered such as checks
for routing status on the LAG.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the `do show vlan` and `do show running-config` command output displays an interface
with VLAN tagging.
#### Test fail criteria ####
This test fails if the `do show vlan` and `do show running-config` command output does not display an interface
with VLAN tagging.

### VLAN commands  ###
#### Description ####
Verify that the `no shutdown` command is working in the VLAN context.

### Test result criteria ###
#### Test pass criteria ####
This test is successful if the Admin column in the table is updated.
#### Test fail criteria ####
This test fails if the Admin column in the table is not updated.

