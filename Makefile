

ifeq ($(KEYBOARD),)

default: qmk-default
clean: qmk-clean
install: qmk-install
remove: qmk-remove
load: qmk-load
unload: qmk-unload

else 

default: keyboard-default
clean: keyboard-clean
install: keyboard-install
remove: keyboard-remove
load: keyboard-load
unload: keyboard-unload

endif

clean-all: qmk-clean keyboard-clean-all
load-all: qmk-load keyboard-load

# QMK kernel module building

TARGET := qmk

KDIR := /lib/modules/$(shell uname -r)/build

ifneq ($(KERNELRELEASE),)

obj-m  := $(TARGET).o
qmk-src := $(patsubst $(PWD)/%,%,$(shell find $(PWD)/module/ -type f -name '*.c'))
qmk-objs := $(patsubst $(PWD)/%.c,%.o,$(shell find $(PWD)/module/ -type f -name '*.c')) lib/libqmk.a

EXTRA_CFLAGS = -I$(PWD)/include -I$(PWD)/lib/libqmk/include -I/usr/include -I/usr/include/arm-linux-gnueabihf
EXTRA_CFLAGS += -L lib -L /usr/lib -L /usr/lib/arm-linux-gnueabihf -l:libqmk.a -lusbgx

else

PWD = $(shell pwd)

include $(KDIR)/tools/scripts/Makefile.include

# QMK library building

$(PWD)/lib/libqmk.a:
	$(call descend,lib/libqmk)

libqmk-clean:
	$(call descend,lib/libqmk,clean)

# Kernel module building

qmk-default: $(PWD)/lib/libqmk.a
	$(call descend,$(KDIR),M=$(PWD) modules)

qmk-clean: libqmk-clean
	$(call descend,$(KDIR),M=$(PWD) clean)

qmk-install: qmk-default
	$(call descend,$(KDIR),M=$(PWD) modules_install)

qmk-remove:
	@echo "* This will delete /lib/modules/`uname -r`/extra/${TARGET}.ko"
	@read -r -p "   Are you sure? [y/N]: " CONTINUE; \
	[ "$$CONTINUE" = "y" ] || [ "$$CONTINUE" = "Y" ] || (exit 1;)
	sudo rm /lib/modules/`uname -r`/extra/${TARGET}.ko

qmk-load: qmk-default
	@sudo rmmod $(TARGET) 2>/dev/null; true
	@sudo modprobe input-polldev
	@echo "  LOAD $(TARGET).ko"
	@sudo insmod ./$(TARGET).ko

qmk-unload:
	echo "  UNLOAD $(TARGET).ko"
	sudo rmmod $(TARGET)

# Keyboard building

keyboard-default:
	$(QUIET_GEN)cpp -nostdinc -Iinclude/ -I$(PWD)/lib/libqmk/include -L lib -lqmk -I/lib/modules/`uname -r`/build/include/ -undef -x assembler-with-cpp keyboards/${KEYBOARD}.dts > keyboards/${KEYBOARD}.tmp
	$(QUIET_GEN)dtc -W no-unit_address_vs_reg -I dts -O dtb -o keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp

keyboard-install: keyboard-default
	$(QUIET_INSTALL)sudo cp keyboards/${KEYBOARD}.dtbo /boot/overlays/${KEYBOARD}.dtbo
	echo "  add \"dtoverlay=${KEYBOARD}\" to your /boot/config.txt"

keyboard-remove:
	echo "* Removing /boot/overlays/${KEYBOARD}.dtbo"
	sudo rm /boot/overlays/${KEYBOARD}.dtbo

keyboard-clean:
	echo "* Cleaning ${KEYBOARD} overlay"
	rm keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp 2>/dev/null; true

keyboard-clean-all:
	echo "* Cleaning all overlays"
	rm keyboards/*.dtbo keyboards/*.tmp 2>/dev/null; true

keyboard-load: keyboard-default
	sudo dtoverlay -r ${KEYBOARD} 2>/dev/null; true
	echo "* Loading ${KEYBOARD} overlay"
	sudo dtoverlay keyboards/${KEYBOARD}.dtbo

keyboard-unload:
	echo "* Unloading ${KEYBOARD} overlay"
	sudo dtoverlay -r ${KEYBOARD}

endif