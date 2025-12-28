# ESP-IDF sample project for building OpenMRNLite as a component
cmake_minimum_required(VERSION 3.16)

# Add the local component directory so ESP-IDF picks up the OpenMRNLite component.
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(openmrnlite_espidf_example)
