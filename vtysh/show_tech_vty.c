/* System SHOW_TECH CLI commands
 *
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
 *
 * File: show_tech_vty.c
 *
 * Purpose: To Run Show Tech Commands from CLI
 */

#include "command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/show_tech_vty.h"
#include "smap.h"
#include "memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"
#include "showtech.h"

VLOG_DEFINE_THIS_MODULE (vtysh_show_tech_cli);


/* Function       : cli_show_tech
 * Resposibility  : Display Show Tech Information
 * Return         : 0 on success 1 otherwise
 */

 int
 cli_show_tech(const char* feature,const char* sub_feature)
 {
   struct feature*      iter;
   struct sub_feature*  iter_sub = NULL;
   struct clicmds*      iter_cli = NULL;
   int                  valid_cmd = 0;

   /* Retrive the Show Tech Configuration Header */
   iter = get_showtech_config(NULL);
   if(iter == NULL)
   {
      VLOG_ERR("Failed to obtain Show Tech Configuration");
      vty_out(vty, "Failed to obtain Show Tech Configuration%s",VTY_NEWLINE);
      return CMD_SUCCESS;
   }


      /* Show Tech All */
   if(feature == NULL)
   {
      /* Show Tech (all) is a valid command, hence update the flag */
      valid_cmd = 1;
      while (iter != NULL)
      {
         vty_out(vty,"====================================================%s"
         ,VTY_NEWLINE);
         vty_out(vty,"Feature %s begins %s", iter->name, VTY_NEWLINE);
         vty_out(vty,"====================================================%s%s"
         ,VTY_NEWLINE,VTY_NEWLINE);

        iter_sub = iter->p_subfeature;

        while (iter_sub)
        {
           /* CLI Commands are grouped under SubFeature, In order to support
            * configurations without any subfeature, we internally add a
            * dummy subfeature and set the flag is_dummy to true.
            * Checking the Dummy Flag before printing Sub Feature Information
            */
            if(!iter_sub->is_dummy)
            {
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
               ,VTY_NEWLINE);
               vty_out(vty,"Sub Feature %s begins %s", iter_sub->name,VTY_NEWLINE);
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
               ,VTY_NEWLINE,VTY_NEWLINE);
            }
            iter_cli = iter_sub->p_clicmds;
            while (iter_cli)
            {
               vty_out(vty,"---------------------------------%s"
               ,VTY_NEWLINE);
               vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
               vty_out(vty,"---------------------------------%s"
               ,VTY_NEWLINE);
               vtysh_execute(iter_cli->command);
              iter_cli = iter_cli->next;
            }
            iter_sub = iter_sub->next;
         }
         iter = iter->next;
      }

   }
   else
   {
      /* Show Tech Feature */
      if(sub_feature == NULL)
      {
         while (iter != NULL)
         {
            if(strcmp(iter->name,feature))
            {
               /* Run till the matched feature */
               iter = iter->next;
               continue;
            }
            valid_cmd = 1;
            vty_out(vty,"====================================================%s"
            ,VTY_NEWLINE);
            vty_out(vty,"Feature %s begins %s", iter->name, VTY_NEWLINE);
            vty_out(vty,"====================================================%s%s"
            ,VTY_NEWLINE,VTY_NEWLINE);

           iter_sub = iter->p_subfeature;

           while (iter_sub)
           {
            /* CLI Commands are grouped under SubFeature, In order to support
            * configurations without any subfeature, we internally add a
            * dummy subfeature and set the flag is_dummy to true.
            * Checking the Dummy Flag before printing Sub Feature Information
            */
              if(!iter_sub->is_dummy)
               {
                  vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                  ,VTY_NEWLINE);
                  vty_out(vty,"Sub Feature %s begins %s", iter_sub->name,VTY_NEWLINE);
                  vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
               ,VTY_NEWLINE,VTY_NEWLINE);
               }
               iter_cli = iter_sub->p_clicmds;
               while (iter_cli)
               {
                  vty_out(vty,"---------------------------------%s"
                  ,VTY_NEWLINE);
                  vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
                  vty_out(vty,"---------------------------------%s"
               ,VTY_NEWLINE);
                  vtysh_execute(iter_cli->command);
                 iter_cli = iter_cli->next;
               }
               iter_sub = iter_sub->next;
            }
            iter = iter->next;
         }


      }
            /* Show Tech Sub Feature */
      else
      {
         while (iter != NULL)
         {
            if(strcmp(iter->name,feature))
            {
               /* Run till the matched feature */
               iter = iter->next;
               continue;
            }
           iter_sub = iter->p_subfeature;

           while (iter_sub)
           {
             /* CLI Commands are grouped under SubFeature, In order to support
            * configurations without any subfeature, we internally add a
            * dummy subfeature and set the flag is_dummy to true.
            * Checking the Dummy Flag before printing Sub Feature Information
            */
             if(iter_sub->is_dummy || strcmp(iter_sub->name,sub_feature))
               {
                  /* Run till the matched sub feature */
                  iter_sub = iter_sub->next;
                  continue;
               }
               if(valid_cmd == 0)
               {
                  vty_out(vty,"====================================================%s"
                  ,VTY_NEWLINE);
                  vty_out(vty,"Feature %s begins %s", iter->name, VTY_NEWLINE);
                  vty_out(vty,"====================================================%s%s"
                  ,VTY_NEWLINE,VTY_NEWLINE);
                  valid_cmd = 1;
               }

               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
               ,VTY_NEWLINE);
               vty_out(vty,"Sub Feature %s begins %s", iter_sub->name,VTY_NEWLINE);
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
               ,VTY_NEWLINE,VTY_NEWLINE);
               iter_cli = iter_sub->p_clicmds;
               while (iter_cli)
               {
                  vty_out(vty,"---------------------------------%s"
                  ,VTY_NEWLINE);
                  vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
                  vty_out(vty,"---------------------------------%s"
               ,VTY_NEWLINE);
                  vtysh_execute(iter_cli->command);
                 iter_cli = iter_cli->next;
               }
               iter_sub = iter_sub->next;
            }
            iter = iter->next;
         }
      }

      /* The Feature or Sub Feature Specified is not valid/found */
      if(valid_cmd == 0)
      {
         if(sub_feature == NULL)
         {
            /* Feature Specific Command */
            vty_out(vty,"Feature %s is not supported%s",feature,VTY_NEWLINE);
         }
         else
         {
            /* Sub Feature Specific Command */
            vty_out(vty,"Sub Feature %s is not supported%s",sub_feature,VTY_NEWLINE);
         }
      }
   }


   return CMD_SUCCESS;
}


