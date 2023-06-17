
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <webgpu/webgpu.h>

namespace util {

WGPUSurface platform_get_surface(GLFWwindow *window, WGPUInstance ins) {

  auto display = glfwGetX11Display();

  auto x11_window = glfwGetX11Window(window);

  WGPUSurfaceDescriptorFromXlibWindow x11_desc{};
  x11_desc.window = x11_window;
  x11_desc.display = display;
  x11_desc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;

  WGPUSurfaceDescriptor desc{};

  desc.label = "X11 surface";
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&x11_desc);

  return wgpuInstanceCreateSurface(ins, &desc);
}

} // namespace util