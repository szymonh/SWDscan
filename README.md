# SWDscan

This Arduino-based Platformio project allows quick identification of SWD ports.

## How does it work?

The app iterates through microcontroller gpio pins following the definition specified using PIN_MASK to find SWD clock and io lines. Pins are enabled by setting corresponding bits of PIN_MASK, i.e. pins 2-8 are enabled for PIN_MASK of 0b111111100 or 0x1fc.

Besides PIN_MASK predefined at build time you can also set the pin mask at runtime using the
_m_ command. Entering 508 or 0x1fc will result in pin_mask set to 0b111111100 so you can update the configuration
without the need to rebuild the project.

## How to use it?

- hook up your Arduino to the target
- use a logic level shifer between Arduino and target if required
- optionally adjust PIN_MASK for your setup, can be done later
- select proper build target
- build the project and upload it to your board
- establish serial connection with baud rate of 115200
- ammend debug options if required
- use option _m_ to set a pin mask at runtime
- use option _h_ for help
- use option _e_ to enumerate pins

## How to configure it?

- _m_ - pin mask set at runtime - select which MCU GPIO pins to check for SWD IO and CLK lines,
value can be provided as either a decimal or hexadecimal integer up to 64 pins,
pin X is enabled by setting a bit at position X to 1 in the pin mask, pin 0 is of least significance,
for example 0x3 enables pins 0 and 1, 0xff enables pins 0 to 7 and 0xfc enables pins 2 to 7

- _d_ - debug level - defaults to normal
  - normal - prints information for each tested combination
  - quiet - prints information only for positive hits
  - verbose - prints bit patterns set and received on the IO line

- _b_ - break on hit - defaults to true
  - true - search is stopped when positive hit is found
  - false - search is continued despite positive hit is found

## What is test mode for?

Test mode allows to check if target SWD IO and CLK lines are connected to specified pins.

## How does it look like?

- help menu
```
> h
+-------------------------------------------+
|                   SWDScan                 |
+-------------------------------------------+
 e - enumerate swd lines
 m - set pin mask, current: 0x1C
 t - test pin pair for swd
 b - break on hit, current: 1
 d - set debug level, current: 1
 h - this help
+-------------------------------------------+
```

- swd enumeration with debug level 1 on a BluePill
```
> d
Choose debug level 0-2 1
Debug set to 1
> e
+-------------------------------------------+
| CLK PIN | IO PIN | ACK | PART NO | MAN ID |
+-------------------------------------------+
|    2    |    3   |  7  |   ffff  |   7ff  |
|    2    |    4   |  7  |   ffff  |   7ff  |
|    3    |    2   |  1  |   ba01  |   23b  |
|    3    |    4   |  7  |   ffff  |   7ff  |
|    4    |    2   |  7  |   ffff  |   7ff  |
|    4    |    3   |  7  |   ffff  |   7ff  |
+----------------- SUCCESS -----------------+
```

- swd enumaration with pin mask set at runtime
```
> m
Enter pin mask 0xfc
Pin mask set to 11111100
> b
Break work on hit? 1/0 1
> d
Choose debug level 0-2 1
Debug set to 1
> e
+-------------------------------------------+
| CLK PIN | IO PIN | ACK | PART NO | MAN ID |
+-------------------------------------------+
|    2    |    3   |  7  |   ffff  |   7ff  |
|    2    |    4   |  7  |   ffff  |   7ff  |
|    2    |    5   |  7  |   ffff  |   7ff  |
|    2    |    6   |  7  |   ffff  |   7ff  |
|    2    |    7   |  7  |   ffff  |   7ff  |
|    3    |    2   |  1  |   ba01  |   23b  |
+----------------- SUCCESS -----------------+
```

- test response on lines 3 and 2 with debug level 1 (valid connection, read part no is 0xba01)
```
> t
Enter SWD CLK PIN NO 3
Enter SWD IO PIN NO 2
+-------------------------------------------+
| CLK PIN | IO PIN | ACK | PART NO | MAN ID |
+-------------------------------------------+
|    3    |    2   |  1  |   ba01  |   23b  |
+----------------- SUCCESS -----------------+
```

- test response on lines 2 and 3 with debug level 1 (invalid connection, read part no is 0xffff)
```
> t
Enter SWD CLK PIN NO 2
Enter SWD IO PIN NO 3
+-------------------------------------------+
| CLK PIN | IO PIN | ACK | PART NO | MAN ID |
+-------------------------------------------+
|    2    |    3   |  7  |   ffff  |   7ff  |
+----------------- FAILURE -----------------+
```

## Nothing found?

In case the target fails to switch to SWD mode:
- put the target into reset state
- put the target into bootloader mode (bootN pins)
- ensure common ground is present between scanner board and target

## No Platformio?

No problem, just copy the contents of src/main.cpp to a new Arduino project, remove the Arduino.h import from the first line and you're ready to go.

## Need JTAG?

Check [JTAGscan](https://github.com/szymonh/JTAGscan) to identify JTAG TMS, TCK, TDO and TDI pins.
