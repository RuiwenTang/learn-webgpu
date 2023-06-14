
#include <GLFW/glfw3.h>
#include <string>

#include <webgpu/webgpu.h>

namespace util {

class App {
public:
  App(std::string title, uint32_t width, uint32_t height);

  virtual ~App() = default;

  void Run();

protected:
  virtual void OnInit() = 0;

  virtual void OnLoop() = 0;

  virtual void OnTerminal() = 0;

  WGPUInstance GetInstance() const { return m_ins; }

  WGPUAdapter GetAdapter() const { return m_adapter; }

  WGPUSurface GetSurface() const { return m_surface; }

private:
  void Init();

  void Loop();

  void Terminal();

  static void RequestAdapterCallback(WGPURequestAdapterStatus status,
                                     WGPUAdapter adapter, char const *message,
                                     void *userdata);

private:
  std::string m_title;
  uint32_t m_width;
  uint32_t m_height;
  GLFWwindow *m_window = nullptr;
  WGPUInstance m_ins = nullptr;
  WGPUSurface m_surface = nullptr;
  WGPUAdapter m_adapter = nullptr;
};

} // namespace util
