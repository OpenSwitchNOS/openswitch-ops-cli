
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


from opsvsi.docker import *
from opsvsi.opsvsitest import *
import time

class bgp_redistributeCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        bgp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(bgp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)


    def test_add_bgp_redistribute_ospf(self):
         info("\n##########  Test to add bgp redistribute "
              "ospf configurations ##########\n")
         redist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("redistribute ospf")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute ospf" in line :
                redist_created = True
         assert redist_created == True, \
            'Test to add bgp redistribute '\
            'ospf configuration failed '
         return True


    def test_add_bgp_redistribute_connected(self):
         info("\n##########  Test to add bgp redistribute "
              "connected configurations ##########\n")
         redist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("redistribute connected")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute connected" in line :
                redist_created = True
         assert redist_created == True, \
            'Test to add bgp redistribute '\
            'connected configuration failed '
         return True

    def test_add_bgp_redistribute_static(self):
         info("\n##########  Test to add bgp redistribute "
              "static configurations ##########\n")
         redist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("redistribute static")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute static" in line :
                redist_created = True
         assert redist_created == True, \
            'Test to add bgp redistribute '\
            'static configuration failed '
         return True

    def test_add_bgp_redistribute_static_route_map(self):
         info("\n##########  Test to add bgp redistribute "
              "static route map configurations ##########\n")
         redist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("route-map rm1 permit 1")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("redistribute static route-map rm1")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute static route-map rm1" in line :
                redist_created = True
         assert redist_created == True, \
            'Test to add bgp redistribute '\
            'static route mapconfiguration failed '
         return True

    def test_delete_bgp_redistribute_static_route_map(self):
         info("\n##########  Test to delete bgp redistribute "
              "static route map configurations ##########\n")
         redist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("no redistribute static route-map rm1")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute static route-map rm1" in line :
                redist_deleted = False
         assert redist_deleted == True, \
            'Test to delete bgp redistribute '\
            'static route map configuration failed '
         return True

    def test_delete_bgp_redistribute_static(self):
         info("\n##########  Test to delete bgp redistribute "
              "static configurations ##########\n")
         redist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("no redistribute static")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute static" in line :
                redist_deleted = False
         assert redist_deleted == True, \
            'Test to delete bgp redistribute '\
            'static configuration failed '
         return True


    def test_delete_bgp_redistribute_ospf(self):
         info("\n##########  Test to delete bgp redistribute "
              "ospf configurations ##########\n")
         redist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("no redistribute ospf")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute ospf" in line :
                redist_deleted = False
         assert redist_deleted == True, \
            'Test to delete bgp redistribute '\
            'ospf configuration failed '
         return True


    def test_delete_bgp_redistribute_connected(self):
         info("\n##########  Test to delete bgp redistribute "
              "connected configurations ##########\n")
         redist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("router bgp 1")
         s1.cmdCLI("no redistribute connected")

         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "redistribute connected" in line :
                redist_deleted = False
         assert redist_deleted == True, \
            'Test to delete bgp redistribute '\
            'connected configuration failed '
         return True


@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_vtysh_bgp_redistribute:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_bgp_redistribute.test = bgp_redistributeCLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_bgp_redistribute.test.net.stop()


    def test_add_bgp_redistribute_connected(self):
        if self.test.test_add_bgp_redistribute_connected() :
           info("\n###  Test to add bgp redistribute "
                "connected configuration - SUCCESS! ###\n")

    def test_add_bgp_redistribute_ospf(self):
        if self.test.test_add_bgp_redistribute_ospf() :
           info("\n###  Test to add bgp redistribute "
                "ospf configuration - SUCCESS! ###\n")

    def test_add_bgp_redistribute_static(self):

        if self.test.test_add_bgp_redistribute_static() :
           info("\n###  Test to add bgp redistribute "
                "static configuration - SUCCESS! ###\n")

    def test_delete_bgp_redistribute_static(self):
        if self.test.test_delete_bgp_redistribute_static() :
           info("\n###  Test to delete bgp redistribute "
                "static configuration - SUCCESS! ###\n")

    def test_add_bgp_redistribute_static_route_map(self):
        if self.test.test_add_bgp_redistribute_static_route_map() :
           info("\n###  Test to add bgp redistribute "
                "static route map configuration - SUCCESS! ###\n")

    def test_delete_bgp_redistribute_static_route_map(self):
        if self.test.test_delete_bgp_redistribute_static_route_map() :
           info("\n###  Test to delete bgp redistribute "
                "static route map configuration - SUCCESS! ###\n")

    def test_delete_bgp_redistribute_ospf(self):
        if self.test.test_delete_bgp_redistribute_ospf() :
           info("\n###  Test to delete bgp redistribute "
                "ospf configuration - SUCCESS! ###\n")

    def test_delete_bgp_redistribute_connected(self):
        if self.test.test_delete_bgp_redistribute_connected() :
           info("\n###  Test to delete bgp redistribute "
                "connected configuration - SUCCESS! ###\n")
