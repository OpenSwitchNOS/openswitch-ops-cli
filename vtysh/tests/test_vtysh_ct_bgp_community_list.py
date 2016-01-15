
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

class bgp_communitylistCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        bgp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(bgp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_add_bgp_community_list(self):
         info("\n##########  Test to add bgp "
              "community list configuration ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip community-list test_name permit regular expression")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip community-list test_name permit regular expression"\
                 in line :
                community_created = True
         assert community_created == True, \
            'Test to add bgp community '\
            'list configuration failed '
         return True

    def test_add_bgp_extcommunity_list(self):
         info("\n##########  Test to add bgp "
              "extcommunity configurations ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip extcommunity-list testname permit regular expression")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname permit regular expression"\
                in line :
                community_created = True
         assert community_created == True, \
            'Test to add bgp '\
            'extcommunity configuration failed '
         return True

    def test_delete_bgp_community_list(self):
         info("\n##########  Test to delete bgp "
              "community list configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip community-list test_name permit regular expression")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip community-list test_name permit regular expression"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete bgp '\
            'community configuration failed '
         return True

    def test_add_bgp_extcommunity_deny_list(self):
         info("\n##########  Test to add bgp "
              "extcommunity list deny configurations ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip extcommunity-list testname1 deny regular expression")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname1 deny regular expression"\
                in line :
                community_created = True
         assert community_created == True, \
            'Test to add bgp '\
            'extcommunity configuration failed '
         return True

    def test_delete_bgp_extcommunity_deny_list(self):
         info("\n##########  Test to delete bgp "
              "extcommunity deny list configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip extcommunity-list testname1 deny regular expression")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname1 deny regular expression"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete bgp '\
            'community configuration failed '
         return True


    def test_delete_bgp_extcommunity_list(self):
         info("\n##########  Test to delete bgp "
              "extcommunity configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip extcommunity-list test_name permit"\
                   " regular expression")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname permit regular expression"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete bgp '\
            'extcommunity configuration failed '
         return True


class Test_vtysh_bgp_community:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_bgp_community.test = bgp_communitylistCLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_bgp_community.test.net.stop()

    def test_add_bgp_community_list_cli(self):
        if self.test.test_add_bgp_community_list() :
           info("\n###  Test to add bgp "
                "community list configuration - SUCCESS! ###\n")

    def test_add_bgp_extcommunity_list_cli(self):
        if self.test.test_add_bgp_extcommunity_list() :
           info("\n###  Test to add bgp "
                "extcommunity configuration - SUCCESS! ###\n")

    def test_delete_bgp_community_list_cli(self):
        if self.test.test_delete_bgp_community_list() :
           info("\n###  Test to delete bgp "
                "community configuration - SUCCESS! ###\n")

    def test_add_bgp_extcommunity_list_deny_cli(self):
        if self.test.test_add_bgp_extcommunity_deny_list() :
           info("\n###  Test to add bgp "
                "extcommunity deny configuration - SUCCESS! ###\n")

    def test_delete_bgp_extcommunity_list_deny_cli(self):
        if self.test.test_delete_bgp_extcommunity_deny_list():
           info("\n###  Test to delete bgp "
                "extcommunity deny configuration - SUCCESS! ###\n")

    def test_delete_bgp_extcommunity_list_cli(self):
        if self.test.test_delete_bgp_extcommunity_list() :
           info("\n###  Test to delete bgp "
                "extcommunity configuration - SUCCESS! ###\n")
