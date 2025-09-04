// source: https://github.com/espressif/esp-iot-solution/blob/4730d91db70df7e6e0a3191d725ab1c5f98ff9ce/examples/usb/device/usb_webcam/bootloader_components/boot_hooks/boot_hooks.c

#ifdef CONFIG_GENERAL_INCLUDE_UVC_MODE
#include "esp_log.h"
#include "soc/rtc_cntl_struct.h"
#include "soc/usb_serial_jtag_reg.h"

/* Function used to tell the linker to include this file
 * with all its symbols.
 */

void bootloader_hooks_include(void)
{
}

void bootloader_before_init(void)
{

  // Disable D+ pullup, to prevent the USB host from retrieving USB-Serial-JTAG's descriptor.
  SET_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_PAD_PULL_OVERRIDE);
  CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_DP_PULLUP);
  CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
}
#endif