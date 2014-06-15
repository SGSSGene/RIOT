#!/bin/sh

export BOARD=msb-430
make -C ../dyn_app
make clean all && \
sudo mspdebug -j olimex "prog bin/msb-430/hello-world.hex" && \
sudo /home/dima/RIOT/dist/tools/pyterm/pyterm.py /dev/ttyUSB0 || sudo /home/dima/RIOT/dist/tools/pyterm/pyterm.py /dev/ttyUSB1
