# OpenMRNLite component for ESP-IDF
# CMAKE_CURRENT_LIST_DIR is examples/esp-idf/components/openmrnlite; go four levels
# up to the repository root where src/ lives.
get_filename_component(OPENMRNLITE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../.. ABSOLUTE)

if(NOT IS_DIRECTORY ${OPENMRNLITE_ROOT}/src)
    message(FATAL_ERROR "Expected OpenMRNLite sources at ${OPENMRNLITE_ROOT}/src; adjust OPENMRNLITE_ROOT if your tree is laid out differently.")
endif()

file(GLOB_RECURSE OPENMRNLITE_SOURCES
    ${OPENMRNLITE_ROOT}/src/ble/*.cpp
    ${OPENMRNLITE_ROOT}/src/dcc/*.cpp
    ${OPENMRNLITE_ROOT}/src/executor/*.cpp
    ${OPENMRNLITE_ROOT}/src/openlcb/*.cpp
    ${OPENMRNLITE_ROOT}/src/utils/*.cpp
    ${OPENMRNLITE_ROOT}/src/OpenMRNLite.cpp
)

file(GLOB FREERTOS_DRIVER_SOURCES
    ${OPENMRNLITE_ROOT}/src/freertos_drivers/common/*.cpp
    ${OPENMRNLITE_ROOT}/src/freertos_drivers/arduino/*.cpp
    ${OPENMRNLITE_ROOT}/src/freertos_drivers/esp32/*.cpp)
list(APPEND OPENMRNLITE_SOURCES ${FREERTOS_DRIVER_SOURCES})

file(GLOB OS_SOURCES
    ${OPENMRNLITE_ROOT}/src/os/*.cpp
    ${OPENMRNLITE_ROOT}/src/os/freertos_impl.c
)
list(APPEND OPENMRNLITE_SOURCES ${OS_SOURCES})

idf_component_register(
    SRCS ${OPENMRNLITE_SOURCES}
    INCLUDE_DIRS ${OPENMRNLITE_ROOT}/src
    PRIV_REQUIRES freertos lwip
)

target_compile_definitions(${COMPONENT_TARGET} PUBLIC __FreeRTOS__=1 OPENMRN_FEATURE_RTOS_FREERTOS=1)
target_compile_options(${COMPONENT_TARGET} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)
target_compile_features(${COMPONENT_TARGET} PUBLIC cxx_std_14)
