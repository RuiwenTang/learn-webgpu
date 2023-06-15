
#include <GLFW/glfw3.h>
#include <string>

#include <glm/glm.hpp>
#include <webgpu/webgpu.h>

namespace util {

class App {
public:
  App(std::string title, uint32_t width, uint32_t height);

  virtual ~App() = default;

  void Run();

  static std::string ReadFile(std::string path);

protected:
  virtual void OnInit() = 0;

  virtual void OnLoop() = 0;

  virtual void OnTerminal() = 0;

  WGPUInstance GetInstance() const { return m_ins; }

  WGPUAdapter GetAdapter() const { return m_adapter; }

  WGPUSurface GetSurface() const { return m_surface; }

  WGPUDevice GetDevice() const { return m_device; }

  WGPUQueue GetQueue() const { return m_queue; }

  WGPUSwapChain GetSwapChain() const { return m_swapchain; }

private:
  void Init();

  void Loop();

  void Terminal();

  static void RequestAdapterCallback(WGPURequestAdapterStatus status,
                                     WGPUAdapter adapter, char const *message,
                                     void *userdata);

  static void DeviceLogCallback(WGPULoggingType type, char const *message,
                                void *userdata);

  static void DeviceErrorCallback(WGPUErrorType type, char const *message,
                                  void *userdata);

private:
  std::string m_title;
  uint32_t m_width;
  uint32_t m_height;
  GLFWwindow *m_window = nullptr;
  WGPUInstance m_ins = nullptr;
  WGPUSurface m_surface = nullptr;
  WGPUAdapter m_adapter = nullptr;
  WGPUDevice m_device = nullptr;
  WGPUQueue m_queue = nullptr;
  WGPUSwapChain m_swapchain = nullptr;
};

} // namespace util
