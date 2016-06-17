# -*- coding: utf-8 -*-

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

from re import search

TOPOLOGY = """
# +-------+
# |       |     +-------+
# |  hs1  <----->  sw1  |
# |       |     +-------+
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
[type=host name="Host 1"] hs1

# Links
hs1:if01 -- sw1:if01
"""


def test_vtysh_ct_ecmp_cli(topology, step):
    sw1 = topology.get("sw1")
    step("1-ECMP enabled validations")
    return_ = sw1("show ip ecmp")
    assert search('(ECMP\sStatus\s*:\sEnabled)', return_) is not None
    assert search('(Source\sIP\s*:\sEnabled)', return_) is not None
    assert search('(Destination\sIP\s*:\sEnabled)', return_) is not None
    assert search('(Source\sPort\s*:\sEnabled)', return_) is not None
    step("2-ECMP disabled validations")
    # Verify ecmp disable operation, multiple hash disable/enable
    sw1("configure terminal")
    # Disable hash function dst-ip and src-ip
    sw1("ip ecmp load-balance dst-ip disable")
    sw1("ip ecmp load-balance src-port disable")
    return_ = sw1("do show ip ecmp")
    assert search('(Source\sIP\s*:\sEnabled)', return_) is not None
    assert search('(Destination\sIP\s*:\sDisabled)', return_) is not None
    assert search('(Source\sPort\s*:\sDisabled)', return_) is not None
    assert search('(Destination\sPort\s*:\sEnabled)', return_) is not None
    sw1("exit")
    step("3-Show running config for ECMP changes")
    return_ = sw1("show running-config")
    assert 'ip ecmp load-balance src-port disable' in return_ and \
           'load-balance dst-ip disable' in return_
