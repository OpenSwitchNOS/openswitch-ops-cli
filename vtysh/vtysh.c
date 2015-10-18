/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * This file is part of GNU Zebra.
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

#ifdef ENABLE_OVSDB
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <arpa/inet.h>
#include <vty_utils.h>
#include "vtysh/vtysh_ovsdb_if.h"
#else
#include <zebra.h>
#endif
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "powersupply_vty.h"
#include "log.h"
#include "bgp_vty.h"
#include "logrotate_vty.h"
#include "fan_vty.h"
#include "temperature_vty.h"
#include "openvswitch/vlog.h"
#include "ovsdb-idl.h"
#include "openswitch-idl.h"
#include <crypt.h>
#include "vswitch-idl.h"

#ifdef ENABLE_OVSDB
#include "intf_vty.h"
#include "vswitch-idl.h"
#include "smap.h"
#include "lldp_vty.h"
#include "vrf_vty.h"
#include "neighbor_vty.h"
#include "intf_vty.h"
#include "l3routes_vty.h"
#include "vlan_vty.h"
#include "led_vty.h"
#include "mgmt_intf_vty.h"
#include "system_vty.h"
#include "lacp_vty.h"
#include "ecmp_vty.h"
#include "dhcp_tftp_vty.h"
#endif

#include "aaa_vty.h"
#include "vtysh_utils.h"
#include <termios.h>


VLOG_DEFINE_THIS_MODULE(vtysh);

#ifdef ENABLE_OVSDB
int enable_mininet_test_prompt = 0;
extern struct ovsdb_idl *idl;
int vtysh_show_startup = 0;

#endif


#define IS_IPV6_GLOBAL_UNICAST(i) !(IN6_IS_ADDR_UNSPECIFIED(i) | IN6_IS_ADDR_LOOPBACK(i) | \
                                    IN6_IS_ADDR_SITELOCAL(i)  |  IN6_IS_ADDR_MULTICAST(i)| \
                                    IN6_IS_ADDR_LINKLOCAL(i))

#define MINIMUM(x,y) (x < y) ? x : y

/* Struct VTY. */
struct vty *vty;

/* VTY shell pager name. */
char *vtysh_pager_name = NULL;

/* VTY shell client structure. */
struct vtysh_client
{
   int fd;
   const char *name;
   int flag;
   const char *path;
} vtysh_client[] =
   {
      { .fd = -1, .name = "zebra", .flag = VTYSH_ZEBRA, .path = ZEBRA_VTYSH_PATH},
      { .fd = -1, .name = "ripd", .flag = VTYSH_RIPD, .path = RIP_VTYSH_PATH},
      { .fd = -1, .name = "ripngd", .flag = VTYSH_RIPNGD, .path = RIPNG_VTYSH_PATH},
      { .fd = -1, .name = "ospfd", .flag = VTYSH_OSPFD, .path = OSPF_VTYSH_PATH},
      { .fd = -1, .name = "ospf6d", .flag = VTYSH_OSPF6D, .path = OSPF6_VTYSH_PATH},
      { .fd = -1, .name = "bgpd", .flag = VTYSH_BGPD, .path = BGP_VTYSH_PATH},
      { .fd = -1, .name = "isisd", .flag = VTYSH_ISISD, .path = ISIS_VTYSH_PATH},
      { .fd = -1, .name = "babeld", .flag = VTYSH_BABELD, .path = BABEL_VTYSH_PATH},
      { .fd = -1, .name = "pimd", .flag = VTYSH_PIMD, .path = PIM_VTYSH_PATH},
   };


/* We need direct access to ripd to implement vtysh_exit_ripd_only. */
static struct vtysh_client *ripd_client = NULL;


/* Using integrated config from Quagga.conf. Default is no. */
int vtysh_writeconfig_integrated = 0;

extern char config_default[];

static void
vclient_close (struct vtysh_client *vclient)
{
   if (vclient->fd >= 0)
   {
      fprintf(stderr,
            "Warning: closing connection to %s because of an I/O error!\n",
            vclient->name);
      close (vclient->fd);
      vclient->fd = -1;
   }
}
#ifndef ENABLE_OVSDB
/* Following filled with debug code to trace a problematic condition
 * under load - it SHOULD handle it. */
#define ERR_WHERE_STRING "vtysh(): vtysh_client_config(): "
static int
vtysh_client_config (struct vtysh_client *vclient, char *line)
{
   int ret;
   char *buf;
   size_t bufsz;
   char *pbuf;
   size_t left;
   char *eoln;
   int nbytes;
   int i;
   int readln;

   if (vclient->fd < 0)
      return CMD_SUCCESS;

   ret = write (vclient->fd, line, strlen (line) + 1);
   if (ret <= 0)
   {
      vclient_close (vclient);
      return CMD_SUCCESS;
   }

   /* Allow enough room for buffer to read more than a few pages from socket. */
   bufsz = 5 * getpagesize() + 1;
   buf = XMALLOC(MTYPE_TMP, bufsz);
   memset(buf, 0, bufsz);
   pbuf = buf;

   while (1)
   {
      if (pbuf >= ((buf + bufsz) -1))
      {
         fprintf (stderr, ERR_WHERE_STRING \
               "warning - pbuf beyond buffer end.\n");
         return CMD_WARNING;
      }

      readln = (buf + bufsz) - pbuf - 1;
      nbytes = read (vclient->fd, pbuf, readln);

      if (nbytes <= 0)
      {

         if (errno == EINTR)
            continue;

         fprintf(stderr, ERR_WHERE_STRING "(%u)", errno);
         perror("");

         if (errno == EAGAIN || errno == EIO)
            continue;

         vclient_close (vclient);
         XFREE(MTYPE_TMP, buf);
         return CMD_SUCCESS;
      }

      pbuf[nbytes] = '\0';

      if (nbytes >= 4)
      {
         i = nbytes - 4;
         if (pbuf[i] == '\0' && pbuf[i + 1] == '\0' && pbuf[i + 2] == '\0')
         {
            ret = pbuf[i + 3];
            break;
         }
      }
      pbuf += nbytes;

      /* See if a line exists in buffer, if so parse and consume it, and
       * reset read position. */
      if ((eoln = strrchr(buf, '\n')) == NULL)
         continue;

      if (eoln >= ((buf + bufsz) - 1))
      {
         fprintf (stderr, ERR_WHERE_STRING \
               "warning - eoln beyond buffer end.\n");
      }
      vtysh_config_parse(buf);

      eoln++;
      left = (size_t)(buf + bufsz - eoln);
      memmove(buf, eoln, left);
      buf[bufsz-1] = '\0';
      pbuf = buf + strlen(buf);
   }

   /* Parse anything left in the buffer. */

   vtysh_config_parse (buf);

   XFREE(MTYPE_TMP, buf);
   return ret;
}
#endif
static int
vtysh_client_execute (struct vtysh_client *vclient, const char *line, FILE *fp)
{
   int ret;
   char buf[1001];
   int nbytes;
   int i;
   int numnulls = 0;

   if (vclient->fd < 0)
      return CMD_SUCCESS;

   ret = write (vclient->fd, line, strlen (line) + 1);
   if (ret <= 0)
   {
      vclient_close (vclient);
      return CMD_SUCCESS;
   }

   while (1)
   {
      nbytes = read (vclient->fd, buf, sizeof(buf)-1);

      if (nbytes <= 0 && errno != EINTR)
      {
         vclient_close (vclient);
         return CMD_SUCCESS;
      }

      if (nbytes > 0)
      {
         if ((numnulls == 3) && (nbytes == 1))
            return buf[0];

         buf[nbytes] = '\0';
         fputs (buf, fp);
         fflush (fp);

         /* check for trailling \0\0\0<ret code>,
          * even if split across reads
          * (see lib/vty.c::vtysh_read)
          */
         if (nbytes >= 4)
         {
            i = nbytes-4;
            numnulls = 0;
         }
         else
            i = 0;

         while (i < nbytes && numnulls < 3)
         {
            if (buf[i++] == '\0')
               numnulls++;
            else
               numnulls = 0;
         }

         /* got 3 or more trailing NULs? */
         if ((numnulls >= 3) && (i < nbytes))
            return (buf[nbytes-1]);
      }
   }
}

void
vtysh_exit_ripd_only (void)
{
   if (ripd_client)
      vtysh_client_execute (ripd_client, "exit", stdout);
}


void
vtysh_pager_init (void)
{
   char *pager_defined;

   pager_defined = getenv ("VTYSH_PAGER");

   if (pager_defined)
      vtysh_pager_name = strdup (pager_defined);
   else
      vtysh_pager_name = strdup ("more");
}

/* Command execution over the vty interface. */
static int
vtysh_execute_func (const char *line, int pager)
{
   int ret, cmd_stat;
   u_int i;
   vector vline;
   struct cmd_element *cmd;
   FILE *fp = NULL;
   int closepager = 0;
   int tried = 0;
   int saved_ret, saved_node;

   /* Split readline string up into the vector. */
   vline = cmd_make_strvec (line);

   if (vline == NULL)
      return CMD_SUCCESS;

   saved_ret = ret = cmd_execute_command (vline, vty, &cmd, 1);
   saved_node = vty->node;

   /* If command doesn't succeeded in current node, try to walk up in node tree.
    * Changing vty->node is enough to try it just out without actual walkup in
    * the vtysh. */
   while (ret != CMD_SUCCESS && ret != CMD_SUCCESS_DAEMON && ret != CMD_WARNING
#ifdef ENABLE_OVSDB
         && ret != CMD_OVSDB_FAILURE
#endif
         && vty->node > CONFIG_NODE)
   {
      vty->node = node_parent(vty->node);
      ret = cmd_execute_command (vline, vty, &cmd, 1);
      tried++;
   }

   /* if the command succeeds in any other node than current, it is not always
    * necessary to move to parent context but to remain with the vty context
    * after execution of the command */
#ifndef ENABLE_OVSDB
   vty->node = saved_node;

   /* If command succeeded in any other node than current (tried > 0) we have
    * to move into node in the vtysh where it succeeded. */
   if (ret == CMD_SUCCESS || ret == CMD_SUCCESS_DAEMON || ret == CMD_WARNING)
   {
      if ((saved_node == BGP_VPNV4_NODE || saved_node == BGP_IPV4_NODE
            || saved_node == BGP_IPV6_NODE || saved_node == BGP_IPV4M_NODE
            || saved_node == BGP_IPV6M_NODE)
            && (tried == 1))
      {
         vtysh_execute("exit-address-family");
      }
      else if ((saved_node == KEYCHAIN_KEY_NODE) && (tried == 1))
      {
         vtysh_execute("exit");
      }
      else if (tried)
      {
         vtysh_execute ("end");
         vtysh_execute ("configure terminal");
      }
   }
   /* If command didn't succeed in any node, continue with return value from
    * first try. */
   else if (tried)
   {
      ret = saved_ret;
   }
#else

   if (ret != CMD_SUCCESS && tried)
   {
      vty->node = saved_node;
      ret = saved_ret;
   }

#endif

   cmd_free_strvec (vline);

   cmd_stat = ret;
   switch (ret)
   {
#ifdef ENABLE_OVSDB
      case CMD_OVSDB_FAILURE:
         fprintf (stdout,"%% Command failed.\n");
         break;
#endif
      case CMD_WARNING:
         if (vty->type == VTY_FILE)
            fprintf (stdout,"Warning...\n");
         break;
      case CMD_ERR_AMBIGUOUS:
         fprintf (stdout,"%% Ambiguous command.\n");
         break;
      case CMD_ERR_NO_MATCH:
         fprintf (stdout,"%% Unknown command.\n");
         break;
      case CMD_ERR_INCOMPLETE:
         fprintf (stdout,"%% Command incomplete.\n");
         break;
      case CMD_SUCCESS_DAEMON:
         {
            /* FIXME: Don't open pager for exit commands. popen() causes problems
             * if exited from vtysh at all. This hack shouldn't cause any problem
             * but is really ugly. */
            if (pager && vtysh_pager_name && (strncmp(line, "exit", 4) != 0))
            {
               fp = popen (vtysh_pager_name, "w");
               if (fp == NULL)
               {
                  perror ("popen failed for pager");
                  fp = stdout;
               }
               else
                  closepager=1;
            }
            else
               fp = stdout;

            if (! strcmp(cmd->string,"configure terminal"))
            {
               for (i = 0; i < array_size(vtysh_client); i++)
               {
                  cmd_stat = vtysh_client_execute(&vtysh_client[i], line, fp);
                  if (cmd_stat == CMD_WARNING)
                     break;
               }

               if (cmd_stat)
               {
                  line = "end";
                  vline = cmd_make_strvec (line);

                  if (vline == NULL)
                  {
                     if (pager && vtysh_pager_name && fp && closepager)
                     {
                        if (pclose (fp) == -1)
                        {
                           perror ("pclose failed for pager");
                        }
                        fp = NULL;
                     }
                     return CMD_SUCCESS;
                  }

                  ret = cmd_execute_command (vline, vty, &cmd, 1);
                  cmd_free_strvec (vline);
                  if (ret != CMD_SUCCESS_DAEMON)
                     break;
               }
               else
                  if (cmd->func)
                  {
                     (*cmd->func) (cmd, vty, 0, 0, NULL);
                     break;
                  }
            }

            cmd_stat = CMD_SUCCESS;
            for (i = 0; i < array_size(vtysh_client); i++)
            {
               if (cmd->daemon & vtysh_client[i].flag)
               {
                  cmd_stat = vtysh_client_execute(&vtysh_client[i], line, fp);
                  if (cmd_stat != CMD_SUCCESS)
                     break;
               }
            }
            if (cmd_stat != CMD_SUCCESS)
               break;

            if (cmd->func)
               (*cmd->func) (cmd, vty, 0, 0, NULL);
         }
   }
   if (pager && vtysh_pager_name && fp && closepager)
   {
      if (pclose (fp) == -1)
      {
         perror ("pclose failed for pager");
      }
      fp = NULL;
   }
   return cmd_stat;
}

int
vtysh_execute_no_pager (const char *line)
{
   return vtysh_execute_func (line, 0);
}

int
vtysh_execute (const char *line)
{
   return vtysh_execute_func (line, 1);
}

/* Configration make from file. */
int
vtysh_config_from_file (struct vty *vty, FILE *fp)
{
   int ret;
   vector vline;
   struct cmd_element *cmd;

   while (fgets (vty->buf, VTY_BUFSIZ, fp))
   {
      if (vty->buf[0] == '!' || vty->buf[1] == '#')
         continue;

      vline = cmd_make_strvec (vty->buf);

      /* In case of comment line. */
      if (vline == NULL)
         continue;

      /* Execute configuration command : this is strict match. */
      ret = cmd_execute_command_strict (vline, vty, &cmd);

      /* Try again with setting node to CONFIG_NODE. */
      if (ret != CMD_SUCCESS
            && ret != CMD_SUCCESS_DAEMON
            && ret != CMD_WARNING)
      {
         if (vty->node == KEYCHAIN_KEY_NODE)
         {
            vty->node = KEYCHAIN_NODE;
            vtysh_exit_ripd_only ();
            ret = cmd_execute_command_strict (vline, vty, &cmd);

            if (ret != CMD_SUCCESS
                  && ret != CMD_SUCCESS_DAEMON
                  && ret != CMD_WARNING)
            {
               vtysh_exit_ripd_only ();
               vty->node = CONFIG_NODE;
               ret = cmd_execute_command_strict (vline, vty, &cmd);
            }
         }
         else
         {
            vtysh_execute ("end");
            vtysh_execute ("configure terminal");
            vty->node = CONFIG_NODE;
            ret = cmd_execute_command_strict (vline, vty, &cmd);
         }
      }

      cmd_free_strvec (vline);

      switch (ret)
      {
         case CMD_WARNING:
            if (vty->type == VTY_FILE)
               fprintf (stdout,"Warning...\n");
            break;
         case CMD_ERR_AMBIGUOUS:
            fprintf (stdout,"%% Ambiguous command.\n");
            break;
         case CMD_ERR_NO_MATCH:
            fprintf (stdout,"%% Unknown command: %s", vty->buf);
            break;
         case CMD_ERR_INCOMPLETE:
            fprintf (stdout,"%% Command incomplete.\n");
            break;
         case CMD_SUCCESS_DAEMON:
            {
               u_int i;
               int cmd_stat = CMD_SUCCESS;

               for (i = 0; i < array_size(vtysh_client); i++)
               {
                  if (cmd->daemon & vtysh_client[i].flag)
                  {
                     cmd_stat = vtysh_client_execute (&vtysh_client[i],
                           vty->buf, stdout);
                     if (cmd_stat != CMD_SUCCESS)
                        break;
                  }
               }
               if (cmd_stat != CMD_SUCCESS)
                  break;

               if (cmd->func)
                  (*cmd->func) (cmd, vty, 0, 0, NULL);
            }
      }
   }
   return CMD_SUCCESS;
}

