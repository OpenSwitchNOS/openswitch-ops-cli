/*
 * Copyright (C) 1997 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
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
/************************************************************************//**
 * @ingroup quagga
 *
 * @file vtysh_ovsdb_config.c
 * Source for config infra to walkthrough ovsdb tables.
 *
 ***************************************************************************/

#include "openvswitch/vlog.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_if.h"
#include "lib/vty.h"
#include "vtysh_ovsdb_config_context.h"
#include "vtysh/utils/vlan_vtysh_utils.h"
#include "vtysh/vtysh_ovsdb_router_context.h"

/* Intialize the module "vtysh_ovsdb_config" used for log macros */
VLOG_DEFINE_THIS_MODULE(vtysh_ovsdb_config);

#define MAX_WAIT_LOOPCNT 1000
#define CONF_DEFAULT_VER  "0.0.0"
extern struct ovsdb_idl *idl;
static vtysh_contextlist * show_run_contextlist = NULL;

/*-----------------------------------------------------------------------------
| Function: vtysh_sh_run_iteratecontextlist
| Responsibility : Iterates over the show running context callback list.
| Parameters:
|     ip: File pointer to write data to.
| Return:
|     vtysh_ret_val: e_vtysh_ok if context callback invoked successfully
|                    else e_vtysh_error.
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_sh_run_iteratecontextlist(FILE *fp)
{
    vtysh_contextlist *current = show_run_contextlist;
    vtysh_contextlist *subcontext_list;
    vtysh_ovsdb_cbmsg msg;
    struct feature_sorted_list *list = NULL;
    const struct shash_node **nodes;
    int idx, count;
    const char *db_ver = NULL;

    VLOG_DBG("readconfig:before- idl 0x%p seq no %d", idl,
             ovsdb_idl_get_seqno(idl));

    msg.fp = fp;
    msg.idl = idl;

    VLOG_DBG("readconfig:after idl 0x%p seq no %d", idl,
             ovsdb_idl_get_seqno(idl));
    fprintf(fp, "!\n");
    db_ver = ovsrec_get_db_version();

    fprintf(fp, "!Version %s %s\n",
            vtysh_ovsdb_os_name_get() ,
            vtysh_ovsdb_switch_version_get() );
    fprintf(fp, "!Schema version %s\n", db_ver ? db_ver : CONF_DEFAULT_VER );

    while (current != NULL)
    {
        list = NULL;
        idx = count = 0;
        if (current->context_callback_init != NULL) {
            list = current->context_callback_init(&msg);
            nodes = list->nodes;
            count = list->count;
        }

        do {
            if (list != NULL) {
                msg.feature_row = nodes[idx]->data;
            }

            msg.disp_header_cfg = false;
            msg.skip_subcontext_list= false;
            msg.contextid = current->index; /* vtysh_contextid */

            if (current->vtysh_context_callback != NULL &&
                e_vtysh_ok != current->vtysh_context_callback(&msg)) {
                VLOG_ERR("Error in callback function with context id: %d\n",
                         current->index);
                return e_vtysh_ok;
            }

            /* Skip iteration over sub-context list. */
            if (msg.skip_subcontext_list) {
                msg.feature_row = NULL;
                if (list != NULL)
                    idx++;
                continue;
            }

            /* Iterate over sub-context list. */
            subcontext_list = current->subcontext_list;
            while (subcontext_list != NULL)
            {
                /* vtysh_*_context_clientid */
                msg.clientid = subcontext_list->index;

                if (subcontext_list->vtysh_context_callback != NULL &&
                    e_vtysh_ok != subcontext_list->vtysh_context_callback(&msg))
                {
                    VLOG_ERR("Error in subcontext callback function with"
                             "subcontext id: %d\n", subcontext_list->index);
                    return e_vtysh_ok;
                }
                subcontext_list = subcontext_list->next;
            }

            msg.feature_row = NULL;
            if (list != NULL) {
                idx++;
            }

        } while (idx < count);

        if (current->context_callback_exit != NULL) {
            current->context_callback_exit(list);
        }

        current = current->next;
    }
    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_cli_print
