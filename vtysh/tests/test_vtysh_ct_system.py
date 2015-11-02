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


class PlatformSystemTests(OpsVsiTest):

    uuid = ''

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

    def initSystemTable(self):

        # Add dummy data for LED in subsystem and led table for simulation.
        # Assume there would be only one entry in subsystem table

        s1 = self.net.switches[0]
        out = s1.ovscmd('ovs-vsctl list subsystem')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                PlatformSystemTests.uuid = _id[1].strip()
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformSystemTests.uuid
                          + ' leds=@led1 -- --id=@led1 create led '
                          + ' id=Led_base state=flashing status=ok')
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformSystemTests.uuid
                          + ' fans=@fan1 -- --id=@fan1 create fan '
                          + ' name=Fan_base speed=normal direction=f2b '
                          + ' rpm=9000 '
                          + ' status=ok')
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformSystemTests.uuid
                          + ' power_supplies=@psu1 -- --id=@psu1 create '
                          + ' Power_supply name=Psu_base status=ok')
                s1.ovscmd('ovs-vsctl -- set Subsystem '
                          + PlatformSystemTests.uuid
                          + ' temp_sensors=@tmp1 -- --id=@tmp1 create '
                          + ' Temp_sensor '
                          + ' name=Temp_base location=Chassis '
                          + ' temperature=20000 '
                          + ' status=normal fan_state=normal')

    def deinitSystemTable(self):
        s1 = self.net.switches[0]

        # Delete dummy data from subsystem and led table to avoid
        # clash with other CT scripts.

        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformSystemTests.uuid
                  + ' leds')
        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformSystemTests.uuid
                  + ' power_supplies')
        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformSystemTests.uuid
                  + ' temp_sensors')
        s1.ovscmd('ovs-vsctl clear subsystem ' + PlatformSystemTests.uuid
                  + ' fans')

    def showSystemTest(self):

        # Test to verify show system command

        s1 = self.net.switches[0]
        counter = 0
        info('''
##########  Test to verify \'show system\' command ##########
''')
        out = s1.cmdCLI('show system')
        lines = out.split('\n')
        for line in lines:
            if 'OpenSwitch Version' in line:
                counter += 1

            if 'Manufacturer' in line:
                out = s1.ovscmd('ovs-vsctl list Subsystem')
                lines1 = out.split('\n')
                for line1 in lines1:
                    if 'other_info' in line1:
                        lines1 = line1.split(',')
                        manufacturerValue = lines1[9].split('=')
                        manufacturerValue = manufacturerValue[1].strip()
                        manufacturer = manufacturerValue.strip()
                if manufacturer in line:
                    counter += 1

            if 'Interface Count' in line:
                out = s1.ovscmd('ovs-vsctl list Subsystem')
                lines1 = out.split('\n')
                for line1 in lines1:
                    if 'other_info' in line1:
                        lines1 = line1.split(',')
                        interfaceValue = lines1[5].split('=')
                        interfaceValue = interfaceValue[1].strip()
                        interfaceCount = interfaceValue.strip()
                if interfaceCount in line[:-32]:
                    counter += 1

            if 'Max Interface Speed' in line:
                out = s1.ovscmd('ovs-vsctl list Subsystem')
                lines1 = out.split('\n')
                for line1 in lines1:
                    if 'other_info' in line1:
                        lines1 = line1.split(',')
                        interfaceSpeedValue = lines1[12].split('=')
                        interfaceSpeedValue = \
                            interfaceSpeedValue[1].strip()
                        interfaceSpeed = interfaceSpeedValue.strip()
                if interfaceSpeed in line[42:]:
                    counter += 1

            if 'Led_base' in line:
                counter += 1

            if 'Fan_base' in line:
                counter += 1

            if 'Psu_base' in line:
                counter += 1

            if 'Temp_base' in line:
                counter += 1

        assert counter is 8, \
            'Test to verify \'show system\' command - FAILED!'
        return True


class Test_sys:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):

        # Initialize the led table with dummy value

        Test_sys.test = PlatformSystemTests()
        Test_sys.test.initSystemTable()

    # show system test.

    def test_show_system_command(self):
        if self.test.showSystemTest():
            info('''
##########  Test to verify \'show system\' command - SUCCESS! ##########
''')

    def teardown_class(cls):

        # Delete Dummy data to avoid clash with other test scripts

        Test_sys.test.deinitSystemTable()

        # Stop the Docker containers, and
        # mininet topology

        Test_sys.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test
