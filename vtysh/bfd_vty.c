/**************************************************************************
 * Copyright (c) 2010-2013, LinkedIn Corp.
 * Copyright 2016 LinkedIn Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * Author: Ravi Jonnadula
 *
 * Purpose: BFD CLI implementation with OPS vtysh.
 **************************************************************************/

#include <stdio.h>
#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"
#include "vtysh/vtysh.h"
#include "log.h"
#include "bgp_vty.h"
#include "bfd_vty.h"
#include "vswitch-idl.h"
#include "openvswitch/vlog.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "util.h"
#include "prefix.h"
#include "sockunion.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include <lib/version.h>
#include "getopt.h"
#include "memory.h"
#include "vtysh/vtysh_user.h"
#include "ovsdb-idl.h"
#include "lib/prefix.h"

extern struct ovsdb_idl *idl;

#define BUF_LEN 10
#define BGP_UPTIME_LEN (25)
#define NET_BUFSZ    18
#define BGP_ATTR_DEFAULT_WEIGHT 32768
#define MAX_ARG_LEN 1024


/********************** Simple error handling ***********************/


/*
 * Depending on the outcome of the db transaction, returns
 * the appropriate value for the cli command execution.
 */
static const char *_undefined = "undefined";
static char itoa_buffer [64];

static char *
safe_print_string(size_t count, char *string)
{
    if ((count > 0) && string) {
        return string;
    }
    return (char*)_undefined;
}

static char *
safe_print_integer(size_t count, const int64_t *iptr)
{
    if ((count > 0) && iptr) {
        sprintf(itoa_buffer, "%"PRId64, *iptr);
        return itoa_buffer;
    }
    return (char*)_undefined;
}

static char *
safe_print_bool(size_t count, bool *bvalue)
{
    if ((count > 0) && bvalue) {
        return *bvalue ? "yes" : "no";
    }
    return (char*)_undefined;
}

#if 0
static char *
safe_print_smap_value(const struct smap *smap, char *key)
{
    const char *value = smap_get(smap, key);
    return value ? (char*)value : (char*)_undefined;
}
#endif

static bool
string_is_an_ip_address(const char *string)
{
    union sockunion su;
    return (str2sockunion(string, &su) >= 0);
}

static const struct ovsrec_bfd_session *
find_matching_bfd_session_object(struct ovsdb_idl *idl, const char *remote)
{
    const struct ovsrec_bfd_session *ovs_bfd_session;

    OVSREC_BFD_SESSION_FOR_EACH(ovs_bfd_session, idl) {
	if (strcmp(ovs_bfd_session->bfd_dst_ip, remote) == 0) {
		return ovs_bfd_session;
	}
    }
    return NULL;
}

static int
cli_bfd_global_timers_cmd_execute(int64_t interval, int64_t min_rx, int64_t multiplier)
{
	const struct ovsrec_bfd *ovs_bfd;
	struct ovsdb_idl_txn *ovsdb_txn;

	/* Start of transaction. */
	START_DB_TXN(ovsdb_txn);

	ovs_bfd = ovsrec_bfd_first(idl);
	if (!ovs_bfd) {
		ovs_bfd = ovsrec_bfd_insert(ovsdb_txn);
		if (!ovs_bfd) {
			vty_out(vty, "\nCreating BFD first record..Failed\n");
			ERRONEOUS_DB_TXN(ovsdb_txn, "%% BFD object creation failed\n");
		}
	}

	ovsrec_bfd_set_min_rx(ovs_bfd, &min_rx, 1);
	ovsrec_bfd_set_min_tx(ovs_bfd, &interval, 1);
	ovsrec_bfd_set_decay_min_rx(ovs_bfd, &multiplier, 1);

	/* End of transaction. */
	END_DB_TXN(ovsdb_txn);

	return CMD_SUCCESS;
}

static int
cli_bfd_session_cmd_execute(const char *remote_str, const char *local_str, char *owner, int64_t asn)
{
	const struct ovsrec_bfd_session *ovs_bfd_session;
	struct ovsdb_idl_txn *ovsdb_txn;
        char error_message[128];

	if (!string_is_an_ip_address(remote_str)) {
		vty_out(vty, "\nInvalid remote address %s\n", remote_str);
		return CMD_WARNING;
	}

	if (!string_is_an_ip_address(local_str)) {
		vty_out(vty, "\nInvalid local address %s\n", local_str);
		return CMD_WARNING;
	}

	/* Start of transaction. */
	START_DB_TXN(ovsdb_txn);

	ovs_bfd_session = find_matching_bfd_session_object(idl, remote_str);
	if (ovs_bfd_session) {
		vty_out(vty, "\nBFD Session for remote %s already exists!\n", remote_str);
		sprintf(error_message, "%% BFD Session for remote %s already exists\n", remote_str);
		ABORT_DB_TXN(ovsdb_txn, error_message);
	}


	ovs_bfd_session = ovsrec_bfd_session_insert(ovsdb_txn);
	if (!ovs_bfd_session) {
		vty_out(vty, "\nFailed to create BFD Session for remote %s local %s\n", remote_str, local_str);
		ERRONEOUS_DB_TXN(ovsdb_txn, "%% BFD Session creation failed\n");
	}

	ovsrec_bfd_session_set_bfd_dst_ip(ovs_bfd_session, remote_str);
	ovsrec_bfd_session_set_bfd_src_ip(ovs_bfd_session, local_str);
	ovsrec_bfd_session_set_owner(ovs_bfd_session, owner);
	ovsrec_bfd_session_set_bgp_asn(ovs_bfd_session, &asn, 1);

	/* End of transaction. */
	END_DB_TXN(ovsdb_txn);

	return CMD_SUCCESS;
}

