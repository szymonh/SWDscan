#include <Arduino.h>

#define PIN_MASK 0b11100

#define RESET_SEQUENCE_LENGTH 64
#define CLOCK_HALF_CYCLE_US 250
#define JTAG_TO_SWD 0xE79E
#define DESIGNER_DEFAULT 0x23B
#define PARTNO_DEFAULT 0xBA01

#define ROW_FORMAT "CLK: %2d | IO: %2d | ACK: %ld | PART: %5lx | MAN: %4lx\r\n"
#define PROMPT "> "

#define getAck(value) (value & 0x7)
#define getManufacturer(value) ((value >> 4) & 0x7FF)
#define getPartNo(value) ((value >> 15) & 0xFFFF)

enum debug_level {
    quiet,
    normal,
    verbose
};

uint64_t pin_mask = PIN_MASK;
uint8_t pin_max = 0;

uint8_t swclk_pin = 2;
uint8_t swdio_pin = 3;

enum debug_level debug = quiet;
bool break_on_hit = false;

/**
* Prepare SWDIO line for master write
*/
void setupMasterWrite()
{
    pinMode(swdio_pin, OUTPUT);
}

/**
 * Prepare SWDIO line for master read
 */
void setupMasterRead()
{
    pinMode(swdio_pin, INPUT_PULLUP);
}

/**
 * Set SWD IO and CLK pin modes
 */
void setupPins()
{
    pinMode(swdio_pin, OUTPUT);
    pinMode(swclk_pin, OUTPUT);
}

/**
 * Reset SWD IO and CLK pins as inputs
 */
void resetPins()
{
    pinMode(swdio_pin, INPUT);
    pinMode(swclk_pin, INPUT);
}

/**
 * Set pins for SWD IO and CLK
 *
 * @param: new swd clock pin to set
 * @param: new swd io pin to set
 */
void setPins(byte new_swdclk_pin, byte new_swdio_pin)
{
    swclk_pin = new_swdclk_pin;
    swdio_pin = new_swdio_pin;
}

/**
 * Clock the SWDCLK line
 *
 * Master both reads and writes on falling edge.
 * Target reads and writed data on rising edge.
 */
void pulseClock()
{
    digitalWrite(swclk_pin, LOW);
    delayMicroseconds(CLOCK_HALF_CYCLE_US);
    digitalWrite(swclk_pin, HIGH);
    delayMicroseconds(CLOCK_HALF_CYCLE_US);
}

/**
 * Reset the line by driving it high for at least 50 cycles
 */
void resetLine()
{
    digitalWrite(swdio_pin, HIGH);
    for (int i=0 ; i<RESET_SEQUENCE_LENGTH ; i++)
    {
        pulseClock();
    }
}

/**
 * Write a single bit to the target
 *
 * @param: binary value eiter 0 or 1
 */
void writeBit(bool value)
{
    digitalWrite(swdio_pin, value);
    pulseClock();
}

/**
 * Write n bits to the target
 *
 * Data is written in LSB order
 *
 * @param: value container
 * @param: number of bits to write
 */
void writeBits(long value, int length)
{
    for (int i=0; i<length; i++)
    {
        writeBit(bitRead(value, i));
    }

    if (debug >= verbose)
    {
        Serial.print(">> ");
        for (int i=0; i<length; i++)
        {
            Serial.print(bitRead(value, i));
        }
        Serial.println("");
    }
}

/**
 * Read n bits from the target
 *
 * Data is read in LSB order
 *
 * @param: value container
 * @param: number of bits to read
 */
void readBits(long *value, int length)
{
    setupMasterRead();
    for (int i=0; i<length; i++)
    {
        byte bitValue = digitalRead(swdio_pin);
        bitWrite(*value, i, bitValue);
        pulseClock();
    }

    if (debug >= verbose)
    {
        Serial.print("<< ");
        for (int i=0; i<length; i++)
        {
            Serial.print(bitRead(*value, i));
        }
        Serial.println("");
    }

    setupMasterWrite();
}

/**
 *  Perform turnaround without driving the SWDIO line
 */
void turnaround()
{
    setupMasterRead();
    pulseClock();
}

/**
 * Switch JTAG TMS to SWD
 *
 * Multiple steps included:
 * - line reset
 * - jtag to swd command write
 * - line reset
 * - line clear
 */
void switchJtagToSwd()
{
    resetLine();
    writeBits(JTAG_TO_SWD, 16);
    resetLine();
    writeBits(0x00, 4);
}

/**
 * Read device id code
 *
 * @param: pointer to a long value
 */
void readIdCode(long *buffer)
{
    writeBits(0xA5, 8);             // readIdCode command 0b10100101
    turnaround();
    readBits(buffer, 36);           // ack + data + parity
    turnaround();
    setupMasterWrite();
    writeBits(0x00, 8);
}

/**
 * Print result in a single row
 *
 * @param: read value including ack and response data
 */
void printResultRow(long value)
{
    char buffer[60];
    sprintf(buffer, ROW_FORMAT,
        swclk_pin,
        swdio_pin,
        getAck(value),
        getPartNo(value),
        getManufacturer(value));
    Serial.print(buffer);
}

/**
 * Test for SWD on specified pins
 *
 * @param: swd clk pin no
 * @param: swd io pin no
 * @return: true if swd lines found
 */
