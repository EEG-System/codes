/*
 * ADS1299.h
 *
 *  Created on: Nov 1, 2019
 *      Author: Gerardo Barreras
 */

#ifndef ADS1299_H_
#define ADS1299_H_

uint8_t transfer(uint8_t Byte1);
void ReadREG(uint8_t Start);
void ReadREGS(uint8_t Start, uint8_t End);
void WriteREG(uint8_t Start, uint8_t Data);
void WriteREGS(uint8_t Start, uint8_t End, uint8_t Data);
void _RESET();
void _SDATAC();
void _START();
void ADSTEST(uint32_t numsamples, uint32_t bytesSend);
void _STOP();
void _RDATAC();
void TEST();
void NORM();

//All of these next commands are SPI commands

//System Commands
#define RESET   0x06    //Command to reset the ADS
#define START   0x08    //Command to Start samples
#define STOP    0x0A    //Command to Stop samples

//Data Read Commands
#define SDATAC  0x11   //Command to stop data continuous
#define RDATAC  0x10   //Command to read data continuous

//Register Commands
#define RREG_S  0x20    // Command to Read register
#define WREG_S  0x40    //Command to Write to Register

extern uint8_t Registers[24];
extern uint32_t NumDaisy;
extern bool TXComplete;

#endif /* ADS1299_H_ */
