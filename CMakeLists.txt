cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(gt_arcade_pico_pro_v32 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

foreach(PLAYER_NUM 1 2)
  set(TARGET_NAME gt_arcade_pico_p${PLAYER_NUM}_pro_v32)
  add_executable(${TARGET_NAME}
    src/main.c
    src/usb_descriptors.c
  )
  target_compile_definitions(${TARGET_NAME} PRIVATE PLAYER_NUM=${PLAYER_NUM})
  target_include_directories(${TARGET_NAME} PRIVATE src)
  target_link_libraries(${TARGET_NAME}
    pico_stdlib
    hardware_adc
    hardware_flash
    tinyusb_device
    tinyusb_board
  )
  pico_add_extra_outputs(${TARGET_NAME})
endforeach()
