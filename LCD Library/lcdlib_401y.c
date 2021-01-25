#include <lcdlib_401y.h>

#define LOWNIB(x)  GPIOK->DATA = (GPIOK->DATA & 0xF0) + (x & 0x0F)

void lcdInit(){
    delay_ms(100);
    // Wait for 100ms after power is applied.
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R9; //Enable port K for GPIO
    GPIOK->AFSEL &= ~(RS + EN + LCD_DATA);
    GPIOK->DIR |= RS + EN + LCD_DATA; //Make pins outputs
    GPIOK->DEN |= RS + EN + LCD_DATA; //Enable digital I/O

    GPIOK->DATA = LCD_CLEAR; //Clear outputs

    GPIOK->DATA = LCD_START; //Start LCD (send 0x03)

    lcdTriggerEN(); // Send 0x03 3 times at 5ms then 100 us
    delay_ms(5);
    lcdTriggerEN();
    delay_ms(5);
    lcdTriggerEN();
    delay_ms(5);

    GPIOK->DATA = LCD_4BIT_MODE; //Switch to 4-bit mode
    lcdTriggerEN();
    delay_ms(5);

    lcdWriteCmd(0x28); // 4-bit, 2 line, 5x8
    lcdWriteCmd(0x08); // Instruction Flow
    lcdWriteCmd(0x01); // Clear LCD
    lcdWriteCmd(0x06); // Auto-Increment
    lcdWriteCmd(0x0C); // Display On, No blink

}

void lcdTriggerEN(){
    GPIOK->DATA |= EN;
    GPIOK->DATA &= ~EN;
}

void lcdWriteData(uint8_t data){
    GPIOK->DATA |= RS; //Set RS to Data
    LOWNIB(data >> 4); //Upper nibble
    lcdTriggerEN();
    LOWNIB(data); //Lower nibble
    lcdTriggerEN();
    delay_us(50); //Delay > 47 us
}

void lcdWriteCmd(uint8_t cmd){
    GPIOK->DATA &= ~RS; //Set RS to Command
    LOWNIB(cmd >> 4); // Upper nibble
    lcdTriggerEN();
    LOWNIB(cmd); // Lower nibble
    lcdTriggerEN();
    delay_ms(5); // Delay > 1.5ms
}

void lcdSetText(char* text, uint32_t x, uint32_t y) {
    int i;
    if (x < 16) {
        x |= 0x80; // Set LCD for first line write
        switch (y){
        case 1:
            x |= 0x40; // Set LCD for second line write
            break;
        case 2:
            x |= 0x60; // Set LCD for first line write reverse
            break;
        case 3:
            x |= 0x20; // Set LCD for second line write reverse
            break;
        }
        lcdWriteCmd(x);
    }
    i = 0;

    while (text[i] != '\0') {
        lcdWriteData(text[i]);
        i++;
    }
}

void lcdSetInt(uint32_t val, uint32_t x, uint32_t y){
    char number_string[16];
    sprintf(number_string, "%d", val); // Convert the integer to character string
    lcdSetText(number_string, x, y);
}

void lcdClear(){
    lcdWriteCmd(CLEAR);
}







