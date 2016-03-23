# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

TOPOLOGY = """
#
# +-------+
# |  sw1  |
# +-------+
#

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def test_ping_cli(topology, step):
    sw1 = topology.get('sw1')

    assert sw1 is not None

    step('### ping IP validations ###')
    ret = sw1('ping 300.300.300.300')
    assert 'Invalid IPv4 address.' in ret

    ret = sw1('ping 1.1.1.1 data-size 65469')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 data-fill pa')
    assert 'Datafill pattern should be in hexadecimal only.' in ret

    ret = sw1('ping 1.1.1.1 data-fill 12341234123412345')
    assert 'PATTERN: 0x1234123412341234' in ret

    ret = sw1('ping 1.1.1.1 repetitions 0')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 repetitions 100000')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 timeout 61')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 interval 61')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 tos 256')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 ip-option include-timestamp 3')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 ip-option include-timestamp-and-address 3')
    assert '% Unknown command.' in ret

    ret = sw1('ping 1.1.1.1 ip-option record-route 3')
    assert '% Unknown command.' in ret

    step('### ping Host validations ###')
    ret = sw1('ping testname data-size 65469')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname data-fill pa')
    assert 'Datafill pattern should be in hexadecimal only.' in ret

    ret = sw1('ping testname data-fill 12341234123412345')
    assert 'PATTERN: 0x1234123412341234' in ret

    ret = sw1('ping testname repetitions 0')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname repetitions 100000')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname timeout 61')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname interval 61')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname tos 256')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname ip-option include-timestamp 3')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname ip-option include-timestamp-and-address 3')
    assert '% Unknown command.' in ret

    ret = sw1('ping testname ip-option record-route 3')
    assert '% Unknown command.' in ret

    step('### ping6 IP validations ###')
    ret = sw1('ping6 1.1::1.1')
    assert 'Invalid IPv6 address.' in ret

    ret = sw1('ping6 1:1::1:1 data-size 65469')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 1:1::1:1 data-fill pa')
    assert 'Datafill pattern should be in hexadecimal only' in ret

    ret = sw1('ping6 1:1::1:1 data-fill 12341234123412345')
    assert 'PATTERN: 0x1234123412341234' in ret

    ret = sw1('ping6 1:1::1:1 repetitions 0')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 1:1::1:1 repetitions 100000')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 1:1::1:1 interval 61')
    assert '% Unknown command.' in ret

    step('### ping6 Host validations ###')
    ret = sw1('ping6 testname data-size 65469')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 testname data-fill pa')
    assert 'Datafill pattern should be in hexadecimal only' in ret

    ret = sw1('ping6 testname data-fill 12341234123412345')
    assert 'PATTERN: 0x1234123412341234' in ret

    ret = sw1('ping6 testname repetitions 0')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 testname repetitions 100000')
    assert '% Unknown command.' in ret

    ret = sw1('ping6 testname interval 61')
    assert '% Unknown command.' in ret

    ret = sw1('ping 127.0.0.1')
    assert "127.0.0.1" in ret

    ret = sw1("ping localhost data-fill dee datagram-size 200"
              " interval 2 repetitions 1 timeout 2 tos 0 ip-option"
              " include-timestamp-and-address")
    assert "PING localhost (127.0.0.1) 200(268) bytes of data." in ret

    ret = sw1('ping localhost')
    assert "PING localhost" in ret

    ret = sw1('ping localhost data-fill dee datagram-size 200 '
              'interval 2 repetitions 1 timeout 2 tos 0 '
              'ip-option include-timestamp')
    assert 'PING localhost (127.0.0.1) 200(268) bytes of data.' in ret

    ret = sw1('ping6 ::1')
    assert "PING ::1(::1)" in ret

    ret = sw1('ping6 ::1 data-fill dee datagram-size 200 '
              'interval 2 repetitions 1')
    assert "PING ::1(::1)" in ret and "200 data bytes" in ret

    ret = sw1('ping6 localhost')
    assert "PING localhost(localhost)" in ret

    ret = sw1('ping6 localhost data-fill dee datagram-size 200 '
              'interval 2 repetitions 1')
    assert "PING localhost(localhost)" in ret and "200 data bytes" in ret
