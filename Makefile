# libsstvenc: Asynchronous Analogue SSTV encoder library.
# © 2024 Stuart Longland VK4MSL
# SPDX-License-Identifier: MIT

#############################################################################
# Compiler options.  
#############################################################################

# Target compiler components and options.  This can be set to the
# prefix of a compiler toolchain for cross-compilation purposes.
# e.g. armv7l-unknown-linux-gnu- (if it's in `${PATH}`) or
# /opt/gnu-arm-toolchain/bin/armv7l-unknown-linux-gnu- (if it's not).
CROSS_COMPILE ?=

# C compiler
CC_NAME ?= gcc
CC ?= $(CROSS_COMPILE)$(CC_NAME)

# C pre-processor flags
CPPFLAGS ?=

# C compiler flags
CFLAGS ?= -O3

# Linker flags
LDFLAGS ?=

# Compile-time libraries
LIBS ?= -lm -lc

#############################################################################
# Other build tool paths
#############################################################################

CLANG_FORMAT ?= clang-format
DOXYGEN ?= doxygen
INSTALL ?= install
LDCONFIG ?= ldconfig
SED ?= sed

#############################################################################
# Installation paths.
#############################################################################

# Where we will put the files on compilation?  We specify both
# a PREFIX where the files will live _on the actual system_ and
# a DESTDIR that sets where we will put the files as packages are built.
PREFIX ?= /usr/local
DESTDIR ?=

# Where do we put executable binaries we compile?
BINDIR_REL ?= bin
BINDIR ?= $(PREFIX)/$(BINDIR_REL)

# Where do we put shared libraries we compile?
LIBDIR_REL ?= lib
LIBDIR ?= $(PREFIX)/$(LIBDIR_REL)

# Where do we put headers?
INCDIR_REL ?= include/libsstvenc
INCDIR ?= $(PREFIX)/$(INCDIR_REL)

# Where do we put documentation?
DOCDIR_REL ?= doc/libsstvenc
DOCDIR ?= $(PREFIX)/$(DOCDIR_REL)

# Where do we put examples?
EXAMPLEDIR_REL ?= $(DOCDIR_REL)/examples
EXAMPLEDIR ?= $(PREFIX)/$(EXAMPLEDIR_REL)

# Where do we put the pkgconfig file?
PKGCONFIG_DIR_REL ?= share/pkgconfig
PKGCONFIG_DIR ?= $(PREFIX)/$(PKGCONFIG_DIR_REL)

#############################################################################
# Compile-time features.
#############################################################################

# Build documentation?  y/n
BUILD_DOCS ?= y

# Build example programs?  y/n
BUILD_EXAMPLES ?= y

# Build standard programs?  y/n
BUILD_PROGS ?= y

# Build the shared library?  y/n  If no, then it'll need to be installed
# already in order to build EXAMPLES and PROGS.
BUILD_LIBS ?= y

#############################################################################
# Build and source paths.
#############################################################################

TOP_DIR ?= $(realpath .)
BUILD_DIR ?= $(TOP_DIR)/build
PROGS_DIR ?= $(TOP_DIR)/progs
EXAMPLE_SRC_DIR ?= $(TOP_DIR)/examples
HEADERS_DIR ?= $(TOP_DIR)/include
SRC_DIR ?= $(TOP_DIR)/src

#############################################################################
# Core build targets.
#############################################################################

.PHONY: all clean docs install pretty

COMPONENTS =

ifeq ($(BUILD_DOCS),y)
COMPONENTS += docs
endif

ifeq ($(BUILD_EXAMPLES),y)
COMPONENTS += examples
endif

ifeq ($(BUILD_LIBS),y)
COMPONENTS += libs
endif

ifeq ($(BUILD_PROGS),y)
COMPONENTS += progs
endif

all: $(patsubst %,build_%,$(COMPONENTS))

docs: build_docs

install: $(patsubst %,install_%,$(COMPONENTS))

clean:
	-rm -fr $(BUILD_DIR)

pretty:
	$(CLANG_FORMAT) -i $$( \
		find $(SRC_DIR) $(HEADERS_DIR) $(EXAMPLE_SRC_DIR) \
			$(PROGS_DIR) -type f -name \*.[ch] )

#############################################################################
# Build directory structure
#############################################################################

$(BUILD_DIR)/.mkdir:
	mkdir $(BUILD_DIR)
ifeq ($(BUILD_DOCS),y)
	mkdir $(BUILD_DIR)/docs
endif
ifeq ($(BUILD_EXAMPLES),y)
	mkdir $(BUILD_DIR)/examples
endif
ifeq ($(BUILD_LIBS),y)
	mkdir $(BUILD_DIR)/libs
endif
ifeq ($(BUILD_PROGS),y)
	mkdir $(BUILD_DIR)/progs
endif
	touch $@

#############################################################################
# libsstvenc shared library
#############################################################################

LIB_NAME := sstvenc
LIB_SO_MAJ := 0
LIB_SO_MIN := 0
LIB_SO_REL := 0
LIB_SONAME_BASE := lib$(LIB_NAME).so
LIB_SONAME_MAJ := $(LIB_SONAME_BASE).$(LIB_SO_MAJ)
LIB_SONAME_VER := $(LIB_SONAME_MAJ).$(LIB_SO_MIN).$(LIB_SO_REL)

