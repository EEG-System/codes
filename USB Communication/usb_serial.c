#include "usb_serial.h"

#include "ti/devices/msp432e4/driverlib/inc/hw_gpio.h"
#include "ti/devices/msp432e4/driverlib/driverlib.h"

#include "ti/usblib/msp432e4/usblib.h"
#include "ti/usblib/msp432e4/usbcdc.h"
#include "ti/usblib/msp432e4/usb-ids.h"
#include "ti/usblib/msp432e4/device/usbdevice.h"
#include "ti/usblib/msp432e4/device/usbdcdc.h"

#include "usb_structs.h"

static volatile bool g_bUSBConfigured = false;

void usb_init(uint32_t g_ui32SysClock)
{
    uint32_t ui32PLLRate;

    //Initalize peripherals for USB
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);

    //Initialize USB hardware
    GPIOD->LOCK = GPIO_LOCK_KEY;
    GPIOD->CR = 0xff;
    ROM_GPIOPinConfigure(GPIO_PD6_USB0EPEN);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    g_bUSBConfigured = false;

    // Initialize the transmit and receive buffers for first serial device.
    USBBufferInit(&g_psTxBuffer);
    USBBufferInit(&g_psRxBuffer);

    // Initialize the serial port
    USBDCDCCompositeInit(0, &g_psCDCDevice, 0);

    // Tell the USB library the CPU clock and the PLL frequency.
    SysCtlVCOGet(SYSCTL_XTAL_25MHZ, &ui32PLLRate);
    USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &g_ui32SysClock);
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    // Initial serial device and pass the device information to the USB library and place the device on the bus.
    USBDCDCInit(0, &g_psCDCDevice);
}

uint32_t usb_read(uint8_t *data, uint32_t size)
{
    return USBBufferRead((tUSBBuffer*)&g_psRxBuffer, data, size);
}

void usb_write(uint8_t *data, uint32_t size)
{
    USBBufferWrite((tUSBBuffer*)&g_psTxBuffer, data, size);
}

uint32_t ControlHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData)
{
    // Which event are we being asked to process?
    switch(ui32Event)
    {
        // We are connected to a host and communication is now possible.
        case USB_EVENT_CONNECTED:
        {
            g_bUSBConfigured = true;


            // Flush our buffers.
            USBBufferFlush(&g_psTxBuffer);
            USBBufferFlush(&g_psRxBuffer);

            break;
        }

        case USB_EVENT_DISCONNECTED:
        {
            g_bUSBConfigured = false;
            break;
        }

        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
        {
            break;
        }

        default:
        {
            break;
        }
    }

    return(0);
}



uint32_t TxHandlerCmd(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData)
{
    switch(ui32Event)
    {
        case USB_EVENT_TX_COMPLETE:
        {
            break;
        }

        default:
        {
            break;
        }
    }
    return(0);
}

uint32_t RxHandlerCmd(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgValue, void *pvMsgData)
{
    switch(ui32Event)
    {
        case USB_EVENT_RX_AVAILABLE:
        {
            break;
        }

        case USB_EVENT_DATA_REMAINING:
        {
            return(0);
        }

        case USB_EVENT_REQUEST_BUFFER:
        {
            return(0);
        }

        default:
        {
            break;
        }
    }

    return 0;
}

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

