# Kernel Module for QMK

This system is an experiment on Linux - specifically the Raspberry Pi - to have the Pi do all gpio/matrix scanning, and sending keycodes directly to the OS. There are many exciting possibilities with a system like this, and this repo is only scratching the surface.

It's separated into a Loadable Kernel Module, and a Device Tree Overlay configuration (which is also loadable). Both can be installed and loaded at boot, or they can be loaded at runtime to aid in development.

These are the options exposed for doing everything all together:

    make KEYBOARD=planck load-all   # builds the kernel module and planck overlay, and loads both
    make clean-all                  # cleans up all the files

## Kernel Module

This is a QMK-inspired kernel module based on `matrix_keypad`, which includes instructions for layers, and can serve as a learning tool for how things work in QMK, via sysfs - writing to the platform device in the filesystem (more documentation will come once this is built-out more).

### Building

    make            # builds the kernel module
    make load       # builds and loads the kernel module
    make unload     # unloads the kernel module
    make install    # builds and installs the kernel module
    make remove     # removes the kernel module
    make clean      # cleans up the build files

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

    make KEYBOARD=planck            # builds the "planck" overlay
    make KEYBOARD=planck load       # builds and loads the overlay
    make KEYBOARD=planck unload     # unloads the overlay
    make KEYBOARD=planck install    # builds and installs the overlay
    make KEYBOARD=planck remove     # removes the overlay
    make KEYBOARD=planck clean      # cleans up the build files

