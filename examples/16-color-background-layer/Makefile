ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Required for library usage
include $(YAUL_INSTALL_ROOT)/share/build.tiled2saturn.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/data.bin;layer1

SH_PROGRAM:= 16-color-background-layer
SH_SRCS:= \
	16-color-background-layer.c

# We explicitly set the sh2 root header files to help clang generation
SH_CFLAGS+= -I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include
SH_CFLAGS+= -Os -I. $(TILED2SATURN_CFLAGS)
SH_LDFLAGS+= $(TILED2SATURN_LDFLAGS)

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= 16 color background layer
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk

assets/data.bin: resources/landscape_512x512_16.bmp resources/landscape.tmx
	$(ECHO)cd resources; tiled2saturn extract landscape.tmx
	$(ECHO)mv resources/data.bin assets/data.bin

.PHONY: .clean-assets
.clean-assets:
	$(ECHO)rm -f \
	    assets/data.bin

clean: .clean-assets
