
add_executable(
        depth-buffer
        main.cc
)

target_compile_definitions(depth-buffer PRIVATE -DASSET_DIR="${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(depth-buffer PRIVATE webgpu util)