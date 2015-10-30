/*
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015, 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/************************************************************************//**
 * @ingroup ops-access-list
 *
 * @file
 * Implementation of Access Control List (ACL) CLI functions
 ***************************************************************************/

#include "command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "json.h"
#include "memory.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/access_list_vty.h"

/** Create logging module */
VLOG_DEFINE_THIS_MODULE(vtysh_access_list_cli);

/** Utilize OVSDB interface code generated from schema */
extern struct ovsdb_idl *idl;

/* = Private Constants = */

/* Misc constants */
#define MAX_ACL_NAME_LENGTH 65 /**< 64 character name + NULL-terminator */
#define IP_VER_STR_LEN 5       /**< "ipv{4|6}" + NULL-terminator */
#define ACL_TRUE_STR "true"

/* Log timer constants */
#define ACL_LOG_TIMER_STR "acl_log_timer"
#define ACL_LOG_TIMER_MIN "30"
#define ACL_LOG_TIMER_MAX "300"
#define ACL_LOG_TIMER_DEFAULT ACL_LOG_TIMER_MAX
#define ACL_LOG_TIMER_DEFAULT_STR "default"

/* Constants related to ACE sequence numbers */
#define ACE_SEQ_MAX 4294967295 /**< Maximum sequence number allowed for an ACE */
#define ACE_SEQ_MAX_STR_LEN 11 /**< ACE_SEQ_MAX in a string + NULL-terminator */
#define ACE_SEQ_AUTO_INCR   10 /**< Amount to increment new ACEs automatically by */

/* https://gcc.gnu.org/onlinedocs/cpp/Stringification.html#Stringification */
#define ACL_NUM_TO_STR_HELPER(x) #x                /**< Preprocessor helper macro */
#define ACL_NUM_TO_STR(x) ACL_NUM_TO_STR_HELPER(x) /**< Preprocessor stringify macro */

/* Common help strings */
#define ACL_STR "Access control list (ACL)\n"
#define ACL_NAME_STR "ACL name\n"
#define ACL_CFG_STR "Display ACL configuration as CLI commands\n"
#define ACL_HITCOUNTS_STR "Hit counts (statistics)\n"
#define ACL_IN_STR "Inbound (ingress) traffic\n"
#define ACL_APPLY_STR "Apply a configuration record\n"
#define ACL_IP_STR "Internet Protocol v4 (IPv4)\n"
#define ACL_INTERFACE_STR "Specify interface\n"
#define ACL_INTERFACE_NAME_STR "Interface Name\n"
#define ACL_INTERFACE_ID_STR "Identifier (Interface Name or VLAN ID)\n"
#define ACL_VLAN_STR "Specify VLAN\n"
#define ACL_VLAN_ID_STR "VLAN ID\n"
#define ACL_ALL_STR "All access-lists\n"

/* Command strings (cmdstr) and Help strings (helpstr) used in vtysh DEFUNs */
#define ACE_SEQ_CMDSTR "<1-" ACL_NUM_TO_STR(ACE_SEQ_MAX) ">"
#define ACE_SEQ_HELPSTR "Access control entry (ACE) sequence number\n"
#define ACE_ACTION_CMDSTR "(deny | permit) "
#define ACE_ACTION_HELPSTR "Deny packets matching this ACE\n" \
                           "Permit packets matching this ACE\n"
#define ACE_ALL_PROTOCOLS_CMDSTR "(any | ah | gre | esp | icmp | igmp |  pim | sctp | tcp | udp | <0-255>) "
#define ACE_ALL_PROTOCOLS_HELPSTR "Any internet protocol number\n" \
                                  "Authenticated header\n" \
                                  "Generic routing encapsulation\n" \
                                  "Encapsulation security payload\n" \
                                  "Internet control message protocol\n" \
                                  "Internet group management protocol\n" \
                                  "Protocol independent multicast\n" \
                                  "Stream control transport protocol\n" \
                                  "Transport control protocol\n" \
                                  "User datagram protocol\n" \
                                  "Specify numeric protocol value\n"
#define ACE_PORT_PROTOCOLS_CMDSTR  "(sctp | tcp | udp) "
#define ACE_PORT_PROTOCOLS_HELPSTR "Stream control transport protocol\n" \
                                   "Transport control protocol\n" \
                                   "User datagram protocol\n"
#define ACE_IP_ADDRESS_CMDSTR "(any | A.B.C.D | A.B.C.D/M | A.B.C.D/W.X.Y.Z) "
#define ACE_SRC_IP_ADDRESS_HELPSTR "Any source IP address\n" \
                                   "Specify source IP host address\n" \
                                   "Specify source IP network address with prefix length\n" \
                                   "Specify source IP network address with network mask\n"
#define ACE_DST_IP_ADDRESS_HELPSTR "Any destination IP address\n" \
                                   "Specify destination IP host address\n" \
                                   "Specify destination IP network address with prefix length\n" \
                                   "Specify destination IP network address with network mask\n"
#define ACE_PORT_OPER_CMDSTR "(eq | gt | lt | neq) <0-65535> "
#define ACE_SRC_PORT_OPER_HELPSTR "Layer 4 source port equal to\n" \
                                  "Layer 4 source port greater than\n" \
                                  "Layer 4 source port less than\n" \
                                  "Layer 4 source port not equal to\n" \
                                  "Layer 4 source port\n"
#define ACE_DST_PORT_OPER_HELPSTR "Layer 4 destination port equal to\n" \
                                  "Layer 4 destination port greater than\n" \
                                  "Layer 4 destination port less than\n" \
                                  "Layer 4 destination port not equal to\n" \
                                  "Layer 4 destination port\n"
#define ACE_PORT_RANGE_CMDSTR "(range) <0-65535> <0-65535> "
#define ACE_SRC_PORT_RANGE_HELPSTR "Layer 4 source port range\n" \
                                   "Layer 4 source minimum port\n" \
                                   "Layer 4 source maximum port\n"
#define ACE_DST_PORT_RANGE_HELPSTR "Layer 4 destination port range\n" \
                                   "Layer 4 destination minimum port\n" \
                                   "Layer 4 destination maximum port\n"
#define ACE_ADDITIONAL_OPTIONS_CMDSTR "{ log | count }"
#define ACE_ADDITIONAL_OPTIONS_HELPSTR "Log packets matching this entry\n" \
                                       "Count packets matching this entry\n"
#define ACE_COMMENT_CMDSTR "(comment) .TEXT"
#define ACE_COMMENT_HELPSTR "Text comment entry\n" \
                            "Comment text\n"

/* = Static/Helper functions = */

/**
 * Look up an ACL by type + name
 *
 * @param  acl_type ACL type string
 * @param  acl_name ACL name string
 *
 * @return          Pointer to ovsrec_acl structure object
 */
static inline const struct ovsrec_acl *
get_acl_by_type_name(const char *acl_type, const char *acl_name)
{
    const static struct ovsrec_acl *acl;

    OVSREC_ACL_FOR_EACH(acl, idl) {
        if ((!strcmp(acl->list_type, acl_type)) &&
            (!strcmp(acl->list_name, acl_name))) {
            return (struct ovsrec_acl *) acl;
        }
    }

    return NULL;
}

/**
 * Returns the current highest sequence number in an ACL
 *
 * @param  acl_row  ACL OVSDB row pointer
 *
 * @return          Pointer to string of highest sequence number ACE
 */
static inline const char *
acl_get_highest_seq(const struct ovsrec_acl *acl_row)
{
    const struct smap_node **aces_sorted;
    const char *seq_str = NULL;

    if ((aces_sorted = smap_sort_numeric(&acl_row->want)) != NULL) {
        seq_str = aces_sorted[smap_count(&acl_row->want) - 1]->key;
        free(aces_sorted);
    }

    return seq_str;
}

/**
 * Look up a Port by name
 *
 * @param  name     Port name string
 *
 * @return          Pointer to ovsrec_port structure object
 */
static inline const struct ovsrec_port *
get_port_by_name(const char *name)
{
    const static struct ovsrec_port *port;

    OVSREC_PORT_FOR_EACH(port, idl) {
        if (!strcmp(port->name, name)) {
            return (struct ovsrec_port *) port;
        }
    }

    return NULL;
}

/**
 * Look up a VLAN by ID (in string form)
 *
 * @param  id_str   VLAN ID string
 *
 * @return          Pointer to ovsrec_vlan structure object
 */
static inline const struct ovsrec_vlan *
get_vlan_by_id_str(const char *id_str)
{
    const static struct ovsrec_vlan *vlan;

    OVSREC_VLAN_FOR_EACH(vlan, idl) {
        if (vlan->id == strtoul(id_str, NULL, 0)) {
            return (struct ovsrec_vlan *) vlan;
        }
    }

    return NULL;
}

/**
 * Helper function to create a json structure whose members are all printable.
 * Present items have a trailing space, missing items are empty strings ("")
 *
 * Caller must call json_destroy() when finished with returned structure
 *
 * @param  ace        json structure
 *
 * @return            pointer to json structure with "printable" members
 */
static struct json *
ace_clone_printable(const struct json *ace)
{
    const struct shash *object = json_object(ace);
    struct json *json;
    void *data;
    int key_idx;

    json = json_object_create();
    for (key_idx = ACE_KEY_MIN_; key_idx < ACE_KEY_N_; key_idx++) {
        data = shash_find_data(object, ace_key_names[key_idx]);
        if (data) {
            const char *data_str;
            /* For booleans, use the key name to indicate "true" */
            if (!strcmp(json_string(data), ACL_TRUE_STR)) {
                data_str = ace_key_names[key_idx];
            /* Otherwise use the json member value */
            } else {
                data_str = json_string(data);
            }
            /* Allocate buffer for data, trailing space, and NULL, thus + 2 */
            char *value = xzalloc(strlen(data_str) + 2);
            strcpy(value, data_str);          /* Copy original string  */
            value[strlen(value)] = ' ';       /* Append trailing space */
            json_object_put_string(json, ace_key_names[key_idx], value);
            free(value);
        } else {
            json_object_put_string(json, ace_key_names[key_idx], "");
        }
    }
    return json;
}

