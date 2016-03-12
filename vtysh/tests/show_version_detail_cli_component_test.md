# show version detail CLI Component Test Case

## Contents
- [show version detail CLI consistency with Package_Info table](#show-version-detail-CLI-consistency-with-Package-Info-table)

## show version detail CLI consistency with Package_Info table

### Objective
Verify that the output of the `show version detail` CLI command is consistent with the
Package_Info table.

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Add two records to the Package_Info table
2. Verify the existence of the two records added using the `show version detail` CLI command.

### Test result criteria
#### Test pass criteria
Added records to the Package_Info table were listed in the `show version detail` CLI output.

#### Test fail criteria
Added records to the Package_Info table were not listed in the `show version detail` CLI output.
