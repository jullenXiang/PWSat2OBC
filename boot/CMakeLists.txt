set(NAME boot)

set(SOURCES
    background.cpp
    comms.cpp
    flash.cpp
    main.cpp
    xmodem.cpp
    fault_handlers.cpp
    boot.cpp
    error_counter.cpp
    dummy_os.cpp
    
    commands/test.cpp
    commands/booting.cpp
    commands/settings.cpp
    commands/upload.cpp
    commands/boot_table.cpp
    commands/check.cpp
)

add_executable(${NAME} ${SOURCES})

set_target_properties(${NAME} PROPERTIES LINK_FLAGS "-T ${LOWER_LINKER_SCRIPT} -L ${MCU_PLATFORM_PATH} -Wl,--check-sections -specs=nano.specs -specs=nosys.specs")

target_link_libraries(${NAME}
      platform
      pld_platform
      emlib
      bspLib
      assert
      base
      boot_params
      fm25w
      gpio
      boot_settings
      program_flash
      error_counter
      version
      standalone_flash_driver
      standalone_spi
)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)

target_generate_padded_bin(${NAME} 65536 0xFF)

target_memory_report(${NAME})

target_asm_listing(${NAME})

target_jlink_flash(${NAME} ${LOWER_ADDRESS})

target_format_sources(${NAME} "${SOURCES}")

set_target_properties(${NAME} PROPERTIES COMPILE_FLAGS "\
    -Wno-unused-parameter \
    -Wno-unused-but-set-variable \
")

target_eclipse_debug_configs(${NAME} Run Attach)