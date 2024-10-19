# source: https://github.com/espressif/esp-iot-solution/blob/4730d91db70df7e6e0a3191d725ab1c5f98ff9ce/examples/usb/device/usb_webcam/bootloader_components/boot_hooks/CMakeLists.txt
if("$ENV{IDF_TARGET}" STREQUAL "esp32s3")
idf_component_register(SRCS "boot_hooks.c")
endif()

# We need to force GCC to integrate this static library into the
# bootloader link. Indeed, by default, as the hooks in the bootloader are weak,
# the linker would just ignore the symbols in the extra. (i.e. not strictly
# required)
# To do so, we need to define the symbol (function) `bootloader_hooks_include`
# within hooks.c source file.