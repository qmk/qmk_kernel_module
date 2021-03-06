# QMK Kernel Module

This system is an experiment on Linux - specifically the Raspberry Pi - to have the Pi do all gpio/matrix scanning, and sending keycodes directly to the OS. There are many exciting possibilities with a system like this, and this repo is only scratching the surface.

It's separated into a Loadable Kernel Module, libqmk (a proof-of-concept for a abstracted version of qmk_firmware), and a Device Tree Overlay configuration (which is also loadable). Both can be installed and loaded at boot, or they can be loaded at runtime to aid in development.

These are the options exposed for doing everything all together:

    make KEYBOARD=planck load-all   # builds the kernel module and planck overlay, and loads both
    make clean-all                  # cleans up all the files

## Kernel Module

This is a QMK-inspired kernel module based on `matrix_keypad`, which includes instructions for layers, and can serve as a learning tool for how things work in QMK, via sysfs - writing to the platform device in the filesystem (more documentation will come once this is built-out more).

### Building

You'll need the Raspberry Pi kernel headers installed, along with the basic tools for building, and some tools for testing:

    sudo apt install raspberrypi-kernel-headers git bc bison flex libssl-dev evtest input-utils

    make                 # builds the kernel module
    sudo make load       # builds and loads the kernel module
    sudo make unload     # unloads the kernel module
    sudo make install    # builds and installs the kernel module
    sudo make remove     # removes the kernel module
    make clean           # cleans up the build files

### Installing

Sometimes the depmod fails - I'm not entirely sure if that's normal, or how the configuration could be changed. The module is dependent on `libcomposite` and `input_polldev` - these modules may need to be added to your `etc/modules` in addition to `qmk`, depending on if you're installing it or not.

## Device Tree Overlays

For `planck`, the Planck PCB wired up to a Raspberry Pi like this:

``` 
COL 0: BCM 20
    1: BCM 21
    2: BCM 6
    3: BCM 24
    4: BCM 23
    5: BCM 22
ROW 0: BCM 12
    1: BCM 13
    2: BCM 16
    3: BCM 19
    4: BCM 25
    5: BCM 10
    6: BCM 9
    7: BCM 11
```

List of event codes can be found [here](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h).

The installation was based on [this guide](http://blog.gegg.us/2017/08/a-matrix-keypad-on-a-raspberry-pi-done-right/).

### Building

    make KEYBOARD=planck                 # builds the "planck" overlay
    sudo make KEYBOARD=planck load       # builds and loads the overlay
    sudo make KEYBOARD=planck unload     # unloads the overlay
    sudo make KEYBOARD=planck install    # builds and installs the overlay
    sudo make KEYBOARD=planck remove     # removes the overlay
    make KEYBOARD=planck clean           # cleans up the build files

## Other info

`sudo apt install xserver-xorg-input-libinput xserver-xorg-input-kbd` may be required to get things working in X (if you've installed the lite version of Raspbian).

### `/boot/config.txt` additions

    dtoverlay=dwc2
    dtoverlay=planck

### `/etc/modules` additions

    libcomposite
    input-polldev
    qmk

## QMK Helper

This passes keycodes onto a computer for the Pi Zero & Zero W and displays other useful info. libusbgx is required for the passthrough. It requires there packages to build:

    sudo apt install autoconf libtool libconfig-dev

You can build it like this:

    cd lib/libusbgx
    autoreconf -i
    ./configure --prefix=build
    make
    make install 

Once that is done, `make -C helper` from the root folder. `qmk_helper` is the console program, and takes these arguments:

    -o open gadget
    -c close gadget
    -t test
    -d daemon mode, open and pass through keycodes

`qmk_ghelper` is the gui version - `make -C helper qmk_ghelper` to build. Both need sudo privegdes to run.

### Git helper

You can run `git config --local include.path ../.gitconfig` in the main repo to have a `libqmk` alias for submodules. Run this from the `lib/libqmk` directory to be able to push easier with `git libqmk push`: `git config url."ssh://git@".insteadOf https://`