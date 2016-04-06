
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

class bgp_prefixCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        bgp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(bgp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_add_bgp_ip_prefix_list_permit_prefix(self):
         info("\n##########  Test to add ip prefix-list permit prefix "
              "configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#1 seq 101 permit 10.0.0.1/8")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#1 seq 101 permit 10.0.0.1/8" in line :
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list permit prefix'\
            'configuration failed '
         return True

    def test_validate_show_ip_prefix_list(self):
         info("\n##########  Test to validate show ip prefix-list"
              "configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("show ip prefix-list");
         lines = dump.split('\n')
         i = 0
         for line in lines:
            if "ip prefix-list test#1: 1 entries" in lines[i] and \
                 "seq 101 permit 10.0.0.1/8" in lines[i+1] :
                plist_created = True
            i = i + 1
         assert plist_created == True, \
            'Test to validate show ip prefix-list'\
            'configuration failed '
         return True

    def test_validate_show_ip_prefix_list_seq(self):
         info("\n##########  Test to validate show ip prefix-list"
              " seq configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("show ip prefix-list test#1 seq 101");
         lines = dump.split('\n')
         for line in lines:
            if "seq 101 permit 10.0.0.1/8" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to validate show ip prefix-list seq'\
            'configuration failed '
         return True

    def test_validate_show_ip_prefix_list_detail(self):
         info("\n##########  Test to validate show ip prefix-list"
              " detail configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("show ip prefix-list detail test#1");
         lines = dump.split('\n')
         i = 0
         for line in lines:
            if "ip prefix-list test#1:" in lines[i] and \
                 "count: 1, sequences: 101 - 101" in lines[i+1] and \
                 "seq 101 permit 10.0.0.1/8" in lines[i+2] :
                plist_created = True
            i = i + 1
         assert plist_created == True, \
            'Test to validate show ip prefix-list'\
            ' detail configuration failed '
         return True

    def test_validate_show_ip_prefix_list_summary(self):
         info("\n##########  Test to validate show ip prefix-list"
              " summary configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         dump = s1.cmdCLI("show ip prefix-list summary test#1");
         lines = dump.split('\n')
         i = 0
         for line in lines:
            if "ip prefix-list test#1:" in lines[i] and \
                 "count: 1, sequences: 101 - 101" in lines[i+1]:
                plist_created = True
            i = i + 1
         assert plist_created == True, \
            'Test to validate show ip prefix-list'\
            ' summary configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_permit_prefix(self):
         info("\n##########  Test to delete ip prefix-list permit prefix "
              "configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#1 seq 101 permit 10.0.0.1/8")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip prefix-list test#1 seq 101 permit 10.0.0.1/8" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list permit prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_permit_prefix(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#1")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#1" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'configuration failed '
         return True



    def test_add_bgp_ip_prefix_list_deny_prefix(self):
         info("\n##########  Test to add ip prefix-list deny prefix "
              "configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#2 seq 102 deny 10.0.0.2/8")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
             if "ip prefix-list test#2 seq 102 deny 10.0.0.2/8" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list deny prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_deny_prefix(self):
         info("\n##########  Test to delete ip prefix-list deny prefix "
              "configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#2 seq 102 deny 10.0.0.2/8")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#2 seq 102 deny 10.0.0.2/8" in line:
                    plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list deny prefix'\
            'configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_deny_prefix(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD deny prefix configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#2")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#2" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'deny prefix configuration failed '
         return True

    def test_add_bgp_ip_prefix_list_permit_prefix_ge(self):
         info("\n##########  Test to add ip prefix-list permit prefix "
              "ge configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#3 seq 103 permit 10.0.0.3/8 ge 10")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#3 seq 103 permit 10.0.0.3/8 ge 10" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list permit prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_permit_prefix_ge(self):
         info("\n##########  Test to delete ip prefix-list permit prefix "
              "ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#3 seq 103 permit 10.0.0.3/8 ge 10")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#3 seq 103 permit 10.0.0.3/8 ge 10" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list permit prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_permit_ge(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#3")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#3" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'permit prefix ge configuration failed '
         return True


    def test_add_bgp_ip_prefix_list_deny_prefix_ge(self):
         info("\n##########  Test to add ip prefix-list deny prefix "
              "ge configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#4 seq 104 deny 10.0.0.4/8 ge 11")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#4 seq 104 deny 10.0.0.4/8 ge 11" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list deny prefix'\
            'ge configuration failed '
         return True


    def test_delete_bgp_ip_prefix_list_deny_prefix_ge(self):
         info("\n##########  Test to delete ip prefix-list deny prefix "
              "ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#4 seq 104 deny 10.0.0.4/8 ge 11")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#4 seq 104 deny 10.0.0.4/8 ge 11" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list deny prefix'\
            'ge configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_deny_ge(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix ge configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#4")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#4" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'deny prefix ge configuration failed '
         return True

    def test_add_bgp_ip_prefix_list_permit_prefix_le(self):
         info("\n##########  Test to add ip prefix-list permit prefix "
              "le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#5 seq 105 permit 10.0.0.3/8 le 10")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5 seq 105 permit 10.0.0.3/8 le 10" in line:
                    plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list permit prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_permit_prefix_le(self):
         info("\n##########  Test to delete ip prefix-list permit prefix "
              "le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#5 seq 105 permit 10.0.0.3/8 le 10")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5 seq 105 permit 10.0.0.3/8 le 10" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list permit prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_permit_le(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#5")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'permit prefix le configuration failed '
         return True


    def test_add_bgp_ip_prefix_list_deny_prefix_le(self):
         info("\n##########  Test to add ip prefix-list deny prefix "
              "le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#6 seq 106 deny 10.0.0.4/8 le 11")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6 seq 106 deny 10.0.0.4/8 le 11" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list deny prefix'\
            'le configuration failed '
         return True


    def test_delete_bgp_ip_prefix_list_deny_prefix_le(self):
         info("\n##########  Test to delete ip prefix-list deny prefix "
              "le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#6 seq 106 deny 10.0.0.4/8 le 11")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6 seq 106 deny 10.0.0.4/8 le 11" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list deny prefix'\
            'le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_deny_le(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD deny prefix le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#6")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'deny prefix le configuration failed '
         return True



    def test_add_bgp_ip_prefix_list_permit_prefix_ge_le(self):
         info("\n##########  Test to add ip prefix-list permit prefix "
              "ge le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#5 seq 105 permit 10.0.0.3/8 ge 10"
                   " le 20")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5 seq 105 permit 10.0.0.3/8 ge 10" in line:
                    plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list permit prefix '\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_permit_prefix_ge_le(self):
         info("\n##########  Test to delete ip prefix-list permit prefix "
              "ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#5 seq 105 permit 10.0.0.3/8 ge"
                   " 10 le 20")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5 seq 105 permit 10.0.0.3/8 ge 10 le 20" \
                in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list permit prefix'\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_permit_ge_le(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#5")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#5" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'permit prefix ge le configuration failed '
         return True


    def test_add_bgp_ip_prefix_list_deny_prefix_ge_le(self):
         info("\n##########  Test to add ip prefix-list deny prefix "
              "ge le configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test#6 seq 106 deny 10.0.0.4/8 ge 11 le 21")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6 seq 106 deny 10.0.0.4/8 ge 11 le 21"\
                in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list deny prefix'\
            'ge le configuration failed '
         return True


    def test_delete_bgp_ip_prefix_list_deny_prefix_ge_le(self):
         info("\n##########  Test to delete ip prefix-list deny prefix "
              "ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#6 seq 106 deny 10.0.0.4/8 ge 11 le 21")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6 seq 106 deny 10.0.0.4/8 ge 11 le 21"\
                in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list deny prefix'\
            'ge le configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_deny_ge_le(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix ge le configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test#6")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test#6" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'deny prefix ge le configuration failed '
         return True

    def test_add_bgp_ip_prefix_list_permit_prefix_any(self):
         info("\n##########  Test to add ip prefix-list permit prefix "
              "any configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test seq 105 permit any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test seq 105 permit any" in line:
                    plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list permit prefix '\
            'any configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_permit_prefix_any(self):
         info("\n##########  Test to delete ip prefix-list permit prefix "
              "any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test seq 105 permit any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test seq 105 permit any" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list permit prefix'\
            'any configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_permit_any(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD permit prefix any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'permit prefix test configuration failed '
         return True

    def test_add_bgp_ip_prefix_list_deny_prefix_any(self):
         info("\n##########  Test to add ip prefix-list deny prefix "
              "any configurations ##########\n")
         plist_created = False
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("ip prefix-list test seq 105 deny any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test seq 105 deny any" in line:
                plist_created = True
         assert plist_created == True, \
            'Test to add ip prefix-list deny prefix '\
            'any configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_deny_prefix_any(self):
         info("\n##########  Test to delete ip prefix-list deny prefix "
              "any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test seq 105 deny any")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test seq 105 deny any" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list deny prefix'\
            'any configuration failed '
         return True

    def test_delete_bgp_ip_prefix_list_word_deny_any(self):
         info("\n##########  Test to delete ip prefix-list permit "
              "WORD deny prefix any configurations ##########\n")
         plist_deleted = True
         s1 = self.net.switches[0]
         s1.cmdCLI("configure terminal")
         s1.cmdCLI("no ip prefix-list test")
         s1.cmdCLI("end")
         dump = s1.cmdCLI("show running-config")
         lines = dump.split('\n')
         for line in lines :
            if "ip prefix-list test" in line:
                plist_deleted = False
         assert plist_deleted == True, \
            'Test to delete ip prefix-list word'\
            'deny prefix test configuration failed '
         return True


@pytest.mark.skipif(True, reason="Disabling old tests")
class Test_vtysh_bgp_prefix:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_bgp_prefix.test = bgp_prefixCLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_bgp_prefix.test.net.stop()
    def test_add_bgp_ip_prefix_cli_permit_prefix(self):
        if self.test.test_add_bgp_ip_prefix_list_permit_prefix() :
           info("\n###  Test to add ip prefix-list permit prefix "
                "configuration - SUCCESS! ###\n")

    def test_validate_show_ip_prefix_list(self):
        if self.test.test_validate_show_ip_prefix_list() :
           info("\n###  Test to validate show ip prefix-list "
                "configuration - SUCCESS! ###\n")

    def test_validate_show_ip_prefix_list_seq(self):
        if self.test.test_validate_show_ip_prefix_list_seq() :
           info("\n###  Test to validate show ip prefix-list "
                " seq configuration - SUCCESS! ###\n")

    def test_validate_show_ip_prefix_list_detail(self):
        if self.test.test_validate_show_ip_prefix_list_detail() :
           info("\n###  Test to validate show ip prefix-list "
                " detail configuration - SUCCESS! ###\n")

    def test_validate_show_ip_prefix_list_summary(self):
        if self.test.test_validate_show_ip_prefix_list_summary() :
           info("\n###  Test to validate show ip prefix-list "
                " summary configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_prefix(self):
        if self.test.test_delete_bgp_ip_prefix_list_permit_prefix() :
           info("\n###  Test to delete ip prefix-list permit prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_permit_prefix(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_permit_prefix() :
           info("\n###  Test to delete ip prefix-list permit "
                "WORD permit prefix configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_deny_prefix(self):
	if self.test.test_add_bgp_ip_prefix_list_deny_prefix() :
           info("\n###  Test to add ip prefix-list deny prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_deny_prefix(self):
        if self.test.test_delete_bgp_ip_prefix_list_deny_prefix() :
           info("\n###  Test to delete ip prefix-list deny prefix "
                "configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_deny_prefix(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_deny_prefix() :
           info("\n###  Test to delete ip prefix-list  "
                "WORD deny prefix configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_permit_prefix_ge(self):
        if self.test.test_add_bgp_ip_prefix_list_permit_prefix_ge():
           info("\n###  Test to add ip prefix-list permit prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_prefix_ge(self):
        if self.test.test_delete_bgp_ip_prefix_list_permit_prefix_ge():
           info("\n###  Test to delete ip prefix-list permit prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_permit_ge(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_permit_ge() :
           info("\n###  Test to delete ip prefix-list permit "
                "WORD permit ge configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_deny_prefix_ge(self):
        if self.test.test_add_bgp_ip_prefix_list_deny_prefix_ge():
           info("\n###  Test to add ip prefix-list deny prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_deny_prefix_ge(self):
        if self.test.test_delete_bgp_ip_prefix_list_deny_prefix_ge():
           info("\n###  Test to delete ip prefix-list deny prefix "
                "ge configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_deny_ge(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_deny_ge() :
           info("\n###  Test to delete ip prefix-list permit "
                "WORD deny ge configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_permit_prefix_le(self):
        if self.test.test_add_bgp_ip_prefix_list_permit_prefix_le():
           info("\n###  Test to add ip prefix-list permit prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_prefix_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_permit_prefix_le():
           info("\n###  Test to delete ip prefix-list permit prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_permit_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_permit_le() :
           info("\n###  Test to delete ip prefix-list permit "
                "WORD permit le configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_deny_prefix_le(self):
        if self.test.test_add_bgp_ip_prefix_list_deny_prefix_le():
           info("\n###  Test to add ip prefix-list deny prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_deny_prefix_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_deny_prefix_le():
           info("\n###  Test to delete ip prefix-list deny prefix "
                "le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_deny_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_deny_le():
           info("\n###  Test to delete ip prefix-list deny prefix "
                "WORD deny le configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_permit_prefix_ge_le(self):
        if self.test.test_add_bgp_ip_prefix_list_permit_prefix_ge_le():
           info("\n###  Test to add ip prefix-list permit prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_prefix_ge_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_permit_prefix_ge_le():
           info("\n###  Test to delete ip prefix-list permit prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_permit_ge_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_permit_ge_le() :
           info("\n###  Test to delete ip prefix-list  "
                "WORD permit ge le configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_deny_prefix_ge_le(self):
        if self.test.test_add_bgp_ip_prefix_list_deny_prefix_ge_le():
           info("\n###  Test to add ip prefix-list deny prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_deny_prefix_ge_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_deny_prefix_ge_le():
           info("\n###  Test to delete ip prefix-list deny prefix "
                "ge le configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_deny_ge_le(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_deny_ge_le() :
           info("\n###  Test to delete ip prefix-list  "
                "WORD deny ge le configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_permit_prefix_any(self):
        if self.test.test_add_bgp_ip_prefix_list_permit_prefix_any():
           info("\n###  Test to add ip prefix-list permit prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_prefix_any(self):
        if self.test.test_delete_bgp_ip_prefix_list_permit_prefix_any():
           info("\n###  Test to delete ip prefix-list permit prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_permit_any(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_permit_any() :
           info("\n###  Test to delete ip prefix-list "
                "WORD permit any configuration - SUCCESS! ###\n")

    def test_add_bgp_ip_prefix_cli_permit_deny_any(self):
        if self.test.test_add_bgp_ip_prefix_list_deny_prefix_any():
           info("\n###  Test to add ip prefix-list deny prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_permit_deny_any(self):
        if self.test.test_delete_bgp_ip_prefix_list_deny_prefix_any():
           info("\n###  Test to delete ip prefix-list deny prefix "
                "any configuration - SUCCESS! ###\n")

    def test_delete_bgp_ip_prefix_cli_word_deny_any(self):
        if self.test.test_delete_bgp_ip_prefix_list_word_deny_any() :
           info("\n###  Test to delete ip prefix-list  "
                "WORD deny any configuration - SUCCESS! ###\n")
