add_executable(v0_14_0_xpbd_demo main.cpp)
target_link_libraries(v0_14_0_xpbd_demo PRIVATE upe)
set_target_properties(v0_14_0_xpbd_demo PROPERTIES
    OUTPUT_NAME "v0_14_0_xpbd_demo"
    FOLDER "examples"
)
