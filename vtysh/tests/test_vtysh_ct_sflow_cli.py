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
from opsvsiutils.systemutil import *
import time


class sflowConfigTest(OpsVsiTest):

    def setupNet(self):
        host_opts = self.getHostOpts()
        switch_opts = self.getSwitchOpts()
        static_topo = SingleSwitchTopo(k=0, hopts=host_opts, sopts=switch_opts)
        self.net = Mininet(static_topo, switch=VsiOpenSwitch,
                           host=Host, link=OpsVsiLink,
                           controller=None, build=True)

    def test_sflow_global_status(self):
        '''
        This function checks whether a new row gets created when you enable
        sflow and a reference to it gets stored in the System table.
        Also, it verifies whether the row name is set as 'global'
        '''
        info("\n\n######## Test to Verify Correct Setting of sFlow 'global' "
            "Status ########\n")
        s1 = self.net.switches[0]
        s1.cmdCLI("configure terminal")

        info("### Verifiying creation of row in sFlow table ###\n")
        s1.cmdCLI("sflow enable")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert 'enabled' in out[word_index + 2], 'creation of row in sFlow\
                table unsuccessful'

        info("### Verifiying row name as 'global' for the sflow row "
                "created ###\n")
        ret = s1.cmd("ovsdb-client monitor sFlow --detach")
        out = ret.split('\n')
        assert 'global' in out[2], 'verification of sflow row name as "global"\
                unsuccessful'

        info("### Verifiying removal of sflow reference from System "
                " for 'sflow disable' ###\n")
        s1.cmdCLI("no sflow enable")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert 'disabled' in out[word_index + 2], 'deletion of reference from \
                System table successful'

    def test_sflow_sampling_rate(self):
        '''
        This function verifies correct setting/unsetting of sflow default and
        non-default sampling rate
        '''
        info("\n\n######## Test to Verify Correct Setting of sFlow Sampling "
        "Rate ########\n")
        s1 = self.net.switches[0]

        info("### Verifiying default sflow sampling rate ###\n")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '4096' in out[word_index + 6], 'default sflow sampling rate \
                not set'

        info("### Setting and Verifiying specific sflow sampling rate ###\n")
        s1.cmdCLI("sflow sampling 50000")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '50000' in out[word_index + 6], 'non-default sflow sampling \
                rate not set'

        info("### Unsetting specific rate set and verifying sflow sampling "
                "getting set back to default ###\n")
        s1.cmdCLI("no sflow sampling")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '4096' in out[word_index + 6], 'default sflow sampling \
                rate not reset'

    def test_sflow_collector(self):
        '''
        Thus function checks whether collector ip, port and vrf gets correctly
        set/unset and whether default port and vrf values get set when not
        passed by the user
        '''
        info("\n\n######## Test to Verify Correct Setting of sFlow "
                "Collector ########\n")
        s1 = self.net.switches[0]

        info("### Passing only IPv4 sflow collector ip and verifying 'ip, "
                "default port and vrf' set ###\n")
        s1.cmdCLI("sflow collector 255.255.255.255")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '255.255.255.255/6343/vrf_default' in out[word_index + 3], \
        'collector ip with default port and vrf not set'

        info("### Passing IPv4 sflow collector ip and port and verifying 'ip, "
                "port and default vrf' set ###\n")
        s1.cmdCLI("sflow collector 255.255.255.254 port 1234")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '255.255.255.254/1234/vrf_default' in out[word_index + 3], \
        'collector ip and port with default vrf not set'

        info("### Passing IPv4 sflow collector ip, port and default vrf and "
                "verifying 'ip, port and vrf set ###\n")
        s1.cmdCLI("sflow collector 255.255.255.253 port 5678 vrf vrf_default")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '255.255.255.253/5678/vrf_default' in out[word_index + 3], \
        'collector ip, port and vrf not set'

        """info("### Passing fourth sflow collector ip and "
                "verifying if error is thrown for more than 3 collectors ###\n")
        ret = s1.cmdCLI("sflow collector 255.255.255.252")
        assert 'Maximum of 3 sFlow collectors allowed.' in ret, \
        'collector ip, port and vrf not set'

        info("### Passing first sflow collector ip again and "
                "verifying if duplictae collector error thrown ###\n")
        s1.cmdCLI("sflow collector 255.255.255.255")
        assert 'sFlow collector already present' in ret, \
        'collector ip, port and vrf not set'"""

        info("### Removing second and third collectors and verifying the 'no'"
                " form of the command ###\n")
        s1.cmdCLI("no sflow collector 255.255.255.254 port 1234")
        s1.cmdCLI("no sflow collector 255.255.255.253 port 5678 vrf "
                "vrf_default")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert not ('255.255.255.254/1234/vrf_default' \
                in out and '255.255.255.253/5678/vrf_default' in out), \
                'collectors could not get deleted'

        info("### Passing non-default vrf and verifying the default vrf "
                "check ###\n")
        ret = s1.cmdCLI("sflow collector 255.255.255.252 vrf vrf1")
        assert 'Only vrf_default is permitted.' in ret, \
        'collector ip, port and vrf not set'

        info("### Passing IPv6 sflow collector ip, port and default vrf and "
                "verifying 'ip, port and vrf set ###\n")
        s1.cmdCLI("sflow collector ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff "
                "port 65535 vrf vrf_default")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/65535/vrf_default' \
                in out[word_index + 4], 'IPv6 collector ip, port and vrf \
                not set'

    def test_sflow_agent_intf(self):
        '''
        This function verifies correct setting/unsetting of L3 agent interface
        and its family and also validates the interface before setting it.
        '''
        info("\n\n######## Test to Verify Correct Setting of sFlow Agent "
        "Interface ########\n")
        s1 = self.net.switches[0]

        info("### Verfiying check for invalid interface ###\n")
        ret = s1.cmdCLI("sflow agent-interface 100")
        assert 'Invalid interface' in ret, 'Interface check not successful'

        info("### Verifying correct setting of L3 agent interface ###\n")
        s1.cmdCLI("interface 19")
        s1.cmdCLI("ip address 10.10.10.10/32")
        s1.cmdCLI("exit")
        s1.cmdCLI("sflow agent-interface 19")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '19' in out[word_index + 5], 'L3 agent-interface not correctly \
                set'

        info("### Verifying correct setting of L3 agent interface and family"
                " ###\n")
        s1.cmdCLI("interface 29")
        s1.cmdCLI("ip address 20.20.20.20/32")
        s1.cmdCLI("exit")
        s1.cmdCLI("sflow agent-interface 29 ipv4")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert '29' in out[word_index + 5] and 'ipv4' in out[word_index + 6], \
                'L3 agent-interface and family not correctly set'

        info("### Verifying correct unsetting of L3 agent interface and family"
                " ###\n")
        s1.cmdCLI("no sflow agent-interface")
        ret = s1.cmdCLI("do show sflow")
        out = ret.split('\n')
        word_index = out.index('sFlow Configuration ')
        assert not ('29' in out[word_index + 5] and 'ipv4' in \
                out[word_index + 6]), 'L3 agent-interface and family not \
                correctly set'

    def test_sflow_show_running(self):
        '''
        This function verifies the output of 'show running-config' command
        across the configuration set in the sFlow table
        '''
        info("\n\n######## Test to Verify 'show running-config' for sFlow "
                "Configuration ########\n")
        s1 = self.net.switches[0]

        s1.cmdCLI("sflow enable")
        s1.cmdCLI("sflow sampling 54321")
        s1.cmdCLI("sflow agent-interface 19 ipv6")
        ret = s1.cmdCLI("do show running-config")
        print ret
        out = ret.split('\n')
        print out
        word_index = out.index('sflow enable')
        assert 'sflow collector 255.255.255.255' in \
                out[word_index+1] and \
                'sflow collector ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ' \
                'port 65535 vrf vrf_default' in out[word_index+2] and \
                'sflow agent-interface 19 ipv6' in \
                out[word_index+3] and 'sflow sampling 54321' in \
                out[word_index+4], 'show running-config failure'
        info("### 'show running-config' verification successful###\n\n\n")

class Test_vtysh_ct_sflow_cli:

    def setup_class(cls):
        Test_vtysh_ct_sflow_cli.test = sflowConfigTest()

    def teardown_class(cls):
        Test_vtysh_ct_sflow_cli.test.net.stop()

    def test_sflow_global_status(self):
        self.test.test_sflow_global_status()

    def test_sflow_sampling_rate(self):
        self.test.test_sflow_sampling_rate()

    def test_sflow_collector(self):
        self.test.test_sflow_collector()

    def test_sflow_agent_intf(self):
        self.test.test_sflow_agent_intf()

    def test_sflow_show_running(self):
        self.test.test_sflow_show_running()

    def __del__(self):
        del self.test
