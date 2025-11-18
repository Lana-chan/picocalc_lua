# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(picolua-api
	VERSION 0.0.1
	DESCRIPTION "picocalc lua interface modules"
)

add_library(picolua-api INTERFACE)

target_sources(picolua-api INTERFACE
	modules.c
	sys.c
	fs.c
	draw.c
	term.c
	sound.c
)

target_link_libraries(picolua-api INTERFACE
	pico_stdlib
	pico_multicore
	hardware_spi
	hardware_i2c
)

target_include_directories(picolua-api INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
)
