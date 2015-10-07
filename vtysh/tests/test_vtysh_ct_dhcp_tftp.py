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


class dhcp_tftpCLItest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        dhcp_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(dhcp_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_dhcp_tftp_add_range(self):
        info("\n##########  Test to add DHCP dynamic configurations"
             " ##########\n")

        range_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("range test-range start-ip-address 10.0.0.1 \
                   end-ip-address 10.255.255.254 \
                   netmask 255.0.0.0 match tags tag1,tag2,tag3 \
                   set tag test-tag broadcast 10.255.255.255 \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
                    and "10.0.0.1" in line \
                    and "10.255.255.254" in line and "255.0.0.0" in line \
                    and "tag1,tag2,tag3" in line and "test-tag" in line \
                    and "10.255.255.255" in line and "60" in line:
                range_created = True

        assert range_created == True, \
        'Test to add DHCP Dynamic configuration' \
        ' failed for "show dhcp-server" output' \
        ' -FAILED!'

        range_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
                and "10.0.0.1" in line \
                and "10.255.255.254" in line and "255.0.0.0" in line \
                and "tag1,tag2,tag3" in line and "test-tag" in line \
                and "10.255.255.255" in line:
                range_created = True

        assert range_created == True, \
        'Test to add DHCP Dynamic configuration' \
        ' failed for "show running" output' \
        ' -FAILED!'

        return True

    def test_dhcp_tftp_add_range_ipv6(self):
        info("\n##########  Test to add DHCP dynamic ipv6 configurations"
             "##########\n")

        range_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("range range-ipv6 start-ip-address 2001:cdba::3257:9652 \
                   end-ip-address 2001:cdba::3257:9655 \
                   prefix-len 64 \
                   match tags v6tag1,v6tag2,v6tag3 \
                   set tag v6-stag \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "range-ipv6" in line \
                    and "2001:cdba::3257:9652" in line \
                    and "2001:cdba::3257:9655" in line and "64" in line \
                    and "v6tag1,v6tag2,v6tag3" in line and "v6-stag" in line \
                    and "60" in line:
                range_created = True

        assert range_created == True, 'Test to add DHCP Dynamic ipv6 ' \
                                      'configuration failed for ' \
                                      '"show dhcp-server" output -FAILED!'

        range_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "range-ipv6" in line \
                and "2001:cdba::3257:9652" in line \
                and "2001:cdba::3257:9655" in line and "64" in line \
                and "v6tag1,v6tag2,v6tag3" in line and "v6-stag" in line:
                range_created = True

        assert range_created == True, 'Test to add DHCP Dynamic ipv6 '\
                                      'configuration failed for ' \
                                      '"show running-config" output- FAILED!'

        return True

    def test_dhcp_tftp_check_range_validation(self):
        info("\n########## Test to check range validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing  start ip address validation

        ret = s1.cmdCLI("range test-range start-ip-address 300.300.300.300 \
                         end-ip-address 192.168.0.254 \
                         netmask 255.255.255.0 match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 192.168.0.255 \
                         lease-duration 60")
        assert '% Unknown command.' in ret, \
            'Start IP address validation failed'
        info('\n### Start IP address validation passed ###\n')

        # Testing end ip address validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1  \
                         end-ip-address 300.300.300.300 \
                         netmask 255.255.255.0 match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 192.168.0.255 \
                         lease-duration 60")
        assert '% Unknown command.' in ret, 'End IP address validation failed'
        info('\n### End IP address validation passed ###\n')

        # Testing netmask validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1  \
                         end-ip-address 192.168.0.254 \
                         netmask 127.0.0.1 match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 192.168.0.255 \
                         lease-duration 60")
        assert '127.0.0.1 is invalid' in ret, 'Netmask validation failed'
        info('\n### Netmask validation passed ###\n')

        # Testing netmask ipv6 validation

        ret = \
            s1.cmdCLI("range testrange start-ip-address 2001:cdba::3257:9642 \
                       end-ip-address 2001:cdba::3257:9648 \
                       netmask 255.255.255.0 match tags tag1,tag2,tag3 \
                       set tag test-tag broadcast 192.168.0.255 \
                       lease-duration 60")
        assert 'Error : netmask configuration not allowed for IPv6' in ret, \
               'netmask ipv6 validation failed'
        info('\n### Netmask ipv6 validation passed ###\n')

        # Testing ip address range validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 10.0.0.1 \
                         netmask 255.255.255.0 match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 192.168.0.255 \
                         lease-duration 60")
        assert 'Invalid IP address range' in ret, 'ip address range'\
                                                  ' validation failed'
        info('\n### IP address range validation passed ###\n')

        # Testing broadcast address validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 192.168.0.254 \
                         netmask 255.255.255.0 match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 300.300.300.300 \
                         lease-duration 60")
        assert ' ' in ret, 'broadcast address validation failed'
        info('\n### Broadcast address validation passed ###\n')

        # Testing broadcast address ipv6 validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 192.168.0.254 \
                         netmask 255.255.255.0 \
                         match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 10.0.0.255 \
                         lease-duration 60")
        assert '10.0.0.255 is invalid' in ret, 'broadcast address'\
                                               ' ipv6 validation failed'
        info('\n### Broadcast address ipv6 validation passed ###\n')

        # Testing match tags validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 192.168.0.254 \
                         match tags tag1,this-tag-length-greater-than-15,tag3 \
                         set tag test-tag broadcast 192.168.0.255 \
                         lease-duration 60")
        assert 'this-tag-length-greater-than-15' in ret, 'match tags'\
                                                         ' validation failed'
        info('\n### Match tags validation passed ###\n')

        # Testing set tag validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 192.168.0.254 \
                         match tags tag1,tag2,tag3 \
                         set tag test-this-tag-greater-than-15 \
                         broadcast 192.168.0.1 \
                         lease-duration 60")
        assert 'test-this-tag-greater-than-15' in ret, 'set tag '\
            'validation failed'
        info('\n### Set tag validation passed ###\n')

        # Testing lease duration  validation

        ret = s1.cmdCLI("range test-range start-ip-address 192.168.0.1 \
                         end-ip-address 192.168.0.254 \
                         match tags tag1,tag2,tag3 \
                         set tag test-tag broadcast 192.168.0.1 \
                         lease-duration 120000")
        assert '% Unknown command.' in ret, 'set tag validation failed'
        info('\n### Lease duration validation passed ###\n')

        return True

    def test_dhcp_tftp_add_static(self):
        info("\n##########  Test to add DHCP static "
             "configuration ##########\n")

        static_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("static 192.168.0.2 \
                   match-mac-addresses aa:bb:cc:dd:ee:ff \
                   set tags tag4,tag5,tag6 \
                   match-client-id testid \
                   match-client-hostname testname \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "192.168.0.2" in line \
                    and "aa:bb:cc:dd:ee:ff" in line \
                    and "testid" in line \
                    and "tag4,tag5,tag6" in line \
                    and "testname" in line \
                    and "60" in line:
                static_created = True

        assert static_created == True, \
        'Test to add DHCP static configuration ' \
        'failed for "show dhcp-server" output ' \
        '-FAILED!'

        static_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "192.168.0.2" in line \
                and "aa:bb:cc:dd:ee:ff" in line \
                and "testid" in line \
                and "tag4,tag5,tag6" in line \
                and "testname" in line:
                static_created = True

        assert static_created == True, \
        'Test to add DHCP static configuration ' \
        'failed for "show running-config" output ' \
        '-FAILED!'
        return True

    def test_dhcp_tftp_add_static_ipv6(self):
        info("\n##########  Test to add DHCP static ipv6 configuration"
             "##########\n")

        static_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("static 2001:cdba::3257:9680 \
                   match-mac-addresses ae:bb:cc:dd:ee:ff \
                   set tags v6-stag1,v6-stag2,v6-stag3 \
                   match-client-id v6testid \
                   match-client-hostname v6testname \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "2001:cdba::3257:9680" in line \
                and "ae:bb:cc:dd:ee:ff" in line \
                and "v6testid" in line \
                and "v6-stag1,v6-stag2,v6-stag3" in line \
                and "v6testname" in line:
                static_created = True

        assert static_created == True, 'Test to add DHCP static ipv6 ' \
                                       'configuration failed for ' \
                                       '"show dhcp-server" output ' \
                                       '-FAILED!'

        static_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "2001:cdba::3257:9680" in line \
                and "ae:bb:cc:dd:ee:ff" in line \
                and "v6testid" in line \
                and "v6-stag1,v6-stag2,v6-stag3" in line \
                and "v6testname" in line:
                static_created = True

        assert static_created == True, 'Test to add DHCP static ipv6 ' \
                                       'configuration failed for ' \
                                       '"show running-config" output ' \
                                       '-FAILED!'
        return True

    def test_dhcp_tftp_check_static_validation(self):
        info("\n########## Test to check range validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing static ip address validation

        ret = s1.cmdCLI("static 300.300.300.300 \
                       match-mac-addresses aa:bb:cc:dd:ee:ff \
                       set tags tag1,tag2,tag3 \
                       match-client-id testid \
                       match-client-hostname testname \
                       lease-duration 60")
        assert '% Unknown command.' in ret, \
            'static ip address validation failed'
        info('\n### Static IP address validation passed ###\n')

        # Testing mac address validation

        ret = s1.cmdCLI("static 150.0.0.1 \
                       match-mac-addresses aabbccddeeff \
                       set tags tag1,tag2,tag3 \
                       match-client-id testid \
                       match-client-hostname testname \
                       lease-duration 60")
        assert 'aabbccddeeff is invalid' in ret, \
            'MAC address validation failed'
        info('\n### MAC address validation passed ###\n')

        # Testing set tags validation

        ret = s1.cmdCLI("static 150.0.0.1 \
                       match-mac-addresses aa:bb:cc:dd:ee:ff \
                       set tags \
                       t1-tag-this-tag-length-greater-than-15,tag2,tag3 \
                       match-client-id testid \
                       match-client-hostname testname \
                       lease-duration 60")
        assert 't1-tag-this-tag-length-greater-than-15 is invalid' in ret, \
               'set tags validation failed'
        info('\n### set tags  validation passed ###\n')

        # Testing client-id validation

        ret = s1.cmdCLI("static 150.0.0.1 \
                       match-mac-addresses aa:bb:cc:dd:ee:ff \
                       set tags tag1,tag2,tag3 \
                       match-client-id this-client-id-length-greater-than-15  \
                       match-client-hostname testname \
                       lease-duration 60")
        assert 'this-client-id-length-greater-than-15 is invalid' in ret, \
               'client-id validation failed'
        info('\n### Client-id  validation passed ###\n')

        # Testing client-hostname validation

        ret = s1.cmdCLI("static 150.0.0.1 \
                       match-mac-addresses aa:bb:cc:dd:ee:ff \
                       set tags tag1,tag2,tag3 \
                       match-client-id testid \
                       match-client-hostname this-hostname-greater-than-15 \
                       lease-duration 60")
        assert 'this-hostname-greater-than-15 is invalid' in ret, \
               'client-hostname validation failed'
        info('\n### Client-hostname  validation passed ###\n')

        # Testing lease duration validation

        return True

    def test_dhcp_tftp_add_option_name(self):
        info("\n##########  Test to add DHCP Option-name ##########\n")

        option_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("option \
                   set option-name opt-name \
                   option-value 192.168.0.1 \
                   match tags mtag1,mtag2,mtag3")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "opt-name" in line \
                    and "192.168.0.1" in line \
                    and "False" in line \
                    and "mtag1,mtag2,mtag3" in line:
                option_created = True

        assert option_created == True, 'Test to add DHCP Option-name ' \
                                       'failed for "show dhcp-server" ' \
                                       'output -FAILED!'

        option_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "opt-name" in line \
                and "192.168.0.1" in line \
                and "mtag1,mtag2,mtag3" in line :
                option_created = True

        assert option_created == True, 'Test to add DHCP Option-name ' \
                                       'failed for "show running-config" ' \
                                       'output -FAILED!'

        return True

    def test_dhcp_tftp_add_option_number(self):
        info("\n##########  Test to add DHCP Option-number ##########\n")

        option_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("option \
                   set option-number 3 \
                   option-value 192.168.0.3 \
                   match tags mtag4,mtag5,mtag6")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "3" in line \
                    and "192.168.0.3" in line \
                    and "False" in line \
                    and "mtag4,mtag5,mtag6" in line:
                option_created = True

        assert option_created == True, 'Test to add DHCP Option-number ' \
                                       'failed for "show dhcp-server" ' \
                                       'output -FAILED!'

        option_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "3" in line \
                and "192.168.0.3" in line \
                and "mtag4,mtag5,mtag6" in line :
                option_created = True

        assert option_created == True, 'Test to add DHCP Option-number ' \
                                       'failed for "show running-config" ' \
                                       'output -FAILED!'
        return True

    def test_dhcp_tftp_option_number_validation(self):
        info("\n########## Test to check DHCP Options using option number"
             "validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing option number validation

        ret = s1.cmdCLI("option \
                       set option-number 300 \
                       option-value 10.0.0.1 \
                       match tags tag1,tag2,tag3")
        assert '% Unknown command.' in ret, 'option-number validation failed'
        info('\n### option-number validation passed ###\n')

        # Testing match tags validation

        ret = s1.cmdCLI("option \
                       set option-number 3 \
                       option-value 10.0.0.1 \
                       match tags tag1,option-name-greater-than-15,tag3")
        assert 'option-name-greater-than-15 is invalid' in ret, \
               'match tag validation failed'
        info('\n### Match tag validation passed ###\n')

        return True

    def test_dhcp_tftp_option_name_validation(self):
        info("\n########## Test to check DHCP Options using option name "
             "validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing option-name validation

        ret = s1.cmdCLI("option \
                       set option-name set-option-name-greater-than-15 \
                       option-value 10.0.0.1 \
                       match tags tag1,tag2,tag3")
        assert 'set-option-name-greater-than-15 is invalid' in ret, \
               'option-name validation failed'
        info('\n### option-name validation passed ###\n')

        # Testing match tags validation

        ret = s1.cmdCLI("option \
                       set option-name Router \
                       option-value 10.0.0.1 \
                       match tags match-option-name-greater-than-15,tag2,tag3")
        assert 'match-option-name-greater-than-15 is invalid' in ret, \
               'match tag validation failed'
        info('\n### Match tag validation passed ###\n')

        return True

    def test_dhcp_tftp_add_match_number(self):
        info("\n##########  Test to add DHCP match number ##########\n")

        match_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("match \
                   set tag stag \
                   match-option-number 4 \
                   match-option-value 192.168.0.4")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "4" in line \
                    and "192.168.0.4" in line \
                    and "stag" in line:
                match_created = True

        assert match_created == True, 'Test to add DHCP match number ' \
                                      'failed for "show dhcp-server" ' \
                                      'output -FAILED!'

        match_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "4" in line \
                and "192.168.0.4" in line \
                and "stag" in line :
                match_created = True

        assert match_created == True, 'Test to add DHCP match number ' \
                                      'failed for "show running-config" ' \
                                      'output -FAILED!'
        return True

    def test_dhcp_tftp_match_number_validation(self):
        info("\n########## Test to check DHCP Match using option number "
             "validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing set tag validation
        ret = s1.cmdCLI("match \
                       set tag name-greater-than-15 \
                       match-option-number 3 \
                       match-option-value 10.0.0.1")

        assert 'name-greater-than-15 is invalid' in ret, \
               'match set tag validation failed'
        info('\n### Match set tag  validation passed ###i\n')

        # Testing option number validation
        ret = s1.cmdCLI("match \
                       set tag test-tag \
                       match-option-number 300 \
                       match-option-value 10.0.0.1")
        assert '% Unknown command.' in ret, \
               'match option number validation failed'
        info('\n### Match option number validation passed ###\n')

        return True

    def test_dhcp_tftp_add_match_name(self):
        info("\n##########  Test to add DHCP match name ##########\n")

        match_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("match \
                   set tag temp-mtag \
                   match-option-name temp-mname \
                   match-option-value 192.168.0.5")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "temp-mname" in line \
                    and "192.168.0.5" in line \
                    and "temp-mtag" in line:
                match_created = True

        assert match_created == True, 'Test to add DHCP match number ' \
                                      'failed for "show dhcp-server" ' \
                                      'output -FAILED!'

        match_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "temp-mname" in line \
                and "192.168.0.5" in line \
                and "temp-mtag" in line :
                match_created = True

        assert match_created == True, 'Test to add DHCP match number ' \
                                      'failed for "show running-config" ' \
                                      'output -FAILED!'

        return True

    def test_dhcp_tftp_match_name_validation(self):
        info("\n########## Test to check DHCP Match using option name "
             "validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing set tag validation
        ret = s1.cmdCLI("match \
                       set tag tag-greater-than-15 \
                       match-option-name tempname \
                       match-option-value 10.0.0.1")
        assert 'tag-greater-than-15 is invalid' in ret, \
               'match set tag validation failed'
        info('\n### match set tag  validation passed ###\n')

        # Testing option number validation
        ret = s1.cmdCLI("match \
                       set tag test-tag \
                       match-option-name tag-name-greater-than-15 \
                       match-option-value 10.0.0.1")
        assert 'tag-name-greater-than-15 is invalid' in ret, \
               'match option name validation failed'
        info('\n### Match option name validation passed ###\n')

        return True

    def test_dhcp_tftp_add_boot(self):
        info("\n##########  Test to add DHCP bootp ##########\n")

        boot_created = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("boot \
                   set file /tmp/testfile \
                   match tag boottag")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "/tmp/testfile" in line \
                    and "boottag" in line:
                boot_created = True

        assert boot_created == True, 'Test to add DHCP bootp ' \
                                     'failed for "show dhcp-server" ' \
                                     'output -FAILED!'

        boot_created = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "/tmp/testfile" in line \
                and "boottag" in line :
                boot_created = True

        assert boot_created == True, 'Test to add DHCP bootp ' \
                                     'failed for "show running-config" ' \
                                     'output -FAILED!'

        return True

    def test_dhcp_tftp_boot_validation(self):
        info("\n########## Test to check DHCP Bootp validation ##########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")

        # Testing set tag validation
        ret = s1.cmdCLI("boot \
                       set file /tmp/tmpfile \
                       match tag boot-tag-name-greater-than-15")
        assert 'boot-tag-name-greater-than-15 is invalid' in ret, \
               'Bootp match tag validation failed'
        info('\n### Bootp tag validation passed ###\n')

        return True

    def test_tftp_server_enable(self):
        info("\n##########  Test to enable tftp server ##########\n")

        tftp_enabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("tftp-server")
        s1.cmdCLI("enable")

        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server : Enabled" in line:
                tftp_enabled = True

        assert tftp_enabled == True, 'Test to enable tftp server ' \
                                     'failed for "show tftp-server" ' \
                                     'output -FAILED!'

        tftp_server_present = False
        prev_line = 0
        enable_present = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        i = 0
        for i,line in enumerate(lines):
            if "tftp-server" in line :
                tftp_server_present = True
                prev_line = i
            elif (prev_line + 1 == i) and (tftp_server_present == True):
                 if "enable" in line:
                    enable_present = True

        assert enable_present == True, 'Test to enable tftp server ' \
                                       'failed for "show running-config" ' \
                                       'output -FAILED!'

        return True

    def test_tftp_secure_enable(self):
        info("\n##########  Test to enable tftp server ##########\n")

        tftp_secure_enabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("tftp-server")
        s1.cmdCLI("secure-mode")

        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server secure mode : Enabled" in line:
                tftp_secure_enabled = True

        assert tftp_secure_enabled == True, 'Test to enable tftp server ' \
                                            'secure  mode failed for ' \
                                            '"show tftp-server" output ' \
                                            '-FAILED!'

        tftp_server_present = False
        prev_line = 0
        prev_line2 = 0
        prev_line3 = 0
        enable_present = False
        secure_mode_present = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for i,line in enumerate(lines):
            if "tftp-server" in line :
                tftp_server_present = True
                prev_line = i
            elif (prev_line + 1 == i) and (tftp_server_present == True):
                 if "enable" in line:
                    enable_present = True
                    prev_line2 = i
            elif (prev_line2 + 1 == i) and (tftp_server_present == True) \
                and (enable_present == True):
                if "secure-mode" in line:
                    secure_mode_present = True

        assert secure_mode_present == True, 'Test to enable tftp server ' \
                                            'secure  mode failed for ' \
                                            '"show running-config" output ' \
                                            '-FAILED!'

        return True

    def test_tftp_server_add_path(self):
        info("\n##########  Test to add tftp path ##########\n")

        tftp_path = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("tftp-server")
        s1.cmdCLI("path /etc/testfile")

        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server file path : /etc/testfile" in line:
                tftp_path = True

        assert tftp_path == True, 'Test to add tftp path ' \
                                  'failed for "show tftp-server" ' \
                                  'output -FAILED!'

        tftp_server_present = False
        prev_line = 0
        prev_line2 = 0
        prev_line3 = 0
        enable_present = False
        secure_mode_present = False
        tftp_path_present = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for i,line in enumerate(lines):
            if "tftp-server" in line :
                tftp_server_present = True
                prev_line = i
            elif (prev_line + 1 == i) and (tftp_server_present == True):
                 if "enable" in line:
                    enable_present = True
                    prev_line2 = i
            elif (prev_line2 + 1 == i) and (tftp_server_present == True) \
                and (enable_present == True):
                if "secure-mode" in line:
                    secure_mode_present = True
                    prev_line3 = i
            elif (prev_line3 + 1 == i) and (tftp_server_present == True) \
                and (enable_present == True) \
                and (secure_mode_present == True):
                if "path /etc/testfile" in line:
                    tftp_path_present = True

        assert tftp_path_present == True, 'Test to add tftp path ' \
                                          'failed for "show running-config" ' \
                                          'output -FAILED!'

        return True

    def test_dhcp_server_show(self):
        info("\n##########  Test to show dhcp server configuration  "\
             "##########\n")

        range_present = False
        static_present = False
        option_name = False
        option_number = False
        match_created = False
        boot_created = False
        show_success = False
        match_number = False

        s1 = self.net.switches[0]
        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
               and "10.0.0.1" in line \
               and "10.255.255.254" in line and "255.0.0.0" in line \
               and "tag1,tag2,tag3" in line and "test-tag" in line \
               and "10.255.255.255" in line and "60" in line:
                range_present = True

            if "192.168.0.2" in line \
               and "aa:bb:cc:dd:ee:ff" in line \
               and "testid" in line \
               and "tag4,tag5,tag6" in line \
               and "testname" in line \
               and "60" in line:
                static_present = True

            if "opt-name" in line \
               and "192.168.0.1" in line \
               and "False" in line \
               and "mtag1,mtag2,mtag3" in line:
                option_name = True

            if "3" in line \
               and "192.168.0.3" in line \
               and "False" in line \
               and "mtag4,mtag5,mtag6" in line:
                option_number = True

            if "4" in line \
               and "192.168.0.4" in line \
               and "stag" in line:
                match_number = True

            if "temp-mname" in line \
               and "192.168.0.5" in line \
               and "temp-mtag" in line:
                match_created = True

            if "/tmp/testfile" in line \
               and "boottag" in line:
                boot_created = True

        if range_present is True \
           and static_present is True \
           and option_name is True \
           and option_number is True \
           and match_number is True \
           and match_created is True \
           and boot_created is True:
               show_success = True

        assert show_success == True, 'Test to show dhcp server ' \
                                     'server configuration failed ' \
                                     'for "show dhcp-server" output ' \
                                     '-FAILED'

        range_present = False
        static_present = False
        option_name = False
        option_number = False
        match_created = False
        boot_created = False
        show_success = False
        match_number = False

        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
               and "10.0.0.1" in line \
               and "10.255.255.254" in line and "255.0.0.0" in line \
               and "tag1,tag2,tag3" in line and "test-tag" in line \
               and "10.255.255.255" in line:
                range_present = True

            if "192.168.0.2" in line \
               and "aa:bb:cc:dd:ee:ff" in line \
               and "testid" in line \
               and "tag4,tag5,tag6" in line \
               and "testname" in line:
                static_present = True

            if "opt-name" in line \
               and "192.168.0.1" in line \
               and "mtag1,mtag2,mtag3" in line :
                option_name = True

            if "3" in line \
               and "192.168.0.3" in line \
               and "mtag4,mtag5,mtag6" in line :
                option_number = True

            if "4" in line \
               and "192.168.0.4" in line \
               and "stag" in line :
                match_number = True

            if "temp-mname" in line \
               and "192.168.0.5" in line \
               and "temp-mtag" in line :
                match_created = True

            if "/tmp/testfile" in line \
               and "boottag" in line :
               boot_created = True

        if range_present is True \
           and static_present is True \
           and option_name is True \
           and option_number is True \
           and match_number is True \
           and match_created is True \
           and boot_created is True :
               show_success = True

        assert show_success == True, 'Test to show dhcp server ' \
                                     'configuration failed for ' \
                                     '"show running-config" output ' \
                                     '-FAILED!'

        return True

    def test_tftp_server_show(self):
        info("\n##########  Test to show tftp server configuration " \
            "##########\n")

        tftp_server = False
        tftp_secure = False
        tftp_path = False
        show_tftp = False

        s1 = self.net.switches[0]
        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server : Enabled" in line:
                tftp_server = True
            if "TFTP server secure mode : Enabled" in line:
                tftp_secure = True
            if "TFTP server file path : /etc/testfile" in line:
                tftp_path = True
        if tftp_server is True \
            and tftp_secure is True \
            and tftp_path is True:
                show_tftp = True

        assert show_tftp == True, 'Test to show tftp server ' \
                                  'configuration failed for ' \
                                  '"show tftp-server" output ' \
                                  '-FAILED'

        tftp_server_present = False
        prev_line = 0
        prev_line2 = 0
        prev_line3 = 0
        enable_present = False
        secure_mode_present = False
        tftp_path_present = False
        show_tftp = False
        i = 0
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for i,line in enumerate(lines):
            if "tftp-server" in line :
                tftp_server_present = True
                prev_line = i
            elif (prev_line + 1 == i) and (tftp_server_present == True):
                 if "enable" in line:
                    enable_present = True
                    prev_line2 = i
            elif (prev_line2 + 1 == i) and (tftp_server_present == True) \
                and (enable_present == True):
                if "secure-mode" in line:
                    secure_mode_present = True
                    prev_line3 = i
            elif (prev_line3 + 1 == i) and (tftp_server_present == True) \
                and (enable_present == True) \
                and (secure_mode_present == True):
                if "path /etc/testfile" in line:
                    tftp_path_present = True

        if tftp_server_present is True \
           and enable_present is True \
           and secure_mode_present is True \
           and tftp_path_present is True:
               show_tftp=True

        assert show_tftp == True, 'Test to tftp server ' \
                                  'configuration failed for ' \
                                  '"show running-config" output ' \
                                  '-FAILED!'

        return True

    def test_tftp_server_disable(self):
        info("\n##########  Test to disable tftp server ##########\n")

        tftp_disabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("tftp-server")
        s1.cmdCLI("no enable")

        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server : Disabled" in line:
                tftp_disabled = True

        assert tftp_disabled == True, 'Test to disable tftp server ' \
                                      'failed for "show tftp-server" ' \
                                      'output -FAILED!'

        tftp_server_present = False
        enable_mode_present = False
        prev_line = 0
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for i, line in enumerate(lines):
            if "tftp-server" in line :
                tftp_server_present = True
                prev_line = i
            elif (i == prev_line+1) and (tftp_server_present == True):
                if "enable" in line:
                    enable_mode_present = True

        assert enable_mode_present == False, 'Test to disable tftp server ' \
                                             'failed for "show running-"' \
                                             'config output -FAILED!'
        return True

    def test_tftp_secure_disable(self):
        info("\n##########  Test to disable tftp server secure mode  "
             "##########\n")

        tftp_secure_disabled = False
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("tftp-server")
        s1.cmdCLI("no secure-mode")

        dump = s1.cmdCLI("do show tftp-server")
        lines = dump.split('\n')
        for line in lines:
            if "TFTP server secure mode : Disabled" in line:
                tftp_secure_disabled = True

        assert tftp_secure_disabled == True, 'Test to disable tftp server '\
                                             'secure mode failed for ' \
                                             '"show tftp-server" output ' \
                                             ' -FAILED!'


        tftp_server_present = False
        secure_mode_present = False
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "tftp-server" in line:
                tftp_server_present = True
            elif tftp_server_present == True:
                if "secure-mode" in line:
                    secure_mode_present = True
        assert secure_mode_present == False, 'Test to disable tftp server '\
                                             'secure mode failed for ' \
                                             '"show running-config" output ' \
                                             ' -FAILED!'

        return True

    def test_dhcp_tftp_del_range(self):
        info("\n##########  Test to delete DHCP dynamic configurations "
             "##########\n")

        range_deleted = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no range test-range start-ip-address 10.0.0.1 \
                   end-ip-address 10.0.0.254 \
                   netmask 255.0.0.0 match tags tag1,tag2,tag3 \
                   set tag test-tag broadcast 10.0.0.255 \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
                    and "10.0.0.1" in line \
                    and "10.0.0.254" in line and "255.0.0.0" in line \
                    and "tag1,tag2,tag3" in line and "test-tag" in line \
                    and "10.0.0.255" in line and "60" in line:
                range_deleted = False

        assert range_deleted == True, \
               'Test to delete DHCP Dynamic configuration failed ' \
               'for "show dhcp-server" output -FAILED!'

        range_deleted = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "test-range" in line \
                and "10.0.0.1"  in line \
                and "10.0.0.254"  in line and "255.0.0.0" in line \
                and "tag1,tag2,tag3"  in line and "test-tag" in line \
                and "10.0.0.255"  in line and "60" in line:
                range_deleted = False

        assert range_deleted == True, \
               'Test to delete DHCP Dynamic configuration failed ' \
               'for "show running-config" output -FAILED!'

        return True

    def test_dhcp_tftp_del_range_ipv6(self):
        info("\n##########  Test to delete DHCP dynamic ipv6 configurations "
             "############\n")

        range_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no range range-ipv6 \
                   start-ip-address 2001:cdba::3257:9652 \
                   end-ip-address 2001:cdba::3257:9655 \
                   prefix-len 64 \
                   match tags v6tag1,v6tag2,v6tag3 \
                   set tag v6-stag \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "range-ipv6" in line \
                    and "2001:cdba::3257:9652" in line \
                    and "2001:cdba::3257:9655" in line and "64" in line \
                    and "v6tag1,v6tag2,v6tag3" in line and "v6-stag" in line \
                    and "60" in line:
                range_created = False

        assert range_created == True, 'Test to delete DHCP Dynamic ipv6 ' \
                                      'configuration failed for ' \
                                      '"show dhcp-server" output ' \
                                      '-FAILED!'

        range_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "range-ipv6" in line \
                and "2001:cdba::3257:9652" in line \
                and "2001:cdba::3257:9655" in line and "64" in line \
                and "v6tag1,v6tag2,v6tag3" in line and "v6-stag" in line \
                and "60" in line:
                range_created = False

        assert range_created == True, 'Test to delete DHCP Dynamic ipv6 ' \
                                      'configuration failed for ' \
                                      '"show running-config" output ' \
                                      '-FAILED!'

        return True

    def test_dhcp_tftp_del_static(self):
        info("\n##########  Test to delete DHCP static configuration "
             "##########\n")

        static_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no static 192.168.0.2 \
                   match-mac-addresses aa:bb:cc:dd:ee:ff \
                   set tags tag4,tag5,tag6 \
                   match-client-id testid \
                   match-client-hostname testname \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "192.168.0.2" in line \
                    and "aa:bb:cc:dd:ee:ff" in line \
                    and "testid" in line \
                    and "tag4,tag5,tag6" in line \
                    and "testname" in line \
                    and "60" in line:
                static_created = False

        assert static_created == True, \
               'Test to delete DHCP static configuration ' \
               'failed for "show dhcp-server" output -FAILED!'

        static_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "192.168.0.2" in line \
                and "aa:bb:cc:dd:ee:ff" in line \
                and "testid" in line \
                and "tag4,tag5,tag6" in line \
                and "testname" in line \
                and "60" in line:
                static_created = False

        assert static_created == True, \
               'Test to delete DHCP static configuration ' \
               'failed for "show running-config" output -FAILED!'

        return True

    def test_dhcp_tftp_del_static_ipv6(self):
        info("\n##########  Test to add DHCP static ipv6 configuration "
             "###########\n")

        static_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no static 2001:cdba::3257:9680 \
                   match-mac-addresses ae:bb:cc:dd:ee:ff \
                   set tags v6-stag1,v6-stag2,v6-stag3 \
                   match-client-id v6testid \
                   match-client-hostname v6testname \
                   lease-duration 60")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "2001:cdba::3257:9680" in line \
                    and "ae:bb:cc:dd:ee:ff" in line \
                    and "v6testid" in line \
                    and "v6-stag1,v6-stag2,v6-stag3" in line \
                    and "v6testname" in line \
                    and "60" in line:
                static_created = False

        assert static_created == True, 'Test to delete DHCP static ipv6 ' \
                                       'configuration failed for ' \
                                       '"show dhcp-server" output -FAILED!'

        static_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "2001:cdba::3257:9680" in line \
                and "ae:bb:cc:dd:ee:ff" in line \
                and "v6testid" in line \
                and "v6-stag1,v6-stag2,v6-stag3" in line \
                and "v6testname" in line \
                and "60" in line:
                static_created = False

        assert static_created == True, 'Test to delete DHCP static ipv6 ' \
                                       'configuration failed for ' \
                                       '"show running-config" output -FAILED!'

        return True

    def test_dhcp_tftp_del_option_name(self):
        info("\n##########  Test to delete DHCP Option-name ##########\n")

        option_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no option \
                   set option-name opt-name \
                   option-value 192.168.0.1 \
                   match tags mtag1,mtag2,mtag3")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "opt-name" in line \
                    and "192.168.0.1" in line \
                    and "False" in line \
                    and "mtag1,mtag2,mtag3" in line:
                option_created = False

        assert option_created == True, \
               'Test to delete DHCP Option-name ' \
               'failed for "show dhcp-server" ' \
               'output-FAILED!'

        option_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "opt-name" in line \
                and "192.168.0.1" in line \
                and "False" in line \
                and "mtag1,mtag2,mtag3" in line :
                option_created = False

        assert option_created == True, \
               'Test to delete DHCP Option-name ' \
               'failed for "show running-config" ' \
               'output -FAILED!'

        return True

    def test_dhcp_tftp_del_option_number(self):
        info("\n##########  Test to delete DHCP Option-number ##########\n")

        option_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no option \
                   set option-number 3 \
                   option-value 192.168.0.3 \
                   match tags mtag4,mtag5,mtag6")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "3" in line \
                    and "192.168.0.3" in line \
                    and "False" in line \
                    and "mtag4,mtag5,mtag6" in line:
                option_created = False

        assert option_created == True, \
               'Test to delete DHCP Option-name ' \
               'with tag matches ' \
               'failed for "show dhcp-server" ' \
               'output -FAILED!'

        option_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "3" in line \
                and "192.168.0.3" in line \
                and "False" in line \
                and "mtag4,mtag5,mtag6" in line :
                option_created = False

        assert option_created == True, \
               'Test to delete DHCP Option-name ' \
               'with tag matches ' \
               'failed for "show running-config" ' \
               'output -FAILED!'

        return True

    def test_dhcp_tftp_del_match_number(self):
        info("\n##########  Test to delete DHCP match number ##########\n")

        match_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no match \
                   set tag stag \
                   match-option-number 4 \
                   match-option-value 192.168.0.4")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "4" in line \
                    and "192.168.0.4" in line \
                    and "stag" in line:
                match_created = False

        assert match_created == True, \
               'Test to delete DHCP match number ' \
               'failed for "show dhcp-server" ' \
               'output -FAILED!'

        match_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "4" in line \
                and "192.168.0.4" in line \
                and "stag" in line :
                match_created = False

        assert match_created == True, \
               'Test to delete DHCP match number ' \
               'failed for "show running-config" ' \
               'output -FAILED!'

        return True

    def test_dhcp_tftp_del_match_name(self):
        info("\n##########  Test to delete DHCP match name ##########\n")

        match_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no match \
                   set tag temp-mtag \
                   match-option-name test-mname \
                   match-option-value 192.168.0.5")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "test-mname" in line \
                    and "192.168.0.5" in line \
                    and "test-mtag" in line:
                match_created = False

        assert match_created == True, \
               'Test to delete DHCP match name ' \
               'failed for "show dhcp-server" ' \
               'output -FAILED!'

        match_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "test-mname" in line \
                and "192.168.0.5" in line \
                and "test-mtag" in line :
                match_created = False

        assert match_created == True, \
               'Test to delete DHCP match name ' \
               'failed for "show running-config" ' \
               'output -FAILED!'

        return True

    def test_dhcp_tftp_del_boot(self):
        info("\n##########  Test to delete DHCP bootp ##########\n")

        boot_created = True
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")
        s1.cmdCLI("dhcp-server")
        s1.cmdCLI("no boot \
                   set file /tmp/testfile \
                   match tag boottag")

        dump = s1.cmdCLI("do show dhcp-server")
        lines = dump.split('\n')
        for line in lines:
            if "/tmp/testfile" in line \
                    and "boottag" in line:
                boot_created = False

        assert boot_created == True, \
               'Test to delete DHCP bootp ' \
               'failed for "show dhcp-server" ' \
               'output -FAILED!'

        boot_created = True
        dump = s1.cmdCLI("do show running-config")
        lines = dump.split('\n')
        for line in lines:
            if "/tmp/testfile" in line \
                and "boottag" in line :
                boot_created = False

        assert boot_created == True, \
               'Test to delete DHCP bootp ' \
               'failed for "show running-config" ' \
               'output -FAILED!'

        return True

class Test_vtysh_dhcp_tftp:

    def setup_class(cls):

        # Create a test topology

        Test_vtysh_dhcp_tftp.test = dhcp_tftpCLItest()

    def teardown_class(cls):

        # Stop the Docker containers, and
        # mininet topology

        Test_vtysh_dhcp_tftp.test.net.stop()

    def test_dhcp_tftp_add_range(self):
        if self.test.test_dhcp_tftp_add_range():
            info("\n###  Test to add DHCP dynamic configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_add_range_ipv6(self):
        if self.test.test_dhcp_tftp_add_range_ipv6():
            info("\n###  Test to add DHCP dynamic ipv6 configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_check_range_validation(self):
        if self.test.test_dhcp_tftp_check_range_validation():
            info("\n###  Test to validate DHCP dynamic configuration"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_add_static(self):
        if self.test.test_dhcp_tftp_add_static():
            info("\n###  Test to add DHCP static configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_add_static_ipv6(self):
        if self.test.test_dhcp_tftp_add_static_ipv6():
            info("\n###  Test to add DHCP static ipv6 configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_check_static_validation(self):
        if self.test.test_dhcp_tftp_check_static_validation():
            info("\n###  Test to validate DHCP static configuration"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_add_option_name(self):
        if self.test.test_dhcp_tftp_add_option_name():
            info("\n###  Test to add DHCP Option-name  - SUCCESS! ###\n")

    def test_dhcp_tftp_option_name_validation(self):
        if self.test.test_dhcp_tftp_option_name_validation():
            info("\n###  Test to validate DHCP Options using option"
                 " name configuration - SUCCESS! ###\n")

    def test_dhcp_tftp_add_option_number(self):
        if self.test.test_dhcp_tftp_add_option_number():
            info("\n###  Test to add DHCP Option-number  - SUCCESS! ###\n")

    def test_dhcp_tftp_option_number_validation(self):
        if self.test.test_dhcp_tftp_option_number_validation():
            info("\n###  Test to validate DHCP Options using option number"
                 " configuration - SUCCESS! ### \n")

    def test_dhcp_tftp_add_match_name(self):
        if self.test.test_dhcp_tftp_add_match_name():
            info("\n###  Test to add DHCP Match-name  - SUCCESS! ###\n")

    def test_dhcp_tftp_add_match_number(self):
        if self.test.test_dhcp_tftp_add_match_number():
            info("\n###  Test to add DHCP Match-number  - SUCCESS! ###\n")

    def test_dhcp_tftp_match_number_validation(self):
        if self.test.test_dhcp_tftp_match_number_validation():
            info("\n###  Test to validate DHCP Match using option"
                 " number configuration - SUCCESS! ###\n")

    def test_dhcp_tftp_match_name_validation(self):
        if self.test.test_dhcp_tftp_match_name_validation():
            info("\n###  Test to validate DHCP Match using option name"
                 " configuration - SUCCESS! ###\n")

    def test_dhcp_tftp_add_boot(self):
        if self.test.test_dhcp_tftp_add_boot():
            info("\n###  Test to add DHCP Boot  - SUCCESS! ###\n")

    def test_dhcp_tftp_boot_validation(self):
        if self.test.test_dhcp_tftp_boot_validation():
            info("\n###  Test to validate DHCP Bootp using configuration"
                 " - SUCCESS! ###\n")

    def test_dhcp_server_show(self):
        if self.test.test_dhcp_server_show():
            info("\n###  Test to show dhcp server confguration"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_range(self):
        if self.test.test_dhcp_tftp_del_range():
            info("\n###  Test to delete dhcp dynamic confguration"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_static(self):
        if self.test.test_dhcp_tftp_del_static():
            info("\n###  Test to delete DHCP static configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_range_ipv6(self):
        if self.test.test_dhcp_tftp_del_range_ipv6():
            info("\n###  Test to delete dhcp dynamic ipv6 confguration"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_static_ipv6(self):
        if self.test.test_dhcp_tftp_del_static():
            info("\n###  Test to delete DHCP static ipv6 configurations"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_option_name(self):
        if self.test.test_dhcp_tftp_del_option_name():
            info("\n###  Test to delete DHCP Option-name - SUCCESS! ###\n")

    def test_dhcp_tftp_del_option_number(self):
        if self.test.test_dhcp_tftp_del_option_number():
            info("\n###  Test to delete DHCP Option-number"
                 " - SUCCESS! ###\n")

    def test_dhcp_tftp_del_match_name(self):
        if self.test.test_dhcp_tftp_del_match_name():
            info("\n###  Test to delete DHCP Match-name  - SUCCESS! ###\n")

    def test_dhcp_tftp_del_match_number(self):
        if self.test.test_dhcp_tftp_del_match_number():
            info("\n###  Test to add delete Match-number  - SUCCESS! ###\n")

    def test_dhcp_tftp_del_boot(self):
        if self.test.test_dhcp_tftp_del_boot():
            info("\n###  Test to delete DHCP Boot  - SUCCESS! ###\n")

    def test_tftp_server_enable(self):
        if self.test.test_tftp_server_enable():
            info("\n###  Test to enable TFTP server  - SUCCESS! ###\n")

    def test_tftp_secure_enable(self):
        if self.test.test_tftp_secure_enable():
            info("\n###  Test to enable TFTP secure mode  - SUCCESS! ###\n")

    def test_tftp_server_add_path(self):
        if self.test.test_tftp_server_add_path():
            info("\n###  Test to add  TFTP path - SUCCESS! ###\n")

    def test_tftp_server_show(self):
        if self.test.test_tftp_server_show():
            info("\n###  Test to show tftp server confguration"
                 " - SUCCESS! ###\n")

    def test_tftp_server_disable(self):
        if self.test.test_tftp_server_disable():
            info("\n###  Test to disable TFTP server  - SUCCESS! ###\n")

    def test_tftp_secure_disable(self):
        if self.test.test_tftp_secure_disable():
            info("\n###  Test to disable TFTP secure mode- SUCCESS! ###\n")
