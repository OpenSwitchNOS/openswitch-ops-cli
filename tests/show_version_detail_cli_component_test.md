# show version detail CLI Component Test Case

## Contents
- [show version detail CLI consistency with Package_Info table](#show-version-detail-CLI-consistency-with-Package-Info-table)
- [show version detail ops CLI consistency with Package_Info table](#show-version-detail-opsCLI-consistency-with-Package-Info-table)

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

## show version detail ops CLI consistency with Package_Info table

### Objective
Verify that the output of the `show version detail ops` CLI command is consistent with the
ops packages in Package_Info table.

### Requirements
Virtual Mininet Test Setup.

### Setup
#### Topology diagram
```
  [s1]
```

### Description
1. Add ops and non-ops records to the Package_Info table
2. Verify the existence of ops record and non-existence of non-ops record using
the `show version detail ops` CLI command.

### Test result criteria
#### Test pass criteria
Added ops record to the Package_Info table is listed in the `show version detail ops` CLI output.
Added non-ops record to the Package_Info table is not listed in the `show version detail ops`
CLI output.

#### Test fail criteria
Added ops record to the Package_Info table is not listed in the `show version detail ops` CLI output.
Added non-ops record to the Package_Info table is listed in the `show version detail ops`
CLI output.
