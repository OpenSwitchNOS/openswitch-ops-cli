#!/usr/bin/python

# Copyright (C) 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import pytest
import re
from  opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *
from opsvsi.docker import *
from opsvsi.opsvsitest import *

# The purpose of this test is to test that ntp config
# works as per the design and we receive the output as provided

SERVER1 = "1.1.1.1"
SERVER2 = "2.2.2.2"
SERVER3 = "3.3.3.3"
SERVER4 = "4.4.4.4"
keyid = '12'
VER = '3'

class myTopo(Topo):
    def build(self, hsts=0, sws=1, **_opts):
        '''Function to build the topology of \
        one host and one switch'''
        self.hsts = hsts
        self.sws = sws
        # Add list of switches
        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

class ntpConfigTest(OpsVsiTest):
	def setupNet(self):
            self.net = Mininet(topo=myTopo(hsts=0, sws=1,
                                       hopts=self.getHostOpts(),
                                       sopts=self.getSwitchOpts()),
                                       switch=VsiOpenSwitch,
                                       host=Host,
                                       link=OpsVsiLink, controller=None,
                                       build=True)

	def ntpConfig(self):
            info('\n### Configure different ntp associations ###\n')
	    s1 = self.net.switches[0]
            s1.cmdCLI("configure terminal")
            s1.cmdCLI("ntp server %s" % SERVER1)
            s1.cmdCLI("ntp server %s" % SERVER2)
            s1.cmdCLI("ntp server %s prefer key-id 12" % SERVER3)
            s1.cmdCLI("ntp server %s version 3" % SERVER4)
            s1.cmdCLI("exit")

        def testNtpAssociationsConfig(self):
            info('\n### Verify ntp associations table ###\n')
            s1 = self.net.switches[0]
            #parse the ntp associations command
            dump = s1.cmdCLI("show ntp associations")
            lines = dump.split('\n')
            count = 0
            for line in lines:
               if SERVER1 in line:
                  info("###found %s in db###\n"% SERVER1)
                  count = count + 1

               if SERVER2 in line:
                  info('###found %s in db###\n'% SERVER2)
                  count = count + 1

               if SERVER3 in line:
                  info('###found %s in db###\n'% SERVER3)
                  count = count + 1

               if (SERVER4 in line and VER in line):
                  info('###found %s and appropriate version in db###\n'% SERVER4)
                  count = count + 1

            assert count == 4, \
                   info('tests are not successful\n')

            info('\n### testNtpAssociationsConfig: Test Passed ###\n')

        def testUnconfigureNtpServers(self):
            info('\n### checking unconfigure commands ###\n')
            s1 = self.net.switches[0]
            s1.cmdCLI("configure terminal")
            s1.cmdCLI("no ntp server %s" % SERVER1)
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp associations")
            count = 0
            lines = dump.split('\n')
            for line in lines:
               if SERVER1 in line:
                  info('\n### no ntp server tests unsuccessful \n')
                  conut = count + 1

            assert count == 0, \
                  info('\n### no ntp server tests unsuccessful : Test failed### \n')

            info('\n### no ntp server tests unsuccessful : Test passed### \n')

        def testNtpAuthConfig(self):
            info('\n### Authentication config test###\n')
            s1 = self.net.switches[0]
            s1.cmdCLI("configure terminal")
            s1.cmdCLI("ntp authentication enable")
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp status")
            lines = dump.split('\n')
            count = 0
            for line in lines:
               if "NTP has been true" in line:
                  info('\n### enable authentication test successful\n')
                  count = count + 1

            s1.cmdCLI("configure terminal")
            s1.cmdCLI("no ntp authentication enable")
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp status")
            lines = dump.split('\n')
            for line in lines:
               if "NTP has been false" in line:
                  info("\n### disable authentication test successful \n")
                  count = count + 1

            assert count == 2, \
                   info('\n### Authentication config: Tests failed!!###\n')

        def testNtpAuthTrustKeys(self):
            info('\n### Authentication config test###\n')
            s1 = self.net.switches[0]
            s1.cmdCLI("configure terminal")
            s1.cmdCLI("ntp authentication-key %s md5 mypassword" % keyid)
            s1.cmdCLI("ntp trusted-key %s" % keyid)
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp authentication-keys")
            lines = dump.split('\n')
            count = 0
            for line in lines:
                if keyid in line:
                   info("\n#### authentication key added successfully ###\n")
                   count = count + 1

            dump = s1.cmdCLI("show ntp trusted-keys")
            lines = dump.split('\n')
            for line in lines:
                if keyid in line:
                   info("\n####Trusted  key added successfully ###\n")
                   count = count + 1

            s1.cmdCLI("configure terminal")
            s1.cmdCLI("no ntp authentication-key %s md5 mypassword" % keyid)
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp authentication-keys")
            lines = dump.split('\n')
            for line in lines:
                if keyid in line:
                   info("\n#### authentication key removed unsuccessful ###\n")
                   count = count - 1


            assert count == 2, \
                   info('\n### Auth and trusted key : Tests failed!!! ###\n')

            info('\n### Auth and trusted key : Tests passed!!! ###\n')

        def testNtpTrustedKeyConf(self):
            info('\n### Authentication config test###\n')
            s1 = self.net.switches[0]
            s1.cmdCLI("configure terminal")
            s1.cmdCLI("ntp authentication-key %s md5 mypassword" % keyid)
            s1.cmdCLI("ntp trusted-key %s" % keyid)
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp trusted-keys")
            count = 0
            lines = dump.split('\n')
            for line in lines:
                if keyid in line:
                   info("\n####Trusted  key added successfully ###\n")
                   count = count + 1

            s1.cmdCLI("configure terminal")
            s1.cmdCLI("no ntp trusted-key %s" % keyid)
            s1.cmdCLI("exit")
            dump = s1.cmdCLI("show ntp trusted-keys")
            lines = dump.split('\n')
            for line in lines:
                if keyid in line:
                   info("\n####Trusted  key removed successfully ###\n")
                   count = count + 1

            assert count ==2, \
                   info("\n### trusted key test unsuccessful: Test failed###\n")

class TestNtpConfig:

	def setup(self):
            pass

        def teardown(self):
            pass

        def setup_class(cls):
            TestNtpConfig.ntpConfigTest = ntpConfigTest()

        def teardown_class(cls):
            # Stop the Docker containers, and
            # mininet topology
            TestNtpConfig.ntpConfigTest.net.stop()

        def __del__(self):
            del self.ntpConfigTest

        def testNtpFull(self):
            info('\n########## Test NTP configuration ##########\n')
            self.ntpConfigTest.ntpConfig()
            self.ntpConfigTest.testNtpAssociationsConfig()
            self.ntpConfigTest.testUnconfigureNtpServers()
            self.ntpConfigTest.testNtpAuthConfig()
            self.ntpConfigTest.testNtpAuthTrustKeys()
            self.ntpConfigTest.testNtpTrustedKeyConf()
            info('\n########## End of test NTP configuration configuration ##########\n')
