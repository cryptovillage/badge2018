# badge2018

To buld the ESP32 firmware:

1. Install the ESP32 toolchain for your OS. You do not need to install the ESP-IDF -- that happens automatically in the next step.
  * For Linux: https://esp-idf.readthedocs.io/en/v3.1-beta1/get-started/linux-setup.html
  * For Windows: https://esp-idf.readthedocs.io/en/v3.1-beta1/get-started/windows-setup.html
  * For OSX: https://esp-idf.readthedocs.io/en/v3.1-beta1/get-started/macos-setup.html

2. Pull in the ESP-IDF, MicroPython, and other dependencies:
`git submodule update --init --recursive`

3. Build mpy_cross:
`cd firmware/esp32/components/micropython/micropython/mpy-cross/ && make && cd ../../../..`

4. Optionally configure the serial port that your badge appears on (under Serial Flasher Config):
`make menuconfig`

5. Build it:
`make -j5`

6. Flash it:
`make flash`

The EFM8 firmware has only been built inside Simplicity Studio.
