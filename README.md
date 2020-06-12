# SWDscan

This Arduino-based Platformio project allows quick identification of SWD ports.

## How does it work?

The app iterates through microcontroller gpio pins following the definition specified using PIN_MASK and PIN_MAX to find SWD clock and io lines. Pins are enabled by setting corresponding bits of PIN_MASK, i.e. pins 2-8 are enabled for PIN_MASK of 0b111111100 or 0x1fc. PIN_MAX limits the tested pin set, for pins 2-8 set a value of 9 or greater.

## How to use it?

- hook up your Arduino to the target 
- use a logic level shifer between Arduino and target if required
- adjust PIN_MASK and PIN_MAX for your setup
- select proper build target
- build the project and upload it to your board
- establish serial connection with baud rate of 115200
- ammend debug options if required
- use option _h_ for help
- use option _e_ to enumerate pins

## How does it look like?

- swd enumeration with debug level 1 on a BluePill
```
> d
Choose debug level 0-2 1
> e
CLK:  2 | IO:  3 | ACK: 7 | PART:  ffff | MAN:  7ff
CLK:  2 | IO:  4 | ACK: 7 | PART:  ffff | MAN:  7ff
CLK:  3 | IO:  2 | ACK: 1 | PART:  ba01 | MAN:  23b
CLK:  3 | IO:  4 | ACK: 7 | PART:  ffff | MAN:  7ff
CLK:  4 | IO:  2 | ACK: 7 | PART:  ffff | MAN:  7ff
CLK:  4 | IO:  3 | ACK: 7 | PART:  ffff | MAN:  7ff
```

- test response on lines 3 and 2 with debug level 1 (valid connection, read part no is 0xba01)
```
> t
Enter SWD CLK PIN NO 3
Enter SWD IO PIN NO 2
CLK:  3 | IO:  2 | ACK: 1 | PART:  ba01 | MAN:  23b
```

- test response on lines 2 and 3 with debug level 1 (invalid connection, read part no is 0xffff)
```
> t
Enter SWD CLK PIN NO 2
Enter SWD IO PIN NO 3
CLK:  2 | IO:  3 | ACK: 7 | PART:  ffff | MAN:  7ff
```

## No Platformio?

No problem, just copy the contents of src/main.cpp to a new Arduino project, remove the Arduino.h import from the first line and you're ready to go.
