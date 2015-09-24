System commands
==========

[TOC]



## System configuration commands <a id="systemconfigcmds"></a>

###  Set fan speed <a id="fanspeed"></a>

#### Syntax

`fan-speed < slow | medium | fast | maximum >`

#### Description

This command will globally set the fan speed to the value indicated by command parameter. This command overrides the fan speed set internally by platform. fan speed value set by the user takes effect depending on platform cooling requirement.

#### Authority

All users.

#### Parameters

This command takes one of the values from slow, medium, fast and maximum. By default fans operate at normal speed.



| Parameter | Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *slow* | choose one| Literal | set fan speed to slow, which is 25% of maximum speed |

| *normal* | choose one| Literal | set fan speed to normal, which is 40% of maximum speed  |

| *medium* | choose one| Literal | set fan speed to medium, which is 65% of maximum speed|

| *fast* | choose one| Literal | set fan speed to fast, which is 80% of maximum speed|

| *max* | choose one| Literal | set fan speed to maximum |



#### Examples

```

switch(config)#fan-speed slow

```



### Unset fan speed <a id="nofanspeed"></a>



#### Syntax

`no fan-speed [< slow | medium | fast | maximum >]`

#### Description

This command will remove the configured fan speed and set it to default.

#### Authority

All users.

#### Parameters

| Parameter | Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *slow* | optional(choose one)| Literal | set fan speed to slow, which is 25% of maximum speed |

| *normal* |optional(choose one)| Literal | set fan speed to normal, which is 40% of maximum speed  |

| *medium* | optional(choose one)| Literal | set fan speed to medium, which is 65% of maximum speed|

| *fast* | optional(choose one)| Literal | set fan speed to fast, which is 80% of maximum speed|

| *max* | optional(choose one)| Literal | set fan speed to maximum |

#### Examples

`

switch(config)#no fan-speed

`

### Set LED state <a id="led"></a>

#### Syntax

`led < led-name > < on | flashing | off >`

#### Description

 This command will set the LED state to 'on' / 'off' / 'flashing'. By default LED state is off.

#### Authority

All users.

#### Parameters

| Parameter 1| Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *led-name* | Required | Literal |LED name of whose state is to be set |





| Parameter 2| Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *off* | choose one| Literal |Select this to switch off LED |

| *on* | choose one| Literal  | Select this to switch on LED |

| *flashing*| choose one|Literal | Select this to blink/flash the LED|



#### Examples

```

switch(config)#led base-loc on

```



### Unset LED state <a id="noled"></a>



#### Syntax

`no led <led-name> [< on | flashing | off >]`



#### Description

This command will turn off the LED.



#### Authority

All users.



#### Parameters



| Parameter 1| Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *led-name* | Required | Literal | LED name of whose state is to be set |





| Parameter 2| Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *off* | Optional(choose one)| Literal |Select this to switch off LED |

| *on* | Optional (choose one)| Literal  | Select this to switch on LED |

| *flashing*| Optional (choose one)|Literal | Select this to blink/flash the LED|





#### Examples

```

switch(config)#no led base-loc

```

## System Display Commands <a id="systemdisplaycmds"></a>

### Show system information <a id="showsystem"></a>



#### Syntax

`show system [ < fan | temperature [ detail ] | led | power-supply >]`



#### Description

This command without any parameter, shows the overall system details including information of physical components such as the fan, temperature sensor, LED, power supply etc. This command also gives detailed information of various physical components based on the given parameter.



#### Authority

All users.



#### Parameters



| Parameter 1 | Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *fan* | choose one| Literal | To display fan information |

| *temperature * | choose one| Literal | To display temperature-sensor information |

| *led* | choose one| Literal | To display LED information |

| *power-supply* | choose one| Literal | To display power-supply information |



| Parameter 2 | Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *detail* | Optional | Literal | To display detailed temperature-sensor information |



#### Examples

