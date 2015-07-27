## Process this file with Automake to create Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

LIBS = @LIBS@ @CURSES@ @LIBPAM@

AM_CFLAGS = $(PICFLAGS)
if ENABLE_OVSDB
AM_CFLAGS += -I $(LOCAL_SYSROOT_DIR)/usr/local/include/ovs
endif

AM_LDFLAGS = $(PILDFLAGS)

bin_PROGRAMS = vtysh

vtysh_SOURCES = vtysh_main.c vtysh.c vtysh_user.c vtysh_config.c
if ENABLE_OVSDB
vtysh_SOURCES += vtysh_ovsdb_if.c lldp_vty.c bgp_vty.c vrf_vty.c vtysh_ovsdb_config.c \
                 vtysh_ovsdb_ovstable.c vtysh_ovsdb_intftable.c intf_vty.c \
                 l3static_vty.c vtysh_ovsdb_routetable.c \
                 led_vty.c system_vty.c vtysh_ovsdb_ledtable.c\
                 fan_vty.c temperature_vty.c vtysh_ovsdb_subsystemtable.c
endif

nodist_vtysh_SOURCES = vtysh_cmd.c
CLEANFILES = vtysh_cmd.c
noinst_HEADERS = vtysh.h vtysh_user.h
if ENABLE_OVSDB
noinst_HEADERS += vtysh_ovsdb_if.h lldp_vty.h bgp_vty.h vrf_vty.h vtysh_ovsdb_config.h \
                  vtysh_ovsdb_ovstable.h vtysh_ovsdb_intftable.h intf_vty.h \
                  l3static_vty.h vtysh_ovsdb_routetable.h \
                  led_vty.h system_vty.h vtysh_ovsdb_ledtable.h \
                  fan_vty.h temperature_vty.h vtysh_ovsdb_subsystemtable.h
endif

vtysh_LDADD = ../lib/libzebra.la @LIBCAP@ @LIBREADLINE@
if ENABLE_OVSDB
vtysh_LDADD += -lovscommon -lovsdb -lpthread
endif

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

