cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

project (Stack)
add_library (Stack STATIC "")

target_include_directories (Stack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

add_subdirectory (ColorConsole)
add_subdirectory (CustomAssert)
add_subdirectory (src)

target_link_libraries (Stack PRIVATE CustomAssert)
target_link_libraries (Stack PRIVATE ColorConsole)
