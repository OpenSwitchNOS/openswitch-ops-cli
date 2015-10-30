#!/usr/bin/python

# Copyright (C) 2015, 2016 Hewlett Packard Enterprise Development LP
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

from opsvsi.docker import *
from opsvsi.opsvsitest import *


class AccessListCliTest(OpsVsiTest):

    def setupNet(self):
        self.net = Mininet(topo=SingleSwitchTopo(k=0, hopts=self.getHostOpts(),
                                                 sopts=self.getSwitchOpts()),
                           switch=VsiOpenSwitch,
                           host=OpsVsiHost,
                           link=OpsVsiLink,
                           controller=None,
                           build=True)

    def test_access_list_create_read_delete(self):
        '''
            Test access-list create/read/delete
        '''
        info('\n########## access-list create/read/delete ##########\n')
        s1 = self.net.switches[0]

        # Enter config context (note 'show' commands prefixed with 'do' below)
        ret = s1.cmdCLI("configure terminal")

        # Enter ACL context (create/update)
        ret = s1.cmdCLI("access-list ip test1")
        # Verify that prompt changes into config-acl context
        assert 'config-acl' in ret, 'Change into ACL context'
        # Exit ACL context
        ret = s1.cmdCLI("exit")
        # Verify that prompt changes into config context
        assert 'acl' not in ret, 'Change out of ACL context'
        # Verify that new ACL shows up
        ret = s1.cmdCLI("do show access-list")
        assert 'Type Name' in ret, 'Header line 1 printed'
        assert 'Seq Action  Proto' in ret, 'Header line 2 printed'
        assert 'Source IP          Port(s)' in ret, 'Header line 3 printed'
        assert 'Destination IP     Port(s)' in ret, 'Header line 4 printed'
        assert 'ip   test1' in ret, 'Created ACL present in show access-list'
        # Verify that new ACL config shows up
        ret = s1.cmdCLI("do show running-config")
        assert 'access-list ip test1' in ret, \
                'Created ACL present in show running-config'

        # Delete ACL
        ret = s1.cmdCLI("no access-list ip test1")
        # Verify delete worked
        ret = s1.cmdCLI("do show running-config")
        assert 'access-list ip test1' not in ret, 'Deleted ACL not present'

        # Return to enable mode
        s1.cmdCLI("exit")

        return True

    def test_access_list_entry_create_read_update_delete(self):
        '''
            Test access-list entry create/read/update/delete
        '''
        info('\n########## access-list entry create/read/update/delete ##########\n')
        s1 = self.net.switches[0]

        # Enter config context (note 'show' commands prefixed with 'do' below)
        s1.cmdCLI("configure terminal")

        # Enter ACL context
        ret = s1.cmdCLI("access-list ip test2")

        # Create simple ACE
        ret = s1.cmdCLI("10 permit any any any")
        # Verify that new ACE shows up
        ret = s1.cmdCLI("do show access-list")
        assert '10 permit' in ret, 'Created ACE present in show access-list'
        # Verify that new ACE config shows up
        ret = s1.cmdCLI("do show running-config")
        assert '10 permit any' in ret, \
                'Created ACE present in show running-config'

        # Update simple ACE to deny
        ret = s1.cmdCLI("10 deny any any any")
        # Verify that updated ACE shows up
        ret = s1.cmdCLI("do show access-list")
        assert 'deny' in ret, 'Updated ACE present in show access-list'
        # Verify that updated ACE config shows up
        ret = s1.cmdCLI("do show running-config")
        assert '10 deny any' in ret, \
                'Updated ACE present in show running-config'

        # Delete simple ACE
        ret = s1.cmdCLI("no 10")
        # Verify ACE delete updated ACL
        ret = s1.cmdCLI("do show access-list")
        assert 'deny ' not in ret, 'ACE no longer present in show access-list'
        # Verify ACE delete updated config
        ret = s1.cmdCLI("do show running-config")
        assert '10 deny any' not in ret, \
                'ACE seq no longer present in show running-config'

        # Clean up ACL
        ret = s1.cmdCLI("no access-list ip test2")
        # Verify ACL delete
        ret = s1.cmdCLI("do show access-list")
        assert 'test2' not in ret, 'ACL no longer present in show access-list'
        # Verify ACL delete from config
        ret = s1.cmdCLI("do show running-config")
        assert 'test2' not in ret, 'ACL no longer present in show access-list'

        # Return to enable mode
        s1.cmdCLI("exit")

        return True

    def test_access_list_apply_replace_unapply(self):
        '''
            Test access-list apply/replace/un-apply
        '''
        info('\n########## access-list apply/replace/un-apply ##########\n')
        s1 = self.net.switches[0]

        # Enter config context (note 'show' commands prefixed with 'do' below)
        s1.cmdCLI("configure terminal")

        # Create two simple ACLs
        ret = s1.cmdCLI("access-list ip test3")
        ret = s1.cmdCLI("10 permit any any any")
        ret = s1.cmdCLI("access-list ip test4")
        ret = s1.cmdCLI("10 deny any any any")

        # Enter interface context
        ret = s1.cmdCLI("interface 1")

        # Apply access-list
        ret = s1.cmdCLI("apply access-list ip test3 in")
        # Verify updated config
        ret = s1.cmdCLI("do show running-config")

        assert 'apply access-list ip test3 in' in ret, \
                'Applied ACL present in show running-config'

        # Apply different access-list (replace)
        ret = s1.cmdCLI("apply access-list ip test4 in")
        # Verify updated config
        ret = s1.cmdCLI("do show running-config")
        assert 'apply access-list ip test4 in' in ret, \
                'Replaced ACL present in show running-config'

        # Un-apply access-list
        ret = s1.cmdCLI("no apply access-list ip test4 in")
        # Verify updated config
        ret = s1.cmdCLI("do show running-config")
        assert 'apply access-list ip test4 in' not in ret, \
                'Applied ACL not present in show running-config'

        # Exit interface context
        ret = s1.cmdCLI("exit")

        # Clean up ACLs
        ret = s1.cmdCLI("no access-list ip test3")
        ret = s1.cmdCLI("no access-list ip test4")
        # Verify ACL delete
        ret = s1.cmdCLI("do show access-list")
        assert 'test3' not in ret, 'ACL no longer present in show access-list'
        assert 'test4' not in ret, 'ACL no longer present in show access-list'
        # Verify ACL delete from config (includes apply configs)
        ret = s1.cmdCLI("do show running-config")
        assert 'test3' not in ret, 'ACL no longer present in show access-list'
        assert 'test4' not in ret, 'ACL no longer present in show access-list'

        # Return to enable mode
        s1.cmdCLI("exit")

        return True

class Test_vtysh_access_list:

    def setup_class(cls):
        # Create a test topology
        Test_vtysh_access_list.test = AccessListCliTest()

    def teardown_class(cls):
        # Stop the Docker containers and mininet topology
        Test_vtysh_access_list.test.net.stop()

    def test_access_list_create_read_delete(self):
        if self.test.test_access_list_create_read_delete():
            info('''
########## access-list create/read/delete - SUCCESS! ##########
''')

    def test_access_list_entry_create_read_update_delete(self):
        if self.test.test_access_list_entry_create_read_update_delete():
            info('''
########## access-list entry create/read/update/delete - SUCCESS! ##########
''')

    def test_access_list_apply_replace_unapply(self):
        if self.test.test_access_list_apply_replace_unapply():
            info('''
########## access-list apply/replace/un-apply - SUCCESS! ##########
''')

    def __del__(self):
        del self.test
