/*
 * Copyright (C) 1997 Kunihiro Ishiguro
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
 * @ingroup cli
 *
 * @file vtysh_ovsdb_dhcp_tftp_context.c
 * Source for registering dhcp-tftp client callback with openvswitch table.
 *
 ***************************************************************************/

#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_config_context.h"
#include "vtysh_ovsdb_dhcp_tftp_context.h"
#include "openswitch-dflt.h"

char dhcpclientname[] = "vtysh_dhcp_tftp_context_dhcp_clientcallback";
char tftpclientname[] = "vtysh_dhcp_tftp_context_tftp_clientcallback";

/*-----------------------------------------------------------------------------
| Function: vtysh_ovsdb_dhcp_tftp_cli_print
| Responsibility : prints the command in given format
| Parameters:
|           p_msg - pointer to object type vtysh_ovsdb_cbmsg
|           fmt - print cli format
| Return: returns e_vtysh_ok if it prints cli else e_vtysh_error.
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_dhcp_tftp_cli_print(vtysh_ovsdb_cbmsg *p_msg, const char *fmt, ...)
{
    va_list args;

    if ((NULL == p_msg) || (NULL == p_msg)) {
        return e_vtysh_error;
    }

    va_start(args, fmt);

    vfprintf(p_msg->fp, fmt, args);
    fflush(p_msg->fp);

    va_end(args);

    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_dhcp_srv_range
| Responsibility : parse alias column in dhcp srv table
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_dhcp_srv_range(vtysh_ovsdb_cbmsg *p_msg,
                                          int *header_print)
{
    const struct ovsrec_dhcpsrv_range *row = NULL;
    row = ovsrec_dhcpsrv_range_first(p_msg->idl);
    int i = 0;

    if (!row) {
        return e_vtysh_ok;
    }

    OVSREC_DHCPSRV_RANGE_FOR_EACH (row, p_msg->idl) {
        if (*header_print == false) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "dhcp-server");
           *header_print = true;
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg,
                                        "%4srange %s start-ip-address %s ", "",
                                        row->name, row->start_ip_address);
        if (row->end_ip_address) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "end-ip-address %s ",
                                            row->end_ip_address);
        }

        if (row->set_tag) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "set tag %s ", row->set_tag);
        }

        if (row->n_match_tags) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match tags ");
        }
        for (i = 0; i < row->n_match_tags; i++) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%s",
                                            row->match_tags[i]);
            if (i != (row->n_match_tags-1)) {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, ",");
            } else {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, " ");
            }
        }

        if (row->netmask) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "netmask %s ", row->netmask);
        }

        if (row->broadcast) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "broadcast %s ",
                                            row->broadcast);
        }

        if (row->prefix_len) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "prefix-len %ld ",
                                            row->prefix_len[0]);
        }

        if (row->lease_duration && row->lease_duration[0] != 60) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "lease-duration %ld ",
                                            row->lease_duration[0]);
        }

        if (row->is_static && *row->is_static == 1) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "static");
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_static_host
| Responsibility : parse alias column in dhcp srv table
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_static_host(vtysh_ovsdb_cbmsg *p_msg,
                                       int *header_print)
{
    const struct ovsrec_dhcpsrv_static_host *row = NULL;
    int i = 0;

    row = ovsrec_dhcpsrv_static_host_first(p_msg->idl);
    if (!row) {
        return e_vtysh_ok;
    }

    OVSREC_DHCPSRV_STATIC_HOST_FOR_EACH (row, p_msg->idl) {
        if (*header_print == false) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "dhcp-server");
            *header_print = true;
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%4sstatic %s ",
                                        "", row->ip_address);

        if (row->n_mac_addresses) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-mac-addresses ");
        }

        for (i = 0; i < row->n_mac_addresses; i++) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg,"%s",row->mac_addresses[i]);
            if (i != (row->n_mac_addresses - 1)) {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, ",");
            } else {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, " ");
            }
        }

        if (row->n_set_tags) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "set tags ");
        }

        for (i = 0; i < row->n_set_tags; i++) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg,"%s",row->set_tags[i]);
            if (i != (row->n_set_tags - 1)) {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, ",");
            } else {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, " ");
            }
        }

        if (row->client_hostname) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-client-hostname %s ",
                                            row->client_hostname);
        }

        if (row->client_id) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-client-id %s ",
                                            row->client_id);
        }

        if (row->lease_duration && row->lease_duration[0] != 60) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "lease-duration %ld ",
                                           row->lease_duration[0]);
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_dhcp_option
| Responsibility : parse alias column in dhcp srv table
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_dhcp_option(vtysh_ovsdb_cbmsg *p_msg,
                                       int* header_print)
{
    const struct ovsrec_dhcpsrv_option *row = NULL;
    int i = 0;

    row = ovsrec_dhcpsrv_option_first (p_msg->idl);
    if (!row) {
        return e_vtysh_ok;
    }

    OVSREC_DHCPSRV_OPTION_FOR_EACH (row, p_msg->idl) {
        if (*header_print == false) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "dhcp-server");
            *header_print = true;
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%4soption set ", "");

        if (row->option_name) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "option-name %s ",
                                            row->option_name);
        }

        if (row->option_number) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "option-number %ld ",
                                            row->option_number[0]);
        }

        if (row->option_value) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "option-value %s ",
                                            row->option_value);
        }

        if (row->n_match_tags > 0) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match tags ");
        }
        for (i = 0; i < row->n_match_tags; i++) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg,"%s",row->match_tags[i]);
            if (i != (row->n_match_tags - 1)) {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, ",");
            } else {
                vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, " ");
            }
        }

        if (row->ipv6 && *row->ipv6 == 1) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "ipv6 ");
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : ettysh_ovsdb_ovstable_parse_dhcp_match
| Responsibility : parse alias column in dhcp srv table
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_dhcp_match(vtysh_ovsdb_cbmsg *p_msg,
                                      int *header_print)
{
    const struct ovsrec_dhcpsrv_match *row = NULL;

    row = ovsrec_dhcpsrv_match_first(p_msg->idl);
    if (!row) {
        return e_vtysh_ok;
    }

    OVSREC_DHCPSRV_MATCH_FOR_EACH (row, p_msg->idl) {
        if (*header_print == false) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "dhcp-server");
            *header_print = true;
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%4smatch set tag ", "");

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%s ", row->set_tag);

        if (row->option_name) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-option-name %s ",
                                            row->option_name);
        }

        if (row->option_number) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-option-number %ld ",
                                            row->option_number[0]);
        }

        if (row->option_value) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "match-option-value %s ",
                                            row->option_value);
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_ovsdb_ovstable_parse_static_host
| Responsibility : parse alias column in dhcp srv table
| Return : vtysh_ret_val, e_vtysh_ok
-----------------------------------------------------------------------------*/
static vtysh_ret_val
vtysh_ovsdb_ovstable_parse_bootp(vtysh_ovsdb_cbmsg *p_msg,
                                 int *header_print)
{
    const struct ovsrec_dhcp_server *row = NULL;
    const struct smap_node *node;

    row = ovsrec_dhcp_server_first(p_msg->idl);

    if (!row) {
        return e_vtysh_ok;
    }

    SMAP_FOR_EACH(node,&row->bootp) {
        if (*header_print == false) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "dhcp-server");
            *header_print = true;
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "%4s", "");

        if (strcmp(node->key,"no_matching_tag") != 0) {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg,
                                            "boot set file %s match tag %s ",
                                             node->value, node->key);
        } else {
            vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "boot set file %s ",
                                            node->value);
        }

        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}

