
# Arpmgrd Test Cases


## Contents

- [Basic functionality](#basic-functionality)
- [Delete IPv4 address](#delete-ipv4-address)
- [Change state to false](#change-state-to-false)
- [Change state to true](#change-state-to-true)
- [Delete neighbor entries](#delete-neighbor-entries)
- [Restartability](#restartability)
- [Transaction failure in OVSDB](#transaction-failure-in-ovsdb)

##  Basic functionality
### Objective
Test case checks basic functionality of arpmgrd.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_functionality.py (Functionality)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
Connect 2 neighbors with IPv4 and IPv6 addresses to
L3 interfaces configured on switch. Ping, ping6 neighbors from switch. Verify neighbors using following CLI commands:

```
show arp
show ipv6 neighbors
```
### Test Result Criteria
#### Test Pass Criteria
2 IPv4 and 2 Ipv6 addresses with Mac addresses should be added to Neighbor table.

Port should be corresponding to the L3 interfaces, and the state is reachable initially. Verify VRF pointing to VRF of Port.
#### Test Fail Criteria

##  Delete IPv4 address
### Objective
Test case checks deletion of IPV4 address on arpmgrd.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_functionality.py (Functionality)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
State change. Delete Ipv4 address on a host keeping dp_hit in status column of
OVSDB empty or set as true. Wait for atleast 45 seconds (max time to stale entry in kernel).
### Test Result Criteria
#### Test Pass Criteria
Kernel will stale entry, and on transition to stale, since dp_hit is not set (Default value: **true**),
arpmgrd will make kernel reject the entry, and entry resolution will fail. OVSDB should reflect this state as failed, MAC address should be empty.
#### Test Fail Criteria

##  Change state to false
### Objective
Test case checks state change in arpmgrd.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_functionality.py (Functionality)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
**DP_HIT false and reprobing**: If dp_hit status is true in Neighbor table, arpmrd will force kernel to reject entry. If it is false, it will not probe when entry becomes `stale`.
The dp_hit status is changed by vswitchd based on ASIC feedback for active traffic for host.

Set dp_hit to false for a neighbor entry, using ovsb-client:

```
ovsdb-client transact
'[ "OpenHalon",
    {
        "op" : "update",
        "table" : "Neighbor",
        "where":[["ip_address","==","192.168.1.2"]],
        "row":{"status":["map",[["dp_hit","false"]]]}
     }
]'
```

Then wait for upto 45 seconds for entry to stale out.

### Test Result Criteria
#### Test Pass Criteria
Entry state  should change from reachable to `stale`, since we will not probe.

**Note**: Neighbors with stale state are still valid entries which will remain programmed in ASIC
#### Test Fail Criteria

##  Change state to true
### Objective
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_functionality.py (Functionality)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
**DP_HIT true and reprobing**: Set dp_hit to false for a neighbor entry, using ovsb-client:
```
ovsdb-client transact
'[ "OpenHalon",
    {
        "op" : "update",
        "table" : "Neighbor",
        "where":[["ip_address","==","192.168.1.2"]],
        "row":{"status":["map",[["dp_hit","true"]]]}
     }
]'
```

### Test Result Criteria
#### Test Pass Criteria
As soon as dp_hit state is set to true arpmgrd it will make kernel probe for entry and
keep refreshing entry whenever it forces stale. Entry state should be reachable.
#### Test Fail Criteria

##  Delete neighbor entries
### Objective
Test case checks deletion of neighbor entries.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_functionality.py (Functionality)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
Enter deletions.
Force the deletions to the kernel by removing IP address on L3 interface or changing IP address to a new subnet.
This will remove all old neighbor entries in kernel for that interface.
### Test Result Criteria
#### Test Pass Criteria
Neighbors in kernel should get removed. Corresponding OVSDB rows should get deleted.
#### Test Fail Criteria

##  Restartability
### Objective
Test case checks arpmgrd restartability.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ct_restartability.py (Restartability)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
        ↓
       [H3]
```

### Description
1. Kill arpmgrd and connect a new host to a new L3 interface, add some static arp entries to kernel.
**Note**: Static arp/neighbor entries feature not supported for BASIL. Used only for testing.
1. Force change of status of some hosts to go to failed or stale.
1. Restart arpmgrd

### Test Result Criteria
#### Test Pass Criteria
Before restart, OVSDB will still show old neighbors without any of the updates of
new neighbors, deleted neighbors, modified eighbors. After restarting arpmgrd, we should
see new neighbors, static neighbors in OVSDB. Deleted neighbors should be deleted
in OVSDB and modified neighbors should reflect the modified states in OVSDB
#### Test Fail Criteria

##  Transaction failure in OVSDB
### Objective
Test case checks arpmgrd working when there is a transaction failure at OVSDB.
### Requirements
- Virtual Mininet Test Setup
- **CT File**: ops-arpmgrd/test/test_arpmgrd_ft_transaction_failures.py (Transaction Failure)

### Setup
#### Topology Diagram
```
[H1] ← [S1] → [H2]
```

### Description
1. Kill ovsdb-server and connect a new host to a new L3 interface.
1. Add some static arp entries to kernel.
**Note**: Static arp/neighbor entries feature not supported for BASIL. Used only for testing.
1. Force deletion of some entries by removing ip address of some existing L3 interface.
1. Force change of status of some hosts to go to failed or stale.
1. Restart ovsdb-server
### Test Result Criteria
#### Test Pass Criteria
Before restart OVSDB will still show old neighbors without any of the updates of new neighbors, deleted neighbors or modified neighbors. After restarting ovsdb-server, you should
see new neighbors, static neighbors in OVSDB. Deleted neighbors should be deleted in OVSDB and modified neighbors should reflect the modified states in OVSDB
#### Test Fail Criteria
