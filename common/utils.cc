
#include "utils.hpp"

namespace util {

// implement in platform file
WGPUSurface platform_get_surface(GLFWwindow *window, WGPUInstance ins);

App::App(std::string title, uint32_t width, uint32_t height)
    : m_title(std::move(title)), m_width(width), m_height(height) {}

void App::Run() {
  Init();

  Loop();

  Terminal();
}

void App::Init() {
  // init window
  glfwInit();
  // no need OpenGL api
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // disable resize since recreate pipeline is not ready
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  // window
  m_window =
      glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

  // init wgpu instance
  {
    WGPUInstanceDescriptor desc{};
    m_ins = wgpuCreateInstance(&desc);
  }

  // init wgpu surface from this window
  m_surface = platform_get_surface(m_window, m_ins);

  // request adatper
  {
    WGPURequestAdapterOptions opts{};

    opts.compatibleSurface = m_surface;
    opts.powerPreference = WGPUPowerPreference_Undefined;
    opts.forceFallbackAdapter = false;

    wgpuInstanceRequestAdapter(m_ins, &opts, &RequestAdapterCallback, this);
  }

  // device
  {
    WGPUDeviceDescriptor desc{};
    m_device = wgpuAdapterCreateDevice(m_adapter, &desc);
  }
  // queue
  m_queue = wgpuDeviceGetQueue(m_device);
  // swapchain
  {
    WGPUSwapChainDescriptor desc = {};
    desc.usage = WGPUTextureUsage_RenderAttachment;
    desc.format = WGPUTextureFormat_BGRA8Unorm;
    desc.width = m_width;
    desc.height = m_height;
    desc.presentMode = WGPUPresentMode_Mailbox;

    m_swapchain = wgpuDeviceCreateSwapChain(m_device, m_surface, &desc);
  }

  OnInit();
}

void App::Loop() {
  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    OnLoop();
  }
}

void App::Terminal() {
  OnTerminal();

  // release surface
  wgpuSurfaceRelease(m_surface);
  // release instance
  wgpuInstanceRelease(m_ins);

  glfwDestroyWindow(m_window);

  glfwTerminate();
}

void App::RequestAdapterCallback(WGPURequestAdapterStatus status,
                                 WGPUAdapter adapter, char const *message,
                                 void *userdata) {
  if (status != WGPURequestAdapterStatus_Success) {
    exit(-1);
  }

  auto app = reinterpret_cast<App *>(userdata);

  app->m_adapter = adapter;
}

} // namespace util
