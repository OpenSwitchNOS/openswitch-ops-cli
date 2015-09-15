OPS-CLI
=====

##Contents
- [High level design of CLI](#High level design of CLI)
- [Responsibilities](#Responsibilities)
- [Design choices](#Design choices)
- [Relationships to external OpenSwitch entities](#Relationships to external OpenSwitch entities)
- [OVSDB schema](#OVSDB schema)
- [Internal structure](#Internal structure)

##High level design of CLI

This documents describes the design of the CLI. For more information regarding how to
create new commands read the [user guide](CLI_user_guide.md).<br/>

The CLI is a modified version of the vtysh shell being used by the Quagga software. The major change that was made to the shell was to add another thread to connect this shell to the Open vSwitch Database (OVSDB). The CLI follows the OpenSwitch architecture, and it has no communication with any other daemons directly. The CLI communicates only with the OVSDB.

##Responsibilities

The main responsibilities of the CLI are:
<ul>
<li>Executing the configuration and show commands</li>
<li>Connecting to the database and updating the appropriate tables</li>
<li>Displaying help strings for CLI tokens and error messages</li>
<li>Displaying the running configuration</li>
</ul>

##Design choices

The major changes to the original vtysh code were to remove any dependencies it had on other Quagga daemons. A new thread in CLI was added to poll the OVS-DB server. The
active polling mechanism was chosen to avoid socket pressure on the server side. The mechanism used was the *poll_loop( )* defined in OpenvSwitch.

A lock is taken to prevent the IDL (Interface Database Library) cache from changing when the main thread operates on it.

##Relationships to external OpenSwitch entities

The CLI is connected only to the OVS-DB server. There is no communication with other daemons directly from the CLI.

##OVSDB schema

The CLI is registered to many tables in the OVSDB. The tables are required for the show and configuration commands. To find which tables or columns in the database are required by a specific daemon, check the documentation of that daemon.

##Internal structure

The OVS-DB thread polls on the latch and the OVS-DB socket file descriptors. The main thread automatically takes the lock before executing action routines, unless the command is defined by using `DEFUN_NOLOCK`.
The main thread then wakes up the OVSDB thread by setting the latch and then then main thread runs the action routine. The lock is released and it again waits for user input.

In case of DEFUN_NOLOCK the lock is not taken and the caller must explicitly take the lock before interacting with IDL in the action routine. Routines which fork into other processes or wait for user input must use this so that the OVSDB thread is not blocked for a long time.
The OVSDB thread must also be woken up from *poll_block( )* by setting the latch to avoid race conditions in *poll_loop( )* structures. For an example of this look at the definition of start-shell command.

                  Main Thread                                      OVSDB thread

    +-------------------------------------------+        +---------------------------------+
    |                                           |        |                                 |
    |               +----------+                |        |       +-----------------+       |
    |    +--------> | readline |                |        |       |Try to take lock | <---+ |
    |    |          +----+-----+                |        |       +------+----------+     | |
    |    |               |                      |        |              |                | |
    |    |               |                      |        |              |                | |
    |    |               |                      |        |      +-------v-----------+    | |
    |    |       +-------v----------+           |        |      | Run ovsdb_idl_run |    | |
    |    |       | Try to take Lock |           |        |      +-------+-----------+    | |
    |    |       +-------+----------+           |        |              |                | |
    |    |               |                      |        |              |                | |
    |    |         +-----v-----+                |        |       +------v-------+        | |
    |    |         | Set latch +-----------------------> |       | Release lock |        | |
    |    |         +-----+-----+                |        |       +------+-------+        | |
    |    |               |                      |        |              |                | |
    |    |       +-------v----------------+     |        |              |                | |
    |    |       | Run the action routine |     |        |     +--------v------------+   | |
    |    |       +-------+----------------+     |        |     | Waiting for latch   |   | |
    |    |               |                      |        |     | or OVSDB IDL update |   | |
    |    |      +--------v---------+            |        |     +--------+------------+   | |
    |    |      | Release the lock |            |        |              |                | |
    |    |      +--------+---------+            |        |              +----------------+ |
    |    |               |                      |        |                                 |
    |    +---------------+                      |        |                                 |
    +-------------------------------------------+        +---------------------------------+
