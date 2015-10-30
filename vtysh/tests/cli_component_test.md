CLI Component Tests
===

## Contents

- [VLAN CLI Component Tests](#vlan-cli-component-tests)

## VLAN CLI Component Tests

- [Create Vlan](#create-vlan)
- [Show Vlan Summary](#show-vlan-summary)
- [Delete Vlan](#delete-vlan)
- [Add access vlan to interface](#add-access-vlan-to-interface)
- [Add trunk vlan to interface](#add-trunk-vlan-to-interface)
- [Add trunk native vlan to interface](#add-trunk-native-vlan-to-interface)
- [Add trunk native tag vlan to interface](#add-trunk-native-tag-vlan-to-interface)
- [Add access vlan to LAG](#add-access-vlan-to-lag)
- [Add trunk vlan to LAG](#add-trunk-vlan-to-lag)
- [Add trunk native vlan to LAG](#add-trunk-native-vlan-to-lag)
- [Add trunk native tag vlan to LAG](#add-trunk-native-tag-vlan-to-lag)
- [Vlan commands](#vlan-commands)

### Create Vlan  ###
#### Description ####
Test to verify the creation of new vlan

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show running-config" has the vlan.
#### Test fail criteria ####
Test case result is a fail if the no the given vlan is not created.

### Show Vlan Summary  ###
#### Description ####
Test to verify `show vlan summary` command

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "show vlan summary" prints the right number of existing vlans.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Delete Vlan  ###
#### Description ####
Test to verify the removal of new vlan

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show running-config" doesn't have the vlan.
#### Test fail criteria ####
Test case result is a fail if the vlan is not removed.

### Add access vlan to interface  ###
#### Description ####
Test to verify the addition of interface to access vlan. Corner cases need to be covered
like checks for status of routing on the interface.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show running-config" has vlan access under interface.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk vlan to interface  ###
#### Description ####
Test to verify the addition of interface to trunk vlan. Corner cases need to be covered
like checks for status of routing on the interface and that interface is not part of LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" has interface in the vlan.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk native vlan to interface  ###
#### Description ####
Test to verify the addition of interface to trunk native vlan. Corner cases need to be covered
like checks for status of routing on the interface and that interface is not part of LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" has interface with native vlan.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk native tag vlan to interface  ###
#### Description ####
Test to verify the addition of interface to trunk native tagging vlan. Corner cases need to be covered
like checks for status of routing on the interface and that interface is not part of LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" interface has vlan tagging.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add access vlan to LAG  ###
#### Description ####
Test to verify the addition of LAG to access vlan. Corner cases need to be covered
like checks for status of routing on the LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show running-config" has vlan access under LAG.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk vlan to LAG  ###
#### Description ####
Test to verify the addition of LAG to trunk vlan. Corner cases need to be covered
like checks for status of routing on the LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" has LAG in the vlan.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk native vlan to LAG  ###
#### Description ####
Test to verify the addition of LAG to trunk native vlan. Corner cases need to be covered
like checks for status of routing on the LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" has LAG with native vlan.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Add trunk native tag vlan to LAG  ###
#### Description ####
Test to verify the addition of LAG to trunk native tagging vlan. Corner cases need to be covered
like checks for status of routing on the LAG.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the "do show vlan" and "do show running-config" interface has vlan tagging.
#### Test fail criteria ####
Test case result is a fail for any other output.

### Vlan commands  ###
#### Description ####
Test to verify `no shutdown` is working in vlan context.

### Test result criteria ###
#### Test pass criteria ####
Test case result is a success if the column in the table is updated.
#### Test fail criteria ####
Test case result is a fail for any other output.

