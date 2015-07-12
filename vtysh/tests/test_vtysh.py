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
    """Custom Topology Example
    H1[h1-eth0]<--->[1]S1[2]<--->[2]S2[1]<--->[h2-eth0]H2
    """

    def build(self, hsts=2, sws=2, **_opts):
        self.hsts = hsts
        self.sws = sws

        "add list of hosts"
        for h in irange(1, hsts):
            host = self.addHost('h%s' % h)

        "add list of switches"
        for s in irange(1, sws):
            switch = self.addSwitch('s%s' % s)

        "Add links between nodes based on custom topo"
        #self.addLink('h1', 's1')
        #self.addLink('h2', 's2')
        #self.addLink('s1', 's2')

class cliDataTypesTest(HalonTest):

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

    def setHostnameTest(self, switch):
        switch.cmdCLI("set-hostname test-host")
        out = switch.cmd("ovs-vsctl list open_vswitch")
        lines = out.split('\n')
        for line in lines:
            if 'hostname' in line and 'test-host' in line:
                return True
        return False

    def getHostnameTest(self, switch):
        out = switch.cmdCLI("get-hostname")
        if 'test-host' in out:
            return True
        return False

    def ifnameDataTypeTest(self, switch):
        out = switch.cmdCLI("conf t")
        out = switch.cmdCLI("interface 1")
        if 'config-if' not in out:
            out = switch.readCLI(switch.cliStdout.fileno(),1024)
        if 'config-if' not in out:
            return False

        out = switch.cmdCLI("exit")
        out = switch.cmdCLI("interface asdf")
        if 'Unknown command' not in out:
            return False;

        out = switch.cmdCLI("end")
        return True;

    def multCxtTest(self, switch):
        out = switch.cmdCLI("conf t")
        out = switch.cmdCLI("interface 12")
        out = switch.cmdCLI("test-interfaceCxt")
        if 'The current context is 12' not in out:
            return False;

        return True;

    def test(self):
        s1 = self.net.switches[0]
        if self.setHostnameTest(s1):
            print 'Passed setHostnameTest'
        else:
            print 'Failed setHostnameTest'

        if self.getHostnameTest(s1):
            print 'Passed getHostnameTest'
        else:
            print 'Failed getHostnameTest'

        if self.ifnameDataTypeTest(s1):
            print 'Passed ifnameDataTypeTest'
        else:
            print 'Failed ifnameDataTypeTest'

        if self.multCxtTest(s1):
            print 'Passed multCxtTest'
        else:
            print 'Failed multCxtTest'
        return True


if __name__ == '__main__':
    test = cliDataTypesTest()

    test.test()

    runCLI = False
    if runCLI is True:
        CLI(test.net);