/* = OVSDB Manipulation Functions = */

/**
 * Print an ACL's configuration as if it were entered into the CLI
 *
 * @param acl_row Pointer to ACL row
 */
static void
print_acl_config(const struct ovsrec_acl *acl_row)
{
    const struct smap_node **aces_sorted;
    struct json *ace, *ace_printable;
    int ace_idx, key_idx;

    /* Print ACL command, type, name */
    vty_out(vty,
            "%s %s %s%s",
            "access-list",
            "ip",
            acl_row->list_name,
            VTY_NEWLINE);

    aces_sorted = smap_sort_numeric(&acl_row->want);
    for (ace_idx = 0; ace_idx < smap_count(&acl_row->want); ace_idx++) {

        ace = json_from_string(aces_sorted[ace_idx]->value);
        ace_printable = ace_clone_printable(ace);

        /* Print each ACL entry as a single line (ala CLI input) */
        vty_out(vty, "  %s ", aces_sorted[ace_idx]->key); /* key = sequence number */
        for (key_idx = ACE_KEY_MIN_; key_idx < ACE_KEY_N_; key_idx++) {
            vty_out(vty, "%s", json_object_get_string(ace_printable, ace_key_names[key_idx]));
        }
        vty_out(vty, "%s", VTY_NEWLINE);

        json_destroy(ace);
        json_destroy(ace_printable);
    }
    free(aces_sorted);
}

/**
 * Print header for ACL(s) to be printed in a tabular format
 */
static void
print_acl_tabular_header(void)
{
    vty_out(vty,
            "%-4s %-10s\n"
            "   %10s %-7s %-5s\n"
            "              %-18s %-17s\n"
            "              %-18s %-17s\n"
            "              %s%s",
            "Type", "Name",
              "Seq", "Action", "Proto",
                "Source IP", "Port(s)",
                "Destination IP", "Port(s)",
                "Additional Parameters",
            VTY_NEWLINE);
    vty_out(vty, "%s%s",
            "-------------------------------------------------------------------------------",
            VTY_NEWLINE);
}

/**
 * Print an ACL's configuration in a tabular format
 *
 * @param acl_row Pointer to ACL to print
 */
static void
print_acl_tabular(const struct ovsrec_acl *acl_row)
{
    const struct smap_node **aces_sorted;
    struct json *ace, *ace_printable;
    int ace_idx;

    /* Print ACL type and name */
    if (!strcmp(acl_row->list_type, "ipv4")) {
        vty_out(vty, "%-4s ", "ip");
    }
    vty_out(vty, "%-64s ", acl_row->list_name);
    vty_out(vty, "%s", VTY_NEWLINE);

    aces_sorted = smap_sort_numeric(&acl_row->want);
    for (ace_idx = 0; ace_idx < smap_count(&acl_row->want); ace_idx++) {

        ace = json_from_string(aces_sorted[ace_idx]->value);
        ace_printable = ace_clone_printable(ace);

        /* Entry sequence number, action, and protocol (if any) */
        vty_out(vty,
                "   %10s %-7s%s %-5s%s",
                aces_sorted[ace_idx]->key, /* key = sequence number */
                json_object_get_string(ace_printable, ace_key_names[ACE_KEY_ACTION]),
                json_object_get_string(ace_printable, ace_key_names[ACE_KEY_COMMENT]),
                json_object_get_string(ace_printable, ace_key_names[ACE_KEY_IP_PROTOCOL]),
                VTY_NEWLINE);
        /* Source IP, port information */
        if (json_object_get_string(ace, ace_key_names[ACE_KEY_SOURCE_IP_ADDRESS])) {
            vty_out(vty,
                    "              %-18s %-5s %-5s %-5s%s",
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_IP_ADDRESS]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT_OPERATOR]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT_MAX]),
                    VTY_NEWLINE);
        }
        /* Destination IP, port information */
        if (json_object_get_string(ace, ace_key_names[ACE_KEY_DESTINATION_IP_ADDRESS])) {
            vty_out(vty,
                    "              %-18s %-5s %-5s %-5s%s",
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_IP_ADDRESS]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT_OPERATOR]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT_MAX]),
                    VTY_NEWLINE);
        }
        /* Additional parameters */
        if (json_object_get_string(ace, ace_key_names[ACE_KEY_LOG]) ||
            json_object_get_string(ace, ace_key_names[ACE_KEY_COUNT])) {
            vty_out(vty,
                "              %s%s%s",
                json_object_get_string(ace_printable, ace_key_names[ACE_KEY_LOG]),
                json_object_get_string(ace_printable, ace_key_names[ACE_KEY_COUNT]),
                VTY_NEWLINE);
        }

        json_destroy(ace);
        json_destroy(ace_printable);
    }
    free(aces_sorted);
}

/**
 * Print information about ACL(s) in specified format
 *
 * @param  acl_type  ACL type string
 * @param  acl_name  ACL name string
 * @param  config    Print as configuration input?
 *
 * @return CMD_SUCCESS
 *
 * @note vtysh_config_context_access_list_clientcallback() also prints data,
 *       but due to its plumbing, there is some duplicated display code.
 */
static int
cli_print_acls(const char *acl_type,
               const char *acl_name,
               const char *config)
{
    const struct ovsrec_acl *acl_row;

    /* ACL specified, print just one */
    if (acl_type && acl_name) {
        acl_row = get_acl_by_type_name(acl_type, acl_name);
        if (!acl_row) {
            vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }
        if (!config)
        {
            print_acl_tabular_header();
            print_acl_tabular(acl_row);
        } else {
            print_acl_config(acl_row);
        }
    /* Print all ACLs */
    } else {
        if (!config)
        {
            print_acl_tabular_header();
        }
        OVSREC_ACL_FOR_EACH(acl_row, idl) {
            if (acl_row) {
                if (!config)
                {
                    print_acl_tabular(acl_row);
                } else {
                    print_acl_config(acl_row);
                }
            }
        }
    }

    return CMD_SUCCESS;
}

/**
 * Create an ACL if it does not exist
 *
 * @param  acl_type  ACL type string
 * @param  acl_name  ACL name string
 *
 * @return           CMD_SUCCESS on success
 */
