/* Hewlett-Packard Company Confidential (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
 *
 * File: ecmp_vty.h
 *
 * Purpose:  To add declarations required for ecmp_vty.c
 */

#ifndef _ECMP_VTY_H
#define _ECMP_VTY_H

#define GET_ECMP_CONFIG_STATUS(row) smap_get_bool(&row->ecmp_config, \
                                  SYSTEM_ECMP_CONFIG_STATUS, \
                                  SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT)

#define GET_ECMP_CONFIG_HASH_SRC_IP_STATUS(row) smap_get_bool(&row->ecmp_config, \
                                                          SYSTEM_ECMP_CONFIG_HASH_SRC_IP, \
                                                          SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT)
#define GET_ECMP_CONFIG_HASH_DST_IP_STATUS(row) smap_get_bool(&row->ecmp_config, \
                                                          SYSTEM_ECMP_CONFIG_HASH_DST_IP, \
                                                          SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT)
#define GET_ECMP_CONFIG_HASH_SRC_PORT_STATUS(row) smap_get_bool(&row->ecmp_config, \
                                                          SYSTEM_ECMP_CONFIG_HASH_SRC_PORT, \
                                                          SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT)
#define GET_ECMP_CONFIG_HASH_DST_PORT_STATUS(row) smap_get_bool(&row->ecmp_config, \
                                                          SYSTEM_ECMP_CONFIG_HASH_DST_PORT, \
                                                          SYSTEM_ECMP_CONFIG_ENABLE_DEFAULT)

void
ecmp_vty_init (void);

#endif /* _ECMP_VTY_H */
