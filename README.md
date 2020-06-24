## A Console for the Spartan Edge Accelerator

### Raison d'être

Espressif provides a barebones console application for ESP32 platforms which exposes some basic memory partitioning commands, wifi connectivity, FreeRTOS task commands, and system functions. To these I've added my own commands (for the sake of convenience) which are capable of:

- mounting the SD card, loading a bitstream, and programming the on-board Spartan-7 Xilinx FPGA.
- scanning for nearby WiFi APs and printing their info.

I'll be adding more commands as I need them.

### Requirements

- ESP IDF and its requirements
- SEA board by Seeedstudio
- A FAT-formatted SD card with your bitstream files put in the /overlay/ folder. Keep in mind that this means the bitstream files have to stick to 8:3 file naming convention.
- PuTTY (or other terminal application)
- NB: Originally compiled on windows

### Usage

You may have to configure your IDF to switch from the default 40MHz
oscillator to the SEA's 26MHz. See IDF docs for instructions.

Open a PuTTY serial session with the SEA board at 115200 baud.
If 115200 baud doesn't work, either reconfigure the IDF or try switching
to 115200*(26MHz/40MHz) = 74880 baud.

Run "help" for a list of commands and their usage.
The bitstream programming command is:

`load_bitstream [filename]`

### References

This code is loosely based on (but does not require):

- [The ESP IDF code samples](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

- [This SEA Arduino library](https://github.com/Pillar1989/spartan-edge-esp32-boot)
