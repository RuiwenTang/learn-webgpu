
#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <webgpu/webgpu.h>

namespace util {

WGPUSurface platform_get_surface(GLFWwindow *window, WGPUInstance ins) {
  NSWindow *ns_window = glfwGetCocoaWindow(window);
  [ns_window.contentView setWantsLayer:YES];
  CAMetalLayer *metal_layer = [CAMetalLayer layer];
  [ns_window.contentView setLayer:metal_layer];

  WGPUSurfaceDescriptorFromMetalLayer metal_desc{};
  metal_desc.layer = metal_layer;
  metal_desc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
  metal_desc.chain.next = nullptr;

  WGPUSurfaceDescriptor desc{};
  desc.label = "WGPU Surface";
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&metal_desc);

  return wgpuInstanceCreateSurface(ins, &desc);
}

} // namespace util
