# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(modules
	VERSION 0.0.1
	DESCRIPTION "picocalc lua interface modules"
	)

add_library(modules INTERFACE)

target_sources(modules INTERFACE
	modules.c
	sys.c
	fs.c
	draw.c
	term.c
	)

target_link_libraries(modules INTERFACE  pico_stdlib hardware_spi hardware_i2c)

target_include_directories(modules INTERFACE ${CMAKE_CURRENT_LIST_DIR})
