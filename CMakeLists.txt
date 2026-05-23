cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(pico_tinyusb_hid_absolute_mouse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico_absolute_mouse
    src/main.c
    src/usb_descriptors.c
)

target_include_directories(pico_absolute_mouse PRIVATE src)

target_link_libraries(pico_absolute_mouse
    pico_stdlib
    hardware_adc
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(pico_absolute_mouse 0)
pico_enable_stdio_uart(pico_absolute_mouse 0)
pico_add_extra_outputs(pico_absolute_mouse)
