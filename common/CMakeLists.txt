

find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(util
  utils.cc
  utils.hpp
)

if(APPLE)
  target_sources(util PUBLIC ${CMAKE_CURRENT_LIST_DIR}/util_platform_darwin.mm)
elseif(WIN32)
  # TODO
else()
  target_sources(util PUBLIC ${CMAKE_CURRENT_LIST_DIR}/util_platform_linux.cc)
endif()

target_include_directories(util PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(util PUBLIC glfw webgpu spdlog::spdlog glm::glm)
