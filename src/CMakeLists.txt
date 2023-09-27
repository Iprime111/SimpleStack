add_subdirectory (Stack)
add_subdirectory (SecureStack)

add_executable (simple_stack main.cpp)
target_link_libraries (simple_stack PUBLIC CustomAssert)
target_link_libraries (simple_stack PUBLIC stack)
