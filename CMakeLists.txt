cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(picolua)
pico_sdk_init()

add_subdirectory(lua-5.4.6/src)
add_subdirectory(drivers)

add_executable(picolua
  picolua.c
  lua_wrapper.c
  #main.c
)

pico_enable_stdio_usb(picolua 0)
pico_enable_stdio_uart(picolua 0)

target_link_libraries(picolua 
  pico_stdlib 
  hardware_irq
  hardware_pwm
  hardware_i2c
  hardware_spi
  hardware_dma
  lua 
  drivers
  hardware_flash
  hardware_irq
  hardware_adc
  hardware_exception
  hardware_pio
  pico_multicore
  )

pico_add_extra_outputs(picolua)
