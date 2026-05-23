cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(gt_arcade_pico_pro_v3 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

function(add_gun TARGET PLAYER_ID PRODUCT_NAME)
    add_executable(${TARGET}
        src/main.c
        src/usb_descriptors.c
        src/calibration.c
    )
    target_compile_definitions(${TARGET} PRIVATE
        PLAYER_ID=${PLAYER_ID}
        USB_PRODUCT_STRING="${PRODUCT_NAME}"
    )
    target_include_directories(${TARGET} PRIVATE src)
    target_link_libraries(${TARGET}
        pico_stdlib
        pico_unique_id
        hardware_adc
        hardware_flash
        tinyusb_device
        tinyusb_board
    )
    pico_enable_stdio_usb(${TARGET} 0)
    pico_enable_stdio_uart(${TARGET} 0)
    pico_add_extra_outputs(${TARGET})
endfunction()

add_gun(gt_arcade_pico_p1_pro_v3 1 "GT Arcade Pico P1 Pro Gun")
add_gun(gt_arcade_pico_p2_pro_v3 2 "GT Arcade Pico P2 Pro Gun")
