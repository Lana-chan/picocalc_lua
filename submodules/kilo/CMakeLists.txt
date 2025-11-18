add_library(kilo "")

target_sources(kilo
    PUBLIC
    kilo.c
)

target_link_libraries(kilo 
	pico_stdlib
	pico_multicore
)

target_include_directories(kilo
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ../pico_fatfs/fatfs
    ../pico_fatfs/fatfs/conf
    ../../drivers
    ../lua-5.4.8
    ../../
)