/* We don't care about the point of the cursor when '?' is typed. */
int
vtysh_rl_describe (void)
{
   int ret;
   unsigned int i;
   vector vline;
   vector describe;
   int width;
   struct cmd_token *token;

   vline = cmd_make_strvec (rl_line_buffer);

   /* In case of '> ?'. */
   if (vline == NULL)
   {
      vline = vector_init (1);
      vector_set (vline, '\0');
   }
   else
      if (rl_end && isspace ((int) rl_line_buffer[rl_end - 1]))
         vector_set (vline, '\0');

   describe = cmd_describe_command (vline, vty, &ret);

   fprintf (stdout,"\n");

   /* Ambiguous and no match error. */
   switch (ret)
   {
      case CMD_ERR_AMBIGUOUS:
         cmd_free_strvec (vline);
         fprintf (stdout,"%% Ambiguous command.\n");
         rl_on_new_line ();
         return 0;
         break;
      case CMD_ERR_NO_MATCH:
         cmd_free_strvec (vline);
         fprintf (stdout,"%% There is no matched command.\n");
         rl_on_new_line ();
         return 0;
         break;
   }

   /* Get width of command string. */
   width = 0;
   for (i = 0; i < vector_active (describe); i++)
      if ((token = vector_slot (describe, i)) != NULL)
      {
         int len;

         if (token->cmd[0] == '\0')
            continue;

         len = strlen (token->cmd);
         if (token->cmd[0] == '.')
            len--;

         if (width < len)
            width = len;
      }
#ifndef ENABLE_OVSDB

   for (i = 0; i < vector_active (describe); i++)
      if ((token = vector_slot (describe, i)) != NULL)
      {
         if (token->cmd[0] == '\0')
            continue;

         if (! token->desc)
            fprintf (stdout,"  %-s\n",
                  token->cmd[0] == '.' ? token->cmd + 1 : token->cmd);
         else
            fprintf (stdout,"  %-*s  %s\n",
                  width,
                  token->cmd[0] == '.' ? token->cmd + 1 : token->cmd,
                  token->desc);
      }
#else
  utils_vtysh_rl_describe_output(vty, describe, width);
#endif

   cmd_free_strvec (vline);
   vector_free (describe);

   rl_on_new_line();

   return 0;
}

/* Result of cmd_complete_command() call will be stored here
 * and used in new_completion() in order to put the space in
 * correct places only. */
int complete_status;

static char *
command_generator (const char *text, int state)
{
   vector vline;
   static char **matched = NULL;
   static int index = 0;

   /* First call. */
   if (! state)
   {
      index = 0;

      if (vty->node == AUTH_NODE || vty->node == AUTH_ENABLE_NODE)
         return NULL;

      vline = cmd_make_strvec (rl_line_buffer);
      if (vline == NULL)
         return NULL;

      if (rl_end && isspace ((int) rl_line_buffer[rl_end - 1]))
         vector_set (vline, '\0');

      matched = cmd_complete_command (vline, vty, &complete_status);
   }

   if (matched && matched[index])
      return matched[index++];

   return NULL;
}

static char **
new_completion (char *text, int start, int end)
{
   char **matches;

   matches = rl_completion_matches (text, command_generator);

   if (matches)
   {
      rl_point = rl_end;
      if (complete_status != CMD_COMPLETE_FULL_MATCH)
         /* only append a space on full match */
         rl_completion_append_character = '\0';
   }

   return matches;
}

#if 0
/* This function is not actually being used. */
static char **
vtysh_completion (char *text, int start, int end)
{
   int ret;
   vector vline;
   char **matched = NULL;

   if (vty->node == AUTH_NODE || vty->node == AUTH_ENABLE_NODE)
      return NULL;

   vline = cmd_make_strvec (rl_line_buffer);
   if (vline == NULL)
      return NULL;

   /* In case of 'help \t'. */
   if (rl_end && isspace ((int) rl_line_buffer[rl_end - 1]))
      vector_set (vline, '\0');

   matched = cmd_complete_command (vline, vty, &ret);

   cmd_free_strvec (vline);

   return (char **) matched;
}
#endif

/* Vty node structures. */
static struct cmd_node bgp_node =
   {
      BGP_NODE,
      "%s(config-router)# ",
   };

#ifndef ENABLE_OVSDB
static struct cmd_node rip_node =
   {
      RIP_NODE,
      "%s(config-router)# ",
   };

static struct cmd_node isis_node =
   {
      ISIS_NODE,
      "%s(config-router)# ",
   };
#endif

static struct cmd_node interface_node =
   {
      INTERFACE_NODE,
      "%s(config-if)# ",
   };

#ifdef ENABLE_OVSDB
static struct cmd_node mgmt_interface_node =
{
  MGMT_INTERFACE_NODE,
  "%s(config-if-mgmt)# ",
};

static struct cmd_node link_aggregation_node =
{
  LINK_AGGREGATION_NODE,
  "%s(config-lag-if)# ",
};

static struct cmd_node vlan_node =
{
  VLAN_NODE,
  "%s(config-vlan)# ",
};

#endif

static struct cmd_node rmap_node =
   {
      RMAP_NODE,
      "%s(config-route-map)# "
   };

static struct cmd_node zebra_node =
   {
      ZEBRA_NODE,
      "%s(config-router)# "
   };

static struct cmd_node bgp_vpnv4_node =
   {
      BGP_VPNV4_NODE,
      "%s(config-router-af)# "
   };

static struct cmd_node bgp_ipv4_node =
   {
      BGP_IPV4_NODE,
      "%s(config-router-af)# "
   };

static struct cmd_node bgp_ipv4m_node =
   {
      BGP_IPV4M_NODE,
      "%s(config-router-af)# "
   };

static struct cmd_node bgp_ipv6_node =
   {
      BGP_IPV6_NODE,
      "%s(config-router-af)# "
   };

static struct cmd_node bgp_ipv6m_node =
   {
      BGP_IPV6M_NODE,
      "%s(config-router-af)# "
   };

#ifndef ENABLE_OVSDB
static struct cmd_node ospf_node =
   {
      OSPF_NODE,
      "%s(config-router)# "
   };

static struct cmd_node ripng_node =
   {
      RIPNG_NODE,
      "%s(config-router)# "
   };

static struct cmd_node ospf6_node =
   {
      OSPF6_NODE,
      "%s(config-ospf6)# "
   };

static struct cmd_node babel_node =
   {
      BABEL_NODE,
      "%s(config-babel)# "
   };
#endif

static struct cmd_node keychain_node =
   {
      KEYCHAIN_NODE,
      "%s(config-keychain)# "
   };

static struct cmd_node keychain_key_node =
   {
      KEYCHAIN_KEY_NODE,
      "%s(config-keychain-key)# "
   };

#ifdef ENABLE_OVSDB
static struct cmd_node vlan_interface_node =
{
  VLAN_INTERFACE_NODE,
  "%s(config-if-vlan)# ",
};

static struct cmd_node dhcp_server_node =
{
  DHCP_SERVER_NODE,
  "%s(config-dhcp-server)# ",
};

static struct cmd_node tftp_server_node =
{
  TFTP_SERVER_NODE,
  "%s(config-tftp-server)# ",
};

#endif
/* Defined in lib/vty.c */
extern struct cmd_node vty_node;

/* When '^Z' is received from vty, move down to the enable mode. */
int
vtysh_end (void)
{
   switch (vty->node)
   {
      case VIEW_NODE:
      case ENABLE_NODE:
         /* Nothing to do. */
         break;
      default:
         vty->node = ENABLE_NODE;
         break;
   }
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_end_all,
      vtysh_end_all_cmd,
      "end",
      "End current mode and change to enable mode\n")
{
   return vtysh_end ();
}
#if 0
DEFUNSH (VTYSH_BGPD,
      router_bgp,
      router_bgp_cmd,
      "router bgp " CMD_AS_RANGE,
      ROUTER_STR
      BGP_STR
      AS_STR)
{
   vty->node = BGP_NODE;
   return CMD_SUCCESS;
}
#endif
#if 0
ALIAS_SH (VTYSH_BGPD,
      router_bgp,
      router_bgp_view_cmd,
      "router bgp " CMD_AS_RANGE " view WORD",
      ROUTER_STR
      BGP_STR
      AS_STR
      "BGP view\n"
      "view name\n")
#endif
DEFUNSH (VTYSH_BGPD,
      address_family_vpnv4,
      address_family_vpnv4_cmd,
      "address-family vpnv4",
      "Enter Address Family command mode\n"
      "Address family\n")
{
   vty->node = BGP_VPNV4_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_vpnv4_unicast,
      address_family_vpnv4_unicast_cmd,
      "address-family vpnv4 unicast",
      "Enter Address Family command mode\n"
      "Address family\n"
      "Address Family Modifier\n")
{
   vty->node = BGP_VPNV4_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_ipv4_unicast,
      address_family_ipv4_unicast_cmd,
      "address-family ipv4 unicast",
      "Enter Address Family command mode\n"
      "Address family\n"
      "Address Family Modifier\n")
{
   vty->node = BGP_IPV4_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_ipv4_multicast,
      address_family_ipv4_multicast_cmd,
      "address-family ipv4 multicast",
      "Enter Address Family command mode\n"
      "Address family\n"
      "Address Family Modifier\n")
{
   vty->node = BGP_IPV4M_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_ipv6,
      address_family_ipv6_cmd,
      "address-family ipv6",
      "Enter Address Family command mode\n"
      "Address family\n")
{
   vty->node = BGP_IPV6_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_ipv6_unicast,
      address_family_ipv6_unicast_cmd,
      "address-family ipv6 unicast",
      "Enter Address Family command mode\n"
      "Address family\n"
      "Address Family Modifier\n")
{
   vty->node = BGP_IPV6_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BGPD,
      address_family_ipv6_multicast,
      address_family_ipv6_multicast_cmd,
      "address-family ipv6 multicast",
      "Enter Address Family command mode\n"
      "Address family\n"
      "Address Family Modifier\n")
{
   vty->node = BGP_IPV6M_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_RIPD,
      key_chain,
      key_chain_cmd,
      "key chain WORD",
      "Authentication key management\n"
      "Key-chain management\n"
      "Key-chain name\n")
{
   vty->node = KEYCHAIN_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_RIPD,
      key,
      key_cmd,
      "key <0-2147483647>",
      "Configure a key\n"
      "Key identifier number\n")
{
   vty->node = KEYCHAIN_KEY_NODE;
   return CMD_SUCCESS;
}

#ifndef ENABLE_OVSDB
DEFUNSH (VTYSH_RIPD,
      router_rip,
      router_rip_cmd,
      "router rip",
      ROUTER_STR
      "RIP")
{
   vty->node = RIP_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_RIPNGD,
      router_ripng,
      router_ripng_cmd,
      "router ripng",
      ROUTER_STR
      "RIPng")
{
   vty->node = RIPNG_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_OSPFD,
      router_ospf,
      router_ospf_cmd,
      "router ospf",
      "Enable a routing process\n"
      "Start OSPF configuration\n")
{
   vty->node = OSPF_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_OSPF6D,
      router_ospf6,
      router_ospf6_cmd,
      "router ospf6",
      OSPF6_ROUTER_STR
      OSPF6_STR)
{
   vty->node = OSPF6_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_BABELD,
      router_babel,
      router_babel_cmd,
      "router babel",
      ROUTER_STR
      "Babel")
{
   vty->node = BABEL_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ISISD,
      router_isis,
      router_isis_cmd,
      "router isis WORD",
      ROUTER_STR
      "ISO IS-IS\n"
      "ISO Routing area tag")
{
   vty->node = ISIS_NODE;
   return CMD_SUCCESS;
}
#endif

#if 0
DEFUNSH (VTYSH_RMAP,
      route_map,
      route_map_cmd,
      "route-map WORD (deny|permit) <1-65535>",
      "Create route-map or enter route-map command mode\n"
      "Route map tag\n"
      "Route map denies set operations\n"
      "Route map permits set operations\n"
      "Sequence to insert to/delete from existing route-map entry\n")
{
   vty->node = RMAP_NODE;
   return CMD_SUCCESS;
}
#endif
#ifndef ENABLE_OVSDB
DEFUNSH (VTYSH_ALL,
      vtysh_line_vty,
      vtysh_line_vty_cmd,
      "line vty",
      "Configure a terminal line\n"
      "Virtual terminal\n")
{
   vty->node = VTY_NODE;
   return CMD_SUCCESS;
}
#endif

DEFUNSH (VTYSH_ALL,
      vtysh_enable,
      vtysh_enable_cmd,
      "enable",
      "Turn on privileged mode command\n")
{
   vty->node = ENABLE_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_disable,
      vtysh_disable_cmd,
      "disable",
      "Turn off privileged mode command\n")
{
   if (vty->node == ENABLE_NODE)
      vty->node = VIEW_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_config_terminal,
      vtysh_config_terminal_cmd,
      "configure terminal",
      "Configuration from vty interface\n"
      "Configuration terminal\n")
{
   vty->node = CONFIG_NODE;
   return CMD_SUCCESS;
}

static int
vtysh_exit (struct vty *vty)
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
      exit (0);
      break;
    case CONFIG_NODE:
      vty->node = ENABLE_NODE;
      break;
    case INTERFACE_NODE:
#ifdef ENABLE_OVSDB
    case VLAN_NODE:
    case MGMT_INTERFACE_NODE:
    case VLAN_INTERFACE_NODE:
    case LINK_AGGREGATION_NODE:
    case DHCP_SERVER_NODE:
    case TFTP_SERVER_NODE:
#endif
    case ZEBRA_NODE:
    case BGP_NODE:
    case RIP_NODE:
    case RIPNG_NODE:
    case OSPF_NODE:
    case OSPF6_NODE:
    case BABEL_NODE:
    case ISIS_NODE:
    case MASC_NODE:
    case RMAP_NODE:
    case VTY_NODE:
    case KEYCHAIN_NODE:
      vtysh_execute("end");
      vtysh_execute("configure terminal");
      vty->node = CONFIG_NODE;
      break;
    case BGP_VPNV4_NODE:
    case BGP_IPV4_NODE:
    case BGP_IPV4M_NODE:
    case BGP_IPV6_NODE:
    case BGP_IPV6M_NODE:
      vty->node = BGP_NODE;
      break;
    case KEYCHAIN_KEY_NODE:
      vty->node = KEYCHAIN_NODE;
      break;
    default:
      break;
    }
  return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_exit_all,
      vtysh_exit_all_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_all,
      vtysh_quit_all_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_BGPD,
      exit_address_family,
      exit_address_family_cmd,
      "exit-address-family",
      "Exit from Address Family configuration mode\n")
{
   if (vty->node == BGP_IPV4_NODE
         || vty->node == BGP_IPV4M_NODE
         || vty->node == BGP_VPNV4_NODE
         || vty->node == BGP_IPV6_NODE
         || vty->node == BGP_IPV6M_NODE)
      vty->node = BGP_NODE;
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ZEBRA,
      vtysh_exit_zebra,
      vtysh_exit_zebra_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_zebra,
      vtysh_quit_zebra_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_RIPD,
      vtysh_exit_ripd,
      vtysh_exit_ripd_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}
#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_ripd,
      vtysh_quit_ripd_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_RIPNGD,
      vtysh_exit_ripngd,
      vtysh_exit_ripngd_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}
