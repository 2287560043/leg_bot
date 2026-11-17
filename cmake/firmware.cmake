set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(HAL vendor/Drivers/STM32H7xx_HAL_Driver)
set(RTOS vendor/FreeRTOS/Source)
set(HAL_UNITS hal hal_cortex hal_rcc hal_rcc_ex hal_pwr hal_pwr_ex hal_gpio
 hal_flash hal_flash_ex hal_dma hal_dma_ex hal_mdma hal_exti hal_spi hal_spi_ex
 hal_uart hal_uart_ex hal_tim hal_tim_ex hal_i2c hal_i2c_ex hal_hsem)
file(GLOB APP_SOURCES CONFIGURE_DEPENDS App/*.c App/*.cpp)
set(SOURCES Core/Src/main.c Core/Src/system_stm32h7xx.c Core/Src/stm32h7xx_it.c
 Core/Src/stm32h7xx_hal_msp.c Core/Src/hal_timebase.c Core/Src/rtos_support.c Core/Src/imu_port.c Core/Src/sysmem.c
 Core/startup_stm32h723xx.s ${APP_SOURCES}
 ${RTOS}/tasks.c ${RTOS}/queue.c ${RTOS}/list.c ${RTOS}/timers.c
 ${RTOS}/event_groups.c ${RTOS}/stream_buffer.c
 ${RTOS}/portable/GCC/ARM_CM4F/port.c ${RTOS}/CMSIS_RTOS_V2/cmsis_os2.c)
foreach(unit IN LISTS HAL_UNITS)
 list(APPEND SOURCES ${HAL}/Src/stm32h7xx_${unit}.c)
endforeach()
add_executable(leg.elf ${SOURCES} vendor/Fusion/FusionAhrs.c
 vendor/EasyLogger/elog.c vendor/EasyLogger/elog_async.c vendor/EasyLogger/elog_utils.c
 vendor/RTT/SEGGER_RTT.c)
option(LEG_ATTITUDE_BENCHMARK "Run comparison kernels at boot; testing only" OFF)
if(LEG_ATTITUDE_BENCHMARK)
 target_sources(leg.elf PRIVATE tests/attitude_compare.cpp)
 target_compile_definitions(leg.elf PRIVATE LEG_BENCH_EMBEDDED)
endif()
target_include_directories(leg.elf PRIVATE Core/Inc App
 ${HAL}/Inc ${HAL}/Inc/Legacy vendor/Drivers/CMSIS/Include
 vendor/Drivers/CMSIS/Device/ST/STM32H7xx/Include ${RTOS}/include
 ${RTOS}/portable/GCC/ARM_CM4F ${RTOS}/CMSIS_RTOS_V2)
target_include_directories(leg.elf SYSTEM PRIVATE vendor/Fusion vendor/EasyLogger vendor/RTT)
target_compile_definitions(leg.elf PRIVATE FUSION_USE_NORMAL_SQRT STM32H723xx USE_HAL_DRIVER USER_VECT_TAB_ADDRESS HSE_VALUE=24000000U)
set(MCU -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
target_compile_options(leg.elf PRIVATE ${MCU} -Og -g3 -Wall -Wextra -ffunction-sections -fdata-sections -fno-common)
# Keep embedded C++ predictable; these options apply only to C++ sources.
target_compile_options(leg.elf PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>")
target_link_options(leg.elf PRIVATE ${MCU} --specs=nano.specs --specs=nosys.specs
 -T${CMAKE_SOURCE_DIR}/STM32H723.ld -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/leg.map -Wl,--print-memory-usage
 -Wl,-u,_printf_float)
set_property(TARGET leg.elf APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/STM32H723.ld)
target_link_libraries(leg.elf PRIVATE m)
find_program(OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(SIZE arm-none-eabi-size REQUIRED)
add_custom_command(TARGET leg.elf POST_BUILD
 COMMAND ${OBJCOPY} -O ihex $<TARGET_FILE:leg.elf> ${CMAKE_BINARY_DIR}/leg.hex
 COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:leg.elf> ${CMAKE_BINARY_DIR}/leg.bin
 COMMAND ${SIZE} $<TARGET_FILE:leg.elf>)
