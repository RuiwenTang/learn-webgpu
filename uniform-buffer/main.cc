
#include "utils.hpp"

#include <array>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <webgpu/webgpu.h>

class UniformBuffer : public util::App {
public:
  UniformBuffer() : util::App("Uniform Buffer", 800, 800) {}

  ~UniformBuffer() override = default;

protected:
  void OnInit() override {
    InitBuffers();
    InitPipeline();
  }

  void OnLoop() override {
    auto texture_view = wgpuSwapChainGetCurrentTextureView(GetSwapChain());

    auto encoder = wgpuDeviceCreateCommandEncoder(GetDevice(), nullptr);

    auto render_pass = BeginRenderPass(texture_view, encoder);

    Draw(render_pass);

    wgpuRenderPassEncoderEnd(render_pass);

    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);

    wgpuRenderPassEncoderRelease(render_pass);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(GetQueue(), 1, &cmd);
    wgpuSwapChainPresent(GetSwapChain());

    wgpuCommandBufferRelease(cmd);
    wgpuTextureViewRelease(texture_view);
  }

  void OnTerminal() override {
    wgpuRenderPipelineRelease(m_pipeline);
    wgpuBindGroupLayoutRelease(m_bind0_layout);
    wgpuPipelineLayoutRelease(m_layout);
    wgpuBufferRelease(m_vertex_buffer);
    wgpuBufferRelease(m_uniform_buffer);
  }

  void InitBuffers() {
    // vertex buffer
    {
      std::vector<float> vertex_data{
          0.f,   0.5f,  1.f, 0.f, 0.f, // x, y, r, g, b
          -0.5f, -0.5f, 0.f, 1.f, 0.f, // x, y, r, g, b
          0.5f,  -0.5f, 0.f, 0.f, 1.f, // x, y, r, g, b
      };

      WGPUBufferDescriptor desc{};
      desc.label = "Vertex buffer";
      desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
      desc.size = vertex_data.size() * sizeof(float);

      m_vertex_buffer = wgpuDeviceCreateBuffer(GetDevice(), &desc);

      wgpuQueueWriteBuffer(GetQueue(), m_vertex_buffer, 0, vertex_data.data(),
                           vertex_data.size() * sizeof(float));
    }

    // uniform buffer
    {
      WGPUBufferDescriptor desc{};
      desc.label = "Vertex buffer";
      desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
      desc.size = sizeof(glm::mat4);

      m_uniform_buffer = wgpuDeviceCreateBuffer(GetDevice(), &desc);
    }
  }

  void InitPipeline() {
    // shader
    auto raw_shader = ReadFile(ASSET_DIR "/buffer.wgsl");
    // shader module
    WGPUShaderModule shader = nullptr;
    {
      WGPUShaderModuleWGSLDescriptor wgsl_desc{};
      wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
      wgsl_desc.code = raw_shader.c_str();

      WGPUShaderModuleDescriptor desc{};
      desc.label = "uniform buffer shader";

      desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgsl_desc);

      shader = wgpuDeviceCreateShaderModule(GetDevice(), &desc);
    }

    // pipeline layout
    {
      // binding group layout
      WGPUBindGroupLayoutEntry entry0{};
      entry0.binding = 0;
      entry0.visibility = WGPUShaderStage_Vertex;
      entry0.buffer.type = WGPUBufferBindingType_Uniform;
      entry0.buffer.minBindingSize = 0;
      entry0.buffer.hasDynamicOffset = false;

      WGPUBindGroupLayoutDescriptor binding_desc{};
      binding_desc.label = "binding 0";
      binding_desc.entryCount = 1;
      binding_desc.entries = &entry0;

      m_bind0_layout =
          wgpuDeviceCreateBindGroupLayout(GetDevice(), &binding_desc);

      WGPUPipelineLayoutDescriptor desc{};
      desc.label = "Uniform buffer pipeline";
      desc.bindGroupLayoutCount = 1;
      desc.bindGroupLayouts = &m_bind0_layout;

      m_layout = wgpuDeviceCreatePipelineLayout(GetDevice(), &desc);
    }

    // vertex layout
    WGPUVertexBufferLayout vertex_layout = {};
    std::array<WGPUVertexAttribute, 2> attrs{};
    attrs[0].format = WGPUVertexFormat_Float32x2;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;
    attrs[1].format = WGPUVertexFormat_Float32x3;
    attrs[1].offset = 2 * sizeof(float);
    attrs[1].shaderLocation = 1;

    vertex_layout.attributeCount = attrs.size();
    vertex_layout.attributes = attrs.data();
    vertex_layout.arrayStride = 5 * sizeof(float);
    vertex_layout.stepMode = WGPUVertexStepMode_Vertex;

    // pipeline descriptor
    WGPURenderPipelineDescriptor desc{};
    desc.label = "Uniform buffer pipeline";

    desc.layout = m_layout;

    desc.vertex.module = shader;
    desc.vertex.entryPoint = "vs_main";
    desc.vertex.bufferCount = 1;
    desc.vertex.buffers = &vertex_layout;

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

  WGPURenderPassEncoder BeginRenderPass(WGPUTextureView texture_view,
                                        WGPUCommandEncoder encoder) {
    WGPURenderPassDescriptor renderpassInfo = {};
    WGPURenderPassColorAttachment colorAttachment = {};

    colorAttachment.view = texture_view;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.clearValue = {1.f, 1.f, 1.f, 1.f};
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    renderpassInfo.colorAttachmentCount = 1;
    renderpassInfo.colorAttachments = &colorAttachment;
    renderpassInfo.depthStencilAttachment = nullptr;

    return wgpuCommandEncoderBeginRenderPass(encoder, &renderpassInfo);
  }

  void Draw(WGPURenderPassEncoder render_pass) {
    m_rotation += 0.1f;

    auto matrix =
        glm::rotate(glm::mat4(1.f), glm::radians(m_rotation), {0.f, 0.f, 1.f});

    wgpuQueueWriteBuffer(GetQueue(), m_uniform_buffer, 0, &matrix,
                         sizeof(matrix));

    wgpuRenderPassEncoderSetPipeline(render_pass, m_pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, m_vertex_buffer, 0,
                                         WGPU_WHOLE_SIZE);

    WGPUBindGroup group0 = {};

    {
      WGPUBindGroupDescriptor desc{};
      desc.label = "Common Group";
      desc.layout = m_bind0_layout;
      desc.entryCount = 1;

      WGPUBindGroupEntry binding0{};
      binding0.binding = 0;
      binding0.buffer = m_uniform_buffer;
      binding0.offset = 0;
      binding0.size = sizeof(glm::mat4);

      desc.entries = &binding0;

      group0 = wgpuDeviceCreateBindGroup(GetDevice(), &desc);
    }

    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, group0, 0, nullptr);

    wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

    wgpuBindGroupRelease(group0);
  }

private:
  WGPUBindGroupLayout m_bind0_layout = {};
  WGPUPipelineLayout m_layout = {};
  WGPURenderPipeline m_pipeline = {};
  WGPUBuffer m_vertex_buffer = {};
  WGPUBuffer m_uniform_buffer = {};
  float m_rotation = 0.f;
};

int main(int argc, const char **argv) {
  UniformBuffer app{};

  app.Run();

  return 0;
}