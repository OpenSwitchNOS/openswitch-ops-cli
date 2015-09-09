/*
 * Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: vtysh_utils.h
 *
 * Purpose: header file for common vtysh utility macros and functions
 */

#ifndef _VTYSH_UTILS_H
#define _VTYSH_UTILS_H

typedef unsigned char boolean;

#define TRUE 1
#define FALSE 0

#define MAX_IPV6_STRING_LENGTH 45
#define MAX_IPV4_OR_IPV6_SUBNET_CIDR_STR_LEN MAX_IPV6_STRING_LENGTH + 3

#define  IS_BROADCAST_IPV4(i)      (((long)(i) & 0xffffffff) == 0xffffffff)
#define  IS_LOOPBACK_IPV4(i)       (((long)(i)) == 0x7F000001)
#define  IS_MULTICAST_IPV4(i)      (((long)(i) & 0xf0000000) == 0xe0000000)
#define  IS_EXPERIMENTAL_IPV4(i)   (((long)(i) & 0xf0000000) == 0xf0000000)
#define  IS_INVALID_IPV4(i)         ((long)(i) == 0)

#define IPV4_ADDR_LEN 4

/* IPv6 macros */

#define IPV6_ADDR_LEN 16

/*
* Unspecified
*/
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) \
   ((*(const u_int32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
    (*(const u_int32_t *)(const void *)(&(a)->s6_addr[1]) == 0) && \
    (*(const u_int32_t *)(const void *)(&(a)->s6_addr[2]) == 0) && \
    (*(const u_int32_t *)(const void *)(&(a)->s6_addr[3]) == 0))
#endif

/*
* Loopback
*/
#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) \
  ((*(const u_int32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
   (*(const u_int32_t *)(const void *)(&(a)->s6_addr[1]) == 0) && \
   (*(const u_int32_t *)(const void *)(&(a)->s6_addr[2]) == 0) && \
   (*(const u_int32_t *)(const void *)(&(a)->s6_addr[3]) == ntohl(1)))
#endif

#ifndef IN6_ARE_ADDR_EQUAL
#define IN6_ARE_ADDR_EQUAL(a, b)                        \
    (memcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)
#endif


/*
* Unicast Scope
* Note that we must check topmost 10 bits only, not 16 bits (see RFC2373).
*/
#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) \
   (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
#endif

#ifndef IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_SITELOCAL(a) \
   (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))
#endif
/*
* Multicast
*/
#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a)        ((a)->s6_addr[0] == 0xff)
#endif

/*
* IPv4 compatible
*/
#ifndef  IN6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4COMPAT(a)         \
        ((*(const u_int32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
         (*(const u_int32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
        (*(const u_int32_t *)(const void *)(&(a)->s6_addr[8]) == 0) && \
         (*(const u_int32_t *)(const void *)(&(a)->s6_addr[12]) != 0) &&        \
         (*(const u_int32_t *)(const void *)(&(a)->s6_addr[12]) != ntohl(1)))
#endif /* IN6_IS_ADDR_V4COMPAT */

/*
* Mapped
*/
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a)               \
        ((*(const u_int32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
         (*(const u_int32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
         (*(const u_int32_t *)(const void *)(&(a)->s6_addr[8]) == ntohl(0x0000ffff)))
#endif /* IN6_IS_ADDR_V4MAPPED */


#endif /* _VTYSH_UTILS_H  */