bool testSwdLines(byte new_swclk_pin, byte new_swdio_pin)
{
    bool result = false;
    long readBuffer = 0;
    setPins(new_swclk_pin, new_swdio_pin);
    setupPins();
    switchJtagToSwd();
    readIdCode(&readBuffer);
    result = getAck(readBuffer) == 1 && getManufacturer(readBuffer) == DESIGNER_DEFAULT;
    if (result || debug >= normal) printResultRow(readBuffer);
    resetPins();
    return result;
}

/**
 * Search for SWD lines
 *
 * Iterates through pins defined with pin_mask and pin_max.
 * Pin n is enabled by setting bit n of pin_mask to 1,
 * pin_max specifies the upper bound of search.
 *
 * @return: true if swd lines found
 */
bool enumerateSwdLines()
{
    bool result = false;
    for (int clk_pin=0; clk_pin<pin_max; clk_pin++)
    {
        if (bitRead(pin_mask, clk_pin) == LOW) continue;
        for (int io_pin=0; io_pin<pin_max; io_pin++)
        {
            if ((bitRead(pin_mask, io_pin) == LOW) || (io_pin == clk_pin)) continue;
            result |= testSwdLines(clk_pin, io_pin);
            if (result && break_on_hit) break;
        }
        if (result && break_on_hit) break;
    }
    return result;
}

/**
 * Determine greatest pin set in provided mask
 *
 * @param: pin mask
 * @return: highest pin number
 */
uint8_t getMaxPinFromMask(uint64_t pin_mask)
{
    uint8_t highest_pin = 0;
    for (uint8_t i=0; i<64; i++)
    {
        if (bitRead(pin_mask, i) == HIGH)
        {
            highest_pin = i;
        }
    }
    return highest_pin + 1;
}

/**
 * Read and discard data on serial port
 */
void clearSerial()
{
    while (Serial.available() > 0)
    {
        Serial.read();
    }
}

/**
 * Read cli byte
 *
 * Waits for input as long as necessary.
 *
 * @return: byte read from serial port
 */
byte readCliByte()
{
    while (true)
    {
        if (Serial.available() > 0)
        {
            byte input = Serial.read();
            Serial.write((input >= 0x20 && input <= 0x7e) ? input : 0x20);
            Serial.println("");
            clearSerial();
            return input;
        }
        delay(100);
    }
}

/**
 * Read long int from cli
 *
 * Waits for input as long as necessary.
 *
 * @return: long uint read from serial port
 */
uint64_t readCliUnsignedInt()
{
    char buffer[20];
    uint8_t idx = 0;

    while (true)
    {
        if (Serial.available() > 0)
        {
            byte input = Serial.read();
            Serial.write((input >= 0x20 && input <= 0x7e) ? input : 0x20);
            if ((idx < sizeof(buffer) - 1) && (input != 0x0d) && (input != 0x0a))
            {
                if (input >= 0x30 && input <= 0x7a)
                    buffer[idx++] = input;
            }
            else
            {
                Serial.println("");
                clearSerial();
                buffer[idx] = 0x00;
                return strtoul(buffer, NULL, 0);
            }
        }
        delay(100);
    }
}

/**
 * Print the prompt
 */
void printPrompt()
{
    Serial.print(PROMPT);
}

/**
 * Minimalistic command line interface
 */
void commandLineInterface()
{
    char selection = readCliByte();
    switch (selection)
    {
        case 'e':
            enumerateSwdLines();
            break;
        case 'm':
            {
                Serial.print("Enter pin mask ");
                pin_mask = readCliUnsignedInt();
                pin_max = getMaxPinFromMask(pin_mask);
                Serial.print("Pin mask set to ");
                Serial.println((unsigned long) pin_mask, BIN);
            }
            break;
        case 't':
            {
                Serial.print("Enter SWD CLK PIN NO ");
                byte user_clk_pin = (byte) readCliUnsignedInt();
                Serial.print("Enter SWD IO PIN NO ");
                byte user_io_pin = (byte) readCliUnsignedInt();
                Serial.print("SWD CLK set to ");
                Serial.println(user_clk_pin, DEC);
                Serial.print("SWD IO set to ");
                Serial.println(user_io_pin, DEC);
                testSwdLines(user_clk_pin, user_io_pin);
            }
            break;
        case 'b':
            {
                Serial.print("Break work on hit? 1/0 ");
                byte choice = readCliByte() - 0x30;
                break_on_hit = choice;
            }
            break;
        case 'd':
            {
                Serial.print("Choose debug level 0-2 ");
                byte choice = readCliByte() - 0x30;
                debug = (enum debug_level) ((choice < 0x03) ? choice : 0x00);
                Serial.print("Debug set to ");
                Serial.println(debug, DEC);
            }
            break;
        case 'h':
        default:
            Serial.println("e - enumerate swd lines");
            Serial.print("m - set pin mask, current: ");
            Serial.println((unsigned long) pin_mask, HEX);
            Serial.println("t - test pin pair for swd");
            Serial.println("b - break on hit");
            Serial.print("d - set debug level, current: ");
            Serial.println(debug, DEC);
            Serial.println("h - this help");
            break;
    }
}

/**
 * Typical Arduino setup
 */
void setup()
{
    pin_max = getMaxPinFromMask(pin_mask);
    Serial.begin(115200);
}

/**
 * Main loop
 */
void loop()
{
    printPrompt();
    commandLineInterface();
}