#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "ti/usblib/msp432e4/usblib.h"
#include "ti/usblib/msp432e4/usb-ids.h"
#include "ti/usblib/msp432e4/device/usbdevice.h"
#include "ti/usblib/msp432e4/device/usbdbulk.h"
#include "ADS1299.h"
#include "usb_serial.h"

#define CS_LOW     GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00)
#define CS_HIGH    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2)
#define DRDY       GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_3)
#define START_HIGH GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, GPIO_PIN_0)
#define START_LOW  GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, 0x00)

/*
 * PL0 -> START
 * PL1 -> RESET
 * PL2 -> CLKSEL
 * PL3 -> DRDY
 * PA2 -> SCKL
 * PA4 -> DIN
 * PA5 -> DOUT
 *
 */

//ADS functions
uint8_t transfer(uint8_t Byte1) //SPI transaction
{
    uint32_t x;
    MAP_SSIDataPut(SSI0_BASE, Byte1);
    MAP_SSIDataGet(SSI0_BASE, &x);
    return x;
}

uint8_t ReadREG(uint8_t Start) //Read value from a register
{
    uint8_t opcode;
    CS_LOW;

    opcode = Start + RREG_S;
    transfer(opcode);
    //Delay added to allow ADS to decode command
    delay_us(2);
    transfer(0x00);
    delay_us(2);
    Registers[Start] = transfer(0x00);

    CS_HIGH;
    return Registers[Start];
}
void ReadREGS(uint8_t Start, uint8_t End)   //Read value from multiple registers
{
    uint32_t i;
    uint8_t opcode;
    CS_LOW;
    opcode = Start + RREG_S;
    transfer(opcode);
    //Delay added to allow ADS to decode command
    delay_us(2);
    transfer(End);
    delay_us(2);
    for (i = 0; i <= End; i++)
    {
        Registers[Start + i] = transfer(0x00);
        delay_us(2);
    }
    CS_HIGH;
    return;
}

void WriteREG(uint8_t Start, uint8_t Data) //Write data to a register
{
    uint32_t opcode = Start + WREG_S;
    CS_LOW;
    transfer(opcode);
    delay_us(2);
    transfer(0x00);
    delay_us(2);
    transfer(Data);
    delay_us(2);
    transfer(0x00);
    CS_HIGH;
    return;
}

void WriteREGS(uint8_t Start, uint8_t End, uint8_t Data) //Write same data to multiple registers.
{
    uint32_t opcode = Start + WREG_S;
    int i;
    CS_LOW;
    transfer(opcode);
    //Delay added to allow ADS to decode command
    delay_us(2);
    transfer(End);
    delay_us(2);
    for (i = 0; i <= (End - Start); i++)
    {
        transfer(Data);
        delay_us(2);
    }
    transfer(0x00);
    CS_HIGH;
    return;
}

void _RESET() //Command to Reset ADS (Can be used instead of pins)
{
    CS_LOW;
    transfer(RESET);
    delay_us(9);
    CS_HIGH;
    return;
}

void _SDATAC()  //Command to allow modification of registers on ADS1299
{
    CS_LOW;
    transfer(SDATAC);
    delay_us(3);
    CS_HIGH;
    return;
}
void _RDATAC() //Command to be able to exit register modification and start acquiring samples
{
    CS_LOW;
    transfer(RDATAC);
    delay_us(3);
    CS_HIGH;
    return;
}

void _START() //Command to start sample acquisition. (Can be used instead of pins)
{
    CS_LOW;
    transfer(START);
    CS_HIGH;
    return;
}

void _STOP() //Command to stop sample acquisition. (Can be used instead of pins)
{
    CS_LOW;
    transfer(STOP);
    CS_HIGH;
    return;
}

