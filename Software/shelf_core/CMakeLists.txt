# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components/common/posix_compat)
project(shelf_core)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "tmp/root_cert_auth.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "tmp/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "tmp/client.key" TEXT)