static int
cli_no_bfd_session_cmd_execute(const char *remote_str, const char *local_str)
{
	const struct ovsrec_bfd_session *ovs_bfd_session;
	struct ovsdb_idl_txn *ovsdb_txn;
        char error_message[128];

	if (!string_is_an_ip_address(remote_str)) {
		vty_out(vty, "\nInvalid remote address %s\n", remote_str);
		return CMD_WARNING;
	}

	if (!string_is_an_ip_address(local_str)) {
		vty_out(vty, "\nInvalid local address %s\n", local_str);
		return CMD_WARNING;
	}

	/* Start of transaction. */
	START_DB_TXN(ovsdb_txn);

	ovs_bfd_session = find_matching_bfd_session_object(idl, remote_str);
	if (!ovs_bfd_session) {
		vty_out(vty, "\nBFD Session for remote %s not found\n", remote_str);
		sprintf(error_message, "%% BFD Session for remote %s not found\n", remote_str);
		ABORT_DB_TXN(ovsdb_txn, error_message);
	}

	ovsrec_bfd_session_delete(ovs_bfd_session);

	/* End of transaction. */
	END_DB_TXN(ovsdb_txn);

	return CMD_SUCCESS;
}

static int
show_bfd_cmd_execute(struct vty *vty)
{
	const struct ovsrec_bfd *ovs_bfd;

	OVSREC_BFD_FOR_EACH(ovs_bfd, idl) {

		if(ovs_bfd->n_enable) {
			vty_out(vty, "Global BFD Status : %s\n",
					safe_print_bool(1, ovs_bfd->enable));
		} else {
			vty_out(vty, "Global BFD Status : Disable\n");
		}

		if(ovs_bfd->n_min_tx) {
			vty_out(vty, "  Interval : %s\n",
					safe_print_integer(ovs_bfd->n_min_tx, ovs_bfd->min_tx));
		}

		if(ovs_bfd->n_min_rx) {
			vty_out(vty, "  Min Rx : %s\n",
					safe_print_integer(ovs_bfd->n_min_rx, ovs_bfd->min_rx));
		}

		if(ovs_bfd->n_decay_min_rx) {
			vty_out(vty, "  Multiplier : %s\n",
					safe_print_integer(ovs_bfd->n_decay_min_rx, ovs_bfd->decay_min_rx));
		}
	}

	return CMD_SUCCESS;
}

static int
show_bfd_session_cmd_execute(struct vty *vty)
{
	const struct ovsrec_bfd_session *ovs_bfd_session;

	OVSREC_BFD_SESSION_FOR_EACH(ovs_bfd_session, idl) {
		if(ovs_bfd_session->bfd_dst_ip) {
			vty_out(vty, "  Neighbor address : %s, Originator: %s (%s%s)\n",
					safe_print_string(1, ovs_bfd_session->bfd_dst_ip),
					((ovs_bfd_session->owner) ?
					 safe_print_string(1, ovs_bfd_session->owner) :
					 "Unknown"),
					((ovs_bfd_session->bgp_asn) ? "ASN: " : ""),
					((ovs_bfd_session->bgp_asn) ?
					 safe_print_integer(ovs_bfd_session->n_bgp_asn, ovs_bfd_session->bgp_asn) :
					 "None"));
		}

		if(ovs_bfd_session->bfd_src_ip) {
			vty_out(vty, "    Local address : %s\n",
					safe_print_string(1, ovs_bfd_session->bfd_src_ip));
		}

		if(ovs_bfd_session->remote_state) {
			vty_out(vty, "    Remote state : %s <%s>\n",
					safe_print_string(1, ovs_bfd_session->remote_state),
					safe_print_string(1, ovs_bfd_session->remote_diagnostic));
		}

		if(ovs_bfd_session->state) {
			vty_out(vty, "    Local state : %s <%s>\n",
					safe_print_string(1, ovs_bfd_session->state),
					safe_print_string(1, ovs_bfd_session->diagnostic));
		}

		if(ovs_bfd_session->effective_min_tx_interval) {
			vty_out(vty, "    Local Tx Interval : %s\n",
					safe_print_integer(1, &ovs_bfd_session->effective_min_tx_interval));
		}

		if(ovs_bfd_session->effective_min_rx_interval) {
			vty_out(vty, "    Local Rx Timeout : %s\n",
					safe_print_integer(1, &ovs_bfd_session->effective_min_rx_interval));
		}

		if(ovs_bfd_session->remote_multiplier) {
			vty_out(vty, "    Remote detect multiplier : %s\n",
					safe_print_integer(1, &ovs_bfd_session->remote_multiplier));
		}

		if(ovs_bfd_session->remote_min_tx_interval) {
			vty_out(vty, "    Remote desired minimum Tx Interval : %s\n",
					safe_print_integer(1, &ovs_bfd_session->remote_min_tx_interval));
		}

		if(ovs_bfd_session->remote_min_rx_interval) {
			vty_out(vty, "    Remote required minimum Rx Interval : %s\n",
					safe_print_integer(1, &ovs_bfd_session->remote_min_rx_interval));
		}


		// TBD more state info

		vty_out(vty,"\n");
	}

	return CMD_SUCCESS;
}