/*-----------------------------------------------------------------------------
| Function : vtysh_dhcp_tftp_context_dhcp_clientcallback
| Responsibility : dhcp client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_dhcp_tftp_context_dhcp_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    int header_print = false;

    vtysh_ovsdb_ovstable_parse_dhcp_srv_range(p_msg, &header_print);
    vtysh_ovsdb_ovstable_parse_static_host(p_msg, &header_print);
    vtysh_ovsdb_ovstable_parse_dhcp_option(p_msg, &header_print);
    vtysh_ovsdb_ovstable_parse_dhcp_match(p_msg, &header_print);
    vtysh_ovsdb_ovstable_parse_bootp(p_msg, &header_print);

    if (header_print == true) {
        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}


/*-----------------------------------------------------------------------------
| Function : vtysh_dhcp_tftp_context_tftp_clientcallback
| Responsibility : client callback routine
| Parameters :
|     void *p_private: void type object typecast to required
| Return : void
-----------------------------------------------------------------------------*/
vtysh_ret_val
vtysh_dhcp_tftp_context_tftp_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_system *row = NULL;
    char *tftp_buff = NULL;
    uint8_t tftp_flag = false;

    row = ovsrec_system_first(p_msg->idl);
    if (!row) {
        return e_vtysh_ok;
    }

    tftp_buff = (char *)smap_get(&row->other_config, "tftp_server_enable");
    if (tftp_buff && !strcmp(tftp_buff, "true")) {
        if (!tftp_flag) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "tftp-server");
            tftp_flag = true;
        }
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "enable");
    }

    tftp_buff = (char *)smap_get(&row->other_config, "tftp_server_secure");
    if (tftp_buff && !strcmp(tftp_buff, "true")) {
        if (!tftp_flag) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "tftp-server");
            tftp_flag = true;
        }
        vtysh_ovsdb_cli_print(p_msg, "%4s%s", "", "secure-mode");
    }

    tftp_buff = (char *)smap_get(&row->other_config, "tftp_server_path");
    if (tftp_buff) {
        if (!tftp_flag) {
            vtysh_ovsdb_cli_print(p_msg, "%s", "tftp-server");
            tftp_flag = true;
        }
        vtysh_ovsdb_cli_print(p_msg, "%4s%s %s", "", "path", tftp_buff);
    }

    if (tftp_flag) {
        vtysh_ovsdb_dhcp_tftp_cli_print(p_msg, "\n");
    }

    return e_vtysh_ok;
}



