/*
 * Prefix list functions.
 * Copyright (C) 1999 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your
 * option) any later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _QUAGGA_PLIST_H
#define _QUAGGA_PLIST_H

#define AFI_ORF_PREFIX 65535

enum prefix_list_type
{
  PREFIX_DENY,
  PREFIX_PERMIT,
};

enum prefix_name_type
{
  PREFIX_TYPE_STRING,
  PREFIX_TYPE_NUMBER
};

struct prefix_list
{
  char *name;
  char *desc;

  struct prefix_master *master;

  enum prefix_name_type type;

  int count;
  int rangecount;

  struct prefix_list_entry *head;
  struct prefix_list_entry *tail;

  struct prefix_list *next;
  struct prefix_list *prev;
};

struct orf_prefix
{
  u_int32_t seq;
  u_char ge;
  u_char le;
  struct prefix p;
};

/* Prototypes. */
extern void prefix_list_init (void);
extern void prefix_list_reset (void);
extern void prefix_list_add_hook (void (*func) (struct prefix_list *));
extern void prefix_list_delete_hook (void (*func) (struct prefix_list *));

extern struct prefix_list *prefix_list_lookup (afi_t, const char *);
extern enum prefix_list_type prefix_list_apply (struct prefix_list *, void *);

extern struct stream * prefix_bgp_orf_entry (struct stream *,
                                             struct prefix_list *,
                                             u_char, u_char, u_char);
extern int prefix_bgp_orf_set (char *, afi_t, struct orf_prefix *, int, int);
extern void prefix_bgp_orf_remove_all (char *);
extern int prefix_bgp_show_prefix_list (struct vty *, afi_t, char *);

#ifdef ENABLE_OVSDB
/* Each prefix-list's entry. */
struct prefix_list_entry
{
  int seq;

  int le;
  int ge;

  enum prefix_list_type type;

  int any;
  struct prefix prefix;

  unsigned long refcnt;
  unsigned long hitcnt;

  struct prefix_list_entry *next;
  struct prefix_list_entry *prev;
};

/* List of struct prefix_list. */
struct prefix_list_list
{
  struct prefix_list *head;
  struct prefix_list *tail;
};

/* Master structure of prefix_list. */
struct prefix_master
{
  /* List of prefix_list which name is number. */
  struct prefix_list_list num;

  /* List of prefix_list which name is string. */
  struct prefix_list_list str;

  /* Whether sequential number is used. */
  int seqnum;

  /* The latest update. */
  struct prefix_list *recent;

  /* Hook function which is executed when new prefix_list is added. */
  void (*add_hook) (struct prefix_list *);

  /* Hook function which is executed when prefix_list is deleted. */
  void (*delete_hook) (struct prefix_list *);
};

extern void prefix_list_entry_add(struct prefix_list *plist,
                                  struct prefix_list_entry *pentry);
extern void prefix_list_entry_free(struct prefix_list_entry *pentry);
extern struct prefix_list * prefix_list_get(afi_t afi, const char *name);

extern struct prefix_list_entry *
prefix_list_entry_make(struct prefix *prefix, enum prefix_list_type type,
                       int seq, int le, int ge, int any);

extern struct prefix_list_entry *
prefix_entry_dup_check(struct prefix_list *plist,
                       struct prefix_list_entry *new);

extern void prefix_list_delete(struct prefix_list *plist);
extern void prefix_list_entry_delete(struct prefix_list *plist,
                                     struct prefix_list_entry *pentry,
                                     int update_list);
extern struct prefix_master *prefix_master_get(afi_t afi);
#endif

#endif /* _QUAGGA_PLIST_H */
