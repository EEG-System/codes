#ifndef ADS1299_H_
#define ADS1299_H_

#include <ti/devices/msp432e4/driverlib/driverlib.h>

/*LIST OF COMMANDS FOR ADS1299 page 40*/
#define WAKEUP      0x02
#define STANDBY     0x04
#define RESET       0x06
#define START       0x08
#define STOP        0x0A

#define RDATAC      0x10
#define SDATAC      0x11
#define RDATA       0x12

#define RREG        0x20
#define WREG        0x40

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
/*Lead-Off Status Registers (Read-Only Registers)*/
#define LOFF_STATP  0x12
#define LOFF_STATN  0x13
/*GPIO and OTHER Registers*/
#define GPIO        0x14
#define MISC1       0x15
#define MISC2       0x16
#define CONFIG4     0x17

void ADS1299_init(uint32_t);
void ADS1299_cmd(uint8_t cmd);
void ADS1299_wreg(uint8_t reg, uint8_t data);
uint8_t ADS1299_rreg(uint8_t reg);
uint8_t *ADS1299_read_data(void);

#endif /* ADS1299_H_ */
