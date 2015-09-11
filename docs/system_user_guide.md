System
=======

[1.Overview](#systemoverview)<br>
[2.Setting up the basic configuration ](#systembasic)
&nbsp;&nbsp;&nbsp;&nbsp;[2.1 Setting the fan speed ](#systemfanspeed)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[2.2 Setting LED state](#systemledstate)<br>
[3 Verifying the configuration ](#systemverify) <br>
&nbsp;&nbsp;&nbsp;&nbsp;[3.1 View fan details](#systemverifyfan)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[3.2 View LED details](#systemverifyled)<br>
[4.CLI ](#systemcli)<br>
[5.Related features](#systemrelatedfeature)<br>

## 1. Overview <a id="systemoverview"></a>##
 System feature allows the users to configure the physical components present in the system.
Some of the physical components of the switch can be<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Fans<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Temperature sensors<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Power supply modules<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LED<br>
This feature lets the user configure fan speed, LED state.

###2. Setting up the basic configuration <a id="systembasic"></a>

#### 2.1 Setting the fan speed <a id="systemfanspeed"></a>

'configure terminal' command changes the vtysh context to config.
>switch# configure terminal<br>
>switch(config)#


'fan-speed *(slow | medium | fast | max )*' lets the user configure the fans to operate at specified speed . By default all the fans operate at normal speed and will change according to the temperature of the system.

>switch(config)# fan-speed slow<br>
>switch(config)#

#### 2.2 Setting LED state <a id="systemledstate"></a>

'configure terminal' command changes the vtysh context to config.
>switch# configure terminal<br>
>switch(config)#


'led WORD *(on | off | flashing)*' lets the user to set the state of the LED . By default all the LEDs will be in off state.
 User should know the name of the LED of whose state is to be set.
 In the example below *'base-loc'* LED is set to *on*.

>switch(config)# led base-loc on <br>
>switch(config)#


###3. Verifying the configuration <a id="systemverify"></a>
####3.1 View fan details <a id="systemverifyfan"></a>
'show system fan' command displays the detailed information of fans in the system.

>switch#show system fan

Fan information

|Name  |       Speed|  Direction |     Status  |      RPM     |
|------|-------------|------------|------------|-----------|
|base-FAN 2R | slow | front-to-back | ok       |     5700    |
|base-FAN 5L | slow | front-to-back | ok       |     6600    |
|base-FAN 3L | slow | front-to-back | ok       |     6600    |
|base-FAN 4L | slow | front-to-back | ok       |     6600    |
|base-FAN 5R | slow | front-to-back | ok       |     5700    |
|base-FAN 2L | slow | front-to-back | ok       |     6650    |
|base-FAN 3R | slow | front-to-back | ok       |     5700    |
|base-FAN 1R | slow | front-to-back | ok       |     5750    |
|base-FAN 1L | slow | front-to-back | ok       |     6700    |
|base-FAN 4R | slow | front-to-back | ok       |     5700    |

&nbsp;&nbsp;&nbsp;Fan speed override is set to : slow|
|------------------------------------------------------|


####3.2 View LED details <a id="systemverifyled"></a>
'show system led' command displays LED information.
>switch#sh system led

|Name  |         State  |   Status |
|-------|----------------|----------|
|base-loc |      on   |    ok      |



## 4. CLI <a id="systemcli"></a>##
<!--Provide a link to the CLI command related to the feature. The CLI files will be generated to a CLI directory.  -->
Click [here](https://openswitch.net/cli_feature_name.html#cli_command_anchor) for the CLI commands related to the system.
