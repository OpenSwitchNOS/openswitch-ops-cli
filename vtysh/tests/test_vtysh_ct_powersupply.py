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


class PlatformPSUTests(OpsVsiTest):

    uuid = ''

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        powersupply_topo = SingleSwitchTopo(
            k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(powersupply_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def initPSUTable(self):

        # Add dummy data for PSU in subsystem and PSU table for simulation.
        # Assume there would be only one entry in subsystem table

        s1 = self.net.switches[0]
        out = s1.ovscmd('ovs-vsctl list subsystem')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                PlatformPSUTests.uuid = _id[1].strip()
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformPSUTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base status=ok'
                          )
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformPSUTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base1 status=fault_input'
                          )
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformPSUTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base2 status=fault_output'
                          )
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformPSUTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base3 status=fault_absent'
                          )
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformPSUTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base4 status=unknown'
                          )

    def deinitPSUTable(self):
        s1 = self.net.switches[0]

        # Delete dummy data from subsystem and PSU table to avoid
        # clash with other CT scripts.

        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformPSUTests.uuid
                  + ' power_supplies')

    def showSystemPSUTest(self):

        # Test to verify show system command

        s1 = self.net.switches[0]
        info('''
########## Test to verify \'show system power-supply\' command ##########
''')
        system_psu_config_present = False
        out = s1.cmdCLI('show system power-supply')
        lines = out.split('\n')
        for line in lines:
            if 'Psu_base' in line:
                if 'ok' in line:
                    system_psu_config_present = True
                    break
                else:
                    system_psu_config_present = False
                    break
            if 'Psu_base1' in line:
                if 'Input Fault' in line:
                    system_psu_config_present = True
                    break
                else:
                    system_psu_config_present = False
                    break
            if 'Psu_base2' in line:
                if 'Output Fault' in line:
                    system_psu_config_present = True
                    break
                else:
                    system_psu_config_present = False
                    break
            if 'Psu_base3' in line:
                if 'Absent' in line:
                    system_psu_config_present = True
                    break
                else:
                    system_psu_config_present = False
                    break
            if 'Psu_base4' in line:
                if 'Unknown' in line:
                    system_psu_config_present = True
                    break
                else:
                    system_psu_config_present = False
                    break
        assert system_psu_config_present is True, \
            'Test to verify \'show system power-supply\' command - FAILED!'
        return True


class Test_psu:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):

        # Initialize the PSU table with dummy value

        Test_psu.test = PlatformPSUTests()
        Test_psu.test.initPSUTable()

    def teardown_class(cls):

        # Delete Dummy data to avoid clash with other test scripts

        Test_psu.test.deinitPSUTable()

        # Stop the Docker containers, and
        # mininet topology

        Test_psu.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test

    # show system test.

    def test_show_system_psu_command(self):
        if self.test.showSystemPSUTest():
            info('''
###### Test to verify \'show system power-supply\' command - SUCCESS! ######
''')
