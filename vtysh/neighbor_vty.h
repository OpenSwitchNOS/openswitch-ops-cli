/*
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
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
