#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "ti/usblib/msp432e4/usblib.h"
#include "ti/usblib/msp432e4/usb-ids.h"
#include "ti/usblib/msp432e4/device/usbdevice.h"
#include "ti/usblib/msp432e4/device/usbdbulk.h"
#include "ADS1299.h"

#define CS_LOW     GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00)
#define CS_HIGH    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2)
#define DRDY       GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_3)
#define START_HIGH GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, GPIO_PIN_0)
#define START_LOW  GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, 0x00)

/*
 * PL0 -> START
 * PL1 -> RESET
 * PL2 -> CS
 * PL3 -> DRDY
 * PA2 -> SCKL
 * PA4 -> DIN
 * PA5 -> DOUT
 */

//ADS functions
uint8_t transfer(uint8_t Byte1) //SPI transaction
{
    uint32_t x;
    MAP_SSIDataPut(SSI0_BASE, Byte1);
    MAP_SSIDataGet(SSI0_BASE, &x);
    return x;
}

void ReadREG(uint8_t Start) //Read value from a register
{
    uint8_t opcode;
    CS_LOW;
    opcode = Start + RREG_S;
    transfer(opcode);
    SysCtlDelay(80); //Delay added to allow ADS to decode command
    transfer(0x00);
    SysCtlDelay(80);
    Registers[Start] = transfer(0x00);
    CS_HIGH;
    return;
}
void ReadREGS(uint8_t Start, uint8_t End)   //Read value from multiple registers
{
    uint32_t i;
    uint8_t opcode;
    CS_LOW;
    opcode = Start + RREG_S;
    transfer(opcode);
    SysCtlDelay(80); //Delay added to allow ADS to decode command
    transfer(End);
    SysCtlDelay(80);
    for (i = 0; i <= End; i++)
    {
        Registers[Start + i] = transfer(0x00);
        SysCtlDelay(80);
    }
    CS_HIGH;
    return;
}

void WriteREG(uint8_t Start, uint8_t Data) //Write data to a register
{
    uint32_t opcode = Start + WREG_S;
    CS_LOW;
    transfer(opcode);
    SysCtlDelay(80);
    transfer(0x00);
    SysCtlDelay(80);
    transfer(Data);
    SysCtlDelay(80);
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
    SysCtlDelay(80); //Delay added to allow ADS to decode command
    transfer(End);
    SysCtlDelay(80);
    for (i = 0; i <= (End - Start); i++)
    {
        transfer(Data);
        SysCtlDelay(80);
    }
    transfer(0x00);
    CS_HIGH;
    return;
}

void _RESET() //Command to Reset ADS (Can be used instead of pins)
{
    CS_LOW;
    transfer(RESET);
    SysCtlDelay(353);
    CS_HIGH;
    return;
}

void _SDATAC()  //Command to allow modification of registers on ADS1299
{
    CS_LOW;
    transfer(SDATAC);
    SysCtlDelay(120);
    CS_HIGH;
    return;
}
void _RDATAC() //Command to be able to exit register modification and start acquiring samples
{
    CS_LOW;
    transfer(RDATAC);
    SysCtlDelay(120);
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

void ADS1299_read_data(NumDaisy)
{
    static uint32_t adc_data[28];
    int i, j, k, index;
    int samplecounter = 0;

    //Setup is over so enter Read data continuous
    _RDATAC();

    //Read CONFIG1 to determine the Data Sample Rate
    switch (Registers[1])
    {
        case 0x95:      //500 SPS
            numSamples = 10;
            bytesSend = 240;
            break;
        case 0x94:      //1K SPS
            numSamples = 20;
            bytesSend = 480;
            break;
        case 0x93:      //2K SPS
            numSamples = 40;
            bytesSend = 960;
            break;
        case 0x92:      //4K SPS
            numSamples = 80;
            bytesSend = 1920;
            break;
        case 0x91:      //8K SPS
            numSamples = 160;
            bytesSend = 3840;
            break;
        default:        //Default Rate of the ADS1299 is 250 SPS
            numSamples = 5;
            bytesSend = 120;
            break;
    }
    //Increase the number of bytes to include the status bytes
    bytesSend += (3 * numSamples);
    //Increase the number of bytes depending on how many ADS1299 are conenected.
    bytesSend *= NumDaisy;
    //Take Start high (Start Conversions)
    START_HIGH;
    //Delay stop first few samples from being zero
    SysCtlDelay(5120000);

    while(sampleCounter < numSamples)
    {
        // Wait for DRDY
        while(DRDY);

        // For loop to go as many times as devices connected to Daisy Chain
        for(i=0;i<NumDaisy;i++)
        {
            CS_LOW;
            //Status Bytes
            for(j=0;j<3;j++)
            {
                adc_data[index] = transfer(0x00);
                index++;
            }

            for(j=0;j<8;j++)
            {
                for(k=0;k<3;k++)
                {
                    adc_data[index] = transfer(0x00);
                    index++;
                }
            }
            CS_HIGH;
        }
        sampleCounter++;
    }
    START_LOW;
    return;

}

void TEST() //Setup ADS to acquire internal test signals
{
    WriteREG(CONFIG2, 0xD0);       //Modify Config2 for test signals
    WriteREGS(CH1SET, CH8SET, Registers[5] | 0x05); //Modify all channels for test signals
    ReadREG(CONFIG2);              //Read back written values
    ReadREGS(CH1SET, CH8SET);
    return;
}

void NORM() //Setup ADS for normal operation
{
    WriteREG(CONFIG2, 0xC0);       //Modify Config2 for normal signals
    WriteREGS(CH1SET, CH8SET, Registers[5] & 0xF8);
    ReadREG(CONFIG2);              //Read back written values
    ReadREGS(CH1SET, CH8SET);
}

void ADS1299_init(uint32_t ui32SysClock)
{
    uint32_t bytesSend = 0;

    //ADS Stuff Setup
    delay_ms(128); //128ms delay needed to ensure the voltages on ADS1299 have stabilized

    //Port that is going to be used to toggle a few of the pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0); //START pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_1); //RESET pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2); //CS pin
    GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_3);  //DRDY pin

    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_PIN_1); //Take RESET high
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2); //Take CS high

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

    MAP_SSIConfigSetExpClk(SSI0_BASE, ui32SysClock, SSI_FRF_MOTO_MODE_1, //15 MHz clock, clock phase 0 polarity 1
    SSI_MODE_MASTER,
                          (ui32SysClock / 8), 8);
    MAP_SSIEnable(SSI0_BASE);

    /* Flush the Receive FIFO */
    while (MAP_SSIDataGetNonBlocking(SSI0_BASE, &bytesSend));
    //SPI setup is complete

    _RESET();
    _SDATAC(); //Enter SDATAC and don't exit until told to by computer
    ReadREGS(0x00, 0x17); //Read All Registers
}

//end of ADS functions