/* Function       : cli_show_tech_list
 * Resposibility  : Display Supported Show Tech Features
 * Return         : 0 on success 1 otherwise
 */

 int
 cli_show_tech_list(void)
 {
   struct feature* iter;
   struct sub_feature* iter_sub = NULL;

   iter = get_showtech_config(NULL);
   if(iter == NULL)
   {
      VLOG_ERR("Failed to obtain Show Tech Configuration");
      vty_out(vty, "Failed to obtain Show Tech Configuration%s",VTY_NEWLINE);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Show Tech Supported Features List %s%s",VTY_NEWLINE,VTY_NEWLINE);
   vty_out(vty,"------------------------------------------------------------%s"
      ,VTY_NEWLINE);
   vty_out(vty,"Feature  SubFeature        Desc%s",VTY_NEWLINE);
   vty_out(vty,"------------------------------------------------------------%s"
      ,VTY_NEWLINE);
   while (iter != NULL)
   {

     vty_out(vty,"%-27.26s%s%s\n", iter->name, iter->desc,VTY_NEWLINE);
     iter_sub = iter->p_subfeature;

     while (iter_sub)
     {
        if(!iter_sub->is_dummy)
        {
           vty_out(vty,"         %-18.17s%s%s\n", iter_sub->name, iter_sub->desc
           ,VTY_NEWLINE);
        }
           iter_sub = iter_sub->next;
     }
     iter = iter->next;
   }


   return CMD_SUCCESS;
}


/*
 * Action routines for Show Tech CLIs
 */
DEFUN_NOLOCK (cli_platform_show_tech,
        cli_platform_show_tech_cmd,
        "show tech",
        SHOW_STR
        SHOW_TECH_STR)
{
    return cli_show_tech(NULL,NULL);
}


/*
 * Action routines for Show Tech List
 */
DEFUN_NOLOCK (cli_platform_show_tech_list,
        cli_platform_show_tech_list_cmd,
        "show tech list",
        SHOW_STR
        SHOW_TECH_STR
        SHOW_TECH_LIST_STR)
{
    return cli_show_tech_list();
}


/*
 * Action routines for Show Tech List
 */
DEFUN_NOLOCK (cli_platform_show_tech_feature,
        cli_platform_show_tech_feature_cmd,
        "show tech FEATURE [SUBFEATURE]",
        SHOW_STR
        SHOW_TECH_STR
        SHOW_TECH_FEATURE_STR
        SHOW_TECH_SUB_FEATURE_STR)
{
   if(argc > 1)
   {
      /* Optional Sub Feature Provided */
    return cli_show_tech(argv[0],argv[1]);
   }
   else
   {
    return cli_show_tech(argv[0],NULL);
   }
}


/*
 * Function        : show_tech_vty_init
 * Resposibility     : Install the cli action routines
 */
void
show_tech_vty_init()
{
    install_element (ENABLE_NODE, &cli_platform_show_tech_cmd);
    install_element (ENABLE_NODE, &cli_platform_show_tech_list_cmd);
    install_element (ENABLE_NODE, &cli_platform_show_tech_feature_cmd);
}
