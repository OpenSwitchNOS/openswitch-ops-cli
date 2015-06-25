## Process this file with automake to produce Makefile.in.

SUBDIRS = lib @VTYSH@

DIST_SUBDIRS = lib \
	  vtysh

EXTRA_DIST = aclocal.m4 SERVICES TODO REPORTING-BUGS INSTALL.quagga.txt \
	update-autotools \
	vtysh/Makefile.in vtysh/Makefile.am

if HAVE_LATEX

HACKING.pdf: HACKING.tex
	$(LATEXMK) -pdf $<

clean-local:
	-$(LATEXMK) -C HACKING.tex

endif

ACLOCAL_AMFLAGS = -I m4
