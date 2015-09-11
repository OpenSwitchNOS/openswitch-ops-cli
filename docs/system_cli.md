System commands
=======


[1 System configuration commands](#systemconfigcmds)<br>

&nbsp;&nbsp;&nbsp;[1.1 Set fan speed](#fanspeed)<br>
&nbsp;&nbsp;&nbsp;[1.2 Unset fan speed](#nofanspeed)<br>
&nbsp;&nbsp;&nbsp;[1.3 Set LED state](#led)<br>
&nbsp;&nbsp;&nbsp;[1.4 Unset LED state](#noled)<br>

[2 System display commands](#systemdisplaycmds)<br>

&nbsp;&nbsp;&nbsp;[2.1 Show system information](#showsystem)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ 2.1.1 Fan information](#showfan)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ 2.1.2 Temperature information](#showtemp)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ 2.1.3 LED information](#showled)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ 2.1.4 Power-supply information](#showpowersupply)<br>



## 1. System configuration commands <a id="systemconfigcmds"></a>##
###  1.1 Set fan speed <a id="fanspeed"></a>###
#### Syntax
    fan-speed ( slow | medium | fast | maximum )
####
#### Description
This command will globally set the fan speed to the value indicated by command parameter. This command overrides the fan speed set internally by platform. fan speed value set by the user takes effect depending on platform cooling requirement.
####
#### Authority
All users
####
#### Parameters
This command takes one of the values from slow, medium, fast and maximum. By default fans operate at normal speed.<br>

slow – 25% of maximum speed.<br>
normal – 40% of maximum speed.<br>
medium - 65% of maximum speed.<br>
fast - 80% of maximum speed.<br>
max – Maximum speed.<br>
####
#### Examples
    switch(config)#fan-speed slow
    switch(config)#fan-speed maximum
####
### 2. Unset fan speed <a id="nofanspeed"></a> ###

#### Syntax
    no fan-speed
####
#### Description
This command will remove the configured fan speed and set it to default.
####
#### Authority
All users
####
#### Parameters
This command does not take a parameter.
####
#### Examples
    switch(config)#no fan-speed
####
### 3. Set LED state <a id="led"></a>###
#### Syntax
    led WORD ( on | flashing | off )
####
#### Description
 This command will set the LED state to 'on' / 'off' / 'flashing'. By default LED state is off.
####
#### Authority
All users
####
#### Parameters
First parameter 'WORD' is the name of the LED whose state is to be set.
Second parameter is the state of the LED to be set.

####
#### Examples
    switch(config)#led base-loc on
    switch(config)#led base-loc flashing
####

### 4. unset LED state <a id="noled"></a> ###

#### Syntax
    no led WORD
####

#### Description
This command will turn off the LED.
####

#### Authority
All users
####

#### Parameters
WORD is the *led-name* of the LED whose state is to be unset.

####

#### Examples
    (config)#no led base-loc
####


##2) System Display Commands <a id="systemdisplaycmds"></a>##
### 2.1. Show system information <a id="showsystem"></a>###

#### Syntax
    show system { fan | temperature { detail } | led | power-supply }
####

#### Description
This command without any parameter, shows the overall system details including information of physical components such as the fan, temperature sensor, LED, power supply etc. This command also gives detailed information of various physical components based on the given parameter.
####

#### Authority
All users
####

#### Parameters
This command may or may not take parameter.<br>
- Without parameter the command displays overall system information
- Parameters can be *fan*, *temperature*, *led*, *power-supply*
####

#### Examples
Command to display overall system information.
    switch#show system
### 2.1.1. System fan information <a id="showfan"></a> ###
#### Syntax
    show system fan
####
#### Description
Command to display detailed fan information.
####
#### Authority
All users
####
#### Parameters
This command does not take a parameter
####
#### Example
    switch#show system fan
####
### 2.1.2. System temperature information<a id="showtemp"></a> ###
#### Syntax
    show system temperature { detail }
####
#### Description
Command to display detailed temperature sensor information.
####
#### Authority
All users
####
#### Parameters
This command may or may not take parameters
- Without a parameter, the command displays brief temperature information.
- This command may take *detail* as a parameter to display detailed temperature sensor information including location details of each temperature sensor.
####
#### Example
    switch#show system temperature
    switch#show system temperature detail
####

### 2.1.3. System LED information <a id="showled"></a> ###
#### Syntax
    show system led
####
#### Description
Command to display detailed LED information.
####
#### Authority
All users
####
#### Parameters
This command does not take a parameter.
####
#### Example
    switch#show system led
####

### 2.1.4. System power-supply information<a id="showpowersupply"></a> ###
#### Syntax
    show system power-supply
####
#### Description
Command to display detailed power-supply information.
####
#### Authority
All users
####
#### Parameters
This command does not take a parameter.
####
#### Examples
    switch#show system power-supply
####
