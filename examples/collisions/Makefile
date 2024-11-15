ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Required for library usage
include $(YAUL_INSTALL_ROOT)/share/build.tiled2saturn.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/data.bin;layer1 \
	assets/BALL.TEX;asset_ball_tex \
	assets/BALL.PAL;asset_ball_pal

SH_PROGRAM:= collisions
SH_SRCS:= \
	collisions.c

# We explicitly set the sh2 root header files to help clang generation
SH_CFLAGS+= -I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include
SH_CFLAGS+= -Os -I. $(TILED2SATURN_CFLAGS)
SH_LDFLAGS+= $(TILED2SATURN_LDFLAGS)

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Collissions
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk

assets/data.bin: resources/background.bmp resources/collisions.tmx
	$(ECHO)cd resources; tiled2saturn extract collisions.tmx
	$(ECHO)mv resources/data.bin assets/data.bin

assets/BALL.PAL: resources/BALL.PAL
	$(ECHO)cp resources/BALL.PAL assets/BALL.PAL

assets/BALL.TEX: resources/BALL.TEX
	$(ECHO)cp resources/BALL.TEX assets/BALL.TEX

.PHONY: .clean-assets
.clean-assets:
	$(ECHO)rm -f \
	    assets/*

clean: .clean-assets
