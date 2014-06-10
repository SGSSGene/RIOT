#!/bin/sh

make && \
sudo mspdebug -j olimex "prog bin/msb-430/hello-world.hex" && \
sudo /home/dima/RIOT/dist/tools/pyterm/pyterm.py /dev/ttyUSB1
