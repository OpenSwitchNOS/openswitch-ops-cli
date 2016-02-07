## Process this file with Automake to create Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

LIBS = @LIBS@ @CURSES@ @LIBPAM@

AM_CFLAGS = $(PICFLAGS)
if ENABLE_OVSDB
AM_CFLAGS += -I $(LOCAL_SYSROOT_DIR)/usr/local/include/ovs
#AM_CFLAGS += -Wall -Werror
endif

AM_LDFLAGS = $(PILDFLAGS)

bin_PROGRAMS = vtysh

vtysh_SOURCES = vtysh_main.c vtysh.c vtysh_user.c vtysh_config.c
if ENABLE_OVSDB
vtysh_SOURCES += vtysh_ovsdb_if.c lldp_vty.c bgp_vty.c vrf_vty.c \
                 vtysh_ovsdb_config.c vtysh_ovsdb_config_context.c \
                 vlan_vty.c \
                 l3routes_vty.c system_vty.c \
                 vtysh_ovsdb_vlan_context.c \
                 sub_intf_vty.c loopback_vty.c \
                 vtysh_ovsdb_router_context.c aaa_vty.c \
                 logrotate_vty.c neighbor_vty.c traceroute_vty.c \
                 traceroute_handler.c \
                 source_interface_selection_vty.c \
                 vtysh_ovsdb_source_interface_context.c \
                 vtysh_ovsdb_intf_lag_context.c \
                 lacp_vty.c ecmp_vty.c dhcp_tftp_vty.c \
                 vtysh_ovsdb_dhcp_tftp_context.c ping_vty.c \
                 ping_handler.c sftp_vty.c \
                 ntp_vty.c ospf_vty.c \
                 vtysh_ovsdb_sftp_context.c \
                 utils/vlan_vtysh_utils.c \
                 utils/intf_vtysh_utils.c
endif

nodist_vtysh_SOURCES = vtysh_cmd.c
CLEANFILES = vtysh_cmd.c

vtysh_LDADD = ../lib/libzebra_cli.la @LIBCAP@ @LIBREADLINE@
if ENABLE_OVSDB
vtysh_LDADD += -lovscommon -lovsdb -lpthread
endif

vtyshincludedir = $(includedir)/vtysh
vtyshinclude_HEADERS = vtysh_utils.h vtysh.h \
        vtysh_ovsdb_if.h vtysh_user.h \
        vtysh_ovsdb_config.h

vtyshutilsincludedir = $(includedir)/vtysh/utils
vtyshutilsinclude_HEADERS = utils/system_vtysh_utils.h \
        utils/vlan_vtysh_utils.h utils/lacp_vtysh_utils.h \
        utils/intf_vtysh_utils.h

examplesdir = $(exampledir)
dist_examples_DATA = vtysh.conf.sample

EXTRA_DIST = extract.pl

if ENABLE_OVSDB
vtysh_cmd_FILES =
else
vtysh_cmd_FILES = $(top_srcdir)/bgpd/*.c $(top_srcdir)/isisd/*.c \
                  $(top_srcdir)/ospfd/*.c $(top_srcdir)/ospf6d/*.c \
                  $(top_srcdir)/ripd/*.c $(top_srcdir)/ripngd/*.c \
                  $(top_srcdir)/babeld/*.c \
                  $(top_srcdir)/pimd/pim_cmd.c \
                  $(top_srcdir)/lib/keychain.c $(top_srcdir)/lib/routemap.c \
                  $(top_srcdir)/lib/filter.c $(top_srcdir)/lib/plist.c \
                  $(top_srcdir)/lib/distribute.c $(top_srcdir)/lib/if_rmap.c \
                  $(top_srcdir)/lib/vty.c $(top_srcdir)/zebra/debug.c \
                  $(top_srcdir)/zebra/interface.c \
                  $(top_srcdir)/zebra/irdp_interface.c \
                  $(top_srcdir)/zebra/rtadv.c $(top_srcdir)/zebra/zebra_vty.c \
                  $(top_srcdir)/zebra/zserv.c $(top_srcdir)/zebra/router-id.c \
                  $(top_srcdir)/zebra/zebra_routemap.c \
                  $(top_srcdir)/zebra/zebra_fpm.c
endif

vtysh_cmd.c: $(vtysh_cmd_FILES)
	./$(EXTRA_DIST) $(vtysh_cmd_FILES) > vtysh_cmd.c

