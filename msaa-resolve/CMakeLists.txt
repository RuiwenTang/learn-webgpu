add_executable(
        msaa-resolve
        main.cc
)

target_compile_definitions(msaa-resolve PRIVATE -DASSET_DIR="${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(msaa-resolve PRIVATE webgpu util)