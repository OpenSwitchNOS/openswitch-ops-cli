# show version detail CLI Feature Test Case

## Contents
- [Verify show version detail CLI functionality](#Verify-show-version-detail-CLI-functionality)

## Verify show version detail CLI functionality

### Objective
Verify the functionality of the `show version detail` CLI command.

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Run the `show version detail` CLI command.
2. Verify the existence of `ops-sysd` in the output. (The `ops-sysd` repository name is present in
   the Source-Repository table if it was correctly populated during sysd initialization.) As a
   result, the `ops-sysd` record should be present in the `show version detail` CLI output.

### Test result criteria
#### Test pass criteria
The `ops-sysd` record is in the `show version detail` CLI output.

#### Test fail criteria
The `ops-sysd` record is not in the `show version detail` CLI output.
