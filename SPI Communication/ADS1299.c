#include "ADS1299.h"

#define CS_HIGH    MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3)
#define CS_LOW     MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0)

void spi_ads_write(uint8_t *data, uint32_t size);
void spi_ads_read(uint8_t *data, uint32_t size);
void ADS1299_test_signal(void);


/*     -> START
 *     -> RST
 * PL3 -> DRDY
 * PA2 -> SCLK
 * PA3 -> CS
 * PA4 -> MOSI
 * PA5 -> MISO
 */
void ADS1299_init(uint32_t g_ui32SysClock)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)));

    //DRDY pin, this is to wait for data conversion to finish
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_3);

    /* Enable clocks to GPIO Port A and configure pins as SSI */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)));

    //CS pin, manual trigger, disable SPI communication to ADS
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    MAP_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);

    MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    MAP_GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
    MAP_GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, (GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5));

    /* Enable the clock to SSI-0 module and configure the SSI Master */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)));

    MAP_SSIConfigSetExpClk(SSI0_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, (g_ui32SysClock/8), 8);
    MAP_SSIEnable(SSI0_BASE);

    //128ms delay needed to ensure the voltages on ADS1299 have stabilized
    SysCtlDelay(5120000);

}

void ADS1299_cmd(uint8_t cmd)
{
    CS_LOW;

    spi_ads_write(&cmd, 1);

    CS_HIGH;
}

void ADS1299_wreg(uint8_t reg, uint8_t data)
{
    uint8_t cmd[2] = {WREG | reg, 0x01};

    CS_LOW;

    //Send command first
    spi_ads_write(cmd, 2);
    SysCtlDelay(80);

    //Then send data
    spi_ads_write(&data, 1);
    SysCtlDelay(80);

    CS_HIGH;
}

uint8_t ADS1299_rreg(uint8_t reg)
{
    uint8_t cmd[2] = {RREG | reg, 0x01};
    uint8_t data;

    CS_LOW;

    //Send command first
    spi_ads_write(cmd, 2);
    SysCtlDelay(80);

    //Then receive data data
    spi_ads_read(&data, 1);
    SysCtlDelay(80);

    CS_HIGH;

    return data;
}

/*[(24 status bits + 24 bits × 8 channels) = 216 bits].
 * The format of the 24 status bits is: (1100 + LOFF_STATP + LOFF_STATN + bits[4:7] of the GPIO register*/

/*4 Bytes of status data + 24 bytes of ADC data*/

uint8_t *ADS1299_read_data(void)
{
    static uint8_t adc_data[28];

    //Wait for data to be ready
    while(MAP_GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_3) == GPIO_PIN_3);
    __delay_cycles(10);

    CS_LOW;

    spi_ads_read(adc_data, 28);

    CS_HIGH;

    return adc_data;
}

/*LOW LEVEL COMMUNICATION FUNCTIONS*/
void spi_ads_write(uint8_t *data, uint32_t size)
{
    uint32_t i;
    for(i = 0; i < size; i++)
    {
        while(MAP_SSIBusy(SSI0_BASE));
        MAP_SSIDataPut(SSI0_BASE, data[i]);
    }
}

void spi_ads_read(uint8_t *data, uint32_t size)
{
    uint32_t i;
    for(i = 0; i < size; i++)
    {
        while(MAP_SSIBusy(SSI0_BASE));
        MAP_SSIDataGet(SSI0_BASE, (uint32_t*)&data[i]);
    }
}

void ADS1299_test_signal(void) {
    /*
     * Following page 2 of
     * https://github.com/EEG-System/additional-documents/blob/main/ADS_Documentation_Notes.pdf
     */
    ADS1299_wreg(CONFIG1, 0x96); // DR = 250 SPS
    ADS1299_wreg(CONFIG2, 0xD0); // T.S. Generated Internally.
    ADS1299_wreg(CONFIG3, 0xE0); // Enable Internal reference buffer.

    /* Testing with one channel */
    ADS1299_wreg(CH1SET, 0x65);


}
void ADS1299_short_test(void) {

    //Set device for DR = f_mod/4096
    ADS1299_wreg(CONFIG1, 0x96);
    ADS1299_wreg(CONFIG2, 0xC0);

    //Set all channels to input short
    ADS1299_wreg(CH1SET, 0x01);
    ADS1299_wreg(CH2SET, 0x01);
    ADS1299_wreg(CH3SET, 0x01);
    ADS1299_wreg(CH4SET, 0x01);
    ADS1299_wreg(CH5SET, 0x01);
    ADS1299_wreg(CH6SET, 0x01);
    ADS1299_wreg(CH7SET, 0x01);
    ADS1299_wreg(CH8SET, 0x01);
}
