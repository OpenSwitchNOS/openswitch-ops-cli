#!/usr/bin/python

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

from time import sleep
from opsvsi.docker import *
from opsvsi.opsvsitest import *


class Runner(OpsVsiTest):

    def setupNet(self):
        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        system_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(system_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)
        self.s1 = self.net.switches[0]

    def setup(self):
        out = self.s1.ovscmd('ovs-vsctl list system')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                self.uuid = _id[1].strip()
            elif 'switch_version' in line:
                self.old_switch_version = line.split(':')[1].strip()

    def teardown(self):
        self.set_version(self.old_switch_version)
        self.net.stop()

    def set_name(self, name):
        pass

    def set_version(self, version):
        self.s1.ovscmd('ovs-vsctl -- set system ' + self.uuid
                  + ' switch_version=' + version)

    def run_show_version_result(self):
        return self.s1.cmdCLI('show version').split('\n')[1]

    def test_show_version(self, name, version):
        self.set_name(name)
        self.set_version(version)
        assert version == self.run_show_version_result()


class TestShowVersion:
    def setup(self):
        self.runner = Runner()
        self.runner.setup()

    def teardown(self):
        self.runner.teardown()
        del self.runner

    def test_normal_version(self):
        return self.runner.test_show_version("OpenSwitch", "1.1.1")

    def test_string_version(self):
        return self.runner.test_show_version("OpenSwitch", "string_version")
