/*
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
 *
 * @file audit_log_utils.c
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libaudit.h>
#include "audit_log_utils.h"

VLOG_DEFINE_THIS_MODULE(audit_log_utils);

/*-----------------------------------------------------------------------------
  | Function : audit_log_user_msg
  | Responsibility : Used to concatenate the "op" and "cfgdata" value.
  |                  And call audit log events.
  | Parameters :
  |     char *op                      : String representing the operation being performed
  |     const char *cfgdata           : Used to store the user executed command
  |     char *hostname                : Used to store local hostname
  |     int result                    : Result of the configuaration operation
  | Return : void
  -----------------------------------------------------------------------------*/
void audit_log_user_msg(char *op, const char *cfgdata, char *hostname, int result)
{
    char aubuf[MAX_CFGDATA_LEN];
    char *cfg;

    strcat(strcpy(aubuf, op)," ");

    /* Encode the configuration data associated with the operation*/
    if (cfgdata != NULL){
       cfg = audit_encode_nv_string("data", cfgdata,0);
       if (cfg != NULL){
           strcat(strncat(aubuf, cfg, (MAX_CFGDATA_LEN-(MAX_OP_DESC_LEN+1)))," ");
           free(cfg);
       }
    }
    /* Call audit log event*/
    audit_log_user_message(audit_fd, AUDIT_USYS_CONFIG, aubuf,hostname, NULL, NULL, !(result));
}
