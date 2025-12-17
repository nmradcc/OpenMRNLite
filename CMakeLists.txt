cmake_minimum_required(VERSION 3.12)

project(OpenMRNLite VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_EXAMPLES "Build example applications" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# RTOS Selection (only one should be enabled)
set(OPENMRN_RTOS "FreeRTOS" CACHE STRING "Select RTOS: FreeRTOS, ThreadX, or CMSIS_RTOS_V2")
set_property(CACHE OPENMRN_RTOS PROPERTY STRINGS "FreeRTOS" "ThreadX" "CMSIS_RTOS_V2")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Require embedded target (cross-compilation)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    message(FATAL_ERROR "OpenMRNLite only supports embedded targets. Please use a toolchain file with CMAKE_SYSTEM_NAME=Generic")
endif()

# Require toolchain file for cross-compilation
if(NOT CMAKE_TOOLCHAIN_FILE)
    message(WARNING "No toolchain file specified. Cross-compilation may not work correctly without CMAKE_TOOLCHAIN_FILE.")
endif()

# Include directories
set(OPENMRNLITE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Collect all source files
file(GLOB_RECURSE OPENMRNLITE_SOURCES
    src/ble/*.cpp
    src/dcc/*.cpp
    src/executor/*.cpp
    src/openlcb/*.cpp
    src/utils/*.cpp
    src/OpenMRNLite.cpp
)

# Add platform-specific drivers only for FreeRTOS (they depend on FreeRTOS features)
if(OPENMRN_RTOS STREQUAL "FreeRTOS")
    file(GLOB_RECURSE FREERTOS_DRIVER_SOURCES
        src/freertos_drivers/common/*.cpp
        src/freertos_drivers/arduino/*.cpp
        src/freertos_drivers/sam/*.cpp
        src/freertos_drivers/stm32/*.cpp
    )
    list(APPEND OPENMRNLITE_SOURCES ${FREERTOS_DRIVER_SOURCES})
endif()

# Add OS layer sources based on RTOS selection
if(OPENMRN_RTOS STREQUAL "ThreadX")
    # For now, ThreadX uses fake mutex/sem implementation, so don't include threadx_impl.c
    # Also exclude MDNS.cpp and OSSelectWakeup.cpp which require full OS support
    file(GLOB OS_SOURCES src/os/*.cpp)
    list(FILTER OS_SOURCES EXCLUDE REGEX ".*/(MDNS|OSSelectWakeup|OS)\\.cpp$")
elseif(OPENMRN_RTOS STREQUAL "CMSIS_RTOS_V2")
    file(GLOB OS_SOURCES src/os/*.cpp src/os/cmsis_rtos2_impl.c)
    list(FILTER OS_SOURCES EXCLUDE REGEX ".*/(MDNS|OSSelectWakeup|OS)\\.cpp$")
else() # FreeRTOS (default)
    file(GLOB OS_SOURCES src/os/*.cpp)
endif()

list(APPEND OPENMRNLITE_SOURCES ${OS_SOURCES})

# Collect all header files
file(GLOB_RECURSE OPENMRNLITE_HEADERS
    src/*.h
    src/*.hxx
)

# Exclude platform-specific C files that may not compile on all platforms
list(FILTER OPENMRNLITE_SOURCES EXCLUDE REGEX ".*os/os\\.c$")

# Create the library
add_library(OpenMRNLite ${OPENMRNLITE_SOURCES})

# Set include directories for the library
target_include_directories(OpenMRNLite
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Compiler flags
if(MSVC)
    target_compile_options(OpenMRNLite PRIVATE /W4)
else()
    target_compile_options(OpenMRNLite PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
endif()

# Compiler definitions
target_compile_definitions(OpenMRNLite PUBLIC
    $<$<CONFIG:Debug>:DEBUG>
)

# Add RTOS-specific definitions
if(OPENMRN_RTOS STREQUAL "ThreadX")
    target_compile_definitions(OpenMRNLite PUBLIC
        OPENMRN_FEATURE_RTOS_THREADX=1
        USING_THREADX=1
    )
    message(STATUS "RTOS: ThreadX")
    
    # Link to stm32cubemx target if it exists (provides ThreadX headers)
    if(TARGET stm32cubemx)
        # Get include directories from stm32cubemx without creating a link dependency
        get_target_property(STM32CUBEMX_INCLUDES stm32cubemx INTERFACE_INCLUDE_DIRECTORIES)
        if(STM32CUBEMX_INCLUDES)
            target_include_directories(OpenMRNLite PRIVATE ${STM32CUBEMX_INCLUDES})
            message(STATUS "  Using stm32cubemx includes for ThreadX")
        endif()
        
        # Get compile definitions from stm32cubemx
        get_target_property(STM32CUBEMX_DEFINES stm32cubemx INTERFACE_COMPILE_DEFINITIONS)
        if(STM32CUBEMX_DEFINES)
            target_compile_definitions(OpenMRNLite PRIVATE ${STM32CUBEMX_DEFINES})
        endif()
    endif()
elseif(OPENMRN_RTOS STREQUAL "CMSIS_RTOS_V2")
    target_compile_definitions(OpenMRNLite PUBLIC
        OPENMRN_FEATURE_RTOS_CMSIS_V2=1
        USING_CMSIS_RTOS_V2=1
    )
    message(STATUS "RTOS: CMSIS-RTOS v2")
    
    # Link to stm32cubemx target if it exists (provides CMSIS-RTOS v2 headers)
    if(TARGET stm32cubemx)
        # Get include directories from stm32cubemx without creating a link dependency
        get_target_property(STM32CUBEMX_INCLUDES stm32cubemx INTERFACE_INCLUDE_DIRECTORIES)
        if(STM32CUBEMX_INCLUDES)
            target_include_directories(OpenMRNLite PRIVATE ${STM32CUBEMX_INCLUDES})
            message(STATUS "  Using stm32cubemx includes for CMSIS-RTOS v2")
        endif()
        
        # Get compile definitions from stm32cubemx
        get_target_property(STM32CUBEMX_DEFINES stm32cubemx INTERFACE_COMPILE_DEFINITIONS)
        if(STM32CUBEMX_DEFINES)
            target_compile_definitions(OpenMRNLite PRIVATE ${STM32CUBEMX_DEFINES})
        endif()
    endif()
else() # FreeRTOS (default)
    target_compile_definitions(OpenMRNLite PUBLIC
        OPENMRN_FEATURE_RTOS_FREERTOS=1
        __FreeRTOS__=1
    )
    message(STATUS "RTOS: FreeRTOS (default)")
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS OpenMRNLite
    EXPORT OpenMRNLiteTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers preserving directory structure
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenMRNLite
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx"
)

# Export targets
install(EXPORT OpenMRNLiteTargets
    FILE OpenMRNLiteTargets.cmake
    NAMESPACE OpenMRNLite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenMRNLite
)

# Create and install package configuration files
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OpenMRNLiteConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenMRNLiteConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenMRNLiteConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenMRNLite
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OpenMRNLiteConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenMRNLiteConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenMRNLite
)

# Build examples if requested
if(BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "OpenMRNLite Configuration Summary:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  RTOS:              ${OPENMRN_RTOS}")
message(STATUS "  Target system:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Target processor:  ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Toolchain file:    ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