| Responsibility : prints the command in given format
| Parameters:
|           p_msg - pointer to object type vtysh_ovsdb_cbmsg
|           fmt - print cli format
|           elipses - vraiable args
| Return: returns e_vtysh_ok if it prints cli else e_vtysh_error.
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_ovsdb_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...)
{
  va_list args;

  if ((NULL == p_msg) || (NULL == p_msg))
  {
    return e_vtysh_error;
  }

  va_start(args, fmt);

  vfprintf(p_msg->fp, fmt, args);
  fprintf(p_msg->fp, "\n");
  fflush(p_msg->fp);

  va_end(args);
  return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_config_logmsg
| Responsibility : logs info/dbg/err/warn level message
| Parameters: loglevel - logging level INFO/DBG/ERR/WARN
|             fmt - log message foramt
|             elipses - variable args
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_config_logmsg(int loglevel, char *fmt, ...)
{

  va_list args;
  va_start(args, fmt);

  switch (loglevel) {

    case VTYSH_OVSDB_CONFIG_ERR:
          VLOG_ERR(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_WARN:
          VLOG_WARN(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_INFO:
          VLOG_INFO(fmt, args);
          break;
    case VTYSH_OVSDB_CONFIG_DBG:
          VLOG_DBG(fmt, args);
          break;
    default :
          break;
  }
  va_end(args);
}

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_init_clients
| Responsibility :   initializes the ovsdb config table client callbacks
| Parameters: void
| Return: void
-----------------------------------------------------------------------------*/
void
vtysh_ovsdb_init_clients(void)
{
  /* register vtysh context table client callbacks */
  vtysh_init_config_context_clients();
  vtysh_init_router_context_clients();
}


/*---------------------------------------------------------------------------
| Function: install_show_run_config_context.
| Responsibility: Registers running-config context with cli.
|                 Context is added to running-config context list.
| Parameters:
|     index: running-config context id to be registered.
|     funcptr: running-config context callback function pointer.
|     init_funcptr: To initialize anything required before
|                   calling context callback function.
|     exit_funcptr: To cleanup initialization done in init_funcptr.
| Return:
|     vtysh_ret_val: e_vtysh_ok if context callback is added to cli
|                    successfully else e_vtysh_error.
---------------------------------------------------------------------------*/
vtysh_ret_val
install_show_run_config_context(vtysh_contextid index,
                          vtysh_ret_val (*funcptr) (void* p_private),
                          struct feature_sorted_list * (*init_funcptr) (void* p_private),
                          void (*exit_funcptr) (struct feature_sorted_list * head))
{
    vtysh_contextlist *current;
    vtysh_contextlist *new_context = (vtysh_contextlist *)
                                malloc (sizeof(vtysh_contextlist));
    if (new_context == NULL) {
        VLOG_ERR("Error while allocating memory in malloc\n");
        return e_vtysh_error;
    }

    new_context->index = index;
    new_context->vtysh_context_callback = funcptr;
    new_context->context_callback_init = init_funcptr;
    new_context->context_callback_exit = exit_funcptr;
    new_context->subcontext_list = NULL;

    if (show_run_contextlist == NULL ||
        show_run_contextlist->index > new_context->index)
    {
        new_context->next = show_run_contextlist;
        show_run_contextlist = new_context;
    }
    else
    {
        current = show_run_contextlist;
        while (current->next != NULL &&
               current->next->index <= new_context->index) {
            current = current->next;
        }

        if (current->index != new_context->index)
        {
            new_context->next = current->next;
            current->next = new_context;
            VLOG_DBG("Installing context %d is successful.\n", new_context->index);
        }
        else
        {
            VLOG_DBG("Context %d already exists.\n", index);
            free(new_context);
        }
    }
    return e_vtysh_ok;
}


/*---------------------------------------------------------------------------
| Function: install_show_run_config_subcontext.
| Responsibility: Registers sub-context with running-config context.
|                 Sub-context is added to sub-context list.
| Parameters:
|     index: running-config context id to be registered.
|     subcontext_index: subcontext id to be registered.
|     funcptr: running-config context callback function pointer
|     init_funcptr: To initialize anything required before
|                   calling context callback function.
|     exit_funcptr: To cleanup initialization done in init_funcptr.
| Return:
|     vtysh_ret_val: e_vtysh_ok if sub-context callback is added to cli
|                    successfully else e_vtysh_error.
---------------------------------------------------------------------------*/
vtysh_ret_val
install_show_run_config_subcontext(vtysh_contextid index,
                          vtysh_contextid subcontext_index,
                          vtysh_ret_val (*funcptr) (void* p_private),
                          struct feature_sorted_list * (*init_funcptr) (void* p_private),
                          void (*exit_funcptr) (struct feature_sorted_list * head))
{
    vtysh_contextlist *current, *new_subcontext, *temp;

    if (show_run_contextlist == NULL)
    {
        VLOG_ERR("No parent context %d to add sub-context %d.\n",
                 index, subcontext_index);
        return e_vtysh_error;
    }
    else
    {
        current = show_run_contextlist;
        while (current->next != NULL &&
               current->index != index) {
            current = current->next;
        }

        if (current->next == NULL && current->index != index)
        {
            VLOG_ERR("No parent context %d to add sub-context %d.\n",
                     index, subcontext_index);
            return e_vtysh_error;
        }

        new_subcontext =
                      (vtysh_contextlist *) malloc (sizeof(vtysh_contextlist));
        if (new_subcontext == NULL) {
            VLOG_ERR("Error while allocating memory in malloc\n");
            return e_vtysh_error;
        }

        new_subcontext->index = subcontext_index;
        new_subcontext->vtysh_context_callback = funcptr;
        new_subcontext->context_callback_init = init_funcptr;
        new_subcontext->context_callback_exit = exit_funcptr;
        new_subcontext->subcontext_list = NULL;

        if (current->subcontext_list == NULL ||
            current->subcontext_list->index > new_subcontext->index)
        {
            new_subcontext->next = current->subcontext_list;
            current->subcontext_list = new_subcontext;
        }
        else
        {
            temp = current->subcontext_list;
            while (temp->next != NULL &&
                   temp->next->index <= new_subcontext->index) {
                temp = temp->next;
            }

            if (temp->index != new_subcontext->index)
            {
                new_subcontext->next = temp->next;
                temp->next = new_subcontext;
                VLOG_DBG("Installing sub-context %d for context %d is"
                         " successful.\n",
                         index, new_subcontext->index);
            }
            else
            {
                VLOG_DBG("Sub-context %d for context %d already exists.\n",
                         index, new_subcontext->index);
                free(new_subcontext);
            }
        }
    }
    return e_vtysh_ok;
}