static int
cli_create_acl_if_needed(const char *acl_type, const char *acl_name)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_system *ovs;
    const struct ovsrec_acl *acl_row;
    const struct ovsrec_acl **acl_info;
    int i;

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get System table */
    ovs = ovsrec_system_first(idl);
    if (!ovs) {
        cli_do_config_abort(transaction);
        assert(0);
        return CMD_OVSDB_FAILURE;
    }

    /* Get parent ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);

    /* Create */
    if (!acl_row) {
        VLOG_DBG("Creating ACL type=%s name=%s", acl_type, acl_name);

        /* Create, populate new ACL table row */
        acl_row = ovsrec_acl_insert(transaction);
        ovsrec_acl_set_list_type(acl_row, acl_type);
        ovsrec_acl_set_list_name(acl_row, acl_name);

        /* Update System (parent) table */
        acl_info = xmalloc(sizeof *ovs->acls * (ovs->n_acls + 1));
        for (i = 0; i < ovs->n_acls; i++) {
            acl_info[i] = ovs->acls[i];
        }
        acl_info[i] = acl_row;
        ovsrec_system_set_acls(ovs, (struct ovsrec_acl **) acl_info, i + 1);
        free(acl_info);
    }
    /* Update */
    else {
        VLOG_DBG("Updating ACL type=%s name=%s", acl_type, acl_name);

        /* Don't actually have to take any action */
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Delete an ACL
 *
 * @param  acl_type  ACL type string
 * @param  acl_name  ACL name string
 *
 * @return           CMD_SUCCESS on success
 *
 */
static int
cli_delete_acl(const char *acl_type, const char *acl_name)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_system *ovs;
    const struct ovsrec_acl *acl_row;
    const struct ovsrec_acl **acl_info;
    int i, n;

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get System table */
    ovs = ovsrec_system_first(idl);
    if (!ovs) {
        cli_do_config_abort(transaction);
        assert(0);
        return CMD_OVSDB_FAILURE;
    }

    /* Get ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);

    /* ACL exists, delete it */
    if (acl_row) {
        VLOG_DBG("Deleting ACL type=%s name=%s", acl_type, acl_name);

        /* Remove ACL row */
        ovsrec_acl_delete(acl_row);

        /* Update System table */
        acl_info = xmalloc(sizeof *ovs->acls * (ovs->n_acls - 1));
        for (i = n = 0; i < ovs->n_acls; i++) {
            if (ovs->acls[i] != acl_row) {
                acl_info[n++] = ovs->acls[i];
            }
        }
        ovsrec_system_set_acls(ovs, (struct ovsrec_acl **) acl_info, n);
        free(acl_info);
    }
    /* No such ACL exists */
    else {
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Create/Update an ACE
 *
 * @param  acl_type                       Type string
 * @param  acl_name                       Name string
 * @param  ace_sequence_number_tmp        Sequence number string (NULL = auto)
 * @param  ace_action                     Action string
 * @param  ace_ip_protocol                IP protocol string
 * @param  ace_source_ip_address          Source IP address string
 * @param  ace_source_port_operator       Operator for source port(s)
 * @param  ace_source_port                First source port
 * @param  ace_source_port_max            Second source port (range only)
 * @param  ace_destination_ip_address     Destination IP address string
 * @param  ace_destination_port_operator  Operator for destination port(s)
 * @param  ace_destination_port           First destination port
 * @param  ace_destination_port_max       Second destination port (range only)
 * @param  ace_log_enabled                Is logging enabled on this entry?
 * @param  ace_count_enabled              Is counting enabled on this entry?
 * @param  ace_comment                    Text comment string
 *
 * @return                                CMD_SUCCESS on success
 */
static int
cli_create_update_ace (const char *acl_type,
                       const char *acl_name,
                       const char *ace_sequence_number_tmp,
                       const char *ace_action,
                       const char *ace_ip_protocol,
                       const char *ace_source_ip_address,
                       const char *ace_source_port_operator,
                       const char *ace_source_port,
                       const char *ace_source_port_max,
                       const char *ace_destination_ip_address,
                       const char *ace_destination_port_operator,
                       const char *ace_destination_port,
                       const char *ace_destination_port_max,
                       const char *ace_log_enabled,
                       const char *ace_count_enabled,
                             char *ace_comment)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_acl *acl_row;
    char *ace_sequence_number;
    bool ace_sequence_number_allocated = false; /* stays false if no auto-seq */
    struct smap aces;
    struct json *ace;
    char *ace_str;

    VLOG_DBG("Create/Update");

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get parent ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);
    if (!acl_row) {
        /* Should not be possible; context should have created if needed */
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* If a sequence number is specified, use it */
    if (ace_sequence_number_tmp) {
        ace_sequence_number = (char *) ace_sequence_number_tmp;
    /* Otherwise set sequence number to the current max + ACE_SEQ_AUTO_INCR */
    } else {
        const char *highest_ace_seq;
        unsigned long highest_ace_seq_numeric;
        if ((highest_ace_seq = acl_get_highest_seq(acl_row)) == NULL) {
            highest_ace_seq_numeric = 0;
        } else {
            highest_ace_seq_numeric = strtoul(highest_ace_seq, NULL, 0);
        }
        if (highest_ace_seq_numeric + ACE_SEQ_AUTO_INCR > ACE_SEQ_MAX) {
            vty_out(vty, "%% Unable to automatically set sequence number%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }
        ace_sequence_number_allocated = true;
        ace_sequence_number = xzalloc(ACE_SEQ_MAX_STR_LEN);
        sprintf(ace_sequence_number, "%lu", highest_ace_seq_numeric + ACE_SEQ_AUTO_INCR);
    }

    /* Initialize ACL Entries as a clone of existing ACEs */
    smap_clone(&aces, &acl_row->want);

    /* Updating an ACE removes/replaces its current data */
    if ((ace_str = CONST_CAST(char*, smap_get(CONST_CAST(struct smap*, &aces),
                                              ace_sequence_number))) != NULL)
    {
        VLOG_DBG("Replacing ACE acl_type=%s acl_name=%s sequence_number=%s entry=%s",
                 acl_type, acl_name, ace_sequence_number, ace_str);
        smap_remove(&aces, ace_sequence_number);
    }

    /* New/updated ACE starts empty, filled in via parameters below */
    ace = json_object_create();

    /* Parse ACE parameters */
    if (ace_action)
        json_object_put_string(ace, ace_key_names[ACE_KEY_ACTION], ace_action);
    if (ace_ip_protocol)
        json_object_put_string(ace, ace_key_names[ACE_KEY_IP_PROTOCOL], ace_ip_protocol);
    if (ace_source_ip_address)
        json_object_put_string(ace, ace_key_names[ACE_KEY_SOURCE_IP_ADDRESS], ace_source_ip_address);
    if (ace_source_port_operator)
        json_object_put_string(ace, ace_key_names[ACE_KEY_SOURCE_PORT_OPERATOR], ace_source_port_operator);
    if (ace_source_port)
        json_object_put_string(ace, ace_key_names[ACE_KEY_SOURCE_PORT], ace_source_port);
    if (ace_source_port_max)
        json_object_put_string(ace, ace_key_names[ACE_KEY_SOURCE_PORT_MAX], ace_source_port_max);
    if (ace_destination_ip_address)
        json_object_put_string(ace, ace_key_names[ACE_KEY_DESTINATION_IP_ADDRESS], ace_destination_ip_address);
    if (ace_destination_port_operator)
        json_object_put_string(ace, ace_key_names[ACE_KEY_DESTINATION_PORT_OPERATOR], ace_destination_port_operator);
    if (ace_destination_port)
        json_object_put_string(ace, ace_key_names[ACE_KEY_DESTINATION_PORT], ace_destination_port);
    if (ace_destination_port_max)
        json_object_put_string(ace, ace_key_names[ACE_KEY_DESTINATION_PORT_MAX], ace_destination_port_max);
    if (ace_log_enabled)
        json_object_put_string(ace, ace_key_names[ACE_KEY_LOG], ACL_TRUE_STR);
    if (ace_count_enabled)
        json_object_put_string(ace, ace_key_names[ACE_KEY_COUNT], ACL_TRUE_STR);
    if (ace_comment)
        json_object_put_string(ace, ace_key_names[ACE_KEY_COMMENT], ace_comment);

    /* Create a JSON string */
    ace_str = json_to_string(ace, 0);
    VLOG_DBG("Constructed ACE JSON string: %s", ace_str);

    /* Add new/updated ACE to ACL's entries */
    smap_add(&aces, ace_sequence_number, ace_str);
    ovsrec_acl_set_want(acl_row, &aces);

    /* Clean up things allocated for us by json library */
    json_destroy(ace);
    free(ace_str);

    /* Clean up automatic sequence number string (if allocated) */
    if (ace_sequence_number_allocated) {
        free(ace_sequence_number);
    }

    /* Clean up comment text buffer (if allocated) */
    if (ace_comment) {
        free(ace_comment);
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Delete an ACE
 *
 * @param  acl_type             ACL type string
 * @param  acl_name             ACL name string
 * @param  ace_sequence_number  ACE parameter string
 *
 * @return                      CMD_SUCCESS on success
 *
 */
static int
cli_delete_ace (const char *acl_type,
                const char *acl_name,
                const char *ace_sequence_number)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_acl *acl_row;
    struct smap aces;
    char *ace_str;

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get parent ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);
    if (!acl_row) {
        /* Should not be possible; context should have created */
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Initialize ACL Entries as a clone of existing ACEs */
    smap_clone(&aces, &acl_row->want);

    /* Remove associated ACE */
    if ((ace_str = CONST_CAST(char*, smap_get(CONST_CAST(struct smap*, &aces),
                                              ace_sequence_number))) != NULL)
    {
        VLOG_DBG("Deleting ACE acl_type=%s acl_name=%s sequence_number=%s entry=%s",
                 acl_type, acl_name, ace_sequence_number, ace_str);
        smap_remove(&aces, ace_sequence_number);
    }
    /* ACE does not exist */
    else {
        vty_out(vty, "%% ACL entry does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Modify ACL's entries (to have one removed) */
    ovsrec_acl_set_want(acl_row, &aces);

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Resequence entries in an ACL
 *
 * @param  acl_type   ACL type string
 * @param  acl_name   ACL string name to apply
 * @param  start      Starting entry sequence number
 * @param  increment  Increment to increase each entry's sequence number by
 *
 * @return            CMD_SUCCESS on success
 */
static int
cli_resequence_acl (const char *acl_type,
                    const char *acl_name,
                    const char *start,
                    const char *increment)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_acl *acl_row;
    const struct smap_node **aces_sorted;
    unsigned long start_num, increment_num, current_num;
    char current[ACE_SEQ_MAX_STR_LEN];
    struct smap new_aces;
    int i;

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get parent ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);
    if (!acl_row) {
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Check for an empty list */
    if (smap_is_empty(&acl_row->want)) {
        vty_out(vty, "%% ACL is empty%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Set numeric values */
    start_num = strtoul(start, NULL, 0);
    increment_num = strtoul(increment, NULL, 0);
    current_num = start_num;

    /* Check that sequence numbers will not exceed maximum a_n = a_0 + (n-1)d
     * Test that formula works for ACE_SEQ_MAX of 4294967295:
     *   use start = 3, increment = 1073741823 on 5-ACE list
     *   input should be accepted
     *   resequence should result in ACE #5 seq=4294967295
     */
    if (start_num + ((smap_count(&acl_row->want) - 1) * increment_num) > ACE_SEQ_MAX) {
        vty_out(vty, "%% Sequence numbers would exceed maximum%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Initialize temporary data structures */
    smap_init(&new_aces);
    sprintf(current, "%lu", current_num);

    /* Sort existing ACEs */
    aces_sorted = smap_sort_numeric(&acl_row->want);

    /* Walk through sorted list, resequencing by adding into new_aces */
    for (i = 0; i < smap_count(&acl_row->want); i++) {
        smap_add(&new_aces, current, aces_sorted[i]->value);
        current_num += increment_num;
        sprintf(current, "%lu", current_num);
    }

    /* Replace ACL's entries with resequenced ones */
    ovsrec_acl_set_want(acl_row, &new_aces);

    /* Clean up temporary data structures */
    free(aces_sorted);
    smap_destroy(&new_aces);

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Display ACLs applied to the specified interface in the given direction
 *
 * @param  interface_type  Interface (Port/VLAN) type string
 * @param  interface_id    Interface (Port/VLAN) identifier string
 * @param  acl_type        ACL type string
 * @param  direction       Direction of traffic ACL is applied to
 * @param  config          Print as configuration input?
 *
 * @return                 CMD_SUCCESS on success
 */
static int
cli_print_applied_acls (const char *interface_type,
                        const char *interface_id,
                        const char *acl_type,
                        const char *direction,
                        const char *config)
{
    /* Port (unfortunately called "interface" in the CLI) */
    if (!strcmp(interface_type, "interface")) {
        const struct ovsrec_port *port_row;

        /* Get Port row */
        port_row = get_port_by_name(interface_id);
        if (!port_row) {
            vty_out(vty, "%% Port does not exist%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

        if (port_row->aclv4_in_want) {
            VLOG_DBG("Found ACL application port=%s list_name=%s",
                     interface_id, port_row->aclv4_in_want->list_name);
            if (!config)
            {
                vty_out(vty, "Direction: Inbound Type: IPv4%s", VTY_NEWLINE);
                print_acl_tabular_header();
                print_acl_tabular(port_row->aclv4_in_want);
            } else {
                print_acl_config(port_row->aclv4_in_want);
            }
        }

        /* Print application commands if printing config */
        if (config && port_row->aclv4_in_want) {
            vty_out(vty, "%s %s\n  %s %s %s %s %s%s",
                    "interface", port_row->name,
                    "apply", "access-list", "ip", port_row->aclv4_in_want->list_name, "in",
                    VTY_NEWLINE);
        }
    } else if (!strcmp(interface_type, "vlan")) {
        const struct ovsrec_vlan *vlan_row;

        /* Get VLAN row */
        vlan_row = get_vlan_by_id_str(interface_id);
        if (!vlan_row) {
            vty_out(vty, "%% VLAN does not exist%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

        if (vlan_row->aclv4_in_want) {
            VLOG_DBG("Found ACL application vlan=%s list_name=%s",
                     interface_id, vlan_row->aclv4_in_want->list_name);
            if (!config)
            {
                vty_out(vty, "Direction: Inbound Type: IPv4%s", VTY_NEWLINE);
                print_acl_tabular_header();
                print_acl_tabular(vlan_row->aclv4_in_want);
            } else {
                print_acl_config(vlan_row->aclv4_in_want);
            }
        }

        /* Print application commands if printing config */
        if (config && vlan_row->aclv4_in_want) {
            vty_out(vty, "%s %" PRId64 "\n  %s %s %s %s %s%s",
                    "vlan", vlan_row->id,
                    "apply", "access-list", "ip", vlan_row->aclv4_in_want->list_name, "in",
                    VTY_NEWLINE);
        }
    }

    return CMD_SUCCESS;
}

/**
 * Apply an ACL to an interface in a specified direction
 *
 * @param  interface_type  Interface (Port/VLAN) type string
 * @param  interface_id    Interface (Port/VLAN) identifier string
 * @param  acl_type        ACL type string
 * @param  acl_name        ACL string name to apply
 * @param  direction       Direction of traffic ACL is applied to
 *
 * @return                 CMD_SUCCESS on success
 */
static int
cli_apply_acl (const char *interface_type,
               const char *interface_id,
               const char *acl_type,
               const char *acl_name,
               const char *direction)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_acl *acl_row;

    VLOG_DBG("Apply");

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);
    if (!acl_row) {
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Port (unfortunately called "interface" in the CLI) */
    if (!strcmp(interface_type, "interface")) {
        const struct ovsrec_port *port_row;
        /* Get Port row */
        port_row = get_port_by_name(interface_id);
        if (!port_row) {
            vty_out(vty, "%% Port does not exist%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

        if (!strcmp(acl_type, "ipv4") && !strcmp(direction, "in")) {
            /* Check if we're replacing an already-applied ACL */
            if (port_row->aclv4_in_want) {
                VLOG_DBG("Old ACL application port=%s acl_name=%s",
                         interface_id, port_row->aclv4_in_want->list_name);
            }
            /* Apply the requested ACL to the Port */
            VLOG_DBG("New ACL application port=%s acl_name=%s", interface_id, acl_name);
            ovsrec_port_set_aclv4_in_want(port_row, acl_row);
        } else {
            vty_out(vty, "%% Unsupported ACL type or direction%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

    } else if (!strcmp(interface_type, "vlan")) {
        const struct ovsrec_vlan *vlan_row;
        /* Get VLAN row */
        vlan_row = get_vlan_by_id_str(interface_id);
        if (!vlan_row) {
            vty_out(vty, "%% VLAN does not exist%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

        if (!strcmp(acl_type, "ipv4") && !strcmp(direction, "in")) {
            /* Check if we're replacing an already-applied ACL */
            if (vlan_row->aclv4_in_want) {
                VLOG_DBG("Old ACL application vlan=%s acl_name=%s",
                         interface_id, vlan_row->aclv4_in_want->list_name);
            }

            /* Apply the requested ACL to the VLAN */
            VLOG_DBG("New ACL application vlan=%s acl_name=%s", interface_id, acl_name);
            ovsrec_vlan_set_aclv4_in_want(vlan_row, acl_row);
        } else {
            vty_out(vty, "%% Unsupported ACL type or direction%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Un-apply an ACL from an interface in a specified direction
 *
 * @param  interface_type  Interface (Port/VLAN) type string
 * @param  interface_id    Interface (Port/VLAN) identifier string
 * @param  acl_type        ACL type string
 * @param  acl_name        ACL name string
 * @param  direction       Direction of traffic ACL is applied to
 *
 * @return                 CMD_SUCCESS on success
 */
static int
cli_unapply_acl (const char *interface_type,
                 const char *interface_id,
                 const char *acl_type,
                 const char *acl_name,
                 const char *direction)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_acl *acl_row;

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get ACL row */
    acl_row = get_acl_by_type_name(acl_type, acl_name);
    if (!acl_row) {
        vty_out(vty, "%% ACL does not exist%s", VTY_NEWLINE);
        cli_do_config_abort(transaction);
        return CMD_SUCCESS;
    }

    /* Port (unfortunately called "interface" in the CLI) */
    if (!strcmp(interface_type, "interface")) {
        const struct ovsrec_port *port_row;
        /* Get Port row */
        port_row = get_port_by_name(interface_id);
        if (!port_row) {
            vty_out(vty, "%% Port does not exist%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

        if (!strcmp(acl_type, "ipv4") && !strcmp(direction, "in")) {
            /* Check that any ACL is currently applied to the port */
            if (!port_row->aclv4_in_want) {
                vty_out(vty, "%% No ACL is applied to port %s", VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

            /* Check that the requested ACL to remove is the one applied to port */
            if (strcmp(acl_name, port_row->aclv4_in_want->list_name)) {
                vty_out(vty, "%% ACL %s is applied to port %s, not %s%s",
                        port_row->aclv4_in_want->list_name,
                        port_row->name, acl_name, VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

            /* Un-apply the requested ACL application from the Port */
            VLOG_DBG("Removing ACL application port=%s acl_name=%s", interface_id, acl_name);
            ovsrec_port_set_aclv4_in_want(port_row, NULL);
        } else {
            vty_out(vty, "%% Unsupported ACL type or direction%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

    } else if (!strcmp(interface_type, "vlan")) {
        const struct ovsrec_vlan *vlan_row;
        /* Get VLAN row */
        vlan_row = get_vlan_by_id_str(interface_id);
        if (!vlan_row) {
            vty_out(vty, "%% VLAN does not exist%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }

        if (!strcmp(acl_type, "ipv4") && !strcmp(direction, "in")) {
            /* Check that any ACL is currently applied to the VLAN */
            if (!vlan_row->aclv4_in_want) {
                vty_out(vty, "%% No ACL is applied to VLAN %s", VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

            /* Check that the requested ACL to remove is the one applied to vlan */
            if (strcmp(acl_name, vlan_row->aclv4_in_want->list_name)) {
                vty_out(vty, "%% ACL %s is applied to VLAN %" PRId64 ", not %s%s",
                        vlan_row->aclv4_in_want->list_name,
                        vlan_row->id, acl_name, VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

            /* Un-apply the requested ACL application from the VLAN */
            VLOG_DBG("Removing ACL application vlan=%s acl_name=%s", interface_id, acl_name);
            ovsrec_vlan_set_aclv4_in_want(vlan_row, NULL);
        } else {
            vty_out(vty, "%% Unsupported ACL type or direction%s", VTY_NEWLINE);
            cli_do_config_abort(transaction);
            return CMD_SUCCESS;
        }
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Print statistics for an ACL (optionally for a specific interface
 * and/or direction)
 *
 * @param  acl_type        ACL type string
 * @param  acl_name        ACL name string
 * @param  interface_type  Interface (Port/VLAN) type string
 * @param  interface_id    Interface (Port/VLAN) identifier string
 * @param  direction       Direction of traffic ACL is applied to
 *
 * @return                 CMD_SUCCESS on success
 */
static int
cli_print_acl_statistics (const char *acl_type,
                          const char *acl_name,
                          const char *interface_type,
                          const char *interface_id,
                          const char *direction)
{
    const struct ovsrec_port *port_row;
    const struct ovsrec_vlan *vlan_row;

    VLOG_DBG("Showing statistics for %s ACL %s %s=%s direction=%s",
            acl_type, acl_name, interface_type, interface_id, direction);

    if (!get_acl_by_type_name(acl_type, acl_name)) {
        vty_out(vty, "%% ACL %s does not exist%s", acl_name, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    /* Port (unfortunately called "interface" in the CLI) */
    if (!strcmp(interface_type, "interface")) {
        /* Get Port row */
        port_row = get_port_by_name(interface_id);
        if (!port_row) {
            vty_out(vty, "%% Port %s does not exist%s", interface_id, VTY_NEWLINE);
            return CMD_SUCCESS;
        }
        if (port_row->aclv4_in_want && !strcmp(port_row->aclv4_in_want->list_name, acl_name)) {
            vty_out(vty,"Statistics for ACL %s:%s", acl_name, VTY_NEWLINE);
            vty_out(vty,"  Interface %s:%s", port_row->name, VTY_NEWLINE);
            /** @todo find out if this should be an array with defined indices */
            if (port_row->aclv4_in_statistics) {
                vty_out(vty,"    Hits: %" PRId64 "%s", port_row->aclv4_in_statistics[0], VTY_NEWLINE);
            } else {
                vty_out(vty,"    Hits: 0%s", VTY_NEWLINE);
            }
        } else {
            vty_out(vty, "%% Specified ACL not applied to interface%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

    } else if (!strcmp(interface_type, "vlan")) {
        /* Get VLAN row */
        vlan_row = get_vlan_by_id_str(interface_id);
        if (!vlan_row) {
            vty_out(vty, "%% VLAN %s does not exist%s", interface_id, VTY_NEWLINE);
            return CMD_SUCCESS;
        }
        if (vlan_row->aclv4_in_want && !strcmp(vlan_row->aclv4_in_want->list_name, acl_name)) {
            vty_out(vty,"Statistics for ACL %s:%s", acl_name, VTY_NEWLINE);
            vty_out(vty,"  VLAN %" PRId64 ":%s", vlan_row->id, VTY_NEWLINE);
            /** @todo find out if this should be an array with defined indices */
            if (vlan_row->aclv4_in_statistics) {
                vty_out(vty,"    Hits: %" PRId64 "%s", vlan_row->aclv4_in_statistics[0], VTY_NEWLINE);
            } else {
                vty_out(vty,"    Hits: 0%s", VTY_NEWLINE);
            }
        } else {
            vty_out(vty, "%% Specified ACL not applied to VLAN%s", VTY_NEWLINE);
            return CMD_SUCCESS;
        }

    /* No interface specified (implicit "all") */
    } else {
        vty_out(vty,"Statistics for ACL %s:%s", acl_name, VTY_NEWLINE);
        OVSREC_PORT_FOR_EACH(port_row, idl) {
            if (port_row->aclv4_in_want && !strcmp(port_row->aclv4_in_want->list_name, acl_name)) {
                vty_out(vty,"  Interface %s:%s", port_row->name, VTY_NEWLINE);
                /** @todo find out if this should be an array with defined indices */
                if (port_row->aclv4_in_statistics) {
                    vty_out(vty,"    Hits: %" PRId64 "%s", port_row->aclv4_in_statistics[0], VTY_NEWLINE);
                } else {
                    vty_out(vty,"    Hits: 0%s", VTY_NEWLINE);
                }
            }
        }
        OVSREC_VLAN_FOR_EACH(vlan_row, idl) {
            if (vlan_row->aclv4_in_want && !strcmp(vlan_row->aclv4_in_want->list_name, acl_name)) {
                vty_out(vty,"  VLAN %" PRId64 ":%s", vlan_row->id, VTY_NEWLINE);
                /** @todo find out if this should be an array with defined indices */
                if (vlan_row->aclv4_in_statistics) {
                    vty_out(vty,"    Hits: %" PRId64 "%s", vlan_row->aclv4_in_statistics[0], VTY_NEWLINE);
                } else {
                    vty_out(vty,"    Hits: 0%s", VTY_NEWLINE);
                }
            }
        }
    }

    return CMD_SUCCESS;
}

/**
 * Clear statistics for an ACL (optionally for a specific ACL, interface,
 * direction)
 *
 * @param  acl_type        ACL type string
 * @param  acl_name        ACL name string
 * @param  interface_type  Interface (Port/VLAN) type string
 * @param  interface_id    Interface (Port/VLAN) identifier string
 * @param  direction       Direction of traffic ACL is applied to
 *
 * @return                 CMD_SUCCESS on success
 */
static int
cli_clear_acl_statistics (const char *acl_type,
                          const char *acl_name,
                          const char *interface_type,
                          const char *interface_id,
                          const char *direction)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_port *port_row;
    const struct ovsrec_vlan *vlan_row;
    const int64_t zero_value = 0;

    VLOG_DBG("Clearing statistics for %s ACL %s %s=%s direction=%s",
            acl_type, acl_name, interface_type, interface_id, direction);

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Single ACL specified */
    if (acl_name) {
        /* Port (unfortunately called "interface" in the CLI) */
        if (!strcmp(interface_type, "interface")) {
            /* Get Port row */
            port_row = get_port_by_name(interface_id);
            if (!port_row) {
                vty_out(vty, "%% Port %s does not exist%s", interface_id, VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }
            if (port_row->aclv4_in_want && !strcmp(port_row->aclv4_in_want->list_name, acl_name)) {
                VLOG_DBG("Clearing ACL statistics port=%s acl_name=%s", interface_id, acl_name);
                /** @todo find out if this should be an array with defined indices */
                ovsrec_port_set_aclv4_in_statistics(port_row, &zero_value, 1);
            } else {
                vty_out(vty, "%% Specified ACL not applied to interface%s", VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

        } else if (!strcmp(interface_type, "vlan")) {
            /* Get VLAN row */
            vlan_row = get_vlan_by_id_str(interface_id);
            if (!vlan_row) {
                vty_out(vty, "%% VLAN %s does not exist%s", interface_id, VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }
            if (vlan_row->aclv4_in_want && !strcmp(vlan_row->aclv4_in_want->list_name, acl_name)) {
                VLOG_DBG("Clearing ACL statistics vlan=%s acl_name=%s", interface_id, acl_name);
                /** @todo find out if this should be an array with defined indices */
                ovsrec_vlan_set_aclv4_in_statistics(vlan_row, &zero_value, 1);
            } else {
                vty_out(vty, "%% Specified ACL not applied to VLAN%s", VTY_NEWLINE);
                cli_do_config_abort(transaction);
                return CMD_SUCCESS;
            }

        /* No interface specified (implicit "all") */
        } else {
            OVSREC_PORT_FOR_EACH(port_row, idl) {
                if (port_row->aclv4_in_want && !strcmp(port_row->aclv4_in_want->list_name, acl_name)) {
                    VLOG_DBG("Clearing ACL statistics port=%s acl_name=%s", port_row->name, acl_name);
                    /** @todo find out if this should be an array with defined indices */
                    ovsrec_port_set_aclv4_in_statistics(port_row, &zero_value, 1);
                }
            }
            OVSREC_VLAN_FOR_EACH(vlan_row, idl) {
                if (vlan_row->aclv4_in_want && !strcmp(vlan_row->aclv4_in_want->list_name, acl_name)) {
                    VLOG_DBG("Clearing ACL statistics vlan=%" PRId64 " acl_name=%s", vlan_row->id, acl_name);
                    /** @todo find out if this should be an array with defined indices */
                    ovsrec_vlan_set_aclv4_in_statistics(vlan_row, &zero_value, 1);
                }
            }

        }
    /* No ACL specified (implicit "all") */
    } else {
        OVSREC_PORT_FOR_EACH(port_row, idl) {
            VLOG_DBG("Clearing ACL statistics port=%s", port_row->name);
            /** @todo find out if this should be an array with defined indices */
            ovsrec_port_set_aclv4_in_statistics(port_row, &zero_value, 1);
        }
        OVSREC_VLAN_FOR_EACH(vlan_row, idl) {
            VLOG_DBG("Clearing ACL statistics vlan=%" PRId64 "", vlan_row->id);
            /** @todo find out if this should be an array with defined indices */
            ovsrec_vlan_set_aclv4_in_statistics(vlan_row, &zero_value, 1);
        }
    }

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/**
 * Set the ACL logging timer to a specified value (in seconds)
 *
 * @param  timer_value ACL log timer frequency (in seconds)
 *
 * @return             CMD_SUCCESS on success
 */
static int
cli_set_acl_log_timer(const char* timer_value)
{
    struct ovsdb_idl_txn *transaction;
    enum ovsdb_idl_txn_status txn_status;
    const struct ovsrec_system *ovs;
    struct smap other_config;

    VLOG_DBG("Setting ACL log timer to %s", timer_value);

    /* Start transaction */
    transaction = cli_do_config_start();
    if (!transaction) {
        VLOG_ERR("Unable to acquire transaction");
        return CMD_OVSDB_FAILURE;
    }

    /* Get System table */
    ovs = ovsrec_system_first(idl);
    if (!ovs) {
        cli_do_config_abort(transaction);
        assert(0);
        return CMD_OVSDB_FAILURE;
    }

    /* Copy current "other_config" column from System table */
    smap_clone(&other_config, &ovs->other_config);

    /* Remove any existing value (smap_add doesn't replace) */
    smap_remove(&other_config, ACL_LOG_TIMER_STR);

    /* Only set "other_config" record for non-default value */
    if (strcmp(timer_value, ACL_LOG_TIMER_DEFAULT_STR))
    {
        smap_add(&other_config, ACL_LOG_TIMER_STR, timer_value);
    }

    /* Set new "other_config" column in System table */
    ovsrec_system_set_other_config(ovs, &other_config);

    /* Complete transaction */
    txn_status = cli_do_config_finish(transaction);
    if (txn_status == TXN_SUCCESS || txn_status == TXN_UNCHANGED) {
        return CMD_SUCCESS;
    } else {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        return CMD_OVSDB_FAILURE;
    }
}

/* = CLI Definitions = */

/**
 * Action routine for creating/updating an ACL (entering context)
 */
DEFUN (cli_access_list,
       cli_access_list_cmd,
       "access-list ip NAME",
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
      )
{
    /* static buffers because CLI context persists past this function */
    static char acl_ip_version[IP_VER_STR_LEN];
    static char acl_name[MAX_ACL_NAME_LENGTH];

    if ((strnlen(argv[0], MAX_ACL_NAME_LENGTH) < MAX_ACL_NAME_LENGTH)) {
        strncpy(acl_ip_version, "ipv4", IP_VER_STR_LEN);
        strncpy(acl_name, argv[0], MAX_ACL_NAME_LENGTH);
    } else {
        return CMD_ERR_NO_MATCH;
    }

    /* Same name can be used with different IP versions; consider name sub-index */
    vty->index = acl_ip_version;
    vty->index_sub = acl_name;
    vty->node = ACCESS_LIST_NODE;

    return cli_create_acl_if_needed(CONST_CAST(char*,vty->index),      /* Type */
                                    CONST_CAST(char*,vty->index_sub)); /* Name */
}

/**
 * Action routine for deleting an ACL
 */
DEFUN (cli_no_access_list,
       cli_no_access_list_cmd,
       "no access-list ip NAME",
       NO_STR
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
      )
{
    return cli_delete_acl("ipv4",
                          CONST_CAST(char*,argv[0]));
}

/**
 * Action routine for showing all ACLs
 */
DEFUN (cli_show_access_list,
       cli_show_access_list_cmd,
       "show access-list { config }",
       SHOW_STR
       ACL_STR
       ACL_CFG_STR
      )
{
    return cli_print_acls(NULL,                       /* Type */
                          NULL,                       /* Name */
                          CONST_CAST(char*,argv[0])); /* Config */
}

/**
 * Action routine for showing all ACLs of a specified type
 */
DEFUN (cli_show_access_list_type,
       cli_show_access_list_type_cmd,
       "show access-list ip { config }",
       SHOW_STR
       ACL_STR
       ACL_IP_STR
       ACL_CFG_STR
      )
{
    return cli_print_acls("ipv4",                     /* Type */
                          NULL,                       /* Name */
                          CONST_CAST(char*,argv[0])); /* Config */
}

/**
 * Action routine for showing a single ACL (specified name + type)
 */
DEFUN (cli_show_access_list_type_name,
       cli_show_access_list_type_name_cmd,
       "show access-list ip NAME { config }",
       SHOW_STR
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
       ACL_CFG_STR
      )
{
    return cli_print_acls("ipv4",                     /* Type */
                          CONST_CAST(char*,argv[0]),  /* Name */
                          CONST_CAST(char*,argv[1])); /* Config */
}

/**
 * Action routine for resequencing an ACL
 */
DEFUN (cli_access_list_resequence,
       cli_access_list_resequence_cmd,
       "access-list ip NAME resequence " ACE_SEQ_CMDSTR " " ACE_SEQ_CMDSTR,
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
       "Re-number entries\n"
       "Starting sequence number\n"
       "Re-sequence increment\n"
      )
{
    return cli_resequence_acl("ipv4",
                              CONST_CAST(char*,argv[0]),
                              CONST_CAST(char*,argv[1]),
                              CONST_CAST(char*,argv[2]));
}

/* ACE create/update command functions.
 * These are PAINFUL to express due to vtysh's lack of handling for optional
 * tokens or sequences in the middle of a command. The relevant combinations
 * are below and result in 18 combinations (and therefore "DEFUN" calls)
 *
 * - With or without sequence number
 * - Layer 4 source port options (3)
 *   - None
 *   - Operation and port specified
 *   - Range and min+max ports specified
 * - Layer 4 destination port options (3)
 *   - None
 *   - Operation and port specified
 *   - Range and min+max ports specified
 *
 * Adding another optional parameter mid-command will double this number again.
 */

/**
 * Action routine for setting an ACE
 */
DEFUN (cli_access_list_entry,
       cli_access_list_entry_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_ALL_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_ALL_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[4]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[5]),        /* Log */
                                 CONST_CAST(char*,argv[6]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a source port operator specified
 */
DEFUN (cli_access_list_entry_src_port_op,
       cli_access_list_entry_src_port_op_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Log */
                                 CONST_CAST(char*,argv[8]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a source port range specified
 */
DEFUN (cli_access_list_entry_src_port_range,
       cli_access_list_entry_src_port_range_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[6]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[8]),        /* Log */
                                 CONST_CAST(char*,argv[9]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a destination port operator specified
 */
DEFUN (cli_access_list_entry_dst_port_op,
       cli_access_list_entry_dst_port_op_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[4]),        /* Destination IP */
                                 CONST_CAST(char*,argv[5]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[6]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Log */
                                 CONST_CAST(char*,argv[8]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a destination port range specified
 */
DEFUN (cli_access_list_entry_dst_port_range,
       cli_access_list_entry_dst_port_range_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[4]),        /* Destination IP */
                                 CONST_CAST(char*,argv[5]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[6]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port 2 */
                                 CONST_CAST(char*,argv[8]),        /* Log */
                                 CONST_CAST(char*,argv[9]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with both source and destination port
 * operators specified
 */
DEFUN (cli_access_list_entry_src_port_op_dst_port_op,
       cli_access_list_entry_src_port_op_dst_port_op_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[9]),        /* Log */
                                 CONST_CAST(char*,argv[10]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with both source and destination port
 * ranges specified
 */
DEFUN (cli_access_list_entry_src_port_range_dst_port_range,
       cli_access_list_entry_src_port_range_dst_port_range_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[6]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Destination IP */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[9]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[10]),       /* Destination Port 2 */
                                 CONST_CAST(char*,argv[11]),       /* Log */
                                 CONST_CAST(char*,argv[12]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with source port operator and destination
 * port range specified
 */
DEFUN (cli_access_list_entry_src_port_op_dst_port_range,
       cli_access_list_entry_src_port_op_dst_port_range_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[9]),        /* Destination Port 2 */
                                 CONST_CAST(char*,argv[10]),       /* Log */
                                 CONST_CAST(char*,argv[11]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with source port range and destination
 * port operator specified
 */
DEFUN (cli_access_list_entry_src_port_range_dst_port_op,
       cli_access_list_entry_src_port_range_dst_port_op_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 CONST_CAST(char*,argv[2]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[3]),        /* Source IP */
                                 CONST_CAST(char*,argv[4]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[6]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Destination IP */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[9]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[10]),       /* Log */
                                 CONST_CAST(char*,argv[11]),       /* Count */
                                 NULL);                            /* Comment */
}

/* ACE commands omitting sequence number */

/**
 * Action routine for setting an ACE without a sequence number
 */
DEFUN (cli_access_list_entry_no_seq,
       cli_access_list_entry_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_ALL_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_ALL_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[3]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[4]),        /* Log */
                                 CONST_CAST(char*,argv[5]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a source port operator specified
 * without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_op_no_seq,
       cli_access_list_entry_src_port_op_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[5]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Log */
                                 CONST_CAST(char*,argv[7]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a source port range specified
 * without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_range_no_seq,
       cli_access_list_entry_src_port_range_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Log */
                                 CONST_CAST(char*,argv[8]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a destination port operator specified
 * without a sequence number
 */
DEFUN (cli_access_list_entry_dst_port_op_no_seq,
       cli_access_list_entry_dst_port_op_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[3]),        /* Destination IP */
                                 CONST_CAST(char*,argv[4]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Log */
                                 CONST_CAST(char*,argv[7]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a destination port range specified
 * without a sequence number
 */
DEFUN (cli_access_list_entry_dst_port_range_no_seq,
       cli_access_list_entry_dst_port_range_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[3]),        /* Destination IP */
                                 CONST_CAST(char*,argv[4]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[5]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[6]),        /* Destination Port 2 */
                                 CONST_CAST(char*,argv[7]),        /* Log */
                                 CONST_CAST(char*,argv[8]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with both source and destination port
 * operators specified without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_op_dst_port_op_no_seq,
       cli_access_list_entry_src_port_op_dst_port_op_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[5]),        /* Destination IP */
                                 CONST_CAST(char*,argv[6]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[8]),        /* Log */
                                 CONST_CAST(char*,argv[9]),        /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with both source and destination port
 * ranges specified without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_range_dst_port_range_no_seq,
       cli_access_list_entry_src_port_range_dst_port_range_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[9]),        /* Destination Port 2 */
                                 CONST_CAST(char*,argv[10]),       /* Log */
                                 CONST_CAST(char*,argv[11]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with source port operator and destination
 * port range specified without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_op_dst_port_range_no_seq,
       cli_access_list_entry_src_port_op_dst_port_range_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_OPER_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_RANGE_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 CONST_CAST(char*,argv[5]),        /* Destination IP */
                                 CONST_CAST(char*,argv[6]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port 1 */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port 2 */
                                 CONST_CAST(char*,argv[9]),        /* Log */
                                 CONST_CAST(char*,argv[10]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with source port range and destination
 * port operator specified without a sequence number
 */
DEFUN (cli_access_list_entry_src_port_range_dst_port_op_no_seq,
       cli_access_list_entry_src_port_range_dst_port_op_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_ACTION_CMDSTR
       ACE_PORT_PROTOCOLS_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_RANGE_CMDSTR
       ACE_IP_ADDRESS_CMDSTR
       ACE_PORT_OPER_CMDSTR
       ACE_ADDITIONAL_OPTIONS_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_ACTION_HELPSTR
       ACE_PORT_PROTOCOLS_HELPSTR
       ACE_SRC_IP_ADDRESS_HELPSTR
       ACE_SRC_PORT_RANGE_HELPSTR
       ACE_DST_IP_ADDRESS_HELPSTR
       ACE_DST_PORT_OPER_HELPSTR
       ACE_ADDITIONAL_OPTIONS_HELPSTR
      )
{
    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 CONST_CAST(char*,argv[1]),        /* IP Protocol */
                                 CONST_CAST(char*,argv[2]),        /* Source IP */
                                 CONST_CAST(char*,argv[3]),        /* Source Port Operator */
                                 CONST_CAST(char*,argv[4]),        /* Source Port 1 */
                                 CONST_CAST(char*,argv[5]),        /* Source Port 2 */
                                 CONST_CAST(char*,argv[6]),        /* Destination IP */
                                 CONST_CAST(char*,argv[7]),        /* Destination Port Operator */
                                 CONST_CAST(char*,argv[8]),        /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 CONST_CAST(char*,argv[9]),        /* Log */
                                 CONST_CAST(char*,argv[10]),       /* Count */
                                 NULL);                            /* Comment */
}

/**
 * Action routine for setting an ACE with a text comment
 */
DEFUN (cli_access_list_entry_comment,
       cli_access_list_entry_comment_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_SEQ_CMDSTR
       ACE_COMMENT_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_SEQ_HELPSTR
       ACE_COMMENT_HELPSTR
      )
{
    /* To be freed after use */
    char *comment_text = argv_concat(argv, argc, 2);

    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 CONST_CAST(char*,argv[0]),        /* Sequence number */
                                 CONST_CAST(char*,argv[1]),        /* Action */
                                 NULL,                             /* IP Protocol */
                                 NULL,                             /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 NULL,                             /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 NULL,                             /* Log */
                                 NULL,                             /* Count */
                                 comment_text);                    /* Comment */
}

/**
 * Action routine for setting an ACE with a text comment without a sequence
 * number
 */
DEFUN (cli_access_list_entry_comment_no_seq,
       cli_access_list_entry_comment_no_seq_cmd,
       /* start of cmdstr, broken up to help readability */
       ACE_COMMENT_CMDSTR
       , /* end of cmdstr, comment to avoid accidental comma loss */

       /* helpstr, newline delimited */
       ACE_COMMENT_HELPSTR
      )
{
    /* To be freed after use */
    char *comment_text = argv_concat(argv, argc, 1);

    return cli_create_update_ace(CONST_CAST(char*,vty->index),     /* Type */
                                 CONST_CAST(char*,vty->index_sub), /* Name */
                                 NULL,                             /* Sequence number */
                                 CONST_CAST(char*,argv[0]),        /* Action */
                                 NULL,                             /* IP Protocol */
                                 NULL,                             /* Source IP */
                                 NULL,                             /* Source Port Operator */
                                 NULL,                             /* Source Port 1 */
                                 NULL,                             /* Source Port 2 */
                                 NULL,                             /* Destination IP */
                                 NULL,                             /* Destination Port Operator */
                                 NULL,                             /* Destination Port 1 */
                                 NULL,                             /* Destination Port 2 */
                                 NULL,                             /* Log */
                                 NULL,                             /* Count */
                                 comment_text);                    /* Comment */
}

/**
 * Action routine for deleting an ACE
 */
DEFUN (cli_no_access_list_entry,
       cli_no_access_list_entry_cmd,
       "no " ACE_SEQ_CMDSTR,
       NO_STR
       ACE_SEQ_HELPSTR
      )
{
    return cli_delete_ace(CONST_CAST(char*,vty->index),     /* Type */
                          CONST_CAST(char*,vty->index_sub), /* Name */
                          CONST_CAST(char*,argv[0]));       /* Sequence number */
}

/**
 * Alternate form that ignores additional tokens when deleting an ACE
 */
ALIAS (cli_no_access_list_entry,
       cli_no_access_list_entry_etc_cmd,
       "no " ACE_SEQ_CMDSTR " ....",
       NO_STR
       ACE_SEQ_HELPSTR
       "(ignored)\n"
      )

/**
 * Action routine for showing applications of ACLs
 */
DEFUN (cli_show_access_list_applied, cli_show_access_list_applied_cmd,
       "show access-list (interface|vlan) ID { ip | in | config }",
       SHOW_STR
       ACL_STR
       ACL_INTERFACE_STR
       ACL_VLAN_STR
       ACL_INTERFACE_ID_STR
       ACL_IP_STR
       ACL_IN_STR
       ACL_CFG_STR
      )
{
    const char ipv4_str[] = "ipv4";
    const char *type_str;
    if (argv[2] && !strcmp(argv[2], "ip")) {
        type_str = ipv4_str;
    } else {
        type_str = argv[2];
    }
    return cli_print_applied_acls(CONST_CAST(char*,argv[0]),  /* interface type */
                                  CONST_CAST(char*,argv[1]),  /* interface id */
                                  CONST_CAST(char*,type_str), /* type */
                                  CONST_CAST(char*,argv[3]),  /* direction */
                                  CONST_CAST(char*,argv[4])); /* config */
}

/**
 * Action routine for applying an ACL to an interface
 */
DEFUN (cli_apply_access_list, cli_apply_access_list_cmd,
       "apply access-list (ip) NAME (in)",
       ACL_APPLY_STR
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
       ACL_IN_STR
      )
{
    const char vlan_str[] = "vlan";
    const char interface_str[] = "interface";
    const char *interface_type_str;
    const char ipv4_str[] = "ipv4";
    const char *type_str;

    if (vty->node == VLAN_NODE) {
        interface_type_str = vlan_str;
    } else if (vty->node == INTERFACE_NODE) {
        interface_type_str = interface_str;
    } else {
        interface_type_str = NULL;
    }
    if (argv[0] && !strcmp(argv[0], "ip")) {
        type_str = ipv4_str;
    } else {
        type_str = argv[0];
    }
    return cli_apply_acl(interface_type_str,           /* interface type */
                         CONST_CAST(char*,vty->index), /* interface id */
                         type_str,                     /* type */
                         CONST_CAST(char*,argv[1]),    /* name */
                         CONST_CAST(char*,argv[2]));   /* direction */
}

/**
 * Action routine for un-applying an ACL from an interface
 */
DEFUN (cli_no_apply_access_list, cli_no_apply_access_list_cmd,
       "no apply access-list (ip) NAME (in)",
       NO_STR
       ACL_APPLY_STR
       ACL_STR
       ACL_IP_STR
       ACL_NAME_STR
       ACL_IN_STR
      )
{
    const char vlan_str[] = "vlan";
    const char interface_str[] = "interface";
    const char *interface_type_str;
    const char ipv4_str[] = "ipv4";
    const char *type_str;

    if (vty->node == VLAN_NODE) {
        interface_type_str = vlan_str;
    } else if (vty->node == INTERFACE_NODE) {
        interface_type_str = interface_str;
    } else {
        interface_type_str = NULL;
    }
    if (argv[0] && !strcmp(argv[0], "ip")) {
        type_str = ipv4_str;
    } else {
        type_str = argv[0];
    }
    return cli_unapply_acl(interface_type_str,           /* interface type */
                           CONST_CAST(char*,vty->index), /* interface id */
                           type_str,                     /* type */
                           CONST_CAST(char*,argv[1]),    /* name */
                           CONST_CAST(char*,argv[2]));   /* direction */
}

/**
 * Action routine for showing ACL statistics on a specified interface
 */
DEFUN (cli_show_access_list_hitcounts,
       cli_show_access_list_hitcounts_cmd,
       "show access-list hitcounts (ip) NAME (interface|vlan) ID { in }",
       SHOW_STR
       ACL_STR
       ACL_HITCOUNTS_STR
       ACL_IP_STR
       ACL_NAME_STR
       ACL_INTERFACE_STR
       ACL_VLAN_STR
       ACL_INTERFACE_ID_STR
       ACL_IN_STR
      )
{
    const char ipv4_str[] = "ipv4";
    const char *type_str;
    if (argv[0] && !strcmp(argv[0], "ip")) {
        type_str = ipv4_str;
    } else {
        type_str = argv[0];
    }
    return cli_print_acl_statistics(CONST_CAST(char*,type_str), /* type */
                                    CONST_CAST(char*,argv[1]),  /* name */
                                    CONST_CAST(char*,argv[2]),  /* interface type */
                                    CONST_CAST(char*,argv[3]),  /* interface id */
                                    CONST_CAST(char*,argv[4])); /* direction */
}

/**
 * Action routine for clearing ACL statistics on a specified interface
 */
DEFUN (cli_clear_access_list_hitcounts,
       cli_clear_access_list_hitcounts_cmd,
       "clear access-list hitcounts (ip) NAME (interface|vlan) ID { in }",
       SHOW_STR
       ACL_STR
       ACL_HITCOUNTS_STR
       ACL_IP_STR
       ACL_NAME_STR
       ACL_INTERFACE_STR
       ACL_VLAN_STR
       ACL_INTERFACE_ID_STR
       ACL_IN_STR
      )
{
    const char ipv4_str[] = "ipv4";
    const char *type_str;
    if (argv[0] && !strcmp(argv[0], "ip")) {
        type_str = ipv4_str;
    } else {
        type_str = argv[0];
    }
    return cli_clear_acl_statistics(CONST_CAST(char*,type_str), /* type */
                                    CONST_CAST(char*,argv[1]),  /* name */
                                    CONST_CAST(char*,argv[2]),  /* interface type */
                                    CONST_CAST(char*,argv[3]),  /* interface id */
                                    CONST_CAST(char*,argv[4])); /* direction */
}

/**
 * Action routine for clearing all ACL statistics on all interfaces
 */
DEFUN (cli_clear_access_list_hitcounts_all,
       cli_clear_access_list_hitcounts_all_cmd,
       "clear access-list hitcounts all { in }",
       CLEAR_STR
       ACL_STR
       ACL_HITCOUNTS_STR
       ACL_ALL_STR
       ACL_IN_STR
      )
{
    return cli_clear_acl_statistics(NULL,                       /* type */
                                    NULL,                       /* name */
                                    NULL,                       /* interface type */
                                    NULL,                       /* interface id */
                                    CONST_CAST(char*,argv[0])); /* direction */
}

/**
 * Action routine for setting ACL log timer to a specified value (or default)
 */
DEFUN (cli_access_list_log_timer, cli_access_list_log_timer_cmd,
       "access-list log-timer (default|<" ACL_LOG_TIMER_MIN "-" ACL_LOG_TIMER_MAX ">)",
       ACL_STR
       "Set ACL log timer length (frequency)\n"
       "Default value (" ACL_LOG_TIMER_DEFAULT " seconds)\n"
       "Specify value (in seconds)\n"
      )
{
    return cli_set_acl_log_timer(CONST_CAST(char*,argv[0])); /* timer_value */
}

/* = Initialization = */

/**
 * Prompt string when in access-list context
 */
static struct cmd_node access_list_node = {
    ACCESS_LIST_NODE,
    "%s(config-acl)# "
};

/** Needed because vtysh_install_default() is static */
extern struct cmd_element config_list_cmd;

/**
 * Install the CLI action routines for ACL
 */
void
access_list_vty_init(void)
{
    install_node(&access_list_node, NULL);
    install_element(ACCESS_LIST_NODE, &config_list_cmd);

    install_element(CONFIG_NODE, &cli_access_list_cmd);
    install_element(CONFIG_NODE, &cli_no_access_list_cmd);
    install_element(CONFIG_NODE, &cli_access_list_resequence_cmd);

    install_element(ENABLE_NODE, &cli_show_access_list_cmd);
    install_element(ENABLE_NODE, &cli_show_access_list_type_cmd);
    install_element(ENABLE_NODE, &cli_show_access_list_type_name_cmd);
    install_element(VIEW_NODE, &cli_show_access_list_cmd);
    install_element(VIEW_NODE, &cli_show_access_list_type_cmd);
    install_element(VIEW_NODE, &cli_show_access_list_type_name_cmd);

    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_dst_port_op_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_dst_port_range_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_dst_port_op_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_dst_port_range_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_dst_port_range_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_dst_port_op_cmd);

    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_dst_port_op_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_dst_port_range_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_dst_port_op_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_dst_port_range_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_op_dst_port_range_no_seq_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_src_port_range_dst_port_op_no_seq_cmd);

    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_comment_cmd);
    install_element(ACCESS_LIST_NODE, &cli_access_list_entry_comment_no_seq_cmd);

    install_element(ACCESS_LIST_NODE, &cli_no_access_list_entry_cmd);
    install_element(ACCESS_LIST_NODE, &cli_no_access_list_entry_etc_cmd);

    install_element(ENABLE_NODE, &cli_show_access_list_applied_cmd);
    install_element(VIEW_NODE, &cli_show_access_list_applied_cmd);

    install_element(INTERFACE_NODE, &cli_apply_access_list_cmd);
    install_element(INTERFACE_NODE, &cli_no_apply_access_list_cmd);
    install_element(VLAN_NODE, &cli_apply_access_list_cmd);
    install_element(VLAN_NODE, &cli_no_apply_access_list_cmd);

    install_element(ENABLE_NODE, &cli_show_access_list_hitcounts_cmd);
    install_element(ENABLE_NODE, &cli_clear_access_list_hitcounts_cmd);
    install_element(ENABLE_NODE, &cli_clear_access_list_hitcounts_all_cmd);
    install_element(VIEW_NODE, &cli_show_access_list_hitcounts_cmd);

    install_element(CONFIG_NODE, &cli_access_list_log_timer_cmd);

    install_element(ACCESS_LIST_NODE, &config_exit_cmd);
    install_element(ACCESS_LIST_NODE, &config_quit_cmd);
    install_element(ACCESS_LIST_NODE, &config_end_cmd);
}

/**
 * Initialize ACL OVSDB tables, columns
 */
void
access_list_ovsdb_init(void)
{
    /* acls column in System table */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_acls);

    /* ACL table, columns */
    ovsdb_idl_add_table(idl, &ovsrec_table_acl);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_list_name);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_list_type);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_cur);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_want);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_want_version);
    ovsdb_idl_add_column(idl, &ovsrec_acl_col_want_status);

    /* ACL columns in Port table */
    ovsdb_idl_add_table(idl, &ovsrec_table_port);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_acl_applied);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_aclv4_in_cur);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_aclv4_in_want);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_aclv4_in_want_version);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_aclv4_in_want_status);
    ovsdb_idl_add_column(idl, &ovsrec_port_col_aclv4_in_statistics);

    /* ACL columns in VLAN table */
    ovsdb_idl_add_table(idl, &ovsrec_table_vlan);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_acl_applied);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_aclv4_in_cur);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_aclv4_in_want);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_aclv4_in_want_version);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_aclv4_in_want_status);
    ovsdb_idl_add_column(idl, &ovsrec_vlan_col_aclv4_in_statistics);
}

/**
 * Client callback routine for access-list (ACL) show running-config handler
 *
 * @param  p_private Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
 *                   structure object
 *
 * @return           e_vtysh_ok on success
 *
 * @note The behavior of this printing callback is a bit different from
 *       cli_print_acls(), so at the moment it doesn't share any code with it.
 */
vtysh_ret_val
vtysh_config_context_access_list_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *) p_private;
    const struct ovsrec_system *ovs;
    const struct ovsrec_acl *acl_row;
    const struct smap_node **aces_sorted;
    struct json *ace, *ace_printable;
    int ace_idx;

    /* Get System table */
    ovs = ovsrec_system_first(idl);
    if (!ovs) {
        assert(0);
        return CMD_OVSDB_FAILURE;
    }

    /* Iterate over each ACL table entry */
    OVSREC_ACL_FOR_EACH(acl_row, p_msg->idl) {
        if (acl_row) {
            vtysh_ovsdb_cli_print(p_msg,
                                  "%s %s %s",
                                  "access-list",
                                  "ip",
                                  acl_row->list_name);

            aces_sorted = smap_sort_numeric(&acl_row->want);
            for (ace_idx = 0; ace_idx < smap_count(&acl_row->want); ace_idx++) {
                ace = json_from_string(aces_sorted[ace_idx]->value);
                ace_printable = ace_clone_printable(ace);

                /* Print as a single line of values (ala CLI input) */
                vtysh_ovsdb_cli_print(p_msg,
                    "  %s %s%s%s%s%s%s%s%s%s%s%s%s%s",
                    aces_sorted[ace_idx]->key, /* key = sequence number */
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_ACTION]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_IP_PROTOCOL]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_IP_ADDRESS]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT_OPERATOR]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_SOURCE_PORT_MAX]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_IP_ADDRESS]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT_OPERATOR]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_DESTINATION_PORT_MAX]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_LOG]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_COUNT]),
                    json_object_get_string(ace_printable, ace_key_names[ACE_KEY_COMMENT]));

                json_destroy(ace);
                json_destroy(ace_printable);
            }
            free(aces_sorted);
        }
    }
    if (smap_get(&ovs->other_config, ACL_LOG_TIMER_STR))
    {
        vtysh_ovsdb_cli_print(p_msg, "access-list log-timer %s",
                              smap_get(&ovs->other_config, ACL_LOG_TIMER_STR));
    }
    return e_vtysh_ok;
}

/**
 * Client name for showing ACL information
 */
char accesslistconfigclientname[]= "vtysh_config_context_access_list_clientcallback";

/**
 * Client callback routine for port access-list (ACL) show running-config handler
 *
 * @param  p_private Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
 *                   structure object
 *
 * @return           e_vtysh_ok on success
 *
 * @note The behavior of this printing callback is a bit different from
 * cli_print_acls(), so at the moment it doesn't share any code with it.
 */
vtysh_ret_val
vtysh_config_context_port_access_list_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *) p_private;
    const struct ovsrec_port *port_row;

    /* Iterate over each Port table entry */
    OVSREC_PORT_FOR_EACH(port_row, p_msg->idl) {
        if (port_row->aclv4_in_want) {
            vtysh_ovsdb_cli_print(p_msg,
                                 "%s %s\n  %s %s %s %s %s",
                                 "interface",
                                 port_row->name,
                                   "apply",
                                   "access-list",
                                   "ip",
                                    port_row->aclv4_in_want->list_name,
                                    "in");
        }
    }
    return e_vtysh_ok;
}

/**
 * Client callback routine for vlan access-list (ACL) show running-config handler
 *
 * @param  p_private Void pointer for holding address of vtysh_ovsdb_cbmsg_ptr
 *                   structure object
 *
 * @return           e_vtysh_ok on success
 *
 * @note The behavior of this printing callback is a bit different from
 * cli_print_acls(), so at the moment it doesn't share any code with it.
 */
vtysh_ret_val
vtysh_config_context_vlan_access_list_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *) p_private;
    const struct ovsrec_vlan *vlan_row;

    /* Iterate over each VLAN table entry */
    OVSREC_VLAN_FOR_EACH(vlan_row, p_msg->idl) {
        if (vlan_row->aclv4_in_want) {
            vtysh_ovsdb_cli_print(p_msg,
                                 "%s %" PRId64 "\n  %s %s %s %s %s",
                                 "vlan",
                                 vlan_row->id,
                                   "apply",
                                   "access-list",
                                   "ip",
                                    vlan_row->aclv4_in_want->list_name,
                                    "in");
        }
    }
    return e_vtysh_ok;
}

/**
 * Client name for showing ACL application to ports
 */
char portaccesslistconfigclientname[]= "vtysh_config_context_port_access_list_clientcallback";

/**
 * Client name for showing ACL application to ports
 */
char vlanaccesslistconfigclientname[]= "vtysh_config_context_vlan_access_list_clientcallback";

/**
 * Register callbacks for access control list configuration show running-config
 * handlers
 *
 * @return  e_vtysh_ok on success
 */
vtysh_ret_val
vtysh_init_access_list_context_clients(void)
{
    vtysh_context_client client;
    vtysh_ret_val retval;

    retval = e_vtysh_error;
    memset(&client, 0, sizeof (vtysh_context_client));
    client.p_client_name = accesslistconfigclientname;
    client.client_id = e_vtysh_config_context_access_list;
    client.p_callback = &vtysh_config_context_access_list_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_config_context,
                                     e_vtysh_config_context_access_list,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "access-list context unable to add access list client callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    memset(&client, 0, sizeof (vtysh_context_client));
    client.p_client_name = portaccesslistconfigclientname;
    client.client_id = e_vtysh_config_context_port_access_list;
    client.p_callback = &vtysh_config_context_port_access_list_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_config_context,
                                     e_vtysh_config_context_port_access_list,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "access-list context unable to add port access list client callback");
        assert(0);
        return retval;
    }

    retval = e_vtysh_error;
    memset(&client, 0, sizeof (vtysh_context_client));
    client.p_client_name = vlanaccesslistconfigclientname;
    client.client_id = e_vtysh_config_context_vlan_access_list;
    client.p_callback = &vtysh_config_context_vlan_access_list_clientcallback;
    retval = vtysh_context_addclient(e_vtysh_config_context,
                                     e_vtysh_config_context_vlan_access_list,
                                     &client);
    if (e_vtysh_ok != retval) {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                                  "access-list context unable to add vlan access list client callback");
        assert(0);
        return retval;
    }

    return retval;
}
