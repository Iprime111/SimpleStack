cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

project (stack)
add_library (stack STATIC "")

target_include_directories (stack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

add_subdirectory (CustomAssert)
add_subdirectory (src)

target_link_libraries (stack PRIVATE CustomAssert)
