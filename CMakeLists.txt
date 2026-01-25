cmake_minimum_required(VERSION 3.12)

# Set system name for cross-compilation (must be before project())
if(NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME Generic)
endif()

# Prevent CMake from adding platform-specific linker flags
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(OpenMRNLite VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_EXAMPLES "Build example applications" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(OPENMRN_ENABLE_BLE "Enable BLE (Bluetooth Low Energy) support" OFF)
option(OPENMRN_ENABLE_DCC "Enable DCC (Digital Command Control) support" OFF)

# ThreadX is the only supported RTOS
message(STATUS "OpenMRNLite: ThreadX-only version")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Require toolchain file for cross-compilation
#if(NOT CMAKE_TOOLCHAIN_FILE)
#    message( "Using local toolchain file: gcc-arm-none-eabi.cmake")
#endif()

# Include directories
set(OPENMRNLITE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Collect all source files
file(GLOB_RECURSE OPENMRNLITE_SOURCES
    src/executor/*.cpp
    src/openlcb/*.cpp
    src/utils/*.cpp
    src/OpenMRNLite.cpp
)

# Add optional BLE support
if(OPENMRN_ENABLE_BLE)
    file(GLOB_RECURSE BLE_SOURCES src/ble/*.cpp)
    list(APPEND OPENMRNLITE_SOURCES ${BLE_SOURCES})
    message(STATUS "BLE support: Enabled")
else()
    message(STATUS "BLE support: Disabled")
endif()

# Add optional DCC support
if(OPENMRN_ENABLE_DCC)
    file(GLOB_RECURSE DCC_SOURCES src/dcc/*.cpp)
    list(APPEND OPENMRNLITE_SOURCES ${DCC_SOURCES})
    message(STATUS "DCC support: Enabled")
else()
    message(STATUS "DCC support: Disabled")
endif()

# ThreadX OS layer sources
file(GLOB OS_SOURCES src/os/*.cpp src/os/threadx_impl.c src/os/os.c)
# Exclude MDNS.cpp and OS.cpp which require full select() support
list(FILTER OS_SOURCES EXCLUDE REGEX ".*/(MDNS|OS)\\.cpp$")
list(APPEND OPENMRNLITE_SOURCES ${OS_SOURCES})

# Collect all header files
file(GLOB_RECURSE OPENMRNLITE_HEADERS
    src/*.h
    src/*.hxx
)

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

# Add optional module definitions
if(OPENMRN_ENABLE_BLE)
    target_compile_definitions(OpenMRNLite PUBLIC OPENMRN_FEATURE_BLE=1)
endif()

if(OPENMRN_ENABLE_DCC)
    target_compile_definitions(OpenMRNLite PUBLIC OPENMRN_FEATURE_DCC=1)
endif()

# ThreadX-specific definitions
target_compile_definitions(OpenMRNLite PUBLIC
    OPENMRN_FEATURE_RTOS_THREADX=1
    USING_THREADX=1
)

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
message(STATUS "  RTOS:              ThreadX (exclusive)")
message(STATUS "  Target system:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Target processor:  ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Toolchain file:    ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  BLE support:       ${OPENMRN_ENABLE_BLE}")
message(STATUS "  DCC support:       ${OPENMRN_ENABLE_DCC}")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
