cmake_minimum_required(VERSION 3.12)

project(OpenMRN_Example_STM32H563_Nucleo C CXX)

# Set the target name
set(TARGET_NAME stm32h563_nucleo_example)

# Add the main source file
add_executable(${TARGET_NAME} main.c)

# Link against OpenMRNLite static library
# Assumes OpenMRNLite is added as a subdirectory in the top-level CMakeLists.txt
# or this example is built as part of the OpenMRNLite project

target_link_libraries(${TARGET_NAME} PRIVATE OpenMRNLite)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set the MCU and toolchain specifics as needed (example for STM32H563)
# These should be set in your toolchain file or parent CMake project
# set(CMAKE_SYSTEM_PROCESSOR cortex-m33)
# set(CMAKE_SYSTEM_NAME Generic)
# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Optionally, set linker script, startup files, etc.
# target_link_options(${TARGET_NAME} PRIVATE -T${CMAKE_SOURCE_DIR}/STM32H563ZITx_FLASH.ld)

# Example: set RTOS to ThreadX (or FreeRTOS, CMSIS_RTOS_V2)
# set(OPENMRN_RTOS "ThreadX" CACHE STRING "Select RTOS" FORCE)

# Add any STM32 HAL/LL drivers or BSP as needed
# target_link_libraries(${TARGET_NAME} PRIVATE stm32h5xx_hal)

# Post-build: create .bin/.hex if needed
# add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
#     COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.bin
# )
