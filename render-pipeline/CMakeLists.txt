add_executable(
        render-pipeline
        main.cc
)

target_compile_definitions(render-pipeline PRIVATE -DASSET_DIR="${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(render-pipeline PRIVATE webgpu util)