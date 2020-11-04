#ifndef USB_SERIAL_H_
#define USB_SERIAL_H_

#include <stdint.h>

void usb_init(uint32_t g_ui32SysClock);
uint32_t usb_read(uint8_t *data, uint32_t size);
void usb_write(uint8_t *data, uint32_t size);

#endif /* USB_SERIAL_H_ */