void ADS1299_read_data(uint8_t NumDaisy)
{
    int i, j, k, stat;
    int index = 0;

    delay_us(9);
    _SDATAC();

    //Take Start high (Start Conversions)
    START_HIGH;
    //Delay stop first few samples from being zero

    _RDATAC();  // Put the Device Back in RDATAC Mode

    // Wait for DRDY
    while(DRDY);


    // For loop to go as many times as devices connected to Daisy Chain
    for(i=0;i<NumDaisy;i++)
    {
        CS_LOW;
        /*
         * Status Bytes
         * 24 status bits is:
         * (1100 + LOFF_STATP + LOFF_STATN + bits[4:7] of the GPIO register).
         */
        for(j=0;j<3;j++)
        {
            uint8_t byte = transfer(0x00);
            stat = (stat << 8) | byte;
        }

        //  Read 24 bits of channel data in 8 3 byte chunks
        for(j=0;j<8;j++)
        {
            for(k=0;k<3;k++)
            {
                uint8_t byte = transfer(0x00);
                ads_data[index] = byte;

                usb_write(&ads_data[index],1);

                index++;
            }
        }

        CS_HIGH;


    }
    index = 0;
    //_SDATAC();
    //START_LOW;

}

void TEST() //Setup ADS to acquire internal test signals
{
    WriteREG(CONFIG2, 0xD0);          //Modify Config2 for test signals
    //WriteREG(CONFIG3, 0xE0);        // If Using Internal Reference, Send This Comman
    WriteREGS(CH1SET, CH8SET, 0x05); //Modify all channels for test signals
    ReadREG(CONFIG2);                  //Read back written values
    ReadREGS(CH1SET, CH8SET);
    return;
}

void NORM() //Setup ADS for normal operation
{
    WriteREG(CONFIG2, 0xC0);       //Modify Config2 for normal signals
    WriteREGS(CH1SET, CH8SET, 0x00);
    ReadREG(CONFIG2);              //Read back written values
    ReadREGS(CH1SET, CH8SET);
}

void SHORT() //Setup ADS for normal operation
{
    WriteREG(CONFIG2, 0xC0);       //Modify Config2 for normal signals
    WriteREGS(CH1SET, CH8SET, 0x01);
    ReadREG(CONFIG2);              //Read back written values
    ReadREGS(CH1SET, CH8SET);
}

void ADS1299_init(uint32_t ui32SysClock)
{
    uint32_t bytesSend = 0;

    //ADS Stuff Setup
    //delay_ms(131); //128ms delay needed to ensure the voltages on ADS1299 have stabilized

    //Port that is going to be used to toggle a few of the pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0); //START pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_1); //RESET pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2); //CS pin
    GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_3);  //DRDY pin

    // Delay for Power-On Reset and Oscillator Start-Up
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_PIN_1); //Take RESET high
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2); //Take CS high
    delay_ms(131);//128ms delay needed to ensure the voltages on ADS1299 have stabilized

    /* Enable clocks to GPIO Port A and configure pins as SSI */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)))
    {
    }

    MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK); // PA2 -> SCLK
    MAP_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    MAP_GPIOPinConfigure(GPIO_PA4_SSI0XDAT0); // PA4 -> DIN
    MAP_GPIOPinConfigure(GPIO_PA5_SSI0XDAT1); // PA5 -> DOUT
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, (GPIO_PIN_2 | GPIO_PIN_3 |
    GPIO_PIN_4 | GPIO_PIN_5));

    /* Enable the clock to SSI-0 module and configure the SSI Master */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)))
    {
    }

    MAP_SSIConfigSetExpClk(SSI0_BASE, ui32SysClock, SSI_FRF_MOTO_MODE_1, //12 MHz clock, clock phase 0 polarity 1
    SSI_MODE_MASTER,
                          (ui32SysClock/10),8);
    MAP_SSIEnable(SSI0_BASE);

    /* Flush the Receive FIFO */
    while (MAP_SSIDataGetNonBlocking(SSI0_BASE, &bytesSend));
    //SPI setup is complete

    //ReadREGS(0x00, 0x17); //Read All Registers
}

//end of ADS functions
