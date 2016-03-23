# -*- coding: utf-8 -*-

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHoutput ANY WARRANTY; withoutput even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


# Topology definition. the topology contains two back to back switches
# having four links between them.


TOPOLOGY = """
# +-------+
# |  sw1  |
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
"""


def get_show_version_detail_cli_ct_result(sw1):
    lines = sw1('show version detail').splitlines()
    lines = [line.replace(' ', '') for line in lines]
    output = ''.join(lines)
    return output


def add_git_entry_to_package_info(sw1):
    sw1('ovsdb-client transact \'["OpenSwitch", \
            {"op":"insert", "table": "Package_Info", "row": \
            {"name": "test-repo-1", "src_type": "git", \
            "src_url": "git.testRepo1.net", \
            "version":"abcdef007" } } ]\'', shell='bash')


def add_other_entry_to_package_info(sw1):
    sw1('ovsdb-client transact \'["OpenSwitch", \
            {"op":"insert", "table": "Package_Info", "row": \
            {"name": "test-repo-2", "src_type": "other", \
            "src_url": "ftp.testRepo2.com/file.tar.gz",\
            "version":"1.0.0" } } ]\'', shell='bash')


def test_vtysh_ct_show_version_detail(topology):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    record1 = "{}{}{}{}".format("PACKAGE:test-repo-1",
                                "VERSION:abcdef007",
                                "SOURCETYPE:git",
                                "SOURCEURL:git.testRepo1.net")
    record2 = "{}{}{}{}".format("PACKAGE:test-repo-2",
                                "VERSION:1.0.0",
                                "SOURCETYPE:other",
                                "SOURCEURL:ftp.testRepo2.com/file.tar.gz")
    add_git_entry_to_package_info(sw1)
    add_other_entry_to_package_info(sw1)
    output = get_show_version_detail_cli_ct_result(sw1)
    assert record1 in output and record2 in output