/*-----------------------------------------------------------------------------
| Function : vtysh_init_dhcp_tftp_context_clients
| Responsibility : registers the client callbacks for dhcp-tftp context
| Parameters :
| Return :
-----------------------------------------------------------------------------*/
int
vtysh_init_dhcp_tftp_context_clients(void)
{
    vtysh_context_client client;
    vtysh_ret_val retval = e_vtysh_error;

    retval = install_show_run_config_context(e_vtysh_dhcp_tftp_context,
                                  NULL, NULL, NULL);
    if(e_vtysh_ok != retval)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                            "config context unable to add dhcp_tftp callback");
        assert(0);
        return retval;
    }

    retval = install_show_run_config_subcontext(e_vtysh_dhcp_tftp_context,
                                  e_vtysh_dhcp_tftp_context_dhcp,
                                  &vtysh_dhcp_tftp_context_dhcp_clientcallback,
                                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                    "dhcp-tftpcontext unable to add dhcp client callback");
        assert(0);
        return retval;
    }

    retval = install_show_run_config_subcontext(e_vtysh_dhcp_tftp_context,
                                  e_vtysh_dhcp_tftp_context_tftp,
                                  &vtysh_dhcp_tftp_context_tftp_clientcallback,
                                  NULL, NULL);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                        "dhcp-tftp context unable to tftp client callback");
        assert(0);
        return retval;
    }

    client.p_client_name = dhcpclientname;
    client.client_id = e_vtysh_dhcp_tftp_context_dhcp;
    client.p_callback = &vtysh_dhcp_tftp_context_dhcp_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_dhcp_tftp_context,
                                     e_vtysh_dhcp_tftp_context_dhcp,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                    "dhcp-tftpcontext unable to add dhcp client callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    memset(&client, 0, sizeof(vtysh_context_client));
    client.p_client_name = tftpclientname;
    client.client_id = e_vtysh_dhcp_tftp_context_tftp;
    client.p_callback = &vtysh_dhcp_tftp_context_tftp_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_dhcp_tftp_context,
                                     e_vtysh_dhcp_tftp_context_tftp,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                        "dhcp-tftp context unable to tftp client callback");
        assert(0);
        return retval;
    }

    return e_vtysh_ok;
}
