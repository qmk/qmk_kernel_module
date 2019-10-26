ifndef VERBOSE
.SILENT:
endif

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

ifneq ($(KERNELRELEASE),)

obj-m  := $(TARGET).o
qmk-y := src/qmk_main.o src/qmk_keymap.o src/qmk_process.o src/qmk_sysfs.o src/qmk_scan.o

else

KDIR ?= /lib/modules/`uname -r`/build
PWD = $(shell pwd)

qmk-default:
	echo "* Making ${TARGET} module"
	make -C $(KDIR) M=$(PWD) modules
	echo "  Complete: ${TARGET}.ko"

qmk-clean:
	echo "* Cleaning ${TARGET} module"
	make -C $(KDIR) M=$(PWD) clean
	echo "  Complete"

qmk-install: qmk-default
	echo "* Installing ${TARGET} module"
	make -C $(KDIR) M=$(PWD) modules_install
	echo "  Complete"

qmk-remove:
	echo "* This will delete /lib/modules/`uname -r`/extra/${TARGET}.ko"
	@read -r -p "   Are you sure? [y/N]: " CONTINUE; \
	[ "$$CONTINUE" = "y" ] || [ "$$CONTINUE" = "Y" ] || (exit 1;)
	sudo rm /lib/modules/`uname -r`/extra/${TARGET}.ko
	echo "  Complete"

qmk-load: qmk-default
	sudo rmmod ${TARGET} 2>/dev/null; true
	echo "* Loading ${TARGET} module"
	sudo insmod ./${TARGET}.ko
	echo "  Complete"

qmk-unload:
	echo "* Unloading ${TARGET} module"
	sudo rmmod ${TARGET}
	echo "  Complete"

# Keyboard building

keyboard-default:
	echo "* Making ${KEYBOARD} overlay"
	cpp -nostdinc -Isrc/ -I/lib/modules/`uname -r`/build/include/ -undef -x assembler-with-cpp keyboards/${KEYBOARD}.dts > keyboards/${KEYBOARD}.tmp
	dtc -W no-unit_address_vs_reg -I dts -O dtb -o keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp
	echo "  Complete: keyboards/${KEYBOARD}.dtbo"

keyboard-install: keyboard-default
	echo "* Installing ${KEYBOARD} overlay"
	sudo cp keyboards/${KEYBOARD}.dtbo /boot/overlays/${KEYBOARD}.dtbo
	echo "  Complete, add \"dtoverlay=${KEYBOARD}\" to your /boot/config.txt"

keyboard-remove:
	echo "* Removing /boot/overlays/${KEYBOARD}.dtbo"
	sudo rm /boot/overlays/${KEYBOARD}.dtbo
	echo "  Complete"

keyboard-clean:
	echo "* Cleaning ${KEYBOARD} overlay"
	rm keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp 2>/dev/null; true
	echo "  Complete"

keyboard-clean-all:
	echo "* Cleaning all overlays"
	rm keyboards/*.dtbo keyboards/*.tmp 2>/dev/null; true
	echo "  Complete"

keyboard-load: keyboard-default
	sudo dtoverlay -r planck 2>/dev/null; true
	echo "* Loading ${KEYBOARD} overlay"
	sudo dtoverlay keyboards/${KEYBOARD}.dtbo
	echo "  Complete"

keyboard-unload:
	echo "* Unloading ${KEYBOARD} overlay"
	sudo dtoverlay -r planck
	echo "  Complete"

endif