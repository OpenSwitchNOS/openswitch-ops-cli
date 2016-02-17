# show version detail CLI Component Test Case

## Contents
- [show version detail CLI consistency with Source_Repository table](#show-version-detail-CLI-consistency-with-Source-Repository-table)

## show version detail CLI consistency with Source_Repository table

### Objective
Verify that the output of the `show version detail` CLI command is consistent with the Source_Repository table.

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Add two records to the Source_Repository table
2. Verify the existence of the two records added using the `show version detail` CLI command.

### Test result criteria
#### Test pass criteria
Added records to the Source_Repository table were listed in the `show version detail` CLI output.

#### Test fail criteria
Added records to the Source_Repository table were not listed in the `show version detail` CLI output.
