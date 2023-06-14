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
  void OnInit() override {
    InitDevice();
    InitSwapChain();

    m_queue = wgpuDeviceGetQueue(m_device);
  }

  void OnLoop() override {
    // acquire current texture
    auto texture_view = wgpuSwapChainGetCurrentTextureView(m_swapchain);

    WGPURenderPassDescriptor renderpassInfo = {};
    WGPURenderPassColorAttachment colorAttachment = {};
    {
      colorAttachment.view = texture_view;
      colorAttachment.resolveTarget = nullptr;
      colorAttachment.clearValue = {1.0f, 0.0f, 0.0f, 1.0f};
      colorAttachment.loadOp = WGPULoadOp_Clear;
      colorAttachment.storeOp = WGPUStoreOp_Store;
      renderpassInfo.colorAttachmentCount = 1;
      renderpassInfo.colorAttachments = &colorAttachment;
      renderpassInfo.depthStencilAttachment = nullptr;
    }

    auto encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    WGPURenderPassEncoder pass =
        wgpuCommandEncoderBeginRenderPass(encoder, &renderpassInfo);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(m_queue, 1, &cmd);
    wgpuSwapChainPresent(m_swapchain);

    wgpuCommandBufferRelease(cmd);
    wgpuTextureViewRelease(texture_view);
  }

  void OnTerminal() override {
    if (m_swapchain) {
      wgpuSwapChainRelease(m_swapchain);
    }

    if (m_device) {
      wgpuDeviceRelease(m_device);
    }
  }

private:
  void InitDevice() {
    WGPUDeviceDescriptor desc{};

    m_device = wgpuAdapterCreateDevice(GetAdapter(), &desc);
    // clear terminal warning
    wgpuDeviceSetDeviceLostCallback(m_device, nullptr, nullptr);

    if (m_device == nullptr) {
      spdlog::error("Failed create wgpu device");
      exit(-1);
    }

    auto count = wgpuDeviceEnumerateFeatures(m_device, nullptr);

    std::vector<WGPUFeatureName> features{count};

    wgpuDeviceEnumerateFeatures(m_device, features.data());

    spdlog::info("device feature count: {}", count);

    for (auto const &feature : features) {
      spdlog::info("device has feature: {:x}", feature);
    }
  }
  void InitSwapChain() {
    WGPUSwapChainDescriptor desc = {};
    desc.usage = WGPUTextureUsage_RenderAttachment;
    desc.format = WGPUTextureFormat_BGRA8Unorm;
    desc.width = 800;
    desc.height = 800;
    desc.presentMode = WGPUPresentMode_Mailbox;

    m_swapchain = wgpuDeviceCreateSwapChain(m_device, GetSurface(), &desc);

    if (m_swapchain == nullptr) {
      spdlog::error("Failed create webgpu swapchain");
      exit(-1);
    }
  }

private:
  WGPUDevice m_device = {};
  WGPUSwapChain m_swapchain = {};
  WGPUQueue m_queue = {};
};

int main(int argc, const char **argv) {
  RenderLoop app{};

  app.Run();

  return 0;
}