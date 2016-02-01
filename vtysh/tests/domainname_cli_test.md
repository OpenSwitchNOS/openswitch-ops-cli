Domain name CLI tests
===

## Contents

- [Test case :Verify to configure system domainname  through CLI without input](#test-case-:verify-to-configure-system-domainname-through-cli-without-input)
- [Test case :Verify to configure system domainname through CLI](#test-case-:verify-to-configure-system-domainname-through-cli)
- [Test case :Verify to display system domainname through CLI](#test-case-:verify-to-display-system-domainname-through-cli)
- [Test case :Verify to reset configured system domainname through CLI](#test-case-:verify-to-reset-configured-system-domainname-through-cli)


### Test case :Verify to configure system domainname  through CLI without input
#### Description ####
Test to verify if domainname is allowed to configure without input
#### Test result criteria ###
##### Pass criteria ####
"% Comma nd incomplete." should be present in output
##### Fail criteria ####
if "% Comma nd incomplete." is not present then the TC fails


### Test case :Verify to configure system domainname through CLI
#### Description ####
Test to verify if domainname is getting configured through CLI
#### Test result criteria ###
##### Pass criteria ####
configured domainname should be present in the output of the command ovs-vsctl list domainname
##### Fail criteria ####
configured domainname is not present in the output of the command ovs-vsctl list domainname

### Test case :Verify to display system domainname through CLI
#### Description ####
Test to verify if domainname is getting displayed with show domainname command
#### Test result criteria ###
##### Pass criteria ####
configured domainname  should be present in output
##### Fail criteria ####
if the configured domainname is not present then the TC fails



### Test case :Verify to reset configured system domainname through CLI
#### Description ####
Test to verify if domainname is getting reset with the no domainname command
#### Test result criteria ###
##### Pass criteria ####
domain_name should not be present in the output
##### Fail criteria ####
if domain_name is present then the TC fails



