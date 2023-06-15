#include "utils.hpp"
#include <spdlog/spdlog.h>

class RenderPipeline : public util::App {
public:
  RenderPipeline() : util::App("Render Pipeline", 800, 800) {}

  ~RenderPipeline() override = default;

protected:
  void OnInit() override {
    // shader string
    auto raw_shader = ReadFile(ASSET_DIR "/triangle.wgsl");

    // shader module
    WGPUShaderModule shader = nullptr;
    {
      WGPUShaderModuleWGSLDescriptor wgsl_desc{};
      wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
      wgsl_desc.source = raw_shader.c_str();

      WGPUShaderModuleDescriptor desc{};
      desc.label = "vertex shader";

      desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgsl_desc);

      shader = wgpuDeviceCreateShaderModule(GetDevice(), &desc);
    }

    // pipeline layout
    WGPUPipelineLayout layout = nullptr;
    {
      // no binding group in this example
      WGPUPipelineLayoutDescriptor desc{};
      desc.label = "simple pipeline";

      layout = wgpuDeviceCreatePipelineLayout(GetDevice(), &desc);
    }

    {
      WGPURenderPipelineDescriptor desc{};

      desc.layout = layout;

      desc.vertex.module = shader;
      desc.vertex.entryPoint = "vs_main";
      // no buffer
      desc.vertex.bufferCount = 0;
      // no constant
      desc.vertex.constantCount = 0;

      // color target
      // this example only has one target
      WGPUBlendState blend_state{};
      blend_state.alpha.operation = WGPUBlendOperation_Add;
      blend_state.alpha.srcFactor = WGPUBlendFactor_One;
      blend_state.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

      blend_state.color = blend_state.alpha;
      WGPUColorTargetState color_target{};
      color_target.writeMask = WGPUColorWriteMask_All;
      color_target.blend = &blend_state;
      // TODO query swapchain texture format
      color_target.format = WGPUTextureFormat_BGRA8Unorm;

      WGPUFragmentState fs_state{};
      fs_state.module = shader;
      fs_state.entryPoint = "fs_main";
      fs_state.targetCount = 1;
      fs_state.targets = &color_target;

      desc.fragment = &fs_state;

      // primitive
      desc.primitive.cullMode = WGPUCullMode_None;
      desc.primitive.frontFace = WGPUFrontFace_CW;
      desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
      desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

      // msaa state
      desc.multisample.count = 1;
      desc.multisample.mask = 0xffffffff;
      desc.multisample.alphaToCoverageEnabled = false;

      m_pipeline = wgpuDeviceCreateRenderPipeline(GetDevice(), &desc);
    }

    if (m_pipeline == nullptr) {
      spdlog::error("Failed create RenderPipeline");

      exit(-1);
    }

    wgpuShaderModuleRelease(shader);
    wgpuPipelineLayoutRelease(layout);
  }

  void OnLoop() override {
    auto texture_view = wgpuSwapChainGetCurrentTextureView(GetSwapChain());

    WGPURenderPassDescriptor renderpassInfo = {};
    WGPURenderPassColorAttachment colorAttachment = {};
    {
      colorAttachment.view = texture_view;
      colorAttachment.resolveTarget = nullptr;
      colorAttachment.clearValue = {1.f, 1.f, 1.f, 1.f};
      colorAttachment.loadOp = WGPULoadOp_Clear;
      colorAttachment.storeOp = WGPUStoreOp_Store;
      renderpassInfo.colorAttachmentCount = 1;
      renderpassInfo.colorAttachments = &colorAttachment;
      renderpassInfo.depthStencilAttachment = nullptr;
    }

    auto encoder = wgpuDeviceCreateCommandEncoder(GetDevice(), nullptr);

    WGPURenderPassEncoder pass =
        wgpuCommandEncoderBeginRenderPass(encoder, &renderpassInfo);

    {
      // use pipeline to render triangle
      wgpuRenderPassEncoderSetPipeline(pass, m_pipeline);
      // draw
      wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
    }

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);

    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(GetQueue(), 1, &cmd);
    wgpuSwapChainPresent(GetSwapChain());

    wgpuCommandBufferRelease(cmd);
    wgpuTextureViewRelease(texture_view);
  }

  void OnTerminal() override { wgpuRenderPipelineRelease(m_pipeline); }

private:
  WGPURenderPipeline m_pipeline = {};
};

int main(int argc, const char **argv) {
  RenderPipeline app{};

  app.Run();

  return 0;
}