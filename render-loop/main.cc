#include "utils.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>
#include <vector>
#include <webgpu/webgpu.h>

class RenderLoop : public util::App {
public:
  RenderLoop() : util::App("Render Loop", 800, 800) {}

  ~RenderLoop() override = default;

protected:
  void OnInit() override {}

  void OnLoop() override {
    // begin render pass
    // acquire current texture
    auto texture_view = wgpuSwapChainGetCurrentTextureView(GetSwapChain());

    WGPURenderPassDescriptor renderpassInfo = {};
    WGPURenderPassColorAttachment colorAttachment = {};
    {
      colorAttachment.view = texture_view;
      colorAttachment.resolveTarget = nullptr;
      colorAttachment.clearValue = {1.f, 0.f, 0.f, 1.f};
      colorAttachment.loadOp = WGPULoadOp_Clear;
      colorAttachment.storeOp = WGPUStoreOp_Store;
      renderpassInfo.colorAttachmentCount = 1;
      renderpassInfo.colorAttachments = &colorAttachment;
      renderpassInfo.depthStencilAttachment = nullptr;
    }

    auto encoder = wgpuDeviceCreateCommandEncoder(GetDevice(), nullptr);

    WGPURenderPassEncoder pass =
        wgpuCommandEncoderBeginRenderPass(encoder, &renderpassInfo);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(GetQueue(), 1, &cmd);
    wgpuSwapChainPresent(GetSwapChain());

    wgpuCommandBufferRelease(cmd);
    wgpuTextureViewRelease(texture_view);
  }

  void OnTerminal() override {}
};

int main(int argc, const char **argv) {
  RenderLoop app{};

  app.Run();

  return 0;
}