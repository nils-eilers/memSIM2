TARGET=memsim2
PREFIX=/usr/local

# Sorting include paths and defines simplifies checking them
INCPATHS=.

# TODO: expand SRC and vpath from INCPATHS
SRC=$(wildcard *.c) $(wildcard util/*.c) $(wildcard os/*.c)  $(wildcard handler/*.c) $(wildcard ../common/*.c) $(wildcard $(ARCH)/*.c)


# locations where make searches C source files
vpath %.c . ../common util os handler $(ARCH)

# Enable quiet compile with make Q=1
Q ?= 0
ifeq ($(Q),1)
  V1=@
  V2=@echo
else
  V1=
  V2=@\#
endif

# Cross compile for Windows with make WIN=y
# Please note doc/README-win32
ifeq ($(WIN),y)
  ARCH    = win32
  CROSS   = i686-w64-mingw32-
  CFLAGS  = -DCURL_STATICLIB -pedantic
  LDFLAGS = -static -Wl,-Bsymbolic-functions -lcurl -lssh2 -lidn -lgcrypt \
            -lgnutls -lcrypt32 -lwldap32 -lz -lnettle -lintl -liconv \
            -lhogweed -lgmp -lgnutls-openssl -lgpg-error -lws2_32
  TARGET  += .exe
else
  ARCH    = posix
  #output of `curl-config --libs`
  #LDFLAGS=-L/usr/lib/i386-linux-gnu -lcurl -Wl,-Bsymbolic-functions
  LDFLAGS =
endif


all: $(TARGET)

BINDIR=bin
# OBJDIR contains temporary object and dependency files
OBJDIR=obj/$(ARCH)/
DEPDIR=$(OBJDIR).dep/

CC=$(CROSS)gcc
LD=$(CROSS)gcc
PKG_CONFIG=$(CROSS)pkg-config

# Create automatic dependency files (*.d) on the fly during compilation
AUTODEP=-MMD -MP -MF $(DEPDIR)$(@F).d

# Sorting include paths and defines simplifies checking them
INCLUDE=$(sort $(addprefix -I,$(INCPATHS)))

CFLAGS+=-std=c99 -Wall -Wextra $(INCLUDE) -D_DEFAULT_SOURCE

# Create object names from .c and .S
_OBJ=$(notdir $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SRC))))
OBJ=$(addprefix $(OBJDIR),$(_OBJ))
# Create dependency names from .o
DEP=$(addprefix $(DEPDIR),$(_OBJ:.o=.o.d))

# locations where make searches C source files
vpath %.c . $(INCPATHS)

# Include automatic dependencies, continue if they don't exist yet
-include $(DEP)


# Compile C files
$(OBJDIR)%.o: %.c
	$(V2) CC $<
	$(V1) $(CC) $(CFLAGS) $(AUTODEP) -c $< -o $@

# Link object files
$(TARGET): $(OBJDIR) $(DEPDIR) $(OBJ)
	$(V2) LD $(notdir $@)
	$(V1) $(LD) $(OBJ) -o $@ $(LDFLAGS)

veryclean: clean
	rm -rf $(TARGET) obj

# Clean directories
clean: objclean depclean

objclean:
	$(V2) Cleaning object files
	$(V1) rm -f $(OBJ)
	@# The object directory gets removed by make depclean

depclean:
	$(V2) Cleaning dependencies
	$(V1) rm -f $(DEP)
	$(V1) if [ -d $(DEPDIR) ]; then rmdir --ignore-fail-on-non-empty $(DEPDIR); fi


# Create build directories
$(OBJDIR):
	$(V1) mkdir -p $@
$(DEPDIR):
	$(V1) mkdir -p $@

install:
	@if [ `id -u` != "0" ] ; then echo "must be root!"; exit 1; fi;
	test -d $(PREFIX)/$(BINDIR) || mkdir -p $(PREFIX)/$(BINDIR)
	install -m 0755 $(TARGET) $(PREFIX)/$(BINDIR)

uninstall:
	@if [ `id -u` != "0" ] ; then echo "must be root!"; exit 1; fi;
	rm -f $(PREFIX)/$(BINDIR)/$(TARGET)


.PHONY: all install uninstall
