/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
#ifndef _BANNER_VTY_H
#define _BANNER_VTY_H

#define MAX_BANNER_LENGTH 4096

/* The strings below define the default banners to be restored by the command
 * "banner default" and "banner exec default". */
#define DEFAULT_BANNER_STR "\nWelcome to OpenSwitch\n"
#define DEFAULT_BANNER_EXEC_STR "\nPlease be responsible\n"

#define SYSTEM_OTHER_CONFIG_MAP_BANNER "banner"
#define SYSTEM_OTHER_CONFIG_MAP_BANNER_EXEC "banner_exec"

enum banner_type {
    BANNER_MOTD,
    BANNER_EXEC
};

void banner_vty_init(void);

#endif
