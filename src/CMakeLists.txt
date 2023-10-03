add_subdirectory (Stack)
add_subdirectory (SecureStack)

target_link_libraries (simple_stack PUBLIC CustomAssert)
target_link_libraries (simple_stack PUBLIC stack)
