# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build
ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.c
					${CMAKE_CURRENT_BINARY_DIR}/_version.c
	COMMAND ${CMAKE_COMMAND} -P
					${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(picolua)
pico_sdk_init()

add_subdirectory(submodules/lua-5.4.8)
add_subdirectory(submodules/pico_fatfs)
add_subdirectory(submodules/rp2040-psram)
add_subdirectory(submodules/kilo)
add_subdirectory(drivers)
add_subdirectory(picolua-api)

add_executable(picolua
	picolua.c
	corelua.c
	${CMAKE_CURRENT_BINARY_DIR}/version.c
	#lua_wrapper.c
	#fs_wrapper.c
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
	hardware_clocks
	#hardware_flash
	#hardware_adc
	#hardware_exception
	#hardware_pio
	pico_multicore
	lua 
	drivers
	pico_fatfs
	kilo
	picolua-api
)

target_compile_definitions(picolua PRIVATE
	# rp2040-psram
	#PSRAM_MUTEX=1
	PSRAM_SPINLOCK=1
	PSRAM_ASYNC=1
	PSRAM_PIN_CS=20
	PSRAM_PIN_SCK=21
	PSRAM_PIN_MOSI=2
	PSRAM_PIN_MISO=3
)

pico_add_extra_outputs(picolua)

add_custom_target(flash
		COMMAND cp picolua.uf2 /run/media/favre/RP2350/
		DEPENDS picolua
)
