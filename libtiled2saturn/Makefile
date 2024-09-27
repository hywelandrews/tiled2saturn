ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
	$(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

ifeq ($(strip $(YAUL_ARCH_SH_PREFIX)),)
  $(error Undefined YAUL_ARCH_SH_PREFIX (tool-chain prefix))
endif

SH_CC:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-gcc
SH_AR:=      $(YAUL_INSTALL_ROOT)/bin/$(YAUL_ARCH_SH_PREFIX)-gcc-ar

SH_CFLAGS_shared:= \
	-flto \
	-ffat-lto-objects \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-Wduplicated-branches \
	-Wduplicated-cond \
	-Wextra \
	-Winit-self \
	-Wmissing-include-dirs \
	-Wno-format \
	-Wnull-dereference \
	-Wshadow \
	-Wunused \
	-fdelete-null-pointer-checks \
	-ffast-math \
	-ffreestanding \
	-fmerge-all-constants \
	-fstrict-aliasing \
	-pedantic \
	-s

SH_CFLAGS:= \
	-O2 \
	-I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include \
	-std=c11 \
	-Wbad-function-cast \
	-Wfatal-errors \
	$(SH_CFLAGS_shared)

install-debug: SH_CFLAGS += -DDEBUG
install-debug: install

install: install-lib
	mkdir -p $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/tiled2saturn
	cp tiled2saturn.h $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/tiled2saturn/

install-lib: libtiled2saturn.a
	mv build/$< $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/lib/
	cp build.tiled2saturn.mk $(YAUL_INSTALL_ROOT)/share/

libtiled2saturn.a: tiled2saturn.o
	$(SH_AR) rcs build/$@ build/$< 

build: 
	mkdir -p $@

tiled2saturn.o: build
	$(SH_CC) -MT $</$@ -MF $</$*.d -MD $(SH_CFLAGS) \
		-c -o $</$@ $*.c

clean:
	rm -r build

uninstall:
	rm -r $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/tiled2saturn
	rm $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/lib/libtiled2saturn.a
	rm $(YAUL_INSTALL_ROOT)/share/build.tiled2saturn.mk

.PHONY: all clean .install

.SUFFIXES:
.SUFFIXES: .c .cc .C .cpp .cxx .sx .o 

.PRECIOUS: %.a %.c %.o

all: install
