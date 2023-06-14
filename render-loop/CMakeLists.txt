add_executable(
  render-loop
  main.cc
)

target_link_libraries(render-loop PRIVATE webgpu util)