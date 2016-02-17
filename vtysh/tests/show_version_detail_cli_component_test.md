# show version detail CLI Component Test Case

## Contents
- [show version detail CLI consistency with Source_Repository table](#show-version-detail-CLI-consistency-with-Source-Repository-table)

## show version detail CLI consistency with Source_Repository table

### Objective
Verify that output of show version detail CLI is consistent with 
Source_Repository table

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Add 2 records to Source_Repository table
2. Verify the existence of the 2 records added via "show version detail" CLI command

### Test result criteria
#### Test pass criteria
Added records to Source_Repository table were listed in "show version detail" CLI output

#### Test fail criteria
Added records to Source_Repository table were not listed in "show version detail" CLI output
