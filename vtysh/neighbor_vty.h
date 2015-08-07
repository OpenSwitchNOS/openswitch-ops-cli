/*
 Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License. You may obtain
 a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 License for the specific language governing permissions and limitations
 under the License.
*/
/****************************************************************************
 * @ingroup cli/vtysh
 *
 * @file neighbor_vty.h
 * To add declarations required for neighbor_vty.c
 *
 ***************************************************************************/

#ifndef _NEIGHBOR_VTY_H
#define _NEIGHBOR_VTY_H

#define EMPTY_STRING    ""

/* ip_address is guaranteed to be nonnull */
#define DISPLAY_NEIGHBOR_IP4_ADDR(vty, row) \
    do {                                    \
        vty_out(vty, "%-16s ", row->ip_address);\
    } while(0)                              \

#define DISPLAY_NEIGHBOR_IP6_ADDR(vty, row) \
    do {                                    \
        vty_out(vty, "%-46s ", row->ip_address);\
    } while(0)                              \

#define DISPLAY_NEIGHBOR_MAC_ADDR(vty, row) \
    do {                                    \
        if (row->mac)                       \
            vty_out(vty, "%-18s ", row->mac);   \
        else                                \
            vty_out(vty, "%-18s ", EMPTY_STRING);\
    } while(0)                              \

/* If port is nonnull, then its name is nonnull too */
#define DISPLAY_NEIGHBOR_PORT_NAME(vty, row)\
    do {                                    \
        if (row->port)                      \
            vty_out(vty, "%-16s ", row->port->name); \
        else                                \
            vty_out(vty, "%-16s ", EMPTY_STRING);   \
    } while(0)                              \

#define DISPLAY_NEIGHBOR_STATE(vty, row)    \
    do {                                    \
        if (row->state)                     \
            vty_out(vty, "%-10s ", row->state); \
        else                                \
            vty_out(vty, "%-10s ", EMPTY_STRING);\
    } while(0)                              \

#define DISPLAY_VTY_NEWLINE(vty)        \
    do {                                \
        vty_out(vty, "%s", VTY_NEWLINE);\
    } while(0)                          \

void
neighbor_vty_init(void);

#endif /* _NEIGHBOR_VTY_H */
