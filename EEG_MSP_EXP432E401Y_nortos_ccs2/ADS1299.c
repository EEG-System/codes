/*
 * ADS1299.c
 *
 *  Created on: Nov 1, 2019
 *      Author: Gerardo Barreras
 */

#include <stdbool.h>
#include <stdint.h>
#include "ti/devices/msp432e4/driverlib/driverlib.h"
#include "ti/usblib/msp432e4/usblib.h"
#include "ti/usblib/msp432e4/usb-ids.h"
#include "ti/usblib/msp432e4/device/usbdevice.h"
#include "ti/usblib/msp432e4/device/usbdbulk.h"
#include "uartstdio.h"
#include "pinout.h"
#include "usb_bulk_structs.h"
#include "ADS1299.h"

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
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    opcode = Start + RREG_S;
    transfer(opcode);
    SysCtlDelay(80); //Delay added to allow ADS to decode command
    transfer(0x00);
    SysCtlDelay(80);
    Registers[Start] = transfer(0x00);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}
void ReadREGS(uint8_t Start, uint8_t End)   //Read value from multiple registers
{
    uint32_t i;
    uint8_t opcode;
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
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
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void WriteREG(uint8_t Start, uint8_t Data) //Write data to a register
{
    uint32_t opcode = Start + WREG_S;
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(opcode);
    SysCtlDelay(80);
    transfer(0x00);
    SysCtlDelay(80);
    transfer(Data);
    SysCtlDelay(80);
    transfer(0x00);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void WriteREGS(uint8_t Start, uint8_t End, uint8_t Data) //Write same data to multiple registers.
{
    uint32_t opcode = Start + WREG_S;
    int i;
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
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
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void _RESET() //Command to Reset ADS (Can be used instead of pins)
{
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(RESET);
    SysCtlDelay(353);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void _SDATAC()  //Command to allow modification of registers on ADS1299
{
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(SDATAC);
    SysCtlDelay(120);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}
void _RDATAC() //Command to be able to exit register modification and start acquiring samples
{
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(RDATAC);
    SysCtlDelay(120);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void _START() //Command to start sample acquisition. (Can be used instead of pins)
{
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(START);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void _STOP() //Command to stop sample acquisition. (Can be used instead of pins)
{
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
    transfer(STOP);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
    return;
}

void ADSTEST(uint32_t numsamples, uint32_t bytesSend) //Command that acquires samples from ADS
{
    tUSBRingBufObject sTxRing;
    int i, j, k;
    int samplecounter = 0;
    USBBufferInfoGet(&g_sTxBuffer, &sTxRing);
    // How much space is there in the transmit buffer?
    uint32_t ui32Space = USBBufferSpaceAvailable(&g_sTxBuffer);
    uint32_t ui32WriteIndex = sTxRing.ui32WriteIndex;
    if (ui32Space == BULK_BUFFER_SIZE - 1)
    {
        TXComplete = false;

        while (samplecounter < numsamples)
        {
            while (GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_3)); //Wait for DRDY

            for (k = 0; k < NumDaisy; k++)
            {
                GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0x00);
                //Status Bytes
                for (i = 0; i < 3; i++)//Status Bytes
                {
                    g_pui8USBTxBuffer[ui32WriteIndex] = transfer(0x00);
                    ui32WriteIndex++;
                    ui32WriteIndex =
                            (ui32WriteIndex == BULK_BUFFER_SIZE) ?
                                    0 : ui32WriteIndex;
                }
                for (i = 0; i < 8; i++)
                {
                    for (j = 0; j < 3; j++)
                    {
                        g_pui8USBTxBuffer[ui32WriteIndex] = transfer(0x00);
                        ui32WriteIndex++;
                        ui32WriteIndex =
                                (ui32WriteIndex == BULK_BUFFER_SIZE) ?
                                        0 : ui32WriteIndex;
                    }
                }
                GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
            }
            samplecounter++;
        }
        USBBufferDataWritten(&g_sTxBuffer, bytesSend);
    }
    return;
}

void TEST() //Setup ADS to acquire internal test signals
{
    WriteREG(0x02, 0xD0);       //Modify Config2 for test signals
    WriteREGS(0x05, 0x0C, Registers[5] | 0x05); //Modify all channels for test signals
    ReadREG(0x02);              //Read back written values
    ReadREGS(0x05, 0x0C);
    return;
}

void NORM() //Setup ADS for normal operation
{
    WriteREG(0x02, 0xC0);       //Modify Config2 for normal signals
    WriteREGS(0x05, 0x0C, Registers[5] & 0xF8);
    ReadREG(0x02);              //Read back written values
    ReadREGS(0x05, 0x0C);
}
//end of ADS functions

