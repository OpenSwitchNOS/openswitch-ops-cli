from mininet.net import *
from mininet.topo import *
from mininet.node import *
from mininet.link import *
from mininet.cli import *
from mininet.log import *
from mininet.util import *
from subprocess import *
from halonvsi.docker import *
from halonvsi.halon import *
import subprocess
import select

class myTopo(Topo):

    def build(self, hsts=2, sws=2, **_opts):
        self.hsts = hsts
        self.sws = sws

        "add list of hosts"
        for h in irange(1, hsts):
            host = self.addHost('h%s' % h)

        "add list of switches"
        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)


class LLDPCliTest(HalonTest):

    def setupNet(self):
        topo = myTopo(hsts = 0,
            sws = 1,
            hopts = self.getHostOpts(),
            sopts = self.getSwitchOpts(),
            switch = HalonSwitch,
            host = HalonHost,
            link = HalonLink, controller = None,
            build = True)

        self.net = Mininet(topo,
                           switch = HalonSwitch,
                           host = HalonHost,
                           link = HalonLink,
                           controller = None,
                           build = True)

    @staticmethod
    def parseCLI(cliOutput):
        "Parse the cli output"

    def enableLLDPFeatureTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("feature lldp")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                return True
        return False

    def disableLLDPFeatureTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("no feature lldp")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_enable' in line:
                return False
        return True

    def setLLDPholdtimeTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp holdtime 7")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold=\"7\"' in line:
                return True
        return False

    def setLLDPDefaultHoldtimeTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp holdtime 4")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_hold' in line:
                return False
        return True

    def setLLDPTimerTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp timer 100")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval=\"100\"' in line:
                return True
        return False

    def setLLDPDefaultTimerTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp timer 30")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_tx_interval' in line:
                return False
        return True

    def setLLDPMgmtAddressTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp management-address 1.1.1.1")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr=\"1.1.1.1\"' in line:
                return True
        return False

    def unsetLLDPMgmtAddressTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("no lldp management-address")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_mgmt_addr' in line:
                return False
        return True

    def setLLDPClearCountersTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp clear counters")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_counters_requested=\"1\"' in line:
                return True
        return False

    def setLLDPClearNeighborsTest(self, switch):
        switch.cmdCLI("conf t")
        switch.cmdCLI("lldp clear neighbors")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'lldp_num_clear_table_requested=\"1\"' in line:
                return True
        return False



    def test(self):
        s1 = self.net.switches[0]
        if self.enableLLDPFeatureTest(s1):
            print 'Passed enableLLDPFeatureTest'
        else:
            print 'Failed enableLLDPFeatureTest'

        if self.disableLLDPFeatureTest(s1):
            print 'Passed disableLLDPFeatureTest'
        else:
            print 'Failed disableLLDPFeatureTest'

        if self.setLLDPholdtimeTest(s1):
            print 'Passed setLLDPholdtimeTest'
        else:
            print 'Failed setLLDPholdtimeTest'

        if self.setLLDPDefaultHoldtimeTest(s1):
            print 'Passed setLLDPDefaultHoldtimeTest'
        else:
            print 'Failed setLLDPDefaultHoldtimeTest'

        if self.setLLDPTimerTest(s1):
            print 'Passed setLLDPTimerTest'
        else:
            print 'Failed setLLDPTimerTest'

        if self.setLLDPDefaultHoldtimeTest(s1):
            print 'Passed setLLDPDefaultTimerTest'
        else:
            print 'Failed setLLDPDefaultTimerTest'

        if self.setLLDPMgmtAddressTest(s1):
            print 'Passed setLLDPMgmtAddressTest'
        else:
            print 'Failed setLLDPMgmtAddressTest'

        if self.unsetLLDPMgmtAddressTest(s1):
            print 'Passed unsetLLDPMgmtAddressTest'
        else:
            print 'Failed unsetLLDPMgmtAddressTest'

        if self.setLLDPClearCountersTest(s1):
            print 'Passed setLLDPClearCountersTest'
        else:
            print 'Failed setLLDPClearCountersTest'

        if self.setLLDPClearNeighborsTest(s1):
            print 'Passed setLLDPClearNeighborsTest'
        else:
            print 'Failed setLLDPClearNeighborsTest'

if __name__ == '__main__':
    test = LLDPCliTest()

    test.test()

    runCLI = False
    if runCLI is True:
        CLI(test.net);
