
#include <cstdlib>

#include <webgpu/webgpu.h>

#include "utils.hpp"
#include <spdlog/spdlog.h>

class HelloInstance : public util::App {
public:
  HelloInstance() : util::App("hello instance", 800, 800) {}

  ~HelloInstance() override = default;

protected:
  void OnInit() override {

    WGPUAdapterProperties props{};

    wgpuAdapterGetProperties(GetAdapter(), &props);

    spdlog::info("adapter vender name: {}", props.vendorName);
    spdlog::info("adapter description: {}", props.driverDescription);
  }

  void OnLoop() override {}

  void OnTerminal() override {}
};

int main(int argc, const char **argv) {
  HelloInstance app{};

  app.Run();

  return 0;
}
