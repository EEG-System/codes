#ifndef LCDLIB_401Y_H_
#define LCDLIB_401Y_H_
// /*********************************************************************
//LCD Driver Functions
//
//by: Elliott Gurrola
//
//    LCD Pinouts
//
//    Pin 1   Ground
//    Pin 2   VCC (+3.3 to +5V)
//    Pin 3   Contrast adjustment
//    Pin 4   Register Select (RS). 0: Command, 1: Data
//    Pin 5   Read/Write (R/W). 0: Write, 1: Read     -> Set to GND for constant write
//    Pin 6   Clock (Enable). Falling edge triggered
//    Pin 7   Bit 0 (Not used in 4-bit operation)
//    Pin 8   Bit 1 (Not used in 4-bit operation)
//    Pin 9   Bit 2 (Not used in 4-bit operation)
//    Pin 10  Bit 3 (Not used in 4-bit operation)
//    Pin 11  Bit 4
//    Pin 12  Bit 5
//    Pin 13  Bit 6
//    Pin 14  Bit 7
//    Pin 15  Backlight Anode (+)
//    Pin 16  Backlight Cathode (-)
//
//    Top level functions available
//        - lcdInit();           // Initialize LCD
//        - lcdClear();          // Clear LCD and move cursor to (0, 0)
//        - lcdSetText(char * text, uint32_t x, uint32_t y); // Write string
//        - lcdSetInt(uint32_t val, uint32_t x, uint32_t y); //Write integer
//    Not important to the user:
//        - lcdWriteData(unsigned char data);
//        - lcdWriteCmd(unigned char cmd);       // Send a command to the LCD
//        - LOWNIB(char l);     // Send a nibble to the LCD
//        - lcdTriggerEN();
//
//    *** DELAY FUNCTION ASSUMES 120 MHz CLOCK ***
//
//
//Modified on: Apr 15, 2013
//by: Luiz (Luis Carlos Bañuelos-Chacon)
//
//Modified on: May 24, 2013
//by: Elias N Jaquez
//
//Modified on: Jan 13, 2021
//by: Juan Andres Lazo -> Adapted for MSP432E401Y Launchpad
//*********************************************************************/
//           Pin Connections
//MICRO                           LCD
//GND, 10k Pot Output             GND
//VCC, 10k Pot Output             VCC
//10k Pot Output (V0)             Contrast
//PK5                             Register Select
//GND                             Read/Write
//PK4                             Clock Enable
//Not Connected                   Data 0
//Not Connected                   Data 1
//Not Connected                   Data 2
//Not Connected                   Data 3
//PK0                             Data 4
//PK1                             Data 5
//PK2                             Data 6
//PK3                             Data 7
//100 Ohm Resistor to VCC         Backlight Anode (+)
//GND                             Backlight Cathode (-)
//*********************************************************************/
#include "msp.h"
#include <string.h>
#include <stdio.h>
// Pins
#define EN           0x10
#define RS           0x20
#define LCD_DATA     0x0F
#define LCD_START    0x03
#define LCD_CLEAR    0x00
#define LCD_4BIT_MODE 0x02
// Commands
#define CLEAR   0x01

// Functions
void lcdInit();                 // Initialize LCD
void lcdTriggerEN();                // Trigger Enable
void lcdWriteData(unsigned char data);      // Send Data (Characters)
void lcdWriteCmd(unsigned char cmd);        // Send Commands
void lcdClear();                // Clear LCD
void lcdSetText(char * text, uint32_t x, uint32_t y); // Write string
void lcdSetInt(uint32_t val, uint32_t x, uint32_t y);      // Write integer

// Delay Functions assuming 120 MHz
#define delay_ms(x)     __delay_cycles((long) x* 120000)
#define delay_us(x)     __delay_cycles((long) x* 120)

#endif /* LCDLIB_401Y_H_ */