```

switch#show system



OpenSwitch Version  :

Product Name        : 5712-54X-O-AC-F



Vendor              : Edgecore

Platform            : x86_64-accton_as5712_54x-r0

Manufacturer        : Accton

Manufacturer Date   : 03/24/2015 02:05:30



Serial Number       : 571254X1512035      Label Revision      : R01H



ONIE Version        : 2014.08.00.05       DIAG Version        : 2.0.1.0

Base MAC Address    : 70:72:cf:fd:e9:b9   Number of MACs      : 74

Interface Count     : 78                  Max Interface Speed : 40000 Mbps



Fan details:



Name           Speed     Status

--------------------------------

base-1L        normal    ok

base-1R        normal    ok

base-2L        normal    ok

base-2R        normal    ok

base-3L        normal    ok

base-3R        normal    ok

base-4L        normal    ok

base-4R        normal    ok

base-5L        normal    ok

base-5R        normal    ok



LED details:



Name      State     Status

-------------------------

base-loc  on        ok



Power supply details:



Name      Status

-----------------------

base-1    ok

base-2    Input Fault



Temperature Sensors:



Location                                          Name      Reading(celsius)

---------------------------------------------------------------------------

front                                             base-1    21.00

side                                              base-3    18.00

back                                              base-2    20.00

```



### System fan information <a id="showfan"></a>

#### Syntax

`show system fan`

#### Description

Command to display detailed fan information.

#### Authority

All users

#### Parameters

This command does not take a parameter

#### Example

```

switch#show system fan



Fan information

------------------------------------------------------

Name         Speed  Direction      Status        RPM

------------------------------------------------------

base-2L      normal front-to-back  ok            9600

base-5R      normal front-to-back  ok            8100

base-3R      normal front-to-back  ok            8100

base-4R      normal front-to-back  ok            8100

base-3L      normal front-to-back  ok            9600

base-5L      normal front-to-back  ok            9600

base-1R      normal front-to-back  ok            8100

base-1L      normal front-to-back  ok            9600

base-2R      normal front-to-back  ok            7950

base-4L      normal front-to-back  ok            9600

------------------------------------------------------

Fan speed override is not configured

------------------------------------------------------

```

### System temperature information<a id="showtemp"></a>

#### Syntax

    show system temperature { detail }

#### Description

Command to display detailed temperature sensor information.Without a parameter, the command displays brief temperature information.

#### Authority

All users.

#### Parameters

| Parameter  | Status   | Syntax         | Description                           |

|:-----------|:----------|:----------------:|:---------------------------------------|

| *detail* | Optional | Literal | To display detailed temperature-sensor information |



#### Example

```

switch#show system temperature



Temperature information

---------------------------------------------------

            Current

Name      temperature    Status         Fan state

            (in C)

---------------------------------------------------

base-1    21.50          normal         normal

base-3    18.50          normal         normal

base-2    20.50          normal         normal

```

```

switch#show system temperature detail



Detailed temperature information

---------------------------------------------------

Name                      :base-1

Location                  :front

Status                    :normal

Fan-state                 :normal

Current temperature(in C) :21.50

Minimum temperature(in C) :19.50

Maximum temperature(in C) :22.00



Name                      :base-3

Location                  :side

Status                    :normal

Fan-state                 :normal

Current temperature(in C) :18.50

Minimum temperature(in C) :17.50

Maximum temperature(in C) :19.50



Name                      :base-2

Location                  :back

Status                    :normal

Fan-state                 :normal

Current temperature(in C) :20.50

Minimum temperature(in C) :18.50

Maximum temperature(in C) :21.00



```



### System LED information <a id="showled"></a>

#### Syntax

`show system led`



#### Description

Command to display detailed LED information.

#### Authority

All users

#### Parameters

This command does not take a parameter.

#### Example

```

switch#show system led



Name           State     Status

-----------------------------------

base-loc       on        ok

```



### System power-supply information<a id="showpowersupply"></a>

#### Syntax

`show system power-supply`

#### Description

Command to display detailed power-supply information.



#### Authority

All users.



#### Parameters

This command does not take a parameter.



#### Examples

```

switch#show system power-supply

Name           Status

-----------------------------

base-1         ok

base-2         Input Fault

```