LIB_HEADERS := $(wildcard $(HEADERS_DIR)/libsstvenc/*.h)
LIB_SOURCES := $(wildcard $(SRC_DIR)/*.c)
LIB_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/libs/%.o,$(LIB_SOURCES))
LIB_DEPENDS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/libs/%.d,$(LIB_SOURCES))

.PHONY: build_libs install_libs

build_libs: $(BUILD_DIR)/libs/$(LIB_SONAME_BASE)

install_libs: build_libs
	# Libraries
	$(INSTALL) -g root -o root -m 0755 -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -g root -o root -m 0755 -t $(DESTDIR)$(LIBDIR) \
		$(BUILD_DIR)/libs/$(LIB_SONAME_BASE)
	$(LDCONFIG) -n $(DESTDIR)$(LIBDIR)
	# Headers
	$(INSTALL) -g root -o root -m 0755 -d $(DESTDIR)$(INCDIR)
	$(INSTALL) -g root -o root -m 0644 -t $(DESTDIR)$(INCDIR) \
		$(LIB_HEADERS)

$(BUILD_DIR)/libs/$(LIB_SONAME_BASE): $(LIB_OBJECTS)
	${CC} $(LDFLAGS) -shared -Wl,-soname,$(LIB_SONAME_MAJ) \
		-o $@ $^ $(LIBS)

$(BUILD_DIR)/libs/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)/.mkdir
	${CC} -I$(HEADERS_DIR) $(CPPFLAGS) $(CCFLAGS) \
		-MM -MT $(patsubst %.d,%.o,$@) -MF $@ -c $<

$(BUILD_DIR)/libs/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/libs/%.d
	${CC} -I$(HEADERS_DIR) $(CPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<

-include $(LIB_DEPENDS)

#############################################################################
# libsstvenc examples
#############################################################################

EXAMPLE_SOURCES := $(wildcard $(EXAMPLE_SRC_DIR)/*.c)

.PHONY: build_examples install_examples
build_examples:

install_examples:
	$(INSTALL) -d -g root -o root -m 0755 $(DESTDIR)$(EXAMPLEDIR)
	$(INSTALL) -g root -o root -m 0644 -t $(DESTDIR)$(EXAMPLEDIR) \
		$(EXAMPLE_SOURCES)

#############################################################################
# libsstvenc programs
#############################################################################

PROGS_SOURCES := $(wildcard $(PROGS_DIR)/*.c)
PROGS_OBJECTS := $(patsubst $(PROGS_DIR)/%.c,\
			$(BUILD_DIR)/progs/%.o,$(PROGS_SOURCES))
PROGS_DEPENDS := $(patsubst $(PROGS_DIR)/%.c,\
		 	$(BUILD_DIR)/progs/%.d,$(PROGS_SOURCES))

# png-to-sstv needs libgd
LIBGD_CFLAGS := $(shell pkg-config gdlib --cflags)
LIBGD_LIBS := $(shell pkg-config gdlib --libs)

.PHONY: build_progs install_progs
build_progs: $(BUILD_DIR)/progs/png-to-sstv

install_progs: build_progs
	$(INSTALL) -d -g root -o root -m 0755 $(DESTDIR)$(BINDIR)
	$(INSTALL) -g root -o root -m 0644 -t $(DESTDIR)$(BINDIR) \
		$(BUILD_DIR)/progs/png-to-sstv

$(BUILD_DIR)/progs/png-to-sstv: $(BUILD_DIR)/progs/png-to-sstv.o \
		| $(BUILD_DIR)/libs/$(LIB_SONAME_BASE)
	$(CC) -L$(BUILD_DIR)/libs -o $@ $^ -lsstvenc $(LIBGD_LIBS) -lm

$(BUILD_DIR)/progs/%.d: $(PROGS_DIR)/%.c | $(BUILD_DIR)/.mkdir
	${CC} -I$(HEADERS_DIR) $(CPPFLAGS) $(CFLAGS) $(LIBGD_CFLAGS) \
		-MM -MT $(patsubst %.d,%.o,$@) -MF $@ -c $<

$(BUILD_DIR)/progs/%.o: $(PROGS_DIR)/%.c | $(BUILD_DIR)/progs/%.d
	${CC} -I$(HEADERS_DIR) $(CPPFLAGS) $(CFLAGS) $(LIBGD_CFLAGS) \
		-o $@ -c $<

#############################################################################
# libsstvenc documentation
#############################################################################

.PHONY: build_docs install_docs
build_docs: $(BUILD_DIR)/docs/html/index.html

$(BUILD_DIR)/docs/html/index.html: $(BUILD_DIR)/docs/Doxyfile \
		$(LIB_HEADERS) $(LIB_SOURCES) $(EXAMPLE_SOURCES) \
		$(PROGS_SOURCES) \
		$(shell find $(TOP_DIR) -type f -name \*.md)
	cd $(TOP_DIR) ; $(DOXYGEN) $<

$(BUILD_DIR)/docs/Doxyfile: $(TOP_DIR)/Doxyfile | $(BUILD_DIR)/.mkdir
	$(SED) -e '/^OUTPUT_DIRECTORY[\t ]\+=/ s|=.*$$|= $(BUILD_DIR)/docs|' \
		$< > $@

install_docs: build_docs
	$(INSTALL) -d -g root -o root -m 0755 $(DESTDIR)$(DOCDIR)
	$(INSTALL) -g root -o root -m 0644 -t $(DESTDIR)$(DOCDIR) \
		$(TOP_DIR)/README.md \
		$(TOP_DIR)/COPYING
	# BSD install does not support recursively copying directories
	# so we just have to trust permissions generated by doxygen here.
	cp -a $(BUILD_DIR)/docs/html $(DESTDIR)$(DOCDIR)
	chown -R root:root $(DESTDIR)$(DOCDIR)/html
