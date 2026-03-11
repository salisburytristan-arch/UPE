add_executable(v0_13_0_showcase main.cpp)
target_link_libraries(v0_13_0_showcase PRIVATE upe)
set_target_properties(v0_13_0_showcase PROPERTIES
    OUTPUT_NAME "v0_13_0_showcase"
    FOLDER "examples"
)
