#ifndef ADS1299_H_
#define ADS1299_H_

#include <ti/devices/msp432e4/driverlib/driverlib.h>

uint8_t transfer(uint8_t Byte1);
void ADS1299_init(uint32_t ui32SysClock);


uint8_t ReadREG(uint8_t Start);
void ReadREGS(uint8_t Start, uint8_t End);
void WriteREG(uint8_t Start, uint8_t Data);
void WriteREGS(uint8_t Start, uint8_t End, uint8_t Data);

void _START();
void _STOP();
void _RESET();

void _SDATAC();
void _RDATAC();
void ADS1299_read_data(uint8_t NumDaisy);

void TEST();
void NORM();
//void SHORT();


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

/*LIST OF REGISTERS FOR ADS1299 page 44*/
/*Read Only ID Registers*/
#define ID          0x00

/*Global Settings Across Channels*/
#define CONFIG1     0x01
#define CONFIG2     0x02
#define CONFIG3     0x03
#define LOFF        0x04

/*Channel-Specific Settings*/
#define CH1SET      0x05
#define CH2SET      0x06
#define CH3SET      0x07
#define CH4SET      0x08
#define CH5SET      0x09
#define CH6SET      0x0A
#define CH7SET      0x0B
#define CH8SET      0x0C
#define BIAS_SENSP  0x0D
#define BIAS_SENSN  0x0E
#define LOFF_SENSP  0x0F
#define LOFF_SENSN  0x10
#define LOFF_FLIP   0x11

/* Data conversion helper */
// 1 LSB
#define LSB     (double)(2*VREF/GAIN)/16777216
#define VREF    4500
#define GAIN    24

extern uint8_t Registers[24];
extern uint8_t NumDaisy;
extern uint8_t ads_data[24];
extern uint32_t channel_data[8];

// Delay Functions assuming 120 MHz
#define delay_ms(x)     __delay_cycles((long) x* 120000)
#define delay_us(x)     __delay_cycles((long) x* 120)

#endif /* ADS1299_H_ */