DEFUN(bfd_global_timers,
      bfd_global_timers_cmd,
      "bfd interval <15-300000> min_rx <15-300000> multiplier <0-255>",
      BFD_STR
      "Transmit interval\n"
      "Allowed range <15-300000> milliseconds (Default: 15)\n"
      "Minimum receive interval\n"
      "Allowed range <15-300000> milliseconds (Default: 15)\n"
      "Multiplier\n"
      "Allowed range <1-255> (Default: 3)\n")
{
	cli_bfd_global_timers_cmd_execute(atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
	return CMD_SUCCESS;
}


#ifdef HAVE_IPV6
DEFUN(bfd_session,
      bfd_session_cmd,
      "bfd session remote (A.B.C.D|X:X::X:X|WORD) local (A.B.C.D|X:X::X:X|WORD) owner WORD asn <1-4294967295>",
      BFD_STR
      "Session\n"
      "Remote\n"
      "Remote address\nRemote IPv6 address\nWord\n"
      "Local\n"
      "Local address\nLocal IPv6 address\nWord\n"
      "Owner\n"
      "Session owner - cli, bgp, other\n"
      "BGP AS-Number\n"
      "AS-Number\n")
#else
DEFUN(bfd_session,
      bfd_session_cmd,
      "bfd session remote A.B.C.D local A.B.C.D owner WORD asn <1-4294967295>",
      BFD_STR
      "Session\n"
      "Remote\n"
      "Remote address\n"
      "Local\n"
      "Local address\n"
      "Owner\n"
      "Session owner - cli, bgp, other\n"
      "BGP AS-Number\n"
      "AS-Number\n")
#endif
{
	char *owner = "bgp";
	uint64_t asn = 1;

	if (argc > 2)
		owner = (char *)argv[2];

	if (argc > 3)
		asn = atoi(argv[3]);

	cli_bfd_session_cmd_execute(argv[0], argv[1], owner, asn);
	return CMD_SUCCESS;
}

#ifdef HAVE_IPV6
DEFUN(no_bfd_session,
      no_bfd_session_cmd,
      "no bfd session remote (A.B.C.D|X:X::X:X|WORD) local (A.B.C.D|X:X::X:X|WORD)",
      NO_STR
      BFD_STR
      "Session\n"
      "Remote\n"
      "Remote address\nRemote IPv6 address\nWord\n"
      "Local\n"
      "Local address\nLocal IPv6 address\nWord\n")
#else
DEFUN(no_bfd_session,
      no_bfd_session_cmd,
      "no bfd session remote A.B.C.D local A.B.C.D",
      NO_STR
      BFD_STR
      "Session\n"
      "Remote\n"
      "Remote address\n"
      "Local\n"
      "Local address\n")
#endif
{
        cli_no_bfd_session_cmd_execute(argv[0], argv[1]);
        return CMD_SUCCESS;
}

DEFUN(vtysh_show_bfd,
      vtysh_show_bfd_cmd,
      "show bfd",
       SHOW_STR
       BFD_STR)
{
	show_bfd_cmd_execute(vty);
	show_bfd_session_cmd_execute(vty);
	return CMD_SUCCESS;
}


DEFUN(vtysh_show_bfd_session,
      vtysh_show_bfd_session_cmd,
      "show bfd neighbors {detailed | summary}",
      SHOW_STR
      BFD_STR
      NEIGHBOR_STR
      "detailed\n"
      "summary\n")
{
	show_bfd_session_cmd_execute(vty);
	return CMD_SUCCESS;
}


void
bfd_vty_init(void)
{
    /* Show bgp command */
    install_element(ENABLE_NODE, &vtysh_show_bfd_session_cmd);
    install_element(VIEW_NODE, &vtysh_show_bfd_session_cmd);

    install_element(ENABLE_NODE, &vtysh_show_bfd_cmd);
    install_element(VIEW_NODE, &vtysh_show_bfd_cmd);

    /* Global configuration commands */
    install_element(CONFIG_NODE, &bfd_global_timers_cmd);
    install_element(CONFIG_NODE, &bfd_session_cmd);
    install_element(CONFIG_NODE, &no_bfd_session_cmd);

}
