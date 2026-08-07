add_executable(picopen_bringup
    src/console.c
    src/main.c
)

target_include_directories(picopen_bringup PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(picopen_bringup PRIVATE
    PICOPEN_VERSION="${PROJECT_VERSION}"
    PICOPEN_BUILD_TYPE="$<CONFIG>"
)

target_compile_options(picopen_bringup PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(picopen_bringup PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    hardware_watchdog
)

# USB stdio gives the untouched-board image a console without assigning any
# PicoCalc peripheral GPIO. UART remains off until the PicoCalc routing and DIP
# switch state are verified.
pico_enable_stdio_usb(picopen_bringup 1)
pico_enable_stdio_uart(picopen_bringup 0)

pico_set_program_name(picopen_bringup "PicoPen Bring-up")
pico_set_program_version(picopen_bringup "${PROJECT_VERSION}")
pico_set_program_description(picopen_bringup
    "Safe first-boot diagnostic for PicoCalc with Pico 2 W")

pico_add_extra_outputs(picopen_bringup)
