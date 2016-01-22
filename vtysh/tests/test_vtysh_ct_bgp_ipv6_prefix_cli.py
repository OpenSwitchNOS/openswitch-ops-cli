
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

class bgp_prefixCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        bgp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(bgp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_add_bgp_ipv6_prefix_list_permit_prefix(self):
         info("\n##########  Test to add ipv6 prefix-list permit prefix "
              "configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#1 seq 101 permit"
                   " 2001:0DB8:0000::/48")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#1 seq 101 permit" \
                " 2001:0DB8:0000::/48" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ipv6 prefix-list permit prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_permit_prefix(self):
         info("\n##########  Test to delete ipv6 prefix-list permit prefix "
              "configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#1 seq 101"
                   " permit 2001:0DB8:0000::/48")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#1 seq 101"\
               " permit 2001:0DB8:0000::/48" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list permit prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_permit_prefix(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#1")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#1" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'configuration failed '
         return True



    def test_add_bgp_ipv6_prefix_list_deny_prefix(self):
         info("\n##########  Test to add ipv6 prefix-list deny prefix "
              "configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#2 seq 102 deny 2001:0DB8:0000::/48")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#2 seq 102 deny 2001:0DB8:0000::/48"\
                in line:
                plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list deny prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_deny_prefix(self):
         info("\n##########  Test to delete ipv6 prefix-list deny prefix "
              "configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#2 seq 102 deny"
                   " 2001:0DB8:0000::/48")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#2 seq 102 deny"\
               " 2001:0DB8:0000::/48" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list deny prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_deny_prefix(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD deny prefix configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#2")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#2" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'deny prefix configuration failed '
         return True

    def test_add_bgp_ipv6_prefix_list_permit_prefix_ge(self):
         info("\n##########  Test to add ipv6 prefix-list permit prefix "
              "ge configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#3 seq 103 permit"
                   " 2001:0DB8:0000::/48 ge 100")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#3 seq 103 permit"\
               " 2001:0DB8:0000::/48 ge 100" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ipv6 prefix-list permit prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_permit_prefix_ge(self):
         info("\n##########  Test to delete ipv6 prefix-list permit prefix "
              "ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#3 seq 103 permit"
                   " 2001:0DB8:0000::/48 ge 100")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#3 seq 103 permit"\
               " 2001:0DB8:0000::/48 ge 100" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list permit prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_permit_ge(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#3")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#3" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'permit prefix ge configuration failed '
         return True


    def test_add_bgp_ipv6_prefix_list_deny_prefix_ge(self):
         info("\n##########  Test to add ipv6 prefix-list deny prefix "
              "ge configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#4 seq 104 deny"
                   " 2001:0DB8:0000::/48 ge 110")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#4 seq 104 deny"\
               " 2001:0DB8:0000::/48 ge 110" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ipv6 prefix-list deny prefix'\
            'ge configuration failed '
         return True


    def test_delete_bgp_ipv6_prefix_list_deny_prefix_ge(self):
         info("\n##########  Test to delete ipv6 prefix-list deny prefix "
              "ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#4 seq 104 deny"
                   " 2001:0DB8:0000::/48 ge 110")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#4 seq 104 deny"\
               " 2001:0DB8:0000::/48 ge 110" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list deny prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_deny_ge(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#4")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#4" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'deny prefix ge configuration failed '
         return True

    def test_add_bgp_ipv6_prefix_list_permit_prefix_le(self):
         info("\n##########  Test to add ipv6 prefix-list permit prefix "
              "le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#5 seq 105 permit"
                   " 2001:0DB8:0000::/48 le 100")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5 seq 105 permit"\
               " 2001:0DB8:0000::/48 le 100" in line:
                plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list permit prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_permit_prefix_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit prefix "
              "le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#5 seq 105 permit"
                   " 2001:0DB8:0000::/48 le 100")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5 seq 105 permit"\
               " 2001:0DB8:0000::/48 le 100" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list permit prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_permit_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#5")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'permit prefix le configuration failed '
         return True


    def test_add_bgp_ipv6_prefix_list_deny_prefix_le(self):
         info("\n##########  Test to add ipv6 prefix-list deny prefix "
              "le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#6 seq 106 deny"
                   " 2001:0DB8:0000::/48 le 110")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#6 seq 106 deny"\
               " 2001:0DB8:0000::/48 le 110" in line:
                plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list deny prefix'\
            'le configuration failed '
         return True


    def test_delete_bgp_ipv6_prefix_list_deny_prefix_le(self):
         info("\n##########  Test to delete ipv6 prefix-list deny prefix "
              "le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#6 seq 106 deny"
                   " 2001:0DB8:0000::/48 le 110")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#6 seq 106 deny"\
               " 2001:0DB8:0000::/48 le 110" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list deny prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_deny_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD deny prefix le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#6")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#6" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'deny prefix le configuration failed '
         return True



    def test_add_bgp_ipv6_prefix_list_permit_prefix_ge_le(self):
         info("\n##########  Test to add ipv6 prefix-list permit prefix "
              "ge le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#5 seq 105 permit"
                   " 2001:0DB8:0000::/48 ge 110 le 120")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5 seq 105 permit"\
               " 2001:0DB8:0000::/48 ge 110 le 120" in line:
                plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list permit prefix '\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_permit_prefix_ge_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit prefix "
              "ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#5 seq 105 permit "
                   "2001:0DB8:0000::/48 ge 110 le 120")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5 seq 105 permit "\
               "2001:0DB8:0000::/48 ge 110 le 120" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list permit prefix'\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_permit_ge_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#5")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')

         for line in lines :
            if "ipv6 prefix-list test#5" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'permit prefix ge le configuration failed '
         return True


    def test_add_bgp_ipv6_prefix_list_deny_prefix_ge_le(self):
         info("\n##########  Test to add ipv6 prefix-list deny prefix "
              "ge le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test#6 seq 106 deny 2001:0DB8:0000::/48"
                   " ge 111 le 121")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#6 seq 106 deny 2001:0DB8:0000::/48"\
               " ge 111 le 121" in line:
                plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list deny prefix'\
            'ge le configuration failed '
         return True


    def test_delete_bgp_ipv6_prefix_list_deny_prefix_ge_le(self):
         info("\n##########  Test to delete ipv6 prefix-list deny prefix "
              "ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#6 seq 106 deny"
                   " 2001:0DB8:0000::/48 ge 111 le 121")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#6 seq 106 deny"\
               " 2001:0DB8:0000::/48 ge 111 le 121" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list deny prefix'\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_deny_ge_le(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test#6")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test#6" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'deny prefix ge le configuration failed '
         return True

    def test_add_bgp_ipv6_prefix_list_permit_prefix_any(self):
         info("\n##########  Test to add ipv6 prefix-list permit prefix "
              "any configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test seq 105 permit any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test seq 105 permit any" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ipv6 prefix-list permit prefix '\
            'any configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_permit_prefix_any(self):
         info("\n##########  Test to delete ipv6 prefix-list permit prefix "
              "any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test seq 105 permit any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test seq 105 permit any" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list permit prefix'\
            'any configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_word_permit_any(self):
         info("\n##########  Test to delete ipv6 prefix-list permit "
              "WORD permit prefix any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test" in line:
                plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'permit prefix test configuration failed '
         return True

    def test_add_bgp_ipv6_prefix_list_deny_prefix_any(self):
         info("\n##########  Test to add ipv6 prefix-list deny prefix "
              "any configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test_delete seq 105 deny any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ipv6 prefix-list test_delete seq 105 deny any" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ipv6 prefix-list deny prefix '\
            'any configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_deny_prefix_any(self):
         info("\n##########  Test to delete ipv6 prefix-list deny prefix "
              "any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test_delete seq 105 deny any")
         s1.cmdCLI("do show running-config")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ipv6 prefix-list test_delete seq 105 deny any" \
                in line :
                    plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list deny prefix'\
            ' any configuration failed '
         return True


    def test_add_bgp_ipv6_prefix_list_line(self):
         info("\n##########  Test to add ipv6 prefix-list "
              "WORD description line configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ipv6 prefix-list test description regular expression")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ipv6 prefix-list test description regular expression" \
                in line:
                    plist_created = True

         assert plist_created == True, \
            'Test to add ipv6 prefix-list word'\
            'description line configuration failed '
         return True

    def test_delete_bgp_ipv6_prefix_list_line(self):
         info("\n##########  Test to delete ipv6 prefix-list "
              "WORD description line configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ipv6 prefix-list test description regular expression")
         dump = s1.cmdCLI("do show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ipv6 prefix-list test description regular expression" \
                in line:
                    plist_deleted = False

         assert plist_deleted == True, \
            'Test to delete ipv6 prefix-list word'\
            'description line configuration failed '
         return True


class Test_vtysh_bgp_prefix:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_bgp_prefix.test = bgp_prefixCLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_bgp_prefix.test.net.stop()
    def test_add_bgp_ipv6_prefix_cli_permit_prefix(self):
        if self.test.test_add_bgp_ipv6_prefix_list_permit_prefix() :
           info("\n###  Test to add ipv6 prefix-list permit prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_prefix(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_permit_prefix() :
           info("\n###  Test to delete ipv6 prefix-list permit prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_permit_prefix(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_permit_prefix() :
           info("\n###  Test to delete ipv6 prefix-list permit "
                "WORD permit prefix configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_deny_prefix(self):
        if self.test.test_add_bgp_ipv6_prefix_list_deny_prefix() :
           info("\n###  Test to add ipv6 prefix-list deny prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_deny_prefix(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_deny_prefix() :
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_deny_prefix(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_deny_prefix() :
           info("\n###  Test to delete ipv6 prefix-list  "
                "WORD deny prefix configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_permit_prefix_ge(self):
        if self.test.test_add_bgp_ipv6_prefix_list_permit_prefix_ge():
           info("\n###  Test to add ipv6 prefix-list permit prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_prefix_ge(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_permit_prefix_ge():
           info("\n###  Test to delete ipv6 prefix-list permit prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_permit_ge(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_permit_ge() :
           info("\n###  Test to delete ipv6 prefix-list permit "
                "WORD permit ge configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_deny_prefix_ge(self):
        if self.test.test_add_bgp_ipv6_prefix_list_deny_prefix_ge():
           info("\n###  Test to add ipv6 prefix-list deny prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_deny_prefix_ge(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_deny_prefix_ge():
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_deny_ge(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_deny_ge() :
           info("\n###  Test to delete ipv6 prefix-list permit "
                "WORD deny ge configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_permit_prefix_le(self):
        if self.test.test_add_bgp_ipv6_prefix_list_permit_prefix_le():
           info("\n###  Test to add ipv6 prefix-list permit prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_prefix_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_permit_prefix_le():
           info("\n###  Test to delete ipv6 prefix-list permit prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_permit_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_permit_le() :
           info("\n###  Test to delete ipv6 prefix-list permit "
                "WORD permit le configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_deny_prefix_le(self):
        if self.test.test_add_bgp_ipv6_prefix_list_deny_prefix_le():
           info("\n###  Test to add ipv6 prefix-list deny prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_deny_prefix_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_deny_prefix_le():
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_deny_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_deny_le():
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "WORD deny le configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_permit_prefix_ge_le(self):
        if self.test.test_add_bgp_ipv6_prefix_list_permit_prefix_ge_le():
           info("\n###  Test to add ipv6 prefix-list permit prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_prefix_ge_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_permit_prefix_ge_le():
           info("\n###  Test to delete ipv6 prefix-list permit prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_permit_ge_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_permit_ge_le() :
           info("\n###  Test to delete ipv6 prefix-list  "
                "WORD permit ge le configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_deny_prefix_ge_le(self):
        if self.test.test_add_bgp_ipv6_prefix_list_deny_prefix_ge_le():
           info("\n###  Test to add ipv6 prefix-list deny prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_deny_prefix_ge_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_deny_prefix_ge_le():
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_deny_ge_le(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_deny_ge_le() :
           info("\n###  Test to delete ipv6 prefix-list  "
                "WORD deny ge le configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_permit_prefix_any(self):
        if self.test.test_add_bgp_ipv6_prefix_list_permit_prefix_any():
           info("\n###  Test to add ipv6 prefix-list permit prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_prefix_any(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_permit_prefix_any():
           info("\n###  Test to delete ipv6 prefix-list permit prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_permit_any(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_word_permit_any() :
           info("\n###  Test to delete ipv6 prefix-list "
                "WORD permit any configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_permit_deny_any(self):
        if self.test.test_add_bgp_ipv6_prefix_list_deny_prefix_any():
           info("\n###  Test to add ipv6 prefix-list deny prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_permit_deny_any(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_deny_prefix_any():
           info("\n###  Test to delete ipv6 prefix-list deny prefix "
                "any configuration - SUCCESS! ###\n")

    def test_add_bgp_ipv6_prefix_cli_word_line(self):
        if self.test.test_add_bgp_ipv6_prefix_list_line() :
           info("\n###  Test to add ipv6 prefix-list  "
                "WORD description line configuration - SUCCESS! ###\n")

    def test_delete_bgp_ipv6_prefix_cli_word_line(self):
        if self.test.test_delete_bgp_ipv6_prefix_list_line() :
           info("\n###  Test to delete ipv6 prefix-list  "
                "WORD description line configuration - SUCCESS! ###\n")
