
//*****************************************************************************
//
// usb_dev_bulk.c - Main routines for the generic bulk device example.
//
//*****************************************************************************
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

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Generic Bulk Device (usb_dev_bulk)</h1>
//!
//! This example provides a generic USB device offering simple bulk data
//! transfer to and from the host.  The device uses a vendor-specific class ID
//! and supports a single bulk IN endpoint and a single bulk OUT endpoint.
//! Data received from the host is assumed to be ASCII text and it is
//! echoed back with the case of all alphabetic characters swapped.
//!
//! A Windows INF file for the device is provided
//! in the C:/ti/simplelink_msp432e4_sdk_xx_xx/tools/usblib/windows_drivers
//! directory.  This INF contains information required to install the WinUSB
//! subsystem on Windows PCs.  WinUSB is a Windows subsystem
//! allowing user mode applications to access the USB device without the need
//! for a vendor-specific kernel mode driver.
//!
//! A sample Windows command-line application, usb_bulk_example, illustrating
//! how to connect to and communicate with the bulk device is also provided.
//! The usb_bulk_example.exe can be accessed from the 
//! <simplelink_msp432e4_sdk_x_xx_xx/tools/examples/usb_bulk_example folder
//
//*****************************************************************************

uint8_t Registers[24];
uint32_t NumDaisy = 1;
int Setup = 1;

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS   (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// The global system tick counter.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount = 0;

//*****************************************************************************
//
// Variables tracking transmit and receive counts.
//
//*****************************************************************************
volatile uint32_t g_ui32TxCount = 0;
volatile uint32_t g_ui32RxCount = 0;

//*****************************************************************************
//
// Flags used to pass commands from interrupt context to the main loop.
//
//*****************************************************************************
#define COMMAND_PACKET_RECEIVED 0x00000001
#define COMMAND_STATUS_UPDATE   0x00000002

volatile uint32_t g_ui32Flags = 0;

//*****************************************************************************
//
// Global flag indicating that a USB configuration has been set.
//
//*****************************************************************************
static volatile bool g_bUSBConfigured = true;

bool TXComplete = true;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

extern void USB0_IRQDeviceHandler(void);
//This dummy handler created as a hack for USB interrupt in
//startup file.  When the USB interrupt has attribute of 'weak'
//and usb.lib is linked to project, the interrupt handler in
//../device/usbdhandler.c file is routed
//to default handler.  The 'weak' attribute only works for
//dynamic libraries and not static library
void USB0_IRQHandler(void)
{

    USB0_IRQDeviceHandler();
}

//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************
void SysTick_Handler(void) // Update our system tick counter.
{
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// Receive new data and echo it back to the host.
//
// \param psDevice points to the instance data for the device whose data is to
// be processed.
// \param pi8Data points to the newly received data in the USB receive buffer.
// \param ui32NumBytes is the number of bytes of data available to be
// processed.
//
// This function is called whenever we receive a notification that data is
// available from the host. We read the data, byte-by-byte and swap the case
// of any alphabetical characters found then write it back out to be
// transmitted back to the host.
//
// \return Returns the number of bytes of data processed.
//
//*****************************************************************************
static uint32_t EchoNewDataToHost(tUSBDBulkDevice *psDevice, uint8_t *pi8Data,
                                  uint_fast32_t ui32NumBytes)
{
    uint_fast32_t ui32Loop, ui32Space, ui32Count;
    uint_fast32_t ui32ReadIndex;
    uint_fast32_t ui32WriteIndex;
    tUSBRingBufObject sTxRing;

    // Get the current buffer information to allow us to write directly to
    // the transmit buffer (we already have enough information from the
    // parameters to access the receive buffer directly).
    USBBufferInfoGet(&g_sTxBuffer, &sTxRing);

    // How much space is there in the transmit buffer?
    ui32Space = USBBufferSpaceAvailable(&g_sTxBuffer);

    // How many characters can we process this time round?
    ui32Loop = (ui32Space < ui32NumBytes) ? ui32Space : ui32NumBytes;
    ui32Count = ui32Loop;

    // Update our receive counter.
    g_ui32RxCount += ui32NumBytes;

    // Set up to process the characters by directly accessing the USB buffers.
    ui32ReadIndex = (uint32_t) (pi8Data - g_pui8USBRxBuffer);
    ui32WriteIndex = sTxRing.ui32WriteIndex;
    //In our case then an 'F' means empty, null.
    int value = 0; //to be used in switch statement.
    switch (g_pui8USBRxBuffer[ui32ReadIndex])
    { //Check the first character to see what you have to do.
    case 'R': //Read register
        ReadREG(g_pui8USBRxBuffer[ui32ReadIndex + 1]);
        g_pui8USBTxBuffer[ui32WriteIndex] = (char) Registers[g_pui8USBRxBuffer[ui32ReadIndex + 1]]; //Send back the Result
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = (char) Registers[g_pui8USBRxBuffer[ui32ReadIndex + 1]]; //Send back the Result
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = (char) Registers[g_pui8USBRxBuffer[ui32ReadIndex + 1]]; //Send back the Result
        break;
    case 'W': //Write Registerms
        if (g_pui8USBRxBuffer[ui32ReadIndex + 1] == '{') // If a bracket is received then write the same value to all the registers
        {
            WriteREGS(0x05, 0x0C, g_pui8USBRxBuffer[ui32ReadIndex + 1]);
            ReadREGS(0x05, 0x0C);
        }
        else    //If no bracket then only write to one register
        {
            WriteREG(g_pui8USBRxBuffer[ui32ReadIndex + 1],
                     g_pui8USBRxBuffer[ui32ReadIndex + 2]);
            ReadREG(g_pui8USBRxBuffer[ui32ReadIndex + 1]);
        }
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = (char) Registers[g_pui8USBRxBuffer[ui32ReadIndex + 1]]; //Send back the Result
        break;
    case 'E':
        Setup = 0;  //Exit setup and start acquiring samples
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;
    case 'T':   //Setup for test signals
        TEST();
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;
    case 'X':
        Setup = 1; //Stop acquiring samples and setup ADC to changes can be made.
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;
    case 'G': //set Gain to ADS
        value = g_pui8USBRxBuffer[ui32ReadIndex + 1];
        value = (Registers[5] & ~(7 << 4)) | (value << 4); //gets gain from usb, ORs to registers, writeback to registers.
        WriteREGS(0x05, 0x0C, value);
        ReadREGS(0x05, 0x0C);
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;

    case 'D': //set Data Rate to ADS
        value = g_pui8USBRxBuffer[ui32ReadIndex + 1];
        value = (Registers[1] & ~(7)) | (value); //gets data rate from usb, ORs to register, writeback to register.

        WriteREG(0x01, value);
        ReadREG(0x01);
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;

    case 'N': //Initiate normal Mode
        NORM();
        g_pui8USBTxBuffer[ui32WriteIndex] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 1] = 'F';
        g_pui8USBTxBuffer[ui32WriteIndex + 2] = 'F';
        break;
    default:
        break;
    }

    USBBufferDataWritten(&g_sTxBuffer, 3);

    // We processed as much data as we can directly from the receive buffer so
    // we need to return the number of bytes to allow the lower layer to
    // update its read pointer appropriately.
    return (ui32Count);
}

//*****************************************************************************
//
// Handles bulk driver notifications related to the transmit channel (data to
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ulEvent identifies the event we are being notified about.
// \param ulMsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t TxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
                   void *pvMsgData)
{
    // We are not required to do anything in response to any transmit event
    // in this example. All we do is update our transmit counter.
    if (ui32Event == USB_EVENT_TX_COMPLETE)
    {
        TXComplete = true;
        g_ui32TxCount += ui32MsgValue;
    }
    return (0);
}

