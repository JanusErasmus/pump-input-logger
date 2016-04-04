AUTOMAKE_OPTIONS = subdir-objects foreign
AM_CPPFLAGS = $(DEPS_CFLAGS) -DHOST_$(host_cpu)
bin_PROGRAMS = loggerd
loggerd_SOURCES = 	src/main.cpp 

loggerd_LDADD = $(DEPS_LIBS)

loggerd_CXXFLAGS = $(DEPS_CFLAGS) -Wall -Woverloaded-virtual -Wredundant-decls
loggerd_CFLAGS = $(DEPS_CFLAGS) -Wall

install-strip-exec:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install-exec

pkg : loggerd
	@echo host = $(host)
	@echo host_cpu= $(host_cpu)
	@echo host_vendor= $(host_vendor)
	@echo host_os= $(host_os)
	@echo buid_dir = $(abs_builddir)
	@DESTDIR=$(abs_builddir)/opkg $(MAKE) install-strip-exec
	@mkdir -p $(abs_builddir)/opkg/CONTROL
	@echo "Package: $(PACKAGE_NAME)" > $(abs_builddir)/opkg/CONTROL/control
	@echo "Version: $(PACKAGE_VERSION)" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Description: GLAM key Command Line interface" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Architecture: $(host_cpu)" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Maintainer: $(PACKAGE_BUGREPORT)" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Source: $(PACKAGE_URL)" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Section: extras" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Priority: extra" >> $(abs_builddir)/opkg/CONTROL/control
	@echo "Depends: $(OPKG_DEPS)" >> $(abs_builddir)/opkg/CONTROL/control
	for CVSDIR in `find $(abs_builddir)/opkg/ -name CVS`; do rm -rf $$CVSDIR; done
	@opkg-build -O -o root -g bin $(abs_builddir)/opkg
	
	

tool-install : 
	@DESTDIR=$(SYSROOT) $(MAKE) install-strip       


.PHONY: pkg