#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_ripngd,
      vtysh_quit_ripngd_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_RMAP,
      vtysh_exit_rmap,
      vtysh_exit_rmap_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_rmap,
      vtysh_quit_rmap_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_BGPD,
      vtysh_exit_bgpd,
      vtysh_exit_bgpd_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_bgpd,
      vtysh_quit_bgpd_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_OSPFD,
      vtysh_exit_ospfd,
      vtysh_exit_ospfd_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_ospfd,
      vtysh_quit_ospfd_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_OSPF6D,
      vtysh_exit_ospf6d,
      vtysh_exit_ospf6d_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_ospf6d,
      vtysh_quit_ospf6d_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_ISISD,
      vtysh_exit_isisd,
      vtysh_exit_isisd_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_isisd,
      vtysh_quit_isisd_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUNSH (VTYSH_ALL,
      vtysh_exit_line_vty,
      vtysh_exit_line_vty_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_line_vty,
      vtysh_quit_line_vty_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif


#ifdef ENABLE_OVSDB
DEFUN (vtysh_dhcp_server,
      vtysh_dhcp_server_cmd,
      "dhcp-server",
      "DHCP Server Configuration\n")
{

   vty->node = DHCP_SERVER_NODE;

   return CMD_SUCCESS;
}

DEFUN (vtysh_exit_dhcp_server,
      vtysh_exit_dhcp_server_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_dhcp_server,
      vtysh_quit_dhcp_server_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

DEFUN (vtysh_tftp_server,
      vtysh_tftp_server_cmd,
      "tftp-server",
      "TFTP Server Configuration\n")
{

   vty->node = TFTP_SERVER_NODE;

   return CMD_SUCCESS;
}

DEFUN (vtysh_exit_tftp_server,
      vtysh_exit_tftp_server_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_tftp_server,
      vtysh_quit_tftp_server_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif
DEFUN (vtysh_interface,
      vtysh_interface_cmd,
      "interface IFNAME",
      "Select an interface to configure\n"
      "Interface's name\n")
{
  vty->node = INTERFACE_NODE;
  static char ifnumber[MAX_IFNAME_LENGTH];

  if (VERIFY_VLAN_IFNAME(argv[0]) == 0) {
  vty->node = VLAN_INTERFACE_NODE;
      GET_VLANIF(ifnumber, argv[0]);
      if (create_vlan_interface(ifnumber) == CMD_OVSDB_FAILURE) {
          return CMD_OVSDB_FAILURE;
      }
  }
  else if (strlen(argv[0]) < MAX_IFNAME_LENGTH)
  {
    strncpy(ifnumber, argv[0], MAX_IFNAME_LENGTH);
  }
  else
  {
    return CMD_ERR_NO_MATCH;
  }
  VLOG_DBG("%s ifnumber = %s\n", __func__, ifnumber);
  vty->index = ifnumber;
  return CMD_SUCCESS;
}

DEFUN (vtysh_interface_vlan,
       vtysh_interface_vlan_cmd,
       "interface vlan VLANID",
       "Select an interface to configure\n"
        VLAN_STR
       "Vlan id within <1-4094> and should not be an internal vlan\n")
{
   vty->node = VLAN_INTERFACE_NODE;
   static char vlan_if[MAX_IFNAME_LENGTH];

   VLANIF_NAME(vlan_if, argv[0]);

   if ((verify_ifname(vlan_if) == 0)) {
       vty->node = CONFIG_NODE;
       return CMD_ERR_NO_MATCH;
   }

   VLOG_DBG("%s vlan interface = %s\n", __func__, vlan_if);

   if (create_vlan_interface(vlan_if) == CMD_OVSDB_FAILURE) {
       vty->node = CONFIG_NODE;
       return CMD_ERR_NO_MATCH;
   }
   vty->index = vlan_if;

   return CMD_SUCCESS;
}

DEFUN (no_vtysh_interface,
      no_vtysh_interface_cmd,
      "no interface IFNAME",
      NO_STR
      "Delete a pseudo interface's configuration\n"
      "Interface's name\n")
{
  vty->node = CONFIG_NODE;
  static char ifnumber[MAX_IFNAME_LENGTH];

  if (VERIFY_VLAN_IFNAME(argv[0]) == 0) {
      GET_VLANIF(ifnumber, argv[0]);
      if (delete_vlan_interface(ifnumber) == CMD_OVSDB_FAILURE) {
          return CMD_OVSDB_FAILURE;
      }
  }
  else if (strlen(argv[0]) < MAX_IFNAME_LENGTH)
  {
    strncpy(ifnumber, argv[0], MAX_IFNAME_LENGTH);
  }
  else
  {
    return CMD_ERR_NO_MATCH;
  }
  vty->index = ifnumber;
  return CMD_SUCCESS;
}

DEFUN (no_vtysh_interface_vlan,
       no_vtysh_interface_vlan_cmd,
       "no interface vlan VLANID",
       NO_STR
       "Delete a pseudo interface's configuration\n"
       "VLAN interface\n"
       "Vlan id within <1-4094> and should not be an internal vlan\n")
{
   vty->node = CONFIG_NODE;
   static char vlan_if[MAX_IFNAME_LENGTH];

   VLANIF_NAME(vlan_if, argv[0]);

   if ((verify_ifname(vlan_if) == 0)) {
       return CMD_OVSDB_FAILURE;
   }

   VLOG_DBG("%s: vlan interface = %s\n", __func__, vlan_if);

   if (delete_vlan_interface(vlan_if) == CMD_OVSDB_FAILURE) {
       return CMD_OVSDB_FAILURE;
   }
   vty->index = vlan_if;

   return CMD_SUCCESS;
}

DEFUN(vtysh_vlan,
    vtysh_vlan_cmd,
    "vlan <1-4094>",
    VLAN_STR
    "VLAN identifier\n")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_bridge *default_bridge_row = NULL;
    bool vlan_found = false;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_vlan **vlans = NULL;
    int i = 0;
    int vlan_id = atoi(argv[0]);
    static char vlan[5] = { 0 };
    static char vlan_name[9] = { 0 };
    snprintf(vlan, 5, "%s", argv[0]);
    snprintf(vlan_name, 9, "%s%s", "vlan", argv[0]);

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row != NULL)
    {
        OVSREC_VLAN_FOR_EACH(vlan_row, idl)
        {
            if (vlan_row->id == vlan_id)
            {
                vlan_found = true;
                break;
            }
        }
    }

    if (!vlan_found)
    {
        status_txn = cli_do_config_start();

        if (status_txn == NULL)
        {
            VLOG_DBG("Transaction creation failed by cli_do_config_start().Function=%s, Line=%d", __func__, __LINE__);
            cli_do_config_abort(status_txn);
            vty_out(vty, "Failed to create the vlan%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

        vlan_row = ovsrec_vlan_insert(status_txn);
        ovsrec_vlan_set_id(vlan_row, vlan_id);
        ovsrec_vlan_set_name(vlan_row, vlan_name);
        ovsrec_vlan_set_admin(vlan_row, OVSREC_VLAN_ADMIN_DOWN);
        ovsrec_vlan_set_oper_state(vlan_row, OVSREC_VLAN_OPER_STATE_DOWN);
        ovsrec_vlan_set_oper_state_reason(vlan_row, OVSREC_VLAN_OPER_STATE_REASON_ADMIN_DOWN);

        default_bridge_row = ovsrec_bridge_first(idl);
        if (default_bridge_row != NULL)
        {
            OVSREC_BRIDGE_FOR_EACH(bridge_row, idl)
            {
                if (strcmp(bridge_row->name, DEFAULT_BRIDGE_NAME) == 0)
                {
                    default_bridge_row = (struct ovsrec_bridge*)bridge_row;
                    break;
                }
            }

            if (default_bridge_row == NULL)
            {
                VLOG_DBG("Couldn't find default bridge. Function=%s, Line=%d", __func__, __LINE__);
                cli_do_config_abort(status_txn);
                vty_out(vty, "Failed to create the vlan%s", VTY_NEWLINE);
                return CMD_SUCCESS;
            }
        }

        vlans = xmalloc(sizeof(*default_bridge_row->vlans) *
            (default_bridge_row->n_vlans + 1));
        for (i = 0; i < default_bridge_row->n_vlans; i++)
        {
            vlans[i] = default_bridge_row->vlans[i];
        }
        vlans[default_bridge_row->n_vlans] = CONST_CAST(struct ovsrec_vlan*,vlan_row);
        ovsrec_bridge_set_vlans(default_bridge_row, vlans,
            default_bridge_row->n_vlans + 1);

        status = cli_do_config_finish(status_txn);
        free(vlans);
        if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
        {
            vty->node = VLAN_NODE;
            vty->index = (char *) vlan;
        }
        else
        {
            VLOG_DBG("Transaction failed to create vlan. Function:%s, LINE:%d", __func__, __LINE__);
            vty_out(vty, "Failed to create the vlan%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }
    else
    {
        vty->node = VLAN_NODE;
        vty->index = (char *) vlan;
        return CMD_SUCCESS;
    }
    return CMD_SUCCESS;
}

DEFUN(vtysh_no_vlan,
    vtysh_no_vlan_cmd,
    "no vlan <1-4094>",
    NO_STR
    VLAN_STR
    "VLAN Identifier\n")
{
    const struct ovsrec_vlan *vlan_row = NULL;
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_bridge *default_bridge_row = NULL;
    bool vlan_found = false;
    struct ovsdb_idl_txn *status_txn = NULL;
    enum ovsdb_idl_txn_status status;
    struct ovsrec_vlan **vlans = NULL;
    int i = 0, n = 0;
    int vlan_id = atoi(argv[0]);
    static char vlan_name[9] = { 0 };

    snprintf(vlan_name, 9, "%s%s", "vlan", argv[0]);

    vlan_row = ovsrec_vlan_first(idl);
    if (vlan_row != NULL)
    {
        OVSREC_VLAN_FOR_EACH(vlan_row, idl)
        {
            if (vlan_row->id == vlan_id)
            {
                vlan_found = true;
                break;
            }
        }
    }

    if (vlan_found)
    {
        status_txn = cli_do_config_start();

        if (status_txn == NULL)
        {
            VLOG_DBG("Trasaction creation failed by cli_do_config_start().Function=%s, Line=%d", __func__, __LINE__);
            cli_do_config_abort(status_txn);
            vty_out(vty, "Failed to create the vlan%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

        default_bridge_row = ovsrec_bridge_first(idl);
        if (default_bridge_row != NULL)
        {
            OVSREC_BRIDGE_FOR_EACH(bridge_row, idl)
            {
                if (strcmp(bridge_row->name, DEFAULT_BRIDGE_NAME) == 0)
                {
                    default_bridge_row = (struct ovsrec_bridge*)bridge_row;
                    break;
                }
            }

            if (default_bridge_row == NULL)
            {
                VLOG_DBG("Couldn't find default bridge. Function=%s, Line=%d", __func__, __LINE__);
                cli_do_config_abort(status_txn);
                vty_out(vty, "Failed to create the vlan%s", VTY_NEWLINE);
                return CMD_SUCCESS;
            }
        }

        vlans = xmalloc(sizeof(*default_bridge_row->vlans) *
            (default_bridge_row->n_vlans - 1));
        for (i = n = 0; i < default_bridge_row->n_vlans; i++)
        {
            if (vlan_row != default_bridge_row->vlans[i])
            {
                vlans[n++] = default_bridge_row->vlans[i];
            }
        }
        ovsrec_bridge_set_vlans(default_bridge_row, vlans,
            default_bridge_row->n_vlans - 1);

        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            int64_t* trunks = NULL;
            int trunk_count = port_row->n_trunks;
            for (i = 0; i < port_row->n_trunks; i++)
            {
                if (vlan_id == port_row->trunks[i])
                {
                    trunks = xmalloc(sizeof *port_row->trunks * (port_row->n_trunks - 1));
                    for (i = n = 0; i < port_row->n_trunks; i++)
                    {
                        if (vlan_id != port_row->trunks[i])
                        {
                            trunks[n++] = port_row->trunks[i];
                        }
                    }
                    trunk_count = port_row->n_trunks - 1;
                    ovsrec_port_set_trunks(port_row, trunks, trunk_count);
                    break;
                }
            }
            if (port_row->n_tag == 1 && *port_row->tag == vlan_row->id)
            {
                int64_t* tag = NULL;
                int tag_count = 0;
                ovsrec_port_set_tag(port_row, tag, tag_count);
            }
        }

        ovsrec_vlan_delete(vlan_row);

        status = cli_do_config_finish(status_txn);
        free(vlans);
        if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
        {
            return CMD_SUCCESS;
        }
        else
        {
            VLOG_DBG("Transaction failed to delete vlan. Function:%s, LINE:%d", __func__, __LINE__);
            vty_out(vty, "Failed to delete the vlan%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }
    }
    else
    {
        vty_out(vty, "Couldn't find the VLAN %d. Make sure it's configured%s", vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

DEFUN (vtysh_intf_link_aggregation,
       vtysh_intf_link_aggregation_cmd,
       "interface lag <1-2000>",
       "Select an interface to configure\n"
       "Configure link-aggregation parameters\n"
       "LAG number ranges from 1 to 2000\n")
{
  const struct ovsrec_port *port_row = NULL;
  bool port_found = false;
  struct ovsdb_idl_txn *txn = NULL;
  enum ovsdb_idl_txn_status status_txn;
  static char lag_number[LAG_NAME_LENGTH]={0};
  const struct ovsrec_vrf *default_vrf_row = NULL;
  const struct ovsrec_vrf *vrf_row = NULL;
  int i=0;
  struct ovsrec_port **ports = NULL;

  snprintf(lag_number, LAG_NAME_LENGTH, "%s%s","lag", argv[0]);

  OVSREC_PORT_FOR_EACH(port_row, idl)
  {
    if (strcmp(port_row->name, lag_number) == 0)
    {
      port_found = true;
      break;
    }
  }

  if(!port_found)
  {
    if(maximum_lag_interfaces == MAX_LAG_INTERFACES)
    {
      vty_out(vty, "Cannot create LAG interface. Maximum LAG interface count is already reached.%s",VTY_NEWLINE);
      return CMD_SUCCESS;
    }
    txn = cli_do_config_start();
    if (txn == NULL)
    {
      VLOG_DBG("Transaction creation failed by %s. Function=%s, Line=%d",
               " cli_do_config_start()", __func__, __LINE__);
          cli_do_config_abort(txn);
          return CMD_OVSDB_FAILURE;
    }

    port_row = ovsrec_port_insert(txn);
    ovsrec_port_set_name(port_row, lag_number);

    OVSREC_VRF_FOR_EACH (vrf_row, idl)
    {
        if (strcmp(vrf_row->name, DEFAULT_VRF_NAME) == 0) {
            default_vrf_row = vrf_row;
            break;
        }
    }

    if(default_vrf_row == NULL)
    {
      assert(0);
      VLOG_DBG("Couldn't fetch default VRF row. Function=%s, Line=%d",
                __func__, __LINE__);
      cli_do_config_abort(txn);
      return CMD_OVSDB_FAILURE;
    }

    ports = xmalloc(sizeof *default_vrf_row->ports *
                   (default_vrf_row->n_ports + 1));
    for (i = 0; i < default_vrf_row->n_ports; i++)
    {
      ports[i] = default_vrf_row->ports[i];
    }
    ports[default_vrf_row->n_ports] = CONST_CAST(struct ovsrec_port*,port_row);
    ovsrec_vrf_set_ports(default_vrf_row, ports,
                         default_vrf_row->n_ports + 1);
    free(ports);

    status_txn = cli_do_config_finish(txn);
    if(status_txn == TXN_SUCCESS || status_txn == TXN_UNCHANGED)
    {
      maximum_lag_interfaces++;
      vty->node = LINK_AGGREGATION_NODE;
      vty->index = lag_number;
      return CMD_SUCCESS;
    }
    else
    {
      VLOG_ERR("Transaction commit failed in function=%s, line=%d",__func__,__LINE__);
      return CMD_OVSDB_FAILURE;
    }
  }
  else
  {
    vty->node = LINK_AGGREGATION_NODE;
    vty->index = lag_number;
    return CMD_SUCCESS;
  }
}

DEFUN (vtysh_interface_mgmt,
       vtysh_interface_mgmt_cmd,
       "interface mgmt",
       "Select an interface to configure\n"
       "Configure management interface\n")
{
  vty->node = MGMT_INTERFACE_NODE;
  return CMD_SUCCESS;
}
#else
DEFUNSH (VTYSH_INTERFACE,
      vtysh_interface,
      vtysh_interface_cmd,
      "interface IFNAME",
      "Select an interface to configure\n"
      "Interface's name\n")
{
  vty->node = INTERFACE_NODE;
  static char ifnumber[5];
  if (strlen(argv[0]) < 5)
    memcpy(ifnumber, argv[0], strlen(argv));
  vty->index = ifnumber;
  return CMD_SUCCESS;
}
#endif
#ifndef ENABLE_OVSDB
/* TODO Implement "no interface command in isisd. */
DEFSH (VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_RIPNGD|VTYSH_OSPFD|VTYSH_OSPF6D,
      vtysh_no_interface_cmd,
      "no interface IFNAME",
      NO_STR
      "Delete a pseudo interface's configuration\n"
      "Interface's name\n")
/* TODO Implement interface description commands in ripngd, ospf6d
 * and isisd. */
DEFSH (VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_OSPFD,
      interface_desc_cmd,
      "description .LINE",
      "Interface specific description\n"
      "Characters describing this interface\n")

DEFSH (VTYSH_ZEBRA|VTYSH_RIPD|VTYSH_OSPFD,
      no_interface_desc_cmd,
      "no description",
      NO_STR
      "Interface specific description\n")
#endif

DEFUNSH (VTYSH_INTERFACE,
      vtysh_exit_interface,
      vtysh_exit_interface_cmd,
      "exit",
      "Exit current mode and down to previous mode\n")
{
   return vtysh_exit (vty);
}

#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_interface,
      vtysh_quit_interface_cmd,
      "quit",
      "Exit current mode and down to previous mode\n")
#endif

#ifdef ENABLE_OVSDB
DEFUNSH (VTYSH_MGMT_INTF,
             vtysh_exit_mgmt_interface,
             vtysh_exit_mgmt_interface_cmd,
             "exit",
             "Exit current mode and down to previous mode\n")
{
  return vtysh_exit (vty);
}
#ifndef ENABLE_OVSDB
ALIAS (vtysh_exit_mgmt_interface,
       vtysh_quit_mgmt_interface_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif /* ifndef ENABLE_OVSDB */
#endif
/* Memory */
#ifndef ENABLE_OVSDB
DEFUN (vtysh_show_memory,
      vtysh_show_memory_cmd,
      "show memory",
      SHOW_STR
      "Memory statistics\n")
{
   unsigned int i;
   int ret = CMD_SUCCESS;
   char line[] = "show memory\n";

   for (i = 0; i < array_size(vtysh_client); i++)
      if ( vtysh_client[i].fd >= 0 )
      {
         fprintf (stdout, "Memory statistics for %s:\n",
               vtysh_client[i].name);
         ret = vtysh_client_execute (&vtysh_client[i], line, stdout);
         fprintf (stdout,"\n");
      }

   return ret;
}
#endif

#ifndef ENABLE_OVSDB
/* Logging commands. */
DEFUN (vtysh_show_logging,
      vtysh_show_logging_cmd,
      "show logging",
      SHOW_STR
      "Show current logging configuration\n")
{
   unsigned int i;
   int ret = CMD_SUCCESS;
   char line[] = "show logging\n";

   for (i = 0; i < array_size(vtysh_client); i++)
      if ( vtysh_client[i].fd >= 0 )
      {
         fprintf (stdout,"Logging configuration for %s:\n",
               vtysh_client[i].name);
         ret = vtysh_client_execute (&vtysh_client[i], line, stdout);
         fprintf (stdout,"\n");
      }

   return ret;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_stdout,
      vtysh_log_stdout_cmd,
      "log stdout",
      "Logging control\n"
      "Set stdout logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_stdout_level,
      vtysh_log_stdout_level_cmd,
      "log stdout "LOG_LEVELS,
      "Logging control\n"
      "Set stdout logging level\n"
      LOG_LEVEL_DESC)
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_stdout,
      no_vtysh_log_stdout_cmd,
      "no log stdout [LEVEL]",
      NO_STR
      "Logging control\n"
      "Cancel logging to stdout\n"
      "Logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_file,
      vtysh_log_file_cmd,
      "log file FILENAME",
      "Logging control\n"
      "Logging to file\n"
      "Logging filename\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_file_level,
      vtysh_log_file_level_cmd,
      "log file FILENAME "LOG_LEVELS,
      "Logging control\n"
      "Logging to file\n"
      "Logging filename\n"
      LOG_LEVEL_DESC)
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_file,
      no_vtysh_log_file_cmd,
      "no log file [FILENAME]",
      NO_STR
      "Logging control\n"
      "Cancel logging to file\n"
      "Logging file name\n")
{
   return CMD_SUCCESS;
}

ALIAS_SH (VTYSH_ALL,
      no_vtysh_log_file,
      no_vtysh_log_file_level_cmd,
      "no log file FILENAME LEVEL",
      NO_STR
      "Logging control\n"
      "Cancel logging to file\n"
      "Logging file name\n"
      "Logging level\n")

DEFUNSH (VTYSH_ALL,
      vtysh_log_monitor,
      vtysh_log_monitor_cmd,
      "log monitor",
      "Logging control\n"
      "Set terminal line (monitor) logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_monitor_level,
      vtysh_log_monitor_level_cmd,
      "log monitor "LOG_LEVELS,
      "Logging control\n"
      "Set terminal line (monitor) logging level\n"
      LOG_LEVEL_DESC)
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_monitor,
      no_vtysh_log_monitor_cmd,
      "no log monitor [LEVEL]",
      NO_STR
      "Logging control\n"
      "Disable terminal line (monitor) logging\n"
      "Logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_syslog,
      vtysh_log_syslog_cmd,
      "log syslog",
      "Logging control\n"
      "Set syslog logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_syslog_level,
      vtysh_log_syslog_level_cmd,
      "log syslog "LOG_LEVELS,
      "Logging control\n"
      "Set syslog logging level\n"
      LOG_LEVEL_DESC)
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_syslog,
      no_vtysh_log_syslog_cmd,
      "no log syslog [LEVEL]",
      NO_STR
      "Logging control\n"
      "Cancel logging to syslog\n"
      "Logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_facility,
      vtysh_log_facility_cmd,
      "log facility "LOG_FACILITIES,
      "Logging control\n"
      "Facility parameter for syslog messages\n"
      LOG_FACILITY_DESC)

{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_facility,
      no_vtysh_log_facility_cmd,
      "no log facility [FACILITY]",
      NO_STR
      "Logging control\n"
      "Reset syslog facility to default (daemon)\n"
      "Syslog facility\n")

{
   return CMD_SUCCESS;
}

DEFUNSH_DEPRECATED (VTYSH_ALL,
      vtysh_log_trap,
      vtysh_log_trap_cmd,
      "log trap "LOG_LEVELS,
      "Logging control\n"
      "(Deprecated) Set logging level and default for all destinations\n"
      LOG_LEVEL_DESC)

{
   return CMD_SUCCESS;
}

DEFUNSH_DEPRECATED (VTYSH_ALL,
      no_vtysh_log_trap,
      no_vtysh_log_trap_cmd,
      "no log trap [LEVEL]",
      NO_STR
      "Logging control\n"
      "Permit all logging information\n"
      "Logging level\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_record_priority,
      vtysh_log_record_priority_cmd,
      "log record-priority",
      "Logging control\n"
      "Log the priority of the message within the message\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_record_priority,
      no_vtysh_log_record_priority_cmd,
      "no log record-priority",
      NO_STR
      "Logging control\n"
      "Do not log the priority of the message within the message\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_log_timestamp_precision,
      vtysh_log_timestamp_precision_cmd,
      "log timestamp precision <0-6>",
      "Logging control\n"
      "Timestamp configuration\n"
      "Set the timestamp precision\n"
      "Number of subsecond digits\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_log_timestamp_precision,
      no_vtysh_log_timestamp_precision_cmd,
      "no log timestamp precision",
      NO_STR
      "Logging control\n"
      "Timestamp configuration\n"
      "Reset the timestamp precision to the default value of 0\n")
{
   return CMD_SUCCESS;
}
#endif
#ifndef ENABLE_OVSDB

DEFUNSH (VTYSH_ALL,
      vtysh_service_password_encrypt,
      vtysh_service_password_encrypt_cmd,
      "service password-encryption",
      "Set up miscellaneous service\n"
      "Enable encrypted passwords\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_service_password_encrypt,
      no_vtysh_service_password_encrypt_cmd,
      "no service password-encryption",
      NO_STR
      "Set up miscellaneous service\n"
      "Enable encrypted passwords\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_config_password,
      vtysh_password_cmd,
      "password (8|) WORD",
      "Assign the terminal connection password\n"
      "Specifies a HIDDEN password will follow\n"
      "dummy string \n"
      "The HIDDEN line password string\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_password_text,
      vtysh_password_text_cmd,
      "password LINE",
      "Assign the terminal connection password\n"
      "The UNENCRYPTED (cleartext) line password\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_config_enable_password,
      vtysh_enable_password_cmd,
      "enable password (8|) WORD",
      "Modify enable password parameters\n"
      "Assign the privileged level password\n"
      "Specifies a HIDDEN password will follow\n"
      "dummy string \n"
      "The HIDDEN 'enable' password string\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      vtysh_enable_password_text,
      vtysh_enable_password_text_cmd,
      "enable password LINE",
      "Modify enable password parameters\n"
      "Assign the privileged level password\n"
      "The UNENCRYPTED (cleartext) 'enable' password\n")
{
   return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
      no_vtysh_config_enable_password,
      no_vtysh_enable_password_cmd,
      "no enable password",
      NO_STR
      "Modify enable password parameters\n"
      "Assign the privileged level password\n")
{
   return CMD_SUCCESS;
}
#endif

#ifndef ENABLE_OVSDB
DEFUN (vtysh_write_terminal,
      vtysh_write_terminal_cmd,
      "write terminal",
      "Write running configuration to memory, network, or terminal\n"
      "Write to terminal\n")
{
   u_int i;
   int ret;
   char line[] = "write terminal\n";
   FILE *fp = NULL;

   if (vtysh_pager_name)
   {
      fp = popen (vtysh_pager_name, "w");
      if (fp == NULL)
      {
         perror ("popen");
         exit (1);
      }
   }
   else
      fp = stdout;

   vty_out (vty, "Building configuration...%s", VTY_NEWLINE);
   vty_out (vty, "%sCurrent configuration:%s", VTY_NEWLINE,
         VTY_NEWLINE);
   vty_out (vty, "!%s", VTY_NEWLINE);

   for (i = 0; i < array_size(vtysh_client); i++)
      ret = vtysh_client_config (&vtysh_client[i], line);

   /* Integrate vtysh specific configuration. */
   vtysh_config_write ();

   vtysh_config_dump (fp);

   if (vtysh_pager_name && fp)
   {
      fflush (fp);
      if (pclose (fp) == -1)
      {
         perror ("pclose");
         exit (1);
      }
      fp = NULL;
   }

   vty_out (vty, "end%s", VTY_NEWLINE);

   return CMD_SUCCESS;
}
#endif

DEFUN (vtysh_integrated_config,
      vtysh_integrated_config_cmd,
      "service integrated-vtysh-config",
      "Set up miscellaneous service\n"
      "Write configuration into integrated file\n")
{
   vtysh_writeconfig_integrated = 1;
   return CMD_SUCCESS;
}
#ifndef ENABLE_OVSDB
DEFUN (no_vtysh_integrated_config,
      no_vtysh_integrated_config_cmd,
      "no service integrated-vtysh-config",
      NO_STR
      "Set up miscellaneous service\n"
      "Write configuration into integrated file\n")
{
   vtysh_writeconfig_integrated = 0;
   return CMD_SUCCESS;
}
static int
write_config_integrated(void)
{
   u_int i;
   char line[] = "write terminal\n";
   FILE *fp;
   char *integrate_sav = NULL;

   integrate_sav = malloc (strlen (integrate_default) +
         strlen (CONF_BACKUP_EXT) + 1);
   strcpy (integrate_sav, integrate_default);
   strcat (integrate_sav, CONF_BACKUP_EXT);

   fprintf (stdout,"Building Configuration...\n");

   /* Move current configuration file to backup config file. */
   unlink (integrate_sav);
   rename (integrate_default, integrate_sav);
   free (integrate_sav);

   fp = fopen (integrate_default, "w");
   if (fp == NULL)
   {
      fprintf (stdout,"%% Can't open configuration file %s.\n",
            integrate_default);
      return CMD_SUCCESS;
   }

   for (i = 0; i < array_size(vtysh_client); i++)
      vtysh_client_config (&vtysh_client[i], line);

   vtysh_config_dump (fp);

   fclose (fp);

   if (chmod (integrate_default, CONFIGFILE_MASK) != 0)
   {
      fprintf (stdout,"%% Can't chmod configuration file %s: %s (%d)\n",
            integrate_default, safe_strerror(errno), errno);
      return CMD_WARNING;
   }

   fprintf(stdout,"Integrated configuration saved to %s\n",integrate_default);

   fprintf (stdout,"[OK]\n");

   return CMD_SUCCESS;
}
#endif
#ifndef ENABLE_OVSDB
DEFUN (vtysh_write_memory,
      vtysh_write_memory_cmd,
      "write memory",
      "Write running configuration to memory, network, or terminal\n"
      "Write configuration to the file (same as write file)\n")
{
   int ret = CMD_SUCCESS;
   char line[] = "write memory\n";
   u_int i;

   /* If integrated Quagga.conf explicitely set. */
   if (vtysh_writeconfig_integrated)
      return write_config_integrated();

   fprintf (stdout,"Building Configuration...\n");

   for (i = 0; i < array_size(vtysh_client); i++)
      ret = vtysh_client_execute (&vtysh_client[i], line, stdout);

   fprintf (stdout,"[OK]\n");

   return ret;
}

ALIAS (vtysh_write_memory,
      vtysh_copy_runningconfig_startupconfig_cmd,
      "copy running-config startup-config",
      "Copy from one file to another\n"
      "Copy from current system configuration\n"
      "Copy to startup configuration\n")

ALIAS (vtysh_write_memory,
      vtysh_write_file_cmd,
      "write file",
      "Write running configuration to memory, network, or terminal\n"
      "Write configuration to the file (same as write memory)\n")

ALIAS (vtysh_write_memory,
       vtysh_write_cmd,
       "write",
       "Write running configuration to memory, network, or terminal\n")
#endif

#ifdef ENABLE_OVSDB
DEFUN (vtysh_show_running_config,
      vtysh_show_running_config_cmd,
      "show running-config",
      SHOW_STR
      "Current running configuration\n")
{
   FILE *fp = NULL;

   fp = stdout;
   if (!vtysh_show_startup)
   {
       fprintf(fp, "Current configuration:\n");
   }

   vtysh_ovsdb_read_config(fp);
   return CMD_SUCCESS;
}


DEFUN_HIDDEN (vtysh_show_context_client_list,
              vtysh_show_context_client_list_cmd,
              "show context-client-list",
              SHOW_STR
              "Vtysh Context Table Client List\n")
{
   vty_out (vty, "%sCurrent Context Table client list %s", VTY_NEWLINE, VTY_NEWLINE);

   vtysh_context_table_list_clients (vty);
   return CMD_SUCCESS;
}
#else
ALIAS (vtysh_write_terminal,
      vtysh_show_running_config_cmd,
      "show running-config",
      SHOW_STR
      "Current operating configuration\n")
#endif /* ENABLE_OVSDB */

#ifndef ENABLE_OVSDB
DEFUN (vtysh_terminal_length,
      vtysh_terminal_length_cmd,
      "terminal length <0-512>",
      "Set terminal line parameters\n"
      "Set number of lines on a screen\n"
      "Number of lines on screen (0 for no pausing)\n")
{
   int lines;
   char *endptr = NULL;
   char default_pager[10];

   lines = strtol (argv[0], &endptr, 10);
   if (lines < 0 || lines > 512 || *endptr != '\0')
   {
      vty_out (vty, "length is malformed%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   if (vtysh_pager_name)
   {
      free (vtysh_pager_name);
      vtysh_pager_name = NULL;
   }

   if (lines != 0)
   {
      snprintf(default_pager, 10, "more -%i", lines);
      vtysh_pager_name = strdup (default_pager);
   }

   return CMD_SUCCESS;
}

DEFUN (vtysh_terminal_no_length,
      vtysh_terminal_no_length_cmd,
      "terminal no length",
      "Set terminal line parameters\n"
      NO_STR
      "Set number of lines on a screen\n")
{
   if (vtysh_pager_name)
   {
      free (vtysh_pager_name);
      vtysh_pager_name = NULL;
   }

   vtysh_pager_init();
   return CMD_SUCCESS;
}
#endif

#ifndef ENABLE_OVSDB
DEFUN (vtysh_show_daemons,
      vtysh_show_daemons_cmd,
      "show daemons",
      SHOW_STR
      "Show list of running daemons\n")
{
   u_int i;

   for (i = 0; i < array_size(vtysh_client); i++)
      if ( vtysh_client[i].fd >= 0 )
         vty_out(vty, " %s", vtysh_client[i].name);
   vty_out(vty, "%s", VTY_NEWLINE);

   return CMD_SUCCESS;
}

#endif

#ifdef ENABLE_OVSDB
/* Execute command in child process. */
int
execute_command (const char *command, int argc, const char *arg[])
{
  pid_t pid = 0;
  int status = 0;
  int ret = 0;
  int index = 0;
  char **cmd_argv = NULL;
  /* Call fork(). */
  pid = fork ();

   if (pid < 0)
   {
      /* Failure of fork(). */
      VLOG_ERR ("execute_command(): Can't fork");
      fprintf (stderr, "Can't fork: %s\n", safe_strerror (errno));
      exit (1);
    }
  else if (pid == 0)
    {
      cmd_argv = (char **) malloc(sizeof(char *) * (argc + 2));

      if (cmd_argv == NULL)
        {
          VLOG_ERR ("execute_command(): Memory allocation failed, error:%d",errno);
          fprintf (stderr, "Memory allocation failed, Can't execute %s: %s\n", command, safe_strerror (errno));
          exit(1);
        }
      cmd_argv[0] = (char *)command;

      for (index = 1; index < (argc + 1); index++)
        {
          cmd_argv[index] = (char *)arg[index - 1];
        }

      cmd_argv[index] = NULL;
      ret = execvp (*cmd_argv, cmd_argv) ;
      /* When execvp suceed, this part is not executed. */
      fprintf (stderr, "Can't execute %s: %s\n", command, safe_strerror (errno));
      if (cmd_argv){
        free(cmd_argv);
      }
      exit(1);
    }
  else
    {
      /* This is parent. */
      execute_flag = 1;
      ret = wait4 (pid, &status, 0, NULL);
      execute_flag = 0;
      if (WIFEXITED(status))
      {
          VLOG_ERR("Child exited normally\n");
          return WEXITSTATUS(status);
      }
      else
      {
          VLOG_ERR("Child exited abnormally\n");
      }
   }
   return ret;
}

int
remove_temp_db(int initialize)
{
    char file_name[]= TEMPORARY_PROCESS_PID;
    char *remove_db[] = {TEMPORARY_STARTUP_DB};
    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    int file_not_present = 0;

    fp = fopen(file_name,"r");
    if (fp == NULL)
    {
        if (initialize == 0)
        {
            VLOG_ERR("Error while opening %s file\n", file_name);
            return -1;
        }
        else
        {
            VLOG_INFO("No %s file present\n",file_name);
            file_not_present = 1;
        }
    }

    if (!file_not_present)
    {
        if (getline(&line, &len, fp) != -1) {
            if (kill(atoi(line), SIGTERM) == -1)
            {
                VLOG_ERR("Failed to kill temp.pid process\n");
                fclose(fp);
                return -1;
            }
        }
        fclose(fp);
    }

    if (access(TEMPORARY_STARTUP_DB, F_OK) == -1)
    {
        VLOG_INFO("No %s file present\n", TEMPORARY_STARTUP_DB);
    }
    else
    {
        if (execute_command ("rm", 1, (const char **)remove_db) == -1)
        {
            VLOG_ERR("Failed to remove temporary DB \n");
            return -1;
        }
    }
    return 0;
}
/* Write startup configuration into the terminal. */
DEFUN (show_startup_config,
       show_startup_config_cmd,
       "show startup-config",
       SHOW_STR
       "Contents of startup configuration\n")
{
  char *arguments[] = {"show", "startup-config"};
  char *temp_args[] = {"-D", TEMPORARY_STARTUP_SOCKET, "-c", "show running-config "};
  char *copy_db[] = {OVSDB_PATH, TEMPORARY_STARTUP_DB};
  char *run_server[] = {"--pidfile=/var/run/openvswitch/temp_startup.pid", "--detach", "--remote", "punix:/var/run/openvswitch/temp_startup.sock", TEMPORARY_STARTUP_DB};
  char *remove_tempstartup_db[] = {"rm", "-f", TEMPORARY_STARTUP_DB_LOCK};
  int ret = 0;

  // Check if temporary DB exists and OVSDB server running. If yes, remove it.
  remove_temp_db(1);

  // Copy current ovsdb to temporary DB.
  if (execute_command ("cp", 2, (const char **)copy_db) == -1)
  {
      vty_out(vty, "%s%s", STARTUP_CONFIG_ERR, VTY_NEWLINE);
      VLOG_ERR("Failed to copy OVSDB to temporary DB\n");
      return CMD_SUCCESS;
  }

  // Run ovsdb-server for temporary DB.
  if (execute_command ("ovsdb-server", 5, (const char **)run_server) == -1)
  {
      vty_out(vty, "%s%s", STARTUP_CONFIG_ERR, VTY_NEWLINE);
      VLOG_ERR("Failed to run ovsdb-server for temporary DB\n");
      remove_temp_db(1);
      return CMD_SUCCESS;
  }

  // Copy startup config to temporary DB.
  ret = execute_command ("cfgdbutil", 2, (const char **)arguments);
  if (ret == -1)
  {
      vty_out(vty, "%s%s", STARTUP_CONFIG_ERR, VTY_NEWLINE);
      VLOG_ERR("Failed to run cfgdbutil\n");
      remove_temp_db(1);
      return CMD_SUCCESS;
  }
  else if (ret == 2)
  {
      VLOG_ERR("No saved configuration exists\n");
      remove_temp_db(1);
      return CMD_SUCCESS;
  }

  // Run a vtysh session on temporary DB.
  if (execute_command ("vtysh", 4, (const char **)temp_args) == -1)
  {
      vty_out(vty, "%s%s", STARTUP_CONFIG_ERR, VTY_NEWLINE);
      VLOG_ERR("Failed to invoke vtysh on Temporary DB\n");
      remove_temp_db(1);
      vtysh_show_startup = 0;
      return CMD_SUCCESS;
  }
  vtysh_show_startup = 0;

  // Remove temporary DB and kill the ovsdb-server to temporary DB.
  if (remove_temp_db(0))
  {
      vty_out(vty, "%s%s", STARTUP_CONFIG_ERR, VTY_NEWLINE);
      return CMD_SUCCESS;
  }

  if (execute_command ("sudo", 3, (const char **)remove_tempstartup_db)  == -1)
  {
      VLOG_ERR("Failed to remove temporary DB lock\n");
      return -1;
  }

  return CMD_SUCCESS;
}

DEFUN (vtysh_copy_runningconfig,
       vtysh_copy_runningconfig_startupconfig_cmd,
       "copy running-config startup-config",
       COPY_STR
       "Copy from current system running configuration\n"
       "Copy to startup configuration\n")
{
  char *arguments[] = {"copy", "running-config", "startup-config"};
  execute_command ("cfgdbutil", 3, (const char **)arguments);
  return CMD_SUCCESS;
}

DEFUN (vtysh_copy_startupconfig,
       vtysh_copy_startupconfig_runningconfig_cmd,
       "copy startup-config running-config",
       COPY_STR
       "Copy from startup configuration\n"
       "Copy to current system running configuration\n")
{
  char *arguments[] = {"copy", "startup-config", "running-config"};
  execute_command ("cfgdbutil", 3, (const char **)arguments);
  return CMD_SUCCESS;
}
#ifndef ENABLE_OVSDB
DEFUN (vtysh_ping,
      vtysh_ping_cmd,
      "ping WORD",
      "Send echo messages\n"
      "Ping destination address or hostname\n")
{
  execute_command ("ping", 1, argv);
  return CMD_SUCCESS;
}

ALIAS (vtysh_ping,
      vtysh_ping_ip_cmd,
      "ping ip WORD",
      "Send echo messages\n"
      "IP echo\n"
      "Ping destination address or hostname\n")

DEFUN (vtysh_traceroute,
      vtysh_traceroute_cmd,
      "traceroute WORD",
      "Trace route to destination\n"
      "Trace route to destination address or hostname\n")
{
  execute_command ("traceroute", 1, argv);
  return CMD_SUCCESS;
}

ALIAS (vtysh_traceroute,
      vtysh_traceroute_ip_cmd,
      "traceroute ip WORD",
      "Trace route to destination\n"
      "IP trace\n"
      "Trace route to destination address or hostname\n")

#ifdef HAVE_IPV6
DEFUN (vtysh_ping6,
      vtysh_ping6_cmd,
      "ping ipv6 WORD",
      "Send echo messages\n"
      "IPv6 echo\n"
      "Ping destination address or hostname\n")
{
  execute_command ("ping6", 1, argv);
  return CMD_SUCCESS;
}

DEFUN (vtysh_traceroute6,
      vtysh_traceroute6_cmd,
      "traceroute ipv6 WORD",
      "Trace route to destination\n"
      "IPv6 trace\n"
      "Trace route to destination address or hostname\n")
{
  execute_command ("traceroute6", 1, argv);
  return CMD_SUCCESS;
}
#endif

DEFUN (vtysh_telnet,
      vtysh_telnet_cmd,
      "telnet WORD",
      "Open a telnet connection\n"
      "IP address or hostname of a remote system\n")
{
  execute_command ("telnet", 1, argv);
  return CMD_SUCCESS;
}

DEFUN (vtysh_telnet_port,
      vtysh_telnet_port_cmd,
      "telnet WORD PORT",
      "Open a telnet connection\n"
      "IP address or hostname of a remote system\n"
      "TCP Port number\n")
{
  execute_command ("telnet", 2, argv);
  return CMD_SUCCESS;
}

DEFUN (vtysh_ssh,
      vtysh_ssh_cmd,
      "ssh WORD",
      "Open an ssh connection\n"
      "[user@]host\n")
{
  execute_command ("ssh", 1, argv);
  return CMD_SUCCESS;
}
#endif /* ENABLE_OVSDB */

DEFUN_NOLOCK (vtysh_start_shell,
       vtysh_start_shell_cmd,
       "start-shell",
#ifndef ENABLE_OVSDB
       "Start UNIX shell\n")
#else
       "Start Bash shell\n")
#endif
{
#ifdef ENABLE_OVSDB
  execute_command ("bash", 0, NULL);
#else
  execute_command ("sh", 0, NULL);
#endif
  return CMD_SUCCESS;
}

DEFUN (vtysh_start_bash,
      vtysh_start_bash_cmd,
      "start-shell bash",
      "Start UNIX shell\n"
      "Start bash\n")
{
  execute_command ("bash", 0, NULL);
  return CMD_SUCCESS;
}

/*Function to get the number of users under given group*/
static int
get_group_user_count(const char *group_name)
{
     FILE *group, *passwd;
     char groupLine[256] = {0};
     char passwdLine[256] = {0};
     char * pch;
     int i = 0,count = 0;

     group = fopen("/etc/group", "r");
     passwd = fopen("/etc/passwd", "r");

     while (fgets(groupLine, sizeof(groupLine), group) != NULL) {
         if((strstr(groupLine, group_name)) != NULL)
             break;
     }

     pch = strtok (groupLine,":");
     while (pch != NULL) {
         pch = strtok (NULL, ":");
         if (i++ == 1)
             break;
     }

     while (fgets(passwdLine, sizeof(passwdLine), passwd) != NULL) {
          if((strstr(passwdLine, pch)) != NULL)
             count ++;
     }

     fclose(group);
     fclose(passwd);
     return count;
}

/*Function to check whether user is member of the given group*/
static int
check_user_group( const char *user, const char *group_name)
{
       int j, ngroups;
       gid_t *groups;
       struct passwd *pw;
       struct group *gr;

       ngroups = 10;
       groups = malloc(ngroups * sizeof (gid_t));
       if (groups == NULL) {
           VLOG_DBG("Malloc failed. Function = %s, Line = %d", __func__, __LINE__);
           return false;
       }

       /* Fetch passwd structure (contains first group ID for user) */

       pw = getpwnam(user);
       if (pw == NULL) {
           VLOG_DBG("Invalid User. Function = %s, Line = %d", __func__,__LINE__);
           free(groups);
           return false;
       }

       /* Retrieve group list */

       if (getgrouplist(user, pw->pw_gid, groups, &ngroups) == -1) {
           VLOG_DBG("Retrieving group list failed. Function = %s, Line = %d", __func__, __LINE__);
           free(groups);
           return false;
       }

       /* check user exist in ovsdb_users group*/
       for (j = 0; j < ngroups; j++) {
           gr = getgrgid(groups[j]);
           if (gr != NULL) {
               if (!strcmp(gr->gr_name,group_name)) {
                   free(groups);
                   return true;
               }
           }
       }
       free(groups);
       return false;
}

/* Prompt for user to enter password */
static char*
get_password(const char *prompt)
{
    struct termios oflags, nflags;
    enum { sizeof_passwd = 128 };
    char *ret;
    int i;
    /* disabling echo */
    tcflush(fileno(stdin),TCIFLUSH);
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_iflag &= ~(IUCLC|IXON|IXOFF|IXANY);
    nflags.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
    if(tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        VLOG_DBG("setattr error");
        get_password(prompt);
    }
    write(STDIN_FILENO,prompt,strlen(prompt));

    ret = malloc(sizeof_passwd);
    i = 0;
    while (1) {
        int r = read(STDIN_FILENO, &ret[i], 1);
        if ((i == 0 && r == 0) || r < 0 ) {                     /* EOF (^D) with no password */
            ret  = NULL;
            break;
        }
        if (r == 0 || ret[i] == '\r' || ret[i] == '\n' || ++i == sizeof_passwd-1 ) {   /* EOF EOL */ /* EOL *//* line limit */
            ret[i] = '\0';
            break;
        }
    }
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        vty_out(vty,"tcsetattr");
        return CMD_SUCCESS;
    }
    return ret;
}

/*Function to set the user passsword */
static int
set_user_passwd(const char *user)
{
    int ret;
    struct crypt_data data;
    data.initialized = 0;

    char *password = NULL;
    char *passwd = NULL;
    char *cp = NULL;
    const char *arg[4];
    arg[0] = USERMOD;
    arg[1] = "-p";
    arg[3] = CONST_CAST(char*, user);

    /* Cannot change the password for the user root */
    if (!strcmp(user,"root"))
    {
        vty_out(vty, "Permission denied.\n");
        return CMD_SUCCESS;
    }
    ret = check_user_group(user, OVSDB_GROUP);

    /* Change the passwd if user is in ovsdb_user list */
    if (ret==1)
    {
        vty_out(vty,"Changing password for user %s %s", user, VTY_NEWLINE);
        passwd = get_password("Enter new password: ");
        if (!passwd)
        {
            vty_out(vty, "%s", VTY_NEWLINE);
            vty_out(vty, "Entered empty password.");
        }

        vty_out(vty, "%s", VTY_NEWLINE);
        cp = get_password("Confirm new password: ");
        if (!cp)
        {
            vty_out(vty, "%s", VTY_NEWLINE);
            vty_out(vty,"Entered empty password.");
        }
        if (strcmp(passwd,cp) != 0)
        {
            vty_out(vty, "%s", VTY_NEWLINE);
            vty_out(vty,"Passwords do not match. Password unchanged.%s", VTY_NEWLINE);
            free(passwd);
            free(cp);
            return CMD_SUCCESS;
        }
        else
        {
            vty_out(vty, "%s", VTY_NEWLINE);
            vty_out(vty, "Password updated successfully.%s", VTY_NEWLINE);
        }
        /* Encrypt the password. String 'ab' is used to perturb the */
        /* algorithm in  one of 4096 different ways. */
        password = crypt_r(passwd,"ab",&data);
        arg[2]=password;
        execute_command("sudo", 4, (const char **)arg);
        free(passwd);
        free(cp);
        return CMD_SUCCESS;
    }
    else
    {
        vty_out(vty, "Unknown User: %s.\n", user);
        return CMD_SUCCESS;
    }

}
#ifdef ENABLE_OVSDB
DEFUN (vtysh_passwd,
       vtysh_passwd_cmd,
       "password WORD",
       "Change user password \n"
       "User whose password is to be changed\n")
{
    return set_user_passwd(argv[0]);
}
#endif

DEFUN (vtysh_start_zsh,
      vtysh_start_zsh_cmd,
      "start-shell zsh",
      "Start UNIX shell\n"
      "Start Z shell\n")
{
  execute_command ("zsh", 0, NULL);
  return CMD_SUCCESS;
}
#endif

static void
vtysh_install_default (enum node_type node)
{
   install_element (node, &config_list_cmd);
}

/* Making connection to protocol daemon. */
static int
vtysh_connect (struct vtysh_client *vclient)
{
   int ret;
   int sock, len;
   struct sockaddr_un addr;
   struct stat s_stat;

   /* Stat socket to see if we have permission to access it. */
   ret = stat (vclient->path, &s_stat);
   if (ret < 0 && errno != ENOENT)
   {
      fprintf  (stderr, "vtysh_connect(%s): stat = %s\n",
            vclient->path, safe_strerror(errno));
      exit(1);
   }

   if (ret >= 0)
   {
      if (! S_ISSOCK(s_stat.st_mode))
      {
         fprintf (stderr, "vtysh_connect(%s): Not a socket\n",
               vclient->path);
         exit (1);
      }

   }

   sock = socket (AF_UNIX, SOCK_STREAM, 0);
   if (sock < 0)
   {
#ifdef DEBUG
      fprintf(stderr, "vtysh_connect(%s): socket = %s\n", vclient->path,
            safe_strerror(errno));
#endif /* DEBUG */
      return -1;
   }

   memset (&addr, 0, sizeof (struct sockaddr_un));
   addr.sun_family = AF_UNIX;
   strncpy (addr.sun_path, vclient->path, strlen (vclient->path));
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
   len = addr.sun_len = SUN_LEN(&addr);
#else
   len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */

   ret = connect (sock, (struct sockaddr *) &addr, len);
   if (ret < 0)
   {
#ifdef DEBUG
      fprintf(stderr, "vtysh_connect(%s): connect = %s\n", vclient->path,
            safe_strerror(errno));
#endif /* DEBUG */
      close (sock);
      return -1;
   }
   vclient->fd = sock;

   return 0;
}

int
vtysh_connect_all(const char *daemon_name)
{
   u_int i;
   int rc = 0;
   int matches = 0;

   for (i = 0; i < array_size(vtysh_client); i++)
   {
      if (!daemon_name || !strcmp(daemon_name, vtysh_client[i].name))
      {
         matches++;
         if (vtysh_connect(&vtysh_client[i]) == 0)
            rc++;
         /* We need direct access to ripd in vtysh_exit_ripd_only. */
         if (vtysh_client[i].flag == VTYSH_RIPD)
            ripd_client = &vtysh_client[i];
      }
   }
   if (!matches)
      fprintf(stderr, "Error: no daemons match name %s!\n", daemon_name);
   return rc;
}

/* To disable readline's filename completion. */
static char *
vtysh_completion_entry_function (const char *ignore, int invoking_key)
{
   return NULL;
}

void
vtysh_readline_init (void)
{
   /* readline related settings. */
   rl_bind_key ('?', (rl_command_func_t *) vtysh_rl_describe);
   rl_completion_entry_function = vtysh_completion_entry_function;
   rl_attempted_completion_function = (rl_completion_func_t *)new_completion;
}

char *
vtysh_prompt (void)
{
   static struct utsname names;
   static char buf[100];
   const char*hostname;
   extern struct host host;

#ifdef ENABLE_OVSDB
   const struct ovsrec_system *ovs = NULL;
   const char *val;
   ovs = ovsrec_system_first(idl);

   if(ovs)
   {
      val = smap_get(&ovs->mgmt_intf_status, SYSTEM_MGMT_INTF_MAP_HOSTNAME);
      if (val != NULL)
      {
         if (host.name)
            XFREE (MTYPE_HOST, host.name);

         host.name = XSTRDUP (MTYPE_HOST, val);
      }
   }
#endif

   hostname = host.name;
   if (!hostname)
   {
     uname (&names);
     hostname = names.nodename;
     /* do XSTRDUP to avoid crash as it will be free'd later */
     host.name = XSTRDUP (MTYPE_HOST,hostname);
   }

#ifdef ENABLE_OVSDB
   static char newhost[100];
   const char* temphost = cmd_prompt(vty->node);
   strcpy(newhost, temphost);
   if (enable_mininet_test_prompt == 1)
   {
      int len = strlen(temphost);
      int x = 127;
      newhost[len - 1] = (char)x;
   }
   snprintf (buf, sizeof buf, newhost, hostname);
#else
   snprintf (buf, sizeof buf, cmd_prompt (vty->node), hostname);
#endif

   return buf;
}

#ifdef ENABLE_OVSDB

/* Function to validate user.*/
static int
validate_user(const char *user)
{

    /* User names must match [a-z_][a-z0-9_-]*[$] */
    if (('\0' == *user) ||
         !((('a' <= *user) && ('z' >= *user)) || ('_' == *user)))
    {
        return false;
    }

    /* Maximum allowed length of user name is 32 character.*/
    if (strlen (user) > USER_NAME_MAX_LENGTH)
    {
        return false;
    }

    while ('\0' != *++user)
    {
        if (!(( ('a' <= *user) && ('z' >= *user)) ||
            ( ('0' <= *user) && ('9' >= *user) ) ||
            ('_' == *user) ||
            ('-' == *user) ||
            ( ('$' == *user) && ('\0' == *(user + 1)) )
            ))
        {
            return false;
        }
    }

    return true;
}

/* Function to create new user with password and add it to the ovsdb_users group*/
static int
create_new_vtysh_user(const char *user)
{
    struct crypt_data data;
    data.initialized = 0;
    int ret = 0, cmd_ret = 0;
    char *password = NULL;
    char *passwd = NULL;
    char *cp = NULL;
    /* If user already exist then don't create new user */
    ret = check_user_group(user, OVSDB_GROUP);
    if (!validate_user(user))
    {
        vty_out(vty, "useradd: Invalid user name '%s'.%s", user, VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }
    if ((ret ==1) || (!strcmp("root", user)))
    {
        vty_out(vty, "User %s already exists.%s", user, VTY_NEWLINE);
        return CMD_ERR_NOTHING_TODO;
    }

    vty_out(vty,"Adding user %s%s", user, VTY_NEWLINE);
    passwd = get_password("Enter password: ");
    if (!passwd)
    {
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty, "Entered empty password");
    }
    vty_out(vty, "%s", VTY_NEWLINE);
    cp = get_password("Confirm password: ");
    if (!cp)
    {
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty,"Entered empty password");
    }
    if (strcmp(passwd,cp) != 0) {
        vty_out(vty, "%s", VTY_NEWLINE);
        vty_out(vty,"Passwords do not match. ");
        vty_out(vty,"User %s not added.%s", user, VTY_NEWLINE);
        free(cp);
        free(passwd);
        return CMD_ERR_NOTHING_TODO;
    }
    /* Encrypt the password. String 'ab' is used to perturb the */
    /* algorithm in  one of 4096 different ways. */
    password = crypt_r(passwd,"ab",&data);
    const char *arg[8];
    arg[0] = USERADD;
    arg[1] = "-p";
    arg[2]= password;
    arg[3] = "-g";
    arg[4] = OVSDB_GROUP;
    arg[5] = "-s";
    arg[6] = VTYSH_PROMPT;
    arg[7] = CONST_CAST(char*, user);
    vty_out(vty, "%s", VTY_NEWLINE);
    cmd_ret = execute_command("sudo", 8,(const char **)arg);
    if(cmd_ret == 0)
    {
        vty_out(vty, "User added successfully.%s", VTY_NEWLINE);
        free(cp);
        free(passwd);
        return CMD_SUCCESS;
    }
    free(cp);
    free(passwd);
    return CMD_ERR_NOTHING_TODO;
}

DEFUN(vtysh_user_add,
       vtysh_user_add_cmd,
       "user add WORD",
       "User account\n"
       "Adding a new user account\n"
       "User name to be added\n")
{
    return create_new_vtysh_user(argv[0]);

}

/* Delete user account. */
static int
delete_user(const char *user)
{
    struct passwd *pw;
    int ret;
    const char *arg[3];

    if (!strcmp(user, "root")) {
        vty_out(vty, "Permission denied. Cannot remove the root user.%s",
                VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    /* Avoid system users. */
    if (!check_user_group(user, OVSDB_GROUP)) {
        vty_out(vty, "Unknown user.%s", VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    pw = getpwuid(geteuid());
    if (pw && !(strcmp(pw->pw_name, user))) {
        vty_out(vty, "Permission denied. You are logged in as %s.%s", user,
                VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    if (get_group_user_count(OVSDB_GROUP) <= 1) {
        vty_out(vty, "Cannot delete the last user %s.%s", user, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    /* Call out to external command to delete the user. */
    arg[0] = USERDEL;
    arg[1] = "-r";
    arg[2] = CONST_CAST(char*, user);
    ret = execute_command("sudo", 3, (const char **)arg);
    if (ret == 0)
        vty_out(vty, "User removed successfully.%s", VTY_NEWLINE);

    return CMD_SUCCESS;
}

DEFUN(vtysh_user_del,
       vtysh_user_del_cmd,
       "user remove WORD",
       "User account\n"
       "Delete a user account\n"
       "User name to be deleted\n")
{
    return delete_user(argv[0]);
}

DEFUN(vtysh_reboot,
      vtysh_reboot_cmd,
      "reboot",
      "Reload the switch\n")
{
   char *arg[1];
   arg[0] = "/sbin/reboot";
   execute_command("sudo", 1 ,(const char **)arg);
   return CMD_SUCCESS;
}

DEFUN (vtysh_demo_mac_tok,
       vtysh_demo_mac_tok_cmd,
       "demo_mac_tok MAC",
       "Demo command to check MAC type token\n"
       "MAC address\n")
{
   vty_out(vty,"MAC type token verified:%s%s",argv[0],VTY_NEWLINE);
   return CMD_SUCCESS;
}
#endif



#ifdef ENABLE_OVSDB

int vtysh_alias_count = 0;
struct vtysh_alias_data *vtysh_aliases[VTYSH_MAX_ALIAS_SUPPORTED] = {NULL};
char vtysh_alias_cmd_help_string[] = VTYSH_ALIAS_CMD_HELPSTRING;


/*
  * Function       : vtysh_alias_load_alias_table
  * Responsibility : Reloads the alias information from ovsdb
  * Parameters     : void
  * Return         : success/failure
 */
int
vty_alias_load_alias_table(void)
{
    const struct ovsrec_cli_alias *alias_row = NULL;

    vtysh_alias_count = 0;

    OVSREC_CLI_ALIAS_FOR_EACH(alias_row, idl)
    {
        vtysh_aliases[vtysh_alias_count] =
            (struct vtysh_alias_data*) malloc(sizeof(struct vtysh_alias_data));
        memset(vtysh_aliases[vtysh_alias_count], 0,
                sizeof(struct vtysh_alias_data));

        strncpy(vtysh_aliases[vtysh_alias_count]->alias_def_str,
                alias_row->alias_name, VTYSH_MAX_ALIAS_DEF_LEN);
        strncpy(vtysh_aliases[vtysh_alias_count]->alias_list_str,
                alias_row->alias_definition, VTYSH_MAX_ALIAS_LIST_LEN);

        strncpy(vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args,
                vtysh_aliases[vtysh_alias_count]->alias_def_str,
                VTYSH_MAX_ALIAS_DEF_LEN);
        strcat(vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args,
                " .LINE");

        /* Prepare the command element & install the command in config node */
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element.string =
            vtysh_aliases[vtysh_alias_count]->alias_def_str;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element.func   =
            vtysh_alias_callback;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element.doc    =
            vtysh_alias_cmd_help_string;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element.attr   = CMD_ATTR_NOLOCK;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element.daemon = 0;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.string =
            vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.func =
            vtysh_alias_callback;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.doc =
            vtysh_alias_cmd_help_string;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.attr = CMD_ATTR_NOLOCK;
        vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.daemon = 0;

        /* install the new commands with alias definition as token */
        install_element(CONFIG_NODE,
               &vtysh_aliases[vtysh_alias_count]->alias_cmd_element);
        install_element(CONFIG_NODE,
               &vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args);
        vtysh_alias_count++;
    }

    return CMD_SUCCESS;
}

/*
 * Function       : vtysh_alias_save_alias
 * Responsibility : Adds the given alias into the ovsdb
 * Parameters     : alias name, definition
 * Return         : success/failure
 */
int
vtysh_alias_save_alias(char *name, char *definition)
{
   struct ovsrec_cli_alias *alias_row = NULL;
   struct ovsdb_idl_txn *status_txn = NULL;
   enum ovsdb_idl_txn_status status;

   status_txn = cli_do_config_start();

   if (status_txn == NULL) {
      VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
      cli_do_config_abort(status_txn);
      return CMD_OVSDB_FAILURE;
   }
   alias_row = ovsrec_cli_alias_insert(status_txn);
   ovsrec_cli_alias_set_alias_name(alias_row, name);
   ovsrec_cli_alias_set_alias_definition(alias_row, definition);
   status = cli_do_config_finish(status_txn);
   if ((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
      && (status != TXN_UNCHANGED))
   {
      VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
      return CMD_OVSDB_FAILURE;
   }

   return CMD_SUCCESS;
}


/*
 * Function       : vtysh_alias_delete_alias
 * Responsibility : Removes the given alias from the ovsdb
 * Parameters     : alias name
 * Return         : success/failure
*/
int
vtysh_alias_delete_alias(char *name)
{
   const struct ovsrec_cli_alias *alias_row = NULL;
   struct ovsdb_idl_txn *status_txn = NULL;
   enum ovsdb_idl_txn_status status;


   OVSREC_CLI_ALIAS_FOR_EACH (alias_row, idl)
   {
      if (strcmp(alias_row->alias_name, name) == 0) {
         status_txn = cli_do_config_start();

         if (status_txn == NULL) {
            VLOG_ERR(OVSDB_TXN_CREATE_ERROR);
            cli_do_config_abort(status_txn);
            return CMD_OVSDB_FAILURE;
         }
         ovsrec_cli_alias_delete(alias_row);
         status = cli_do_config_finish(status_txn);
         if ((status != TXN_SUCCESS) && (status != TXN_INCOMPLETE)
               && (status != TXN_UNCHANGED))
         {
            VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
            return CMD_OVSDB_FAILURE;
         }

         return CMD_SUCCESS;
      }
   }
   return CMD_SUCCESS;
}


DEFUN (vtysh_alias_cli,
      vtysh_alias_cli_cmd,
      "alias WORD .LINE",
      "Create a short name for the specified command(s)\n"
      "Alias command (Max Length 30 characters)\n"
      "Alias definition. Multiple commands should be separated by \";\". "
      "Parameters $1, $2, etc. in the body are replaced by "
      "the corresponding argument from the command line. "
      "Extra arguments are appended at the end. (Max length 400 characters)\n")
{
   int i = 0, ret_val = 0;

   if (argc == 0) return CMD_WARNING;

   /* Check if it is alias deletion */
   if (vty_flags & CMD_FLAG_NO_CMD)
   {
      int found = 0;

      for (i = 0; i < vtysh_alias_count; i++)
      {
         if(NULL == vtysh_aliases[i])
         {
            assert(0);
            /* Data integrity failure */
         }
         if(strcmp(vtysh_aliases[i]->alias_def_str, argv[0]) == 0)
         {
            vtysh_alias_delete_alias(vtysh_aliases[i]->alias_def_str);
#ifdef VTY_INFRA_FIXED
            cmd_terminate_element(&vtysh_aliases[i]->alias_cmd_element);
            cmd_terminate_element(&vtysh_aliases[i]->alias_cmd_element_with_args);
            free(vtysh_aliases[i]);
            vtysh_aliases[i] = vtysh_aliases[vtysh_alias_count-1];
            vtysh_aliases[vtysh_alias_count-1] = NULL;
            vtysh_alias_count--;
#else
            vtysh_aliases[i]->alias_cmd_element.attr |= CMD_ATTR_HIDDEN;
            vtysh_aliases[i]->alias_cmd_element.attr |= CMD_ATTR_NOT_ENABLED;
            vtysh_aliases[i]->alias_cmd_element.attr |= CMD_ATTR_DISABLED;
            vtysh_aliases[i]->alias_cmd_element_with_args.attr |=
                CMD_ATTR_HIDDEN;
            vtysh_aliases[i]->alias_cmd_element_with_args.attr |=
                CMD_ATTR_NOT_ENABLED;
            vtysh_aliases[i]->alias_cmd_element_with_args.attr |=
                CMD_ATTR_DISABLED;
            //TODO :
            /* free cannot be done as cmd element is still referred by vector
               */
            //free(vtysh_aliases[i]);
            vtysh_aliases[i] = vtysh_aliases[vtysh_alias_count-1];
            vtysh_aliases[vtysh_alias_count-1] = NULL;
            vtysh_alias_count--;
#endif
            found = 1;
            break;
         }
      }
      if (0 == found)
      {
         vty_out(vty, VTYSH_ERROR_ALIAS_NOT_FOUND, argv[0]);
         return CMD_SUCCESS;
      }
      return CMD_SUCCESS;
   }

   if (vtysh_alias_count >= VTYSH_MAX_ALIAS_SUPPORTED)
   {
      vty_out(vty, VTYSH_ERROR_MAX_ALIASES_EXCEEDED);
      return CMD_SUCCESS;
   }

   if (strlen(argv[0]) > VTYSH_MAX_ALIAS_DEF_LEN)
   {
      vty_out(vty, VTYSH_ERROR_MAX_ALIAS_LEN_EXCEEDED);
      return CMD_SUCCESS;
   }

   /* check if command already exists */
   ret_val =  cmd_try_execute_command (vty, CONST_CAST(char*,argv[0]));
   if(CMD_ERR_NO_MATCH != ret_val)
   {
      vty_out(vty, VTYSH_ERROR_ALIAS_NAME_ALREADY_EXISTS);
      return CMD_SUCCESS;
   }

   if(NULL != vtysh_aliases[vtysh_alias_count])
   {
      assert(0);
      /* Data integrity failure */
      free(vtysh_aliases[vtysh_alias_count]);
   }
   vtysh_aliases[vtysh_alias_count] =
       (struct vtysh_alias_data*) malloc(sizeof(struct vtysh_alias_data));
   memset(vtysh_aliases[vtysh_alias_count], 0,
           sizeof(struct vtysh_alias_data));

   strncpy(vtysh_aliases[vtysh_alias_count]->alias_def_str, argv[0],
           VTYSH_MAX_ALIAS_DEF_LEN);
   strncpy(vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args, argv[0],
         VTYSH_MAX_ALIAS_DEF_LEN_WITH_ARGS);
   strcat(vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args, " .LINE");

   for (i = 1; i < argc; i++)
   {
      /* Read each args, and append to the command string */
      if(VTYSH_MAX_ALIAS_LIST_LEN <=
              strlen(vtysh_aliases[vtysh_alias_count]->alias_list_str) +
              strlen(argv[i]) + 2)
      {
         free(vtysh_aliases[vtysh_alias_count]);
         vtysh_aliases[vtysh_alias_count] = NULL;
         vty_out(vty, VTYSH_ERROR_MAX_ALIAS_LEN_EXCEEDED);
         return CMD_SUCCESS;
      }
      strcat(vtysh_aliases[vtysh_alias_count]->alias_list_str, argv[i]);
      strcat(vtysh_aliases[vtysh_alias_count]->alias_list_str, " ");
   }

   /* Prepare the command element and install the command in config node */
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element.string =
       vtysh_aliases[vtysh_alias_count]->alias_def_str;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element.func =
       vtysh_alias_callback;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element.doc =
       vtysh_alias_cmd_help_string;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element.attr = CMD_ATTR_NOLOCK;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element.daemon = 0;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.string =
       vtysh_aliases[vtysh_alias_count]->alias_def_str_with_args;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.func =
       vtysh_alias_callback;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.doc =
       vtysh_alias_cmd_help_string;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.attr = CMD_ATTR_NOLOCK;
   vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args.daemon = 0;

   vtysh_alias_save_alias(vtysh_aliases[vtysh_alias_count]->alias_def_str,
         vtysh_aliases[vtysh_alias_count]->alias_list_str);

   /* install the new commands with alias definition as token */
   install_element(CONFIG_NODE,
           &vtysh_aliases[vtysh_alias_count]->alias_cmd_element);
   install_element(CONFIG_NODE,
           &vtysh_aliases[vtysh_alias_count]->alias_cmd_element_with_args);

   vtysh_alias_count++;

   return CMD_SUCCESS;
}

DEFUN_NO_FORM (vtysh_alias_cli,
      vtysh_alias_cli_cmd,
      "alias WORD",
      "Create a short name for the specified command(s)\n"
      "Alias command name\n");


/*
 * Function       : vtysh_alias_string_to_int
 * Responsibility : Function to convert part of string to integer
 * Parameters     : string to convert
 * Return         : integer
*/
int
vtysh_alias_string_to_int(char *str)
{
   int i = 0, ret = 0;
   if(NULL == str)
   {
      return 0;
   }
   while(str[i] != '\0')
   {
      if(str[i] >= '0' && str[i] <= '9')
      {
         ret = ret * 10 + str[i]-'0';
         i++;
      }
      else
      {
         return ret;
      }
   }

   return ret;
}


/*
 * Function       : vtysh_alias_callback
 * Responsibility : Generic Callback function for Aliases
 * Parameters     : cmd element, vty, argc/argv, flags
 * Return         : cmd error
 */
int
vtysh_alias_callback(struct cmd_element *self, struct vty *vty,
      int vty_flags, int argc, const char *argv[])
{
   char cmd_now[VTYSH_MAX_ALIAS_LIST_LEN] = {0};
   int len = 0, i = 0;
   char *found_cmd = NULL;
   int arg_count = 0, max_arg_count = 0;
   char current_cmd[VTYSH_MAX_ALIAS_LIST_LEN] = {0};
   char *prev_buf = vty->buf;

   for(len = 0; len <= vty->length && vty->buf[len] != ' '; len++)
   {
      cmd_now[len] = vty->buf[len];
   }
   cmd_now[len] = '\0';

   for (i = 0; i < vtysh_alias_count; i++)
   {
      if(NULL == vtysh_aliases[i])
      {
         assert(0);
         /* Data integrity failure */
      }
      if(strncmp(vtysh_aliases[i]->alias_def_str, cmd_now,
              strlen(cmd_now)) == 0)
      {
         found_cmd = vtysh_aliases[i]->alias_list_str;
         break;
      }
   }

   if(NULL == found_cmd)
   {
      vty_out(vty, VTYSH_ERROR_ALIAS_NOT_FOUND, cmd_now);
      return CMD_SUCCESS;
   }

   for(len = 0, i = 0;
           len <= VTYSH_MAX_ALIAS_LIST_LEN && found_cmd[len] != '\0'; len++)
   {
      if(found_cmd[len] == '$')
      {
         arg_count = vtysh_alias_string_to_int(&(found_cmd[len+1]));
         if( (arg_count != 0) &&( arg_count <= argc))
         {
            strcat(current_cmd, argv[arg_count-1]);
            strcat(current_cmd, " ");
            i+= strlen(current_cmd);
            if(max_arg_count < arg_count)
            {
               max_arg_count = arg_count;
            }
            while(arg_count)
            {
               len++;
               arg_count/=10;
            }
         }
         else
         {
            current_cmd[i++] = found_cmd[len];
         }
      }
      else if (found_cmd[len] == ';')
      {
         /* Execute the command */
         char *strt = current_cmd;

         while(*strt == ' ')
         {
            strt++;
            if(*strt == '\0') break;
         }

         if((strncmp(cmd_now, strt, strlen(cmd_now)) == 0) &&
               (*(strt + strlen(cmd_now) + 1) == ' ' ||
                       *(strt + strlen(cmd_now) + 1) == '\0'))
         {
            vty_out(vty, VTYSH_ERROR_ALIAS_LOOP_ALIAS);
            return CMD_SUCCESS;
         }
         vty->buf = strt;
         vty->length = strlen(strt);
         //vty_out(vty, "Executing the command \"%s\" %s", strt, VTY_NEWLINE);
         vty_command (vty, vty->buf);

         memset(current_cmd, 0, VTYSH_MAX_ALIAS_LIST_LEN);
         i = 0;
      }
      else
      {
         current_cmd[i++] = found_cmd[len];
      }
   }

   max_arg_count++; /* To find unused args */

   while(argc >= max_arg_count)
   {
      strcat(current_cmd, " ");
      strcat(current_cmd, argv[max_arg_count - 1]);
      max_arg_count++;
   }
   if(current_cmd[0] != 0)
   {
      /* Execute the command */
      char *strt = current_cmd;

      while(*strt == ' ')
      {
         strt++;
         if(*strt == '\0') break;
      }
      if((strncmp(cmd_now, strt, strlen(cmd_now)) == 0) &&
            (*(strt + strlen(cmd_now) + 1) == ' ' ||
                    *(strt + strlen(cmd_now) + 1) == '\0'))
      {
         vty_out(vty, VTYSH_ERROR_ALIAS_LOOP_ALIAS);
         return CMD_SUCCESS;
      }
      //vty_out(vty, "Executing the command \"%s\" \n", current_cmd);
      vty->buf = strt;
      vty->length = strlen(strt);
      vty_command (vty, vty->buf);
   }
   vty->buf = prev_buf;
   return CMD_SUCCESS;
}


DEFUN (vtysh_show_alias_cli,
      vtysh_show_alias_cli_cmd,
      "show alias",
      SHOW_STR
      "Short names configured for a set of commands\n")
{
   int i = 0;

   vty_out(vty, " %-30s %s %s", "Alias Name", "Alias Definition", VTY_NEWLINE);
   vty_out(vty, " ----------------------------------------"
           "---------------------------------------%s", VTY_NEWLINE);

   for (i = 0; i < vtysh_alias_count; i++)
   {
      if(NULL == vtysh_aliases[i])
      {
         assert(0);
         /* Data integrity failure */
      }

      {
         vty_out(vty, " %-30s %s %s", vtysh_aliases[i]->alias_def_str,
               vtysh_aliases[i]->alias_list_str, VTY_NEWLINE);
      }
   }
   return CMD_SUCCESS;
}


/*
 * Function : alias_vty_init
 * Responsibility : install the alias commands
 * Parameters :   void
 * Return : void
 */
void
alias_vty_init()
{
   install_element(CONFIG_NODE, &vtysh_alias_cli_cmd);
   install_element(CONFIG_NODE, &no_vtysh_alias_cli_cmd);
   install_element(ENABLE_NODE, &vtysh_show_alias_cli_cmd);

   vty_alias_load_alias_table();
}

int is_valid_ip_address(const char *ip_value)
{
    struct in_addr addr;
    struct in6_addr addrv6;
    boolean  is_ipv4 = TRUE;
    unsigned short ip_length;
    char ip_tmp[MAX_IPV6_STRING_LENGTH];

    memset(ip_tmp, 0, MAX_IPV6_STRING_LENGTH);
    memset (&addr, 0, sizeof (struct in_addr));
    memset (&addrv6, 0, sizeof (struct in6_addr));

    if(NULL == ip_value)
    {
        VLOG_ERR("Invalid IPv4 or IPv6 address\n");
        return FALSE;
    }

    ip_length = strlen(ip_value);
    strncpy(ip_tmp,ip_value,ip_length);
    ip_tmp[ip_length + 1] = '\0';
    strtok(ip_tmp,"/");

    if(inet_pton(AF_INET, ip_tmp, &addr) <= 0)
    {
        if(inet_pton(AF_INET6, ip_tmp, &addrv6) <= 0)
        {
            VLOG_ERR("Invalid IPv4 or IPv6 address\n");
            return FALSE;
        }
        is_ipv4 = FALSE;
    }

    if((is_ipv4) && (!IS_VALID_IPV4(htonl(addr.s_addr))))
    {
        VLOG_ERR("IPv4: Broadcast, multicast and loopback addresses are not allowed\n");
        return FALSE;
    }

    if((!is_ipv4) && (!IS_IPV6_GLOBAL_UNICAST(&addrv6)))
    {
        VLOG_ERR("IPv6: Link-local, multicast and loopback addresses are not allowed\n");
        return FALSE;
    }

    return TRUE;
}

#endif /* ENABLE_OVSDB */

void
vtysh_init_vty (void)
{
   /* Make vty structure. */
   vty = vty_new ();
   vty->type = VTY_SHELL;
   vty->node = VIEW_NODE;

   /* Initialize commands. */
   cmd_init (0);

   /* Install nodes. */
   install_node (&bgp_node, NULL);
#ifndef ENABLE_OVSDB
   install_node (&rip_node, NULL);
#endif
   install_node (&interface_node, NULL);
#ifdef ENABLE_OVSDB
   install_node (&vlan_node, NULL);
   install_node (&mgmt_interface_node, NULL);
   install_node (&link_aggregation_node, NULL);
   install_node (&vlan_interface_node, NULL);
#endif
   install_node (&rmap_node, NULL);
   install_node (&zebra_node, NULL);
   install_node (&bgp_vpnv4_node, NULL);
   install_node (&bgp_ipv4_node, NULL);
   install_node (&bgp_ipv4m_node, NULL);
/* #ifdef HAVE_IPV6 */
   install_node (&bgp_ipv6_node, NULL);
   install_node (&bgp_ipv6m_node, NULL);
/* #endif */
#ifndef ENABLE_OVSDB
   install_node (&ospf_node, NULL);
/* #ifdef HAVE_IPV6 */
   install_node (&ripng_node, NULL);
   install_node (&ospf6_node, NULL);
/* #endif */
   install_node (&babel_node, NULL);
#endif
   install_node (&keychain_node, NULL);
   install_node (&keychain_key_node, NULL);
#ifndef ENABLE_OVSDB
   install_node (&isis_node, NULL);
#endif
   install_node (&vty_node, NULL);
   install_node (&dhcp_server_node, NULL);
   install_node (&tftp_server_node, NULL);

   vtysh_install_default (VIEW_NODE);
   vtysh_install_default (ENABLE_NODE);
   vtysh_install_default (CONFIG_NODE);
   vtysh_install_default (BGP_NODE);
#ifndef ENABLE_OVSDB
   vtysh_install_default (RIP_NODE);
#endif
   vtysh_install_default (INTERFACE_NODE);
#ifdef ENABLE_OVSDB
   vtysh_install_default (VLAN_NODE);
   vtysh_install_default (MGMT_INTERFACE_NODE);
   vtysh_install_default (LINK_AGGREGATION_NODE);
   vtysh_install_default (VLAN_INTERFACE_NODE);
   vtysh_install_default (DHCP_SERVER_NODE);
   vtysh_install_default (TFTP_SERVER_NODE);
#endif
   vtysh_install_default (RMAP_NODE);
   vtysh_install_default (ZEBRA_NODE);
   vtysh_install_default (BGP_VPNV4_NODE);
   vtysh_install_default (BGP_IPV4_NODE);
   vtysh_install_default (BGP_IPV4M_NODE);
   vtysh_install_default (BGP_IPV6_NODE);
   vtysh_install_default (BGP_IPV6M_NODE);
#ifndef ENABLE_OVSDB
   vtysh_install_default (OSPF_NODE);
   vtysh_install_default (RIPNG_NODE);
   vtysh_install_default (OSPF6_NODE);
   vtysh_install_default (BABEL_NODE);
   vtysh_install_default (ISIS_NODE);
#endif
   vtysh_install_default (KEYCHAIN_NODE);
   vtysh_install_default (KEYCHAIN_KEY_NODE);
   vtysh_install_default (VTY_NODE);

#ifdef ENABLE_OVSDB
  install_element (VIEW_NODE, &vtysh_show_context_client_list_cmd);
  install_element (ENABLE_NODE, &vtysh_show_context_client_list_cmd);
  install_element(CONFIG_NODE, &vtysh_demo_mac_tok_cmd);

   install_element (CONFIG_NODE, &vtysh_dhcp_server_cmd);
   install_element (DHCP_SERVER_NODE, &config_exit_cmd);
   install_element (DHCP_SERVER_NODE, &config_end_cmd);

   install_element (CONFIG_NODE, &vtysh_tftp_server_cmd);
   install_element (TFTP_SERVER_NODE, &config_exit_cmd);
   install_element (TFTP_SERVER_NODE, &config_end_cmd);
#endif /* ENABLE_OVSDB */

   install_element (VIEW_NODE, &vtysh_enable_cmd);
   install_element (ENABLE_NODE, &vtysh_config_terminal_cmd);
   install_element (ENABLE_NODE, &vtysh_disable_cmd);
#ifndef ENABLE_OVSDB
   install_element (BGP_NODE, &vtysh_quit_bgpd_cmd);
   install_element (LINK_AGGREGATION_NODE, &vtysh_quit_mgmt_interface_cmd);
   install_element (OSPF6_NODE, &vtysh_quit_ospf6d_cmd);
   install_element (OSPF_NODE, &vtysh_quit_ospfd_cmd);
   install_element (RIPNG_NODE, &vtysh_quit_ripngd_cmd);
   install_element (VLAN_INTERFACE_NODE, &vtysh_quit_interface_cmd);
   install_element (BGP_IPV4M_NODE, &vtysh_quit_bgpd_cmd);
   install_element (RIP_NODE, &vtysh_quit_ripd_cmd);
   install_element (BGP_IPV4_NODE, &vtysh_quit_bgpd_cmd);
   install_element (ENABLE_NODE, &vtysh_quit_all_cmd);
   install_element (BGP_IPV6_NODE, &vtysh_quit_bgpd_cmd);
   install_element (MGMT_INTERFACE_NODE, &vtysh_quit_mgmt_interface_cmd);
   install_element (BGP_VPNV4_NODE, &vtysh_quit_bgpd_cmd);
   install_element (BGP_IPV6M_NODE, &vtysh_quit_bgpd_cmd);
   install_element (KEYCHAIN_NODE, &vtysh_quit_ripd_cmd);
   install_element (ISIS_NODE, &vtysh_quit_isisd_cmd);
   install_element (KEYCHAIN_KEY_NODE, &vtysh_quit_ripd_cmd);
   install_element (VIEW_NODE, &vtysh_quit_all_cmd);
   install_element (RMAP_NODE, &vtysh_quit_rmap_cmd);
   install_element (INTERFACE_NODE, &vtysh_quit_interface_cmd);
   install_element (VTY_NODE, &vtysh_quit_line_vty_cmd);
#endif
   /* "exit" command. */
   install_element (VIEW_NODE, &vtysh_exit_all_cmd);
   install_element (CONFIG_NODE, &vtysh_exit_all_cmd);
   /* install_element (CONFIG_NODE, &vtysh_quit_all_cmd); */
   install_element (ENABLE_NODE, &vtysh_exit_all_cmd);
#ifndef ENABLE_OVSDB
   install_element (RIP_NODE, &vtysh_exit_ripd_cmd);
   install_element (RIPNG_NODE, &vtysh_exit_ripngd_cmd);
   install_element (OSPF_NODE, &vtysh_exit_ospfd_cmd);
   install_element (OSPF6_NODE, &vtysh_exit_ospf6d_cmd);
#endif
   install_element (BGP_NODE, &vtysh_exit_bgpd_cmd);
   install_element (BGP_VPNV4_NODE, &vtysh_exit_bgpd_cmd);
   install_element (BGP_IPV4_NODE, &vtysh_exit_bgpd_cmd);
   install_element (BGP_IPV4M_NODE, &vtysh_exit_bgpd_cmd);
   install_element (BGP_IPV6_NODE, &vtysh_exit_bgpd_cmd);
   install_element (BGP_IPV6M_NODE, &vtysh_exit_bgpd_cmd);

   policy_vty_init();
   bgp_vty_init();
#ifndef ENABLE_OVSDB
   install_element (ISIS_NODE, &vtysh_exit_isisd_cmd);
#endif
   install_element (KEYCHAIN_NODE, &vtysh_exit_ripd_cmd);
   install_element (KEYCHAIN_KEY_NODE, &vtysh_exit_ripd_cmd);
   install_element (RMAP_NODE, &vtysh_exit_rmap_cmd);
   install_element (VTY_NODE, &vtysh_exit_line_vty_cmd);

   /* "end" command. */
   install_element (CONFIG_NODE, &vtysh_end_all_cmd);
   install_element (ENABLE_NODE, &vtysh_end_all_cmd);
#ifndef ENABLE_OVSDB
   install_element (RIP_NODE, &vtysh_end_all_cmd);
   install_element (RIPNG_NODE, &vtysh_end_all_cmd);
   install_element (OSPF_NODE, &vtysh_end_all_cmd);
   install_element (OSPF6_NODE, &vtysh_end_all_cmd);
   install_element (BABEL_NODE, &vtysh_end_all_cmd);
#endif
   install_element (BGP_NODE, &vtysh_end_all_cmd);
   install_element (BGP_IPV4_NODE, &vtysh_end_all_cmd);
   install_element (BGP_IPV4M_NODE, &vtysh_end_all_cmd);
   install_element (BGP_VPNV4_NODE, &vtysh_end_all_cmd);
   install_element (BGP_IPV6_NODE, &vtysh_end_all_cmd);
   install_element (BGP_IPV6M_NODE, &vtysh_end_all_cmd);
#ifndef ENABLE_OVSDB
   install_element (ISIS_NODE, &vtysh_end_all_cmd);
#endif
   install_element (KEYCHAIN_NODE, &vtysh_end_all_cmd);
   install_element (KEYCHAIN_KEY_NODE, &vtysh_end_all_cmd);
   install_element (RMAP_NODE, &vtysh_end_all_cmd);
   install_element (VTY_NODE, &vtysh_end_all_cmd);
#ifndef ENABLE_OVSDB
   install_element (INTERFACE_NODE, &interface_desc_cmd);
   install_element (INTERFACE_NODE, &no_interface_desc_cmd);
#endif
   install_element (INTERFACE_NODE, &vtysh_end_all_cmd);
   install_element (INTERFACE_NODE, &vtysh_exit_interface_cmd);
#ifndef ENABLE_OVSDB
   install_element (CONFIG_NODE, &router_rip_cmd);
#ifdef HAVE_IPV6
   install_element (CONFIG_NODE, &router_ripng_cmd);
#endif
   install_element (CONFIG_NODE, &router_ospf_cmd);
#ifdef HAVE_IPV6
   install_element (CONFIG_NODE, &router_ospf6_cmd);
#endif
   install_element (CONFIG_NODE, &router_babel_cmd);
   install_element (CONFIG_NODE, &router_isis_cmd);
#endif /* ENABLE_OVSDB */

   //install_element (CONFIG_NODE, &router_bgp_cmd);
   //install_element (CONFIG_NODE, &router_bgp_view_cmd);
   install_element (BGP_NODE, &address_family_vpnv4_cmd);
   install_element (BGP_NODE, &address_family_vpnv4_unicast_cmd);
   install_element (BGP_NODE, &address_family_ipv4_unicast_cmd);
   install_element (BGP_NODE, &address_family_ipv4_multicast_cmd);
#ifdef HAVE_IPV6
   install_element (BGP_NODE, &address_family_ipv6_cmd);
   install_element (BGP_NODE, &address_family_ipv6_unicast_cmd);
#endif
   install_element (BGP_VPNV4_NODE, &exit_address_family_cmd);
   install_element (BGP_IPV4_NODE, &exit_address_family_cmd);
   install_element (BGP_IPV4M_NODE, &exit_address_family_cmd);
   install_element (BGP_IPV6_NODE, &exit_address_family_cmd);
   install_element (BGP_IPV6M_NODE, &exit_address_family_cmd);
   install_element (CONFIG_NODE, &key_chain_cmd);
   //install_element (CONFIG_NODE, &route_map_cmd);
#ifndef ENABLE_OVSDB
   install_element (CONFIG_NODE, &vtysh_line_vty_cmd);
#endif
   install_element (KEYCHAIN_NODE, &key_cmd);
   install_element (KEYCHAIN_NODE, &key_chain_cmd);
   install_element (KEYCHAIN_KEY_NODE, &key_chain_cmd);
#ifndef ENABLE_OVSDB
   install_element (CONFIG_NODE, &vtysh_no_interface_cmd);
#endif

#ifdef ENABLE_OVSDB
   install_element (CONFIG_NODE, &vtysh_interface_cmd);
   install_element (CONFIG_NODE, &vtysh_interface_vlan_cmd);
   install_element (CONFIG_NODE, &no_vtysh_interface_cmd);
   install_element (CONFIG_NODE, &no_vtysh_interface_vlan_cmd);
   install_element (VLAN_INTERFACE_NODE, &vtysh_exit_interface_cmd);
   install_element (VLAN_INTERFACE_NODE, &vtysh_end_all_cmd);
#endif

   install_element (ENABLE_NODE, &vtysh_show_running_config_cmd);
#ifdef ENABLE_OVSDB
   install_element (CONFIG_NODE, &vtysh_vlan_cmd);
   install_element (CONFIG_NODE, &vtysh_interface_mgmt_cmd);
   install_element(CONFIG_NODE, &vtysh_no_vlan_cmd);
   install_element (MGMT_INTERFACE_NODE, &vtysh_exit_mgmt_interface_cmd);
   install_element (MGMT_INTERFACE_NODE, &vtysh_end_all_cmd);
   install_element (CONFIG_NODE, &vtysh_intf_link_aggregation_cmd);
   install_element (LINK_AGGREGATION_NODE, &vtysh_exit_mgmt_interface_cmd);
   install_element (LINK_AGGREGATION_NODE, &vtysh_end_all_cmd);
#endif /* ENABLE_OVSDB */
  install_element (ENABLE_NODE, &vtysh_copy_runningconfig_startupconfig_cmd);
#ifdef ENABLE_OVSDB
  install_element (ENABLE_NODE, &vtysh_copy_startupconfig_runningconfig_cmd);
#endif /* ENABLE_OVSDB */
#ifndef ENABLE_OVSDB
  install_element (ENABLE_NODE, &vtysh_write_file_cmd);
  install_element (ENABLE_NODE, &vtysh_write_cmd);
  /* "write terminal" command. */
  install_element (ENABLE_NODE, &vtysh_write_terminal_cmd);
#endif /* ENABLE_OVSDB */
  install_element (CONFIG_NODE, &vtysh_integrated_config_cmd);
#ifndef ENABLE_OVSDB
  install_element (CONFIG_NODE, &no_vtysh_integrated_config_cmd);
  /* "write memory" command. */
  install_element (ENABLE_NODE, &vtysh_write_memory_cmd);
#endif
#ifndef ENABLE_OVSDB
  install_element (VIEW_NODE, &vtysh_terminal_length_cmd);
  install_element (ENABLE_NODE, &vtysh_terminal_length_cmd);
  install_element (VIEW_NODE, &vtysh_terminal_no_length_cmd);
  install_element (ENABLE_NODE, &vtysh_terminal_no_length_cmd);
  install_element (VIEW_NODE, &vtysh_show_daemons_cmd);
  install_element (ENABLE_NODE, &vtysh_show_daemons_cmd);
#endif
#ifdef ENABLE_OVSDB
  install_element (ENABLE_NODE, &show_startup_config_cmd);
#endif /* ENABLE_OVSDB */

#ifndef ENABLE_OVSDB
  install_element (VIEW_NODE, &vtysh_ping_cmd);
  install_element (VIEW_NODE, &vtysh_ping_ip_cmd);
  install_element (VIEW_NODE, &vtysh_traceroute_cmd);
  install_element (VIEW_NODE, &vtysh_traceroute_ip_cmd);
#ifdef HAVE_IPV6
   install_element (VIEW_NODE, &vtysh_ping6_cmd);
   install_element (VIEW_NODE, &vtysh_traceroute6_cmd);
#endif
   install_element (VIEW_NODE, &vtysh_telnet_cmd);
   install_element (VIEW_NODE, &vtysh_telnet_port_cmd);
   install_element (VIEW_NODE, &vtysh_ssh_cmd);
   install_element (ENABLE_NODE, &vtysh_ping_cmd);
   install_element (ENABLE_NODE, &vtysh_ping_ip_cmd);
   install_element (ENABLE_NODE, &vtysh_traceroute_cmd);
   install_element (ENABLE_NODE, &vtysh_traceroute_ip_cmd);
#ifdef HAVE_IPV6
   install_element (ENABLE_NODE, &vtysh_ping6_cmd);
   install_element (ENABLE_NODE, &vtysh_traceroute6_cmd);
#endif
  install_element (ENABLE_NODE, &vtysh_telnet_cmd);
  install_element (ENABLE_NODE, &vtysh_telnet_port_cmd);
  install_element (ENABLE_NODE, &vtysh_ssh_cmd);
#endif /* ENABLE_OVSDB */
  install_element (ENABLE_NODE, &vtysh_start_shell_cmd);
#ifndef ENABLE_OVSDB
  install_element (ENABLE_NODE, &vtysh_start_bash_cmd);
  install_element (ENABLE_NODE, &vtysh_start_zsh_cmd);
  install_element (VIEW_NODE, &vtysh_show_memory_cmd);
  install_element (ENABLE_NODE, &vtysh_show_memory_cmd);
#endif
#ifndef ENABLE_OVSDB
  /* Logging */
  install_element (ENABLE_NODE, &vtysh_show_logging_cmd);
  install_element (VIEW_NODE, &vtysh_show_logging_cmd);
  install_element (CONFIG_NODE, &vtysh_log_stdout_cmd);
  install_element (CONFIG_NODE, &vtysh_log_stdout_level_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_stdout_cmd);
  install_element (CONFIG_NODE, &vtysh_log_file_cmd);
  install_element (CONFIG_NODE, &vtysh_log_file_level_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_file_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_file_level_cmd);
  install_element (CONFIG_NODE, &vtysh_log_monitor_cmd);
  install_element (CONFIG_NODE, &vtysh_log_monitor_level_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_monitor_cmd);
  install_element (CONFIG_NODE, &vtysh_log_syslog_cmd);
  install_element (CONFIG_NODE, &vtysh_log_syslog_level_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_syslog_cmd);
  install_element (CONFIG_NODE, &vtysh_log_trap_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_trap_cmd);
  install_element (CONFIG_NODE, &vtysh_log_facility_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_facility_cmd);
  install_element (CONFIG_NODE, &vtysh_log_record_priority_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_record_priority_cmd);
  install_element (CONFIG_NODE, &vtysh_log_timestamp_precision_cmd);
  install_element (CONFIG_NODE, &no_vtysh_log_timestamp_precision_cmd);
  install_element (CONFIG_NODE, &vtysh_passwd_cmd);
#endif
#ifndef ENABLE_OVSDB
  install_element (CONFIG_NODE, &vtysh_service_password_encrypt_cmd);
  install_element (CONFIG_NODE, &no_vtysh_service_password_encrypt_cmd);
  install_element (CONFIG_NODE, &vtysh_password_cmd);
  install_element (CONFIG_NODE, &vtysh_password_text_cmd);
  install_element (CONFIG_NODE, &vtysh_enable_password_cmd);
  install_element (CONFIG_NODE, &vtysh_enable_password_text_cmd);
  install_element (CONFIG_NODE, &no_vtysh_enable_password_cmd);
#endif
  install_element (ENABLE_NODE, &vtysh_passwd_cmd);
  install_element (ENABLE_NODE, &vtysh_user_add_cmd);
  install_element (ENABLE_NODE, &vtysh_user_del_cmd);

  install_element (ENABLE_NODE, &vtysh_reboot_cmd);

#ifdef ENABLE_OVSDB
  lldp_vty_init();
  vrf_vty_init();
  neighbor_vty_init();
  intf_vty_init();
  l3routes_vty_init();
  vlan_vty_init();
  aaa_vty_init();
   dhcp_tftp_vty_init();
  /* Initialise System LED cli */
  led_vty_init();
  mgmt_intf_vty_init();
  /* Initialise System cli */
  system_vty_init();
  fan_vty_init();
  temperature_vty_init();
  alias_vty_init();
  logrotate_vty_init();

  /* Initialise power supply cli */
  powersupply_vty_init();
  lacp_vty_init();

  /* Initialize ECMP CLI */
  ecmp_vty_init();
#endif
}