//*****************************************************************************
//
// Handles bulk driver notifications related to the receive channel (data from
// the USB host).
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the bulk driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t RxHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue,
                   void *pvMsgData)
{
    // Which event are we being sent?
    switch (ui32Event)
    {
    // We are connected to a host and communication is now possible.
    case USB_EVENT_CONNECTED:
    {
        g_bUSBConfigured = true;
        g_ui32Flags |= COMMAND_STATUS_UPDATE;

        // Flush our buffers.
        USBBufferFlush(&g_sTxBuffer);
        USBBufferFlush(&g_sRxBuffer);
        break;
    }

        // The host has disconnected.
    case USB_EVENT_DISCONNECTED:
    {
        g_bUSBConfigured = false;
        g_ui32Flags |= COMMAND_STATUS_UPDATE;
        Setup = 0; //USB disconnected so stop sampling
        break;
    }

        // A new packet has been received.
    case USB_EVENT_RX_AVAILABLE:
    {
        tUSBDBulkDevice *psDevice;
        // Get a pointer to our instance data from the callback data
        // parameter.
        psDevice = (tUSBDBulkDevice *) pvCBData;

        // Read the new packet and echo it back to the host.
        return (EchoNewDataToHost(psDevice, pvMsgData, ui32MsgValue));
    }

        // Ignore SUSPEND and RESUME for now.
    case USB_EVENT_SUSPEND:
    case USB_EVENT_RESUME:
        break;

        // Ignore all other events and return 0.
    default:
        break;
    }

    return (0);
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************
int main(void)
{
    uint32_t ui32SysClock;
    uint32_t ui32PLLRate;
    uint32_t numSamples = 0;
    uint32_t bytesSend = 0;
#ifdef USE_ULPI
    uint32_t ui32ULPI;
#endif

    // Run from the PLL at 120 MHz.
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
    SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                          120000000);

    //ADS Stuff Setup
    SysCtlDelay(5120000); //128ms delay needed to ensure the voltages on ADS1299 have stabilized

    //Port that is going to be used to toggle a few of the pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0); //START pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_1); //RESET pin
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2); //CS pin
    GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_3); //DRDY pin
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_PIN_1); //Take RESET high
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2); //Take CS high

    /* Enable clocks to GPIO Port A and configure pins as SSI */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)))
    {
    }

    MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    MAP_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    MAP_GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
    MAP_GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
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
    WriteREG(0x03, 0xE0); //Modify Config3
    ReadREG(0x03);  //Verify Write of Config3

    //Setup for normal read
    WriteREGS(0x05, 0x0C, 0x00);
    ReadREGS(0x05, 0x0C);
    //BIAS();
    //TEST();
    _RDATAC();

    // Configure the device pins.
    PinoutSet(false, true);

    // Not configured initially.
    g_bUSBConfigured = false;

    // Enable the system tick.
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    // Initialize the transmit and receive buffers.
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    // Tell the USB library the CPU clock and the PLL frequency.
    SysCtlVCOGet(SYSCTL_XTAL_25MHZ, &ui32PLLRate);
    USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

#ifdef USE_ULPI
    // Tell the USB library to use ULPI interface
    ui32ULPI = USBLIB_FEATURE_ULPI_HS;
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBULPI, &ui32ULPI);
#endif

    // Initialize the USB stack for device mode.
    USBStackModeSet(0, eUSBModeDevice, 0);

    // Pass our device information to the USB library and place the device on the bus.
    USBDBulkInit(0, &g_sBulkDevice);

    // Main Application Loop.
    while (1)
    {
        _SDATAC();
        while (Setup);//Empty while loop so that modifications to register don't interrupt any other process.

        _RDATAC(); //Setup is over so enter Read data continuous

        //Read register to set how many samples will be sent per packet.
        switch (Registers[1])
        {
        case 0x96:      //250 SPS
            numSamples = 5;
            bytesSend = 120;
            break;
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
        bytesSend += (3 * numSamples); //Increase the number of bytes to include the status bytes
        bytesSend *= NumDaisy; //Increase the number of bytes depending on how many ADS1299 are conenected.

        GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, GPIO_PIN_0); //Take Start high (Start Conversions)
        SysCtlDelay(5120000);     //Delay stop first few samples from being zero
        while (!Setup)
        {
            ADSTEST(numSamples, bytesSend);
        }
        GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0, 0x00); //Take Start low (Stop Conversions)
    }
}

