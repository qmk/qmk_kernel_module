# Kernel Module for QMK

## Kernel Module

### Building

## Device Tree Overlays

This build assumes you have raspberrypi/linux checkout'd in `../linux` (this is only needed for a header - this could be included here eventually), and a Planck PCB wired up to a raspberry pi like this:

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

The installation was based on [this guide](http://blog.gegg.us/2017/08/a-matrix-keypad-on-a-raspberry-pi-done-right/);

### Building

`make` will generate the `planck.dtbo`, and `make install` will copy this file to the `/boot/overlays/planck.dtbo` (which uses `sudo`). You'll also need to add this line to the end your `/boot/config.txt` (only once):

    dtoverlay=planck
