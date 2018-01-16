#!/bin/sh

git submodule update --init --recursive
cd firmware/esp32/components/micropython/micropython/mpy-cross
make
cd ../../../..
make


