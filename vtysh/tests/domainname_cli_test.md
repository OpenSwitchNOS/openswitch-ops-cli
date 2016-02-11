# Domain Name CLI Tests


## Contents

- [Test cases](#test-cases)
	- [Verify configuring the system domain name through CLI without input](#verify-configuring-the-system-domain-name-through-cli-without-input)
	- [Verify configuring the system domain name through CLI](#verify-configuring-the-system-domain-name-through-cli)
	- [Verify the system domain name displays through CLI](#verify-the-system-domain-name-displays-through-cli)
	- [Verify that configured system domain name is reset through CLI](#verify-that-configured-system-domain-name-is-reset-through-cli)

## Test cases
###Verify configuring the system domain name through CLI without input
#### Description
Verify if the domain name can be configured without input.
#### Test result criteria
##### Pass criteria
The "% Command incomplete." line should be present in the output.
##### Fail criteria
If the "% Command incomplete." line is not present, then the test case fails.


### Verify configuring the system domain name through CLI
#### Description
Verify if the domain name can be configured through CLI.
#### Test result criteria
##### Pass criteria
The configured domain name should be present in the output of the `ovs-vsctl list domainname` command.
##### Fail criteria
If the configured domain name is not present in the output of the `ovs-vsctl list domainname` command then the test case fails.

### Verify the system domain name displays through CLI
#### Description
Verify if the domain name is displayed using the `show domainname` command.
#### Test result criteria
##### Pass criteria
The configured domain name should be  present in the output.
##### Fail criteria
If the configured domain name is not present, then the test case fails.



### Verify that the configured system domain name is reset through CLI
#### Description
Verify if the domain name is reset with the `no domainname` command.
#### Test result criteria
##### Pass criteria
The domain name should not be  present in the output.
##### Fail criteria ####
If the domain name is present, then the test case fails.
