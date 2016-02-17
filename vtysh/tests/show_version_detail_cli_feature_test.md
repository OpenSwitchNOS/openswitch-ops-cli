# show version detail CLI Feature Test Case

## Contents
- [Verify show version detail CLI functionality](#Verify-show-version-detail-CLI-functionality)

## Verify show version detail CLI functionality

### Objective
Verify the working of 'show version detail' CLI

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Run "show version detail" CLI command
2. Verify the existence "ops-sysd" in the output. ("ops-sysd" repository name will be present
   in Source_Repository table if it was populated correctly during sysd initialization. As a
   result, "ops-sysd" record should be present in "show version detail" CLI output.

### Test result criteria
#### Test pass criteria
"ops-sysd" was found in "show version detail" CLI display

#### Test fail criteria
"ops-sysd" was not found in "show version detail" CLI display
