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


class TemperatureSystemTests(OpsVsiTest):

    uuid = ''

    def setupNet(self):

        # if you override this function, make sure to
        # either pass getNodeOpts() into hopts/sopts of the topology that
        # you build or into addHost/addSwitch calls

        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        tempsensor_topo = SingleSwitchTopo(
            k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(tempsensor_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def initTemp_sensorTable(self):

        # Add dummy data for fans in subsystem and fan table for simulation.
        # Assume there would be only one entry in subsystem table

        s1 = self.net.switches[0]
        print '\n'
        out = s1.ovscmd('ovs-vsctl list subsystem')
        lines = out.split('\n')
        for line in lines:
            if '_uuid' in line:
                _id = line.split(':')
                TemperatureSystemTests.uuid = _id[1].strip()
                out = s1.ovscmd('/usr/bin/ovs-vsctl -- set Subsystem ' +
                                TemperatureSystemTests.uuid +
                                ' temp_sensors=@fan1 -- --id=@fan1 ' +
                                ' create Temp_sensor name=base-1 ' +
                                ' location=Faceplate_side_of_switch_chip_U16 '
                                + ' status=normal fan-state=normal min=0 '
                                + ' max=21000 temperature=20500'
                                )

    def deinitTemp_sensorTable(self):
        s1 = self.net.switches[0]

        # Delete dummy data from subsystem and led table to
        # avoid clash with other CT scripts.

        s1.ovscmd('ovs-vsctl clear subsystem '
                  + TemperatureSystemTests.uuid + ' temp_sensors')

    def showSystemTemperatureTest(self):

        # Test to verify show system command

        s1 = self.net.switches[0]
        counter = 0
        temperature_config_present = False
        info('''
##########  Test to verify \'show system temperature\' command ##########
''')
        out = s1.cmdCLI('show system temperature detail')
        lines = out.split('\n')
        for line in lines:
            if 'base-1' in line:
                counter += 1
            if 'Faceplate_side_of_switch_chip_U16' in line:
                counter += 1
            if 'normal' in line:
                counter += 1
        if counter == 3:
            temperature_config_present = True
        assert temperature_config_present is True, \
            'Test to verify \'show system temperature\' command - FAILED!'
        return True


class Test_sys:

    def setup(self):
        pass

    def teardown(self):
        pass

    def setup_class(cls):

        # Initialize the led table with dummy value

        Test_sys.test = TemperatureSystemTests()
        Test_sys.test.initTemp_sensorTable()

    def teardown_class(cls):

        # Delete Dummy data to avoid clash with other test scripts

        Test_sys.test.deinitTemp_sensorTable()

        # Stop the Docker containers, and
        # mininet topology

        Test_sys.test.net.stop()

    def setup_method(self, method):
        pass

    def teardown_method(self, method):
        pass

    def __del__(self):
        del self.test

    # show system fan test.

    def test_show_system_temperature_command(self):
        if self.test.showSystemTemperatureTest():
            info('''
######  Test to verify \'show system temperature\' command - SUCCESS! ######
''')
