# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(drivers
	VERSION 0.0.1
	DESCRIPTION "picocalc drivers for lcd, keyboard"
	)

add_library(drivers INTERFACE)

target_sources(drivers INTERFACE
	keyboard.c
	lcd.c
	term.c
	draw.c
	fs.c
	multicore.c
	)

target_link_libraries(drivers INTERFACE
	pico_stdlib
	hardware_spi
	hardware_i2c
	rp2040-psram
)

target_include_directories(drivers INTERFACE ${CMAKE_CURRENT_LIST_DIR})
