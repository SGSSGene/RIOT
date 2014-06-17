#!/bin/sh

export BOARD=msb-430
make clean all && \
sudo mspdebug -j olimex "prog bin/msb-430/test_loader.hex" && \
sudo ../../dist/tools/pyterm/pyterm.py /dev/ttyUSB0 || sudo ../../dist/tools/pyterm/pyterm.py /dev/ttyUSB1
