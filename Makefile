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
qmk-y += lib/libqmk.a
qmk-y += src/qmk_main.o
qmk-y += src/qmk_keymap.o 
qmk-y += src/qmk_process.o 
qmk-y += src/qmk_sysfs.o 
qmk-y += src/qmk_scan.o
qmk-y += src/qmk_gadget.o

EXTRA_CFLAGS=-I$(PWD)/include -I$(PWD)/lib -L lib -lqmk

else

# QMK library building

$(PWD)/lib/libqmk.a:
	echo -n "* Making qmk library...................................."
	make -C $(PWD)/lib/qmk
	echo "OK"

libqmk-clean:
	echo -n "* Cleaing qmk library..................................."
	make -C $(PWD)/lib/qmk clean
	echo "OK"

# Kernel module building

KDIR ?= /lib/modules/`uname -r`/build
PWD = $(shell pwd)


qmk-default: $(PWD)/lib/libqmk.a
	echo -n "* Making qmk module....................................."
	make -C $(KDIR) M=$(PWD) modules
	echo "OK"
	echo "  ${TARGET}.ko"

qmk-clean: libqmk-clean
	echo -n "* Cleaning qmk module..................................."
	make -C $(KDIR) M=$(PWD) clean
	echo "OK"

qmk-install: qmk-default
	echo -n "* Installing qmk module................................."
	make -C $(KDIR) M=$(PWD) modules_install
	depmod
	echo "OK"

qmk-remove:
	echo "* This will delete /lib/modules/`uname -r`/extra/${TARGET}.ko"
	@read -r -p "   Are you sure? [y/N]: " CONTINUE; \
	[ "$$CONTINUE" = "y" ] || [ "$$CONTINUE" = "Y" ] || (exit 1;)
	sudo rm /lib/modules/`uname -r`/extra/${TARGET}.ko
	echo "OK"

qmk-load: qmk-default
	sudo rmmod ${TARGET} 2>/dev/null; true
	sudo modprobe input-polldev
	echo -n "* Loading qmk module...................................."
	sudo insmod ./${TARGET}.ko
	echo "OK"

qmk-unload:
	echo -n "* Unloading qmk module.................................."
	sudo rmmod ${TARGET}
	echo "OK"

# Keyboard building

keyboard-default:
	echo "* Making ${KEYBOARD} overlay"
	cpp -nostdinc -Iinclude/ -I/lib/modules/`uname -r`/build/include/ -undef -x assembler-with-cpp keyboards/${KEYBOARD}.dts > keyboards/${KEYBOARD}.tmp
	dtc -W no-unit_address_vs_reg -I dts -O dtb -o keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp
	echo "OK"
	echo "  keyboards/${KEYBOARD}.dtbo"

keyboard-install: keyboard-default
	echo "* Installing ${KEYBOARD} overlay"
	sudo cp keyboards/${KEYBOARD}.dtbo /boot/overlays/${KEYBOARD}.dtbo
	echo "OK, add \"dtoverlay=${KEYBOARD}\" to your /boot/config.txt"

keyboard-remove:
	echo "* Removing /boot/overlays/${KEYBOARD}.dtbo"
	sudo rm /boot/overlays/${KEYBOARD}.dtbo
	echo "OK"

keyboard-clean:
	echo "* Cleaning ${KEYBOARD} overlay"
	rm keyboards/${KEYBOARD}.dtbo keyboards/${KEYBOARD}.tmp 2>/dev/null; true
	echo "OK"

keyboard-clean-all:
	echo "* Cleaning all overlays"
	rm keyboards/*.dtbo keyboards/*.tmp 2>/dev/null; true
	echo "OK"

keyboard-load: keyboard-default
	sudo dtoverlay -r ${KEYBOARD} 2>/dev/null; true
	echo "* Loading ${KEYBOARD} overlay"
	sudo dtoverlay keyboards/${KEYBOARD}.dtbo
	echo "OK"

keyboard-unload:
	echo "* Unloading ${KEYBOARD} overlay"
	sudo dtoverlay -r ${KEYBOARD}
	echo "OK"

endif