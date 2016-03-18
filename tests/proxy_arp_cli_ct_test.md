# Proxy ARP CLI Component Tests

## Contents

- [Proxy ARP on a L3 interface](#proxy-arp-on-a-l3-interface)
- [Proxy ARP on a L3 VLAN interface](#proxy-arp-on-a-l3-vlan-interface)
- [Proxy ARP on a L2 interface](#proxy-arp-on-a-l2-interface)
- [Proxy ARP on a split parent interface](#proxy-arp-on-a-split-parent-interface)
- [Proxy ARP on a split child interface](#proxy-arp-on-a-split-child-interface)

## Proxy ARP on a L3 interface

#### Objective
This test verifies that proxy ARP can be enabled on a L3 interface.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_proxy_arp.py

#### Setup
Single switch topology
#### Description
1. Enable proxy ARP on a L3 port as follows:
```
ops(config-ter)# interface 1
ops(config-if)# ip proxy-arp
ops(config-if)# exit
```

2. Verify that proxy ARP is enabled on the interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
interface 1
        ip proxy-arp
```

3. Verfiy that proxy ARP is enabled on the interface using the `show interface [interface-name]` command.
```
ops(config-ter)# show interface 1
Interface 1 is down (Administratively down)
 Admin state is down
 State information: admin_down
 Hardware: Ethernet, MAC Address: 70:72:cf:c3:31:cd
 Proxy ARP is enabled
 MTU 0
 Half-duplex
 Speed 0 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is off, output flow-control is off
 RX
            0 input packets              0 bytes
            0 input error                0 dropped
            0 CRC/FCS
 TX
            0 output packets             0 bytes
            0 input error                0 dropped
            0 collision
```

4. Verify that proxy ARP is enabled on the interface using the `show running-config interface [interface-name]` command.
```
ops(config-ter)# show running-config interface 1
interface 1
        ip proxy-arp
        exit
```

5. Disable proxy ARP on a L3 port as follows:
```
ops(config-ter)# interface 1
ops(config-if)# no ip proxy-arp
ops(config-if)# exit
```

6. Verify that proxy ARP is disabled on the interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
        no shutdown
```

7. Verify that proxy ARP is disabled on the interface using the `show interface [interface-name]` command.
```
ops(config-ter)# show interface 1
Interface 1 is down (Administratively down)
     Admin state is down
     State information: admin_down
     Hardware: Ethernet, MAC Address: 70:72:cf:c3:31:cd
     MTU 0
     Half-duplex
     Speed 0 Mb/s
     Auto-Negotiation is turned on
     Input flow-control is off, output flow-control is off
     RX
                0 input packets              0 bytes
                0 input error                0 dropped
                0 CRC/FCS
     TX
                0 output packets             0 bytes
                0 input error                0 dropped
                0 collision
```

## Proxy ARP on a L3 VLAN interface

#### Objective
This test verifies that proxy ARP can be enabled and disabled on a L3 VLAN interface.
#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_proxy_arp.py

#### Setup
Single switch topology

#### Description
1. Enable proxy ARP on a L3 VLAN interface as follows:
```
ops(config)# interface vlan 1
ops(config-if-vlan)# ip proxy-arp
ops(config-if-vlan)# exit
```

2. Verify that proxy ARP is enabled on the vlan interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
        no shutdown
interface vlan1
        no shutdown
        ip proxy-arp
```

3. Disable proxy ARP on a L3 VLAN interface as follows:
```
ops(config)# interface vlan 1
ops(config-if-vlan)# no ip proxy-arp
ops(config-if-vlan)# exit
```

4. Verify that proxy ARP is disabled on the vlan interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
        no shutdown
interface vlan1
		no shutdown
```

### Proxy ARP on a L2 interface
#### Objective
This test verifies that proxy ARP cannot be enabled on a L2 interface.

#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_proxy_arp.py

#### Setup
Single switch topology

#### Description
1. Disable routing on a L3 port to make it a L2 inteface as follows:
```
ops(config-ter)# interface 1
ops(config-if)# no routing
ops(config-if)# exit
```

2. Verify that the proxy ARP configuration fails with an error message.
```
ops(config-ter)# interface 1
ops(config-if)# ip proxy-arp
Interface 3 is not L3.
```

### Proxy ARP on a split parent interface
#### Objective
This test verifies that proxy ARP:
- Can be enabled and disabled on a parent interface that is not split.
- Cannot be enabled on a parent interface that is already split.

#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_proxy_arp.py

#### Setup
Single switch topology

#### Description
1. Enable proxy ARP on a split capable parent interface as follows:
```
ops(config-ter)# interface 51
ops(config-if)# ip proxy-arp
ops(config-if)# exit
```

2. Verify that proxy ARP is enabled on a split capable parent interface using the `show interface [interface-name]` command.
```
ops(config-ter)# show interface 51
Interface 51 is down (Administratively down)
 Admin state is down
 State information: admin_down
 Hardware: Ethernet, MAC Address: 70:72:cf:c3:31:cd
 Proxy ARP is enabled
 MTU 0
 Half-duplex
 Speed 0 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is off, output flow-control is off
 RX
            0 input packets              0 bytes
            0 input error                0 dropped
            0 CRC/FCS
 TX
            0 output packets             0 bytes
            0 input error                0 dropped
            0 collision
```

3. Disable proxy ARP on a split capable parent interface as follows:
```
ops(config-ter)# interface 51
ops(config-if)# no ip proxy-arp
ops(config-if)# exit
```

4. Verify that proxy ARP is disabled on a split capable parent interface using the `show interface [interface-name]` command.
```
ops(config-ter)# show interface 51
Interface 51 is down (Administratively down)
 Admin state is down
 State information: admin_down
 Hardware: Ethernet, MAC Address: 70:72:cf:c3:31:cd
 MTU 0
 Half-duplex
 Speed 0 Mb/s
 Auto-Negotiation is turned on
 Input flow-control is off, output flow-control is off
 RX
            0 input packets              0 bytes
            0 input error                0 dropped
            0 CRC/FCS
 TX
            0 output packets             0 bytes
            0 input error                0 dropped
            0 collision
```

5. Verify that the proxy ARP configuration fails with an error message on a split parent interface.
```
ops(config-ter)# interface 51
ops(config-if)# split
Warning: This will remove all L2/L3 configuration on parent interface.
Do you want to continue [y/n]? y
ops(config-if)# ip proxy-arp
This interface has been split. Operation not allowed
ops(config-if)# exit
```

### Proxy ARP on a split child interface

#### Objective
This test verifies that proxy ARP:
- Can be enabled and disabled on a child interface of a parent that is split.
- Cannot be enabled on a child interface of a parent interface that is not split.
- Proxy ARP configuration on a child interface is removed if the parent is reset to no-split.

#### Requirements
- Mininet test setup
- **CT file**: ops-cli/vtysh/tests/test_vtysh_ct_proxy_arp.py

#### Setup
Single switch topology

#### Description
1. Split the parent interface as follows:
```
ops(config-ter)# interface 50
ops(config-if)# split
Warning: This will remove all L2/L3 configuration on parent interface.
Do you want to continue [y/n]? y
ops(config-if)# exit
```

2. Enable the proxy ARP configuration on a child interface of a parent that is split.
```
ops(config-ter)# inter 50-1
ops(config-if)# ip proxy-arp
ops(config-if)# exit
```

3. Verify that proxy ARP is enabled on the child interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
        no shutdown
interface 50
         split
interface 50-1
         ip proxy-arp
```

4. Disable the proxy ARP configuration on a child interface of a parent that is split.
```
ops(config-ter)# inter 50-1
ops(config-if)# no ip proxy-arp
ops(config-if)# exit
```

5. Verify that proxy ARP is disabled on the child interface using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
        no shutdown
interface 50
        split
```

6. Enable the proxy ARP interface on a child interface and reset the split interface on the parent.
```
ops(config-ter)# int 50-1
ops(config-if)# ip proxy-arp
ops(config-ter)# int 50
ops(config-if)# no split
Warning: This will remove all L2/L3 configuration on child interfaces.
Do you want to continue [y/n]? y
```

7. Verify that the proxy ARP configuration on a child interface is disabled if the parent is reset to no split using the `show running-config` command.
```
ops(config-ter)# show running-config
Current configuration:
!
!
!
!
!
vlan 1
    no shutdown
```

8. Verify that the proxy ARP configuration on a child interface of a non-split parent interface fails with an error message.
```
ops(config-ter)# interface 51-1
ops(config-if)# ip proxy-arp
This is a QSFP child interface whose parent interface has not been split. Operation not allowed
ops(config-if)# exit
```
