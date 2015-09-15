#High level design of CLI

This documents describes the design of the CLI. For more information regarding how to
create new commands read the [user guide](CLI_user_guide.md).<br/>

The CLI is a modified version of the vtysh shell being used by Quagga. The major change
was to add another thread to connect this shell to the ovsdb. The CLI follows the OpenSwitch architecture and has no communication with any other daemons directly. CLI communicates only with the database, OVS-DB.

Responsibilities
----------------
The main responsibilities of the CLI are.
<ul>
<li>Executing the configuration and show commands</li>
<li>Connecting to the database and updating the appropriate tables</li>
<li>Displaying help strings for CLI tokens and error messages</li>
<li>Displaying the running configuration</li>
</ul>

Design choices
--------------
The major changes to the original vtysh code were to remove any dependencies it had on other Quagga daemons. A new thread in CLI was added to poll the OVS-DB server. The
active polling mechanism was chosen to avoid socket pressure on the server side. The mechanism used was the *poll_loop( )* defined in OpenvSwitch.

A lock is taken to prevent the IDL (Interface Database Library?) cache from changing when the main thread operates on it.

Relationships to external OpenSwitch entities
---------------------------------------------
The CLI is connected only to the OVS-DB server. There is no communication with other daemons directly from the CLI.

OVSDB-Schema
------------
The CLI is registered to many tables in the ovsdb which are required for show and configuration commands. For finding out what tables or columns of the database, a specific daemon needs please check the documentation of that daemon.

Internal structure
------------------

The OVS-DB thread polls on the latch and the OVS-DB socket file descriptors. The main thread automatically takes the lock before executing action routines (unless the command is defined using DEFUN_NOLOCK).
Then it wakes up the OVSDB thread by setting the latch and then runs the action routine. The lock is released and it again waits for user input.

In case of DEFUN_NOLOCK the lock is not taken and the caller must explicitly take the lock before interacting with IDL in the action routine. Routines which fork into other processes or wait for user input must use this so that the OVSDB thread is not blocked for a long time.
The OVSDB thread must also be woken up from *poll_block( )* by setting the latch to avoid race conditions in *poll_loop( )* structures.

                  Main Thread                                      OVSDB thread

    +-------------------------------------------+        +---------------------------------+
    |                                           |        |                                 |
    |               +----------+                |        |       +-----------------+       |
    |    +--------> | readline( ) |                |        |       |Try to take lock | <---+ |
    |    |          +----+-----+                |        |       +------+----------+     | |
    |    |               |                      |        |              |                | |
    |    |               |                      |        |              |                | |
    |    |               |                      |        |      +-------v-----------+    | |
    |    |       +-------v----------+           |        |      | Run ovsdb_idl_run( ) |    | |
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