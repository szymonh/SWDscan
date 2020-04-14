# SWDenum

This Arduino-based Platformio project allows quick identification of SWD ports.

## How does it work?

The app iterates through microcontroller gpio pins following the definition specified using PIN_MASK and PIN_MAX to find SWD clock and io lines. Pins are enabled by setting corresponding bits of PIN_MASK, i.e. pins 2-8 are enabled for PIN_MASK of 0b111111100 or 0x1fc. PIN_MAX limits the tested pin set, for pins 2-8 set a value of 9 or greater.

## How to use it?

- adjust PIN_MASK and PIN_MAX for your setup
- select proper build target
- build the project and upload it to your board
- establish serial connection with baud rate of 115200
- ammend debug options if required
- use option _h_ for help
- use option _e_ to enumerate pins

## No Platformio?

No problem, just copy the contents of src/main.cpp to a new Arduino project, remove the Arduino.h import from the first line and you're ready to go.
