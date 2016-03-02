
#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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
         info("\n##########  Test to add BGP "
              "community list configuration ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip community-list test_name permit _5000_")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip community-list test_name permit _5000_"\
                 in line :
                community_created = True
         assert community_created == True, \
            'Test to add BGP community '\
            'list configuration failed '
         return True

    def test_add_bgp_extcommunity_list(self):
         info("\n##########  Test to add BGP "
              "extcommunity configurations ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip extcommunity-list testname permit _1000_")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname permit _1000_"\
                in line :
                community_created = True
         assert community_created == True, \
            'Test to add BGP '\
            'extcommunity configuration failed '
         return True

    def test_validate_show_ip_community_list(self):
         info("\n##########  Test to validate show "
              "ip community list configuration ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("do show ip community-list")
         lines = dump.split('\n')
         i = 0
         for line in lines :
             if "ip community-list test_name" in lines[i]\
                 and "permit _5000_" in lines[i+1] :
                community_created = True
             i = i + 1
         assert community_created == True, \
            'Test to validate show ip community '\
            'list configuration failed '
         return True

    def test_validate_show_ip_extcommunity_list(self):
         info("\n##########  Test to validate show "
              "ip extcommunity list configuration ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("do show ip extcommunity-list")
         lines = dump.split('\n')
         i = 0
         for line in lines :
             if "ip extcommunity-list testname" in lines[i]\
                 and "permit _1000_" in lines[i+1] :
                community_created = True
             i = i + 1
         assert community_created == True, \
            'Test to validate show ip extcommunity '\
            'list configuration failed '
         return True

    def test_delete_bgp_community_list(self):
         info("\n##########  Test to delete BGP "
              "community list configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip community-list test_name permit _5000_")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip community-list test_name permit _5000_"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete BGP '\
            'community configuration failed '
         return True

    def test_add_bgp_extcommunity_deny_list(self):
         info("\n##########  Test to add BGP "
              "extcommunity list deny configurations ##########\n")
         community_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip extcommunity-list test_name1 deny 100")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list test_name1 deny 100"\
                in line :
                community_created = True
         assert community_created == True, \
            'Test to add BGP '\
            'extcommunity configuration failed '
         return True

    def test_delete_bgp_extcommunity_deny_list(self):
         info("\n##########  Test to delete BGP "
              "extcommunity deny list configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip extcommunity-list test_name1 deny 100")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list test_name1 deny 100"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete BGP '\
            'community configuration failed '
         return True


    def test_delete_bgp_extcommunity_list(self):
         info("\n##########  Test to delete BGP "
              "extcommunity configurations ##########\n")
         community_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip extcommunity-list test_name permit"\
                   " 200")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip extcommunity-list testname permit 200"\
                in line :
                community_deleted = False
         assert community_deleted == True, \
            'Test to delete BGP '\
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
           info("\n###  Test to add BGP "
                "community list configuration - SUCCESS! ###\n")

    def test_add_bgp_extcommunity_list_cli(self):
        if self.test.test_add_bgp_extcommunity_list() :
           info("\n###  Test to add BGP "
                "extcommunity configuration - SUCCESS! ###\n")

    def test_validate_show_ip_community_list(self):
        if self.test.test_validate_show_ip_community_list() :
           info("\n###  Test to validate show "
                "ip community list configuration - SUCCESS! ###\n")

    def test_validate_show_ip_extcommunity_list(self):
        if self.test.test_validate_show_ip_extcommunity_list() :
           info("\n###  Test to validate show "
                "ip extcommunity list configuration - SUCCESS! ###\n")

    def test_delete_bgp_community_list_cli(self):
        if self.test.test_delete_bgp_community_list() :
           info("\n###  Test to delete BGP "
                "community configuration - SUCCESS! ###\n")

    def test_add_bgp_extcommunity_list_deny_cli(self):
        if self.test.test_add_bgp_extcommunity_deny_list() :
           info("\n###  Test to add BGP "
                "extcommunity deny configuration - SUCCESS! ###\n")

    def test_delete_bgp_extcommunity_list_deny_cli(self):
        if self.test.test_delete_bgp_extcommunity_deny_list():
           info("\n###  Test to delete BGP "
                "extcommunity deny configuration - SUCCESS! ###\n")

    def test_delete_bgp_extcommunity_list_cli(self):
        if self.test.test_delete_bgp_extcommunity_list() :
           info("\n###  Test to delete BGP "
                "extcommunity configuration - SUCCESS! ###\n")
