#!/bin/bash

sudo apt install raspberrypi-kernel-headers git bc bison flex libssl-dev evtest input-utils
make KEYBOARD=clueboard && sudo make KEYBOARD=clueboard install && echo "dtoverlay=clueboard" | sudo tee -a /boot/config.txt
make && sudo make install && echo -e "libcomposite\ninput-polldev\nqmk" | sudo tee -a /etc/modules