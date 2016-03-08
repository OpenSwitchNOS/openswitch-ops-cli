# -*- coding: utf-8 -*-
# (C) Copyright 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
##########################################################################

"""
OpenSwitch Test for switchd related configurations.
"""

# from pytest import set_trace
# from time import sleep

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
"""


def settracerouteiptest(dut01):
    cmdout = dut01("traceroute 1.1.1.1")
    assert '1.1.1.1' in cmdout


def settraceroutehosttest(dut01):
    cmdout = dut01("traceroute localhost")
    assert 'localhost' in cmdout


def settracerouteipmaxttltest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 maxttl 30")
    assert '1.1.1.1' in cmdout


def settraceroutehostmaxttltest(dut01):
    cmdout = dut01("traceroute localhost maxttl 30")
    assert 'localhost' in cmdout


def settracerouteipminttltest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 minttl 1")
    assert '1.1.1.1' in cmdout


def settraceroutehostminttltest(dut01):
    cmdout = dut01("traceroute localhost minttl 1")
    assert 'localhost' in cmdout


def settraceroutehostdstporttest(dut01):
    cmdout = dut01("traceroute localhost dstport 33434")
    assert 'localhost' in cmdout


def settracerouteipdstporttest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 dstport 33434")
    assert '1.1.1.1' in cmdout


def settraceroutehostprobestest(dut01):
    cmdout = dut01("traceroute localhost probes 3")
    assert 'localhost' in cmdout


def settracerouteipprobestest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 probes 3")
    assert '1.1.1.1' in cmdout


def settraceroutehosttimeouttest(dut01):
    cmdout = dut01("traceroute localhost timeout 3")
    assert 'localhost' in cmdout


def settracerouteiptimeouttest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 timeout 3")
    assert '1.1.1.1' in cmdout


def settracerouteipoptionstest(dut01):
    cmdout = dut01("traceroute 1.1.1.1 ip-option loosesourceroute 1.1.1.8")
    assert '1.1.1.1' in cmdout


def settraceroutehostipoptionstest(dut01):
    cmdout = dut01("traceroute localhost ip-option loosesourceroute 1.1.1.8")
    assert 'localhost' in cmdout


def settraceroute6iptest(dut01):
    cmdout = dut01("traceroute6 0:0::0:1")
    assert '0:0::0:1' in cmdout


def settraceroute6hosttest(dut01):
    cmdout = dut01("traceroute6 localhost")
    assert 'localhost' in cmdout


def settraceroute6ipmaxttltest(dut01):
    cmdout = dut01("traceroute6 0:0::0:1 maxttl 30")
    assert '0:0::0:1' in cmdout


def settraceroute6hostmaxttltest(dut01):
    cmdout = dut01("traceroute6 localhost maxttl 30")
    assert 'localhost' in cmdout


def settraceroute6iptimeouttest(dut01):
    cmdout = dut01("traceroute6 0:0::0:1 timeout 3")
    assert '0:0::0:1' in cmdout


def settraceroute6hosttimeouttest(dut01):
    cmdout = dut01("traceroute6 localhost timeout 3")
    assert 'localhost' in cmdout


def settraceroute6ipprobestest(dut01):
    cmdout = dut01("traceroute6 0:0::0:1 probes 3")
    assert '0:0::0:1' in cmdout


def settraceroute6hostprobestest(dut01):
    cmdout = dut01("traceroute6 localhost probes 3")
    assert 'localhost' in cmdout


def settraceroute6ipdstporttest(dut01):
    cmdout = dut01("traceroute6 0:0::0:1 dstport 33434")
    assert '0:0::0:1' in cmdout


def settraceroute6hostdstporttest(dut01):
    cmdout = dut01("traceroute6 localhost dstport 33434")
    assert 'localhost' in cmdout


def test_vtysh_ct_traceroute(topology, step):
    dut01obj = topology.get("ops1")
    assert dut01obj is not None

    settracerouteiptest(dut01obj)

    settraceroutehosttest(dut01obj)

    settracerouteipmaxttltest(dut01obj)

    settraceroutehostmaxttltest(dut01obj)

    settracerouteipminttltest(dut01obj)

    settraceroutehostminttltest(dut01obj)

    settracerouteipdstporttest(dut01obj)

    settraceroutehostdstporttest(dut01obj)

    settracerouteiptimeouttest(dut01obj)

    settraceroutehosttimeouttest(dut01obj)

    settracerouteipprobestest(dut01obj)

    settraceroutehostprobestest(dut01obj)

    settracerouteipoptionstest(dut01obj)

    settraceroutehostipoptionstest(dut01obj)

    settraceroute6iptest(dut01obj)

    settraceroute6hosttest(dut01obj)

    settraceroute6ipmaxttltest(dut01obj)

    settraceroute6hostmaxttltest(dut01obj)

    settraceroute6ipdstporttest(dut01obj)

    settraceroute6hostdstporttest(dut01obj)

    settraceroute6iptimeouttest(dut01obj)

    settraceroute6hosttimeouttest(dut01obj)

    settraceroute6ipprobestest(dut01obj)

    settraceroute6hostprobestest(dut01obj)
