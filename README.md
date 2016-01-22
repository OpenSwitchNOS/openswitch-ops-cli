#OPS-CLI

What is ops-cli?
----------------
The ops-cli module is responsible for the CLI daemon in the [OpenSwitch](http://www.openswitch.net)
project.<br/>
The ops-cli is based on vtysh shell being used in [Quagga](http://www.nongnu.org/quagga/).
It has been modified to communicate with the OVSDB database.

What is the structure of the repository?
----------------------------------------
* `lib/` contains all the CLI infrastructure files.
* `vtysh/tests/` contains all the component tests of ops-cli based on [ops-test-framework](http://git.openswitch.net/openswitch/ops-test-framework).

What is the license?
--------------------
Being heavily based on vtysh, ops-cli inherits its GPL license. For more details refer to [GPL](http://www.gnu.org/licenses/gpl.html)

What other documents are available?
-----------------------------------
For the high level design of ops-cli, refer to [DESIGN.md](http://www.openswitch.net/documents/dev/ops-cli/DESIGN).
For adding new commands, refer to [user guide](CLI_user_guide.md).
For answers to common questions, read [FAQ.md](http://www.openswitch.net/documents/user/openswitch_faq).
For the current list of contributors and maintainers, refer to [AUTHORS.md](https://git.openswitch.net/cgit/openswitch/ops-cli/tree/AUTHORS).

For general information about OpenSwitch project refer to  [http://www.openswitch.net](http://www.openswitch.net).
