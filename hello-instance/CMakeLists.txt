

add_executable(
  hello-instance
  main.cc
)


target_link_libraries(hello-instance PRIVATE webgpu util)