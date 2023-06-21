
#include "utils.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

uint32_t next_offset(uint32_t curr, uint32_t size, uint32_t align) {
  uint32_t tail = curr + size;

  if (tail == 0 || (tail % align) == 0) {
    return 0;
  }

  uint32_t delta = align - (tail % align);

  return tail + delta;
}

class DepthBuffer : public util::App {
public:
  DepthBuffer() : util::App("Depth Buffer", 800, 800) {}

  ~DepthBuffer() override = default;

protected:
  void OnInit() override {
    InitBuffers();
    InitDepthAttachment();
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
    wgpuBufferRelease(m_vertex_buffer);
    wgpuBufferRelease(m_matrix_buffer);

    wgpuBindGroupLayoutRelease(m_group0_layout);
    wgpuRenderPipelineRelease(m_pipeline);
    wgpuPipelineLayoutRelease(m_pipeline_layout);

    wgpuTextureViewRelease(m_depth_attachment);
  }

private:
  void InitBuffers() {
    // vertex buffer
    {
      std::vector<float> data{
          0.f,   0.5f,  // x, y,
          -0.5f, -0.5f, // x, y,
          0.5f,  -0.5f, // x, y,
      };

      WGPUBufferDescriptor desc{};
      desc.label = "Vertex buffer";
      desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
      desc.size = data.size() * sizeof(float);

      m_vertex_buffer = wgpuDeviceCreateBuffer(GetDevice(), &desc);

      wgpuQueueWriteBuffer(GetQueue(), m_vertex_buffer, 0, data.data(),
                           data.size() * sizeof(float));
    }

    // uniform buffer
    {
      auto device = GetDevice();
      // first need to calculate buffer size with uniform buffer offset
      // https://www.w3.org/TR/webgpu/#dom-supported-limits-minuniformbufferoffsetalignment
      WGPUSupportedLimits limits{};
      wgpuDeviceGetLimits(device, &limits);

      auto offset = limits.limits.minUniformBufferOffsetAlignment;

      /**
       * We use one buffer for two draw call, the buffer layout is:
       *
       *      1 - matrix
       *       align offset
       *      1 - color
       *       align offset
       *      2 - matrix
       *       align offset
       *      2 - color
       */

      // calculate offsets
      m_offset_1_matrix = 0;
      m_offset_1_color =
          next_offset(m_offset_1_matrix, sizeof(glm::mat4), offset);
      m_offset_2_matrix =
          next_offset(m_offset_1_color, sizeof(glm::vec4), offset);
      m_offset_2_color =
          next_offset(m_offset_2_matrix, sizeof(glm::mat4), offset);

      // size of this g-buffer
      uint32_t tail = next_offset(m_offset_2_color, sizeof(glm::vec4), offset);

      WGPUBufferDescriptor desc{};
      desc.label = "Uniform buffer";
      desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
      desc.size = tail;

      m_matrix_buffer = wgpuDeviceCreateBuffer(device, &desc);

      // fill buffer data
      glm::mat4 mat1 = glm::translate(glm::mat4(1.f), {0.2f, 0.2f, 0.f});

      glm::vec4 color1{83.f / 255.f, 109.f / 255.f, 254.f / 255.f, 1.f};

      glm::mat4 mat2 = glm::translate(glm::mat4(1.f), {-0.2f, -0.2f, 0.5f});

      glm::vec4 color2{0.f, 137.f / 255.f, 123.f / 255.f, 1.f};

      wgpuQueueWriteBuffer(GetQueue(), m_matrix_buffer, m_offset_1_matrix,
                           &mat1, sizeof(mat1));

      wgpuQueueWriteBuffer(GetQueue(), m_matrix_buffer, m_offset_1_color,
                           &color1, sizeof(color1));

      wgpuQueueWriteBuffer(GetQueue(), m_matrix_buffer, m_offset_2_matrix,
                           &mat2, sizeof(mat2));

      wgpuQueueWriteBuffer(GetQueue(), m_matrix_buffer, m_offset_2_color,
                           &color2, sizeof(color2));
    }
  }

  void InitDepthAttachment() {
    // create texture
    WGPUTextureDescriptor tex_desc{};
    tex_desc.label = "MSAA Resolve";
    tex_desc.dimension = WGPUTextureDimension_2D;
    // this need to be equal with surface color format
    tex_desc.format = WGPUTextureFormat_Depth24Plus;
    tex_desc.size.width = 800;
    tex_desc.size.height = 800;
    tex_desc.size.depthOrArrayLayers = 1;
    tex_desc.sampleCount = 1;
    tex_desc.mipLevelCount = 1;
    tex_desc.usage = WGPUTextureUsage_RenderAttachment;

    auto texture = wgpuDeviceCreateTexture(GetDevice(), &tex_desc);

    m_depth_attachment = wgpuTextureCreateView(texture, nullptr);

    wgpuTextureRelease(texture);
  }

  void InitPipeline() {
    // shader
    auto raw_shader = ReadFile(ASSET_DIR "/depth-triangle.wgsl");
    // shader module
    WGPUShaderModule shader = nullptr;
    {
      WGPUShaderModuleWGSLDescriptor wgsl_desc{};
      wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
      wgsl_desc.code = raw_shader.c_str();

      WGPUShaderModuleDescriptor desc{};
      desc.label = "Depth test triangle Shader";

      desc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&wgsl_desc);

      shader = wgpuDeviceCreateShaderModule(GetDevice(), &desc);
    }
    // pipeline layout
    {
      // binding group layout
      std::vector<WGPUBindGroupLayoutEntry> entries(2);

      entries[0].binding = 0;
      entries[0].visibility = WGPUShaderStage_Vertex;
      entries[0].buffer.type = WGPUBufferBindingType_Uniform;
      entries[0].buffer.minBindingSize = 0;
      entries[0].buffer.hasDynamicOffset = false;

      entries[1].binding = 1;
      entries[1].visibility = WGPUShaderStage_Fragment;
      entries[1].buffer.type = WGPUBufferBindingType_Uniform;
      entries[1].buffer.minBindingSize = 0;
      entries[1].buffer.hasDynamicOffset = false;

      WGPUBindGroupLayoutDescriptor group_desc{};
      group_desc.label = "group 0";
      group_desc.entryCount = entries.size();
      group_desc.entries = entries.data();

      m_group0_layout =
          wgpuDeviceCreateBindGroupLayout(GetDevice(), &group_desc);

      WGPUPipelineLayoutDescriptor desc{};
      desc.label = "Depth test pipeline layout";
      desc.bindGroupLayoutCount = 1;
      desc.bindGroupLayouts = &m_group0_layout;

      m_pipeline_layout = wgpuDeviceCreatePipelineLayout(GetDevice(), &desc);
    }

    // vertex layout
    WGPUVertexBufferLayout vertex_layout = {};

    WGPUVertexAttribute attr{};
    attr.format = WGPUVertexFormat_Float32x2;
    attr.offset = 0;
    attr.shaderLocation = 0;

    vertex_layout.attributeCount = 1;
    vertex_layout.attributes = &attr;
    vertex_layout.arrayStride = 2 * sizeof(float);
    vertex_layout.stepMode = WGPUVertexStepMode_Vertex;

    // pipeline descriptor
    WGPURenderPipelineDescriptor desc{};
    desc.label = "Depth test pipeline";

    desc.layout = m_pipeline_layout;

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

    // depth stencil state
    WGPUDepthStencilState depth_stencil_state{};
    depth_stencil_state.depthWriteEnabled = true;
    depth_stencil_state.depthBias = 0;
    depth_stencil_state.depthBiasClamp = 1.f;
    depth_stencil_state.depthCompare = WGPUCompareFunction_Less;
    depth_stencil_state.depthBiasSlopeScale = 1.f;
    depth_stencil_state.stencilReadMask = depth_stencil_state.stencilWriteMask =
        0xff;
    depth_stencil_state.stencilFront.compare = WGPUCompareFunction_Always;
    depth_stencil_state.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilFront.failOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilFront.passOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilBack = depth_stencil_state.stencilFront;
    depth_stencil_state.format = WGPUTextureFormat_Depth24Plus;

    desc.depthStencil = &depth_stencil_state;

    // msaa state
    // this should be equal with msaa texture config
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

    // depth attachment
    WGPURenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.depthClearValue = 1.f;
    depthAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthAttachment.depthStoreOp = WGPUStoreOp_Discard;
    depthAttachment.depthReadOnly = false;
    depthAttachment.view = m_depth_attachment;

    renderpassInfo.depthStencilAttachment = &depthAttachment;

    return wgpuCommandEncoderBeginRenderPass(encoder, &renderpassInfo);
  }

  void Draw(WGPURenderPassEncoder render_pass) {
    wgpuRenderPassEncoderSetPipeline(render_pass, m_pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, m_vertex_buffer, 0,
                                         WGPU_WHOLE_SIZE);

    // group for first draw call
    WGPUBindGroup group0 = {};
    // group for second draw call
    WGPUBindGroup group1 = {};
    {
      WGPUBindGroupDescriptor desc{};
      desc.label = "Group 0";
      desc.layout = m_group0_layout;

      std::vector<WGPUBindGroupEntry> bindings(2);

      bindings[0].binding = 0;
      bindings[0].buffer = m_matrix_buffer;
      bindings[0].offset = m_offset_1_matrix;
      bindings[0].size = sizeof(glm::mat4);

      bindings[1].binding = 1;
      bindings[1].buffer = m_matrix_buffer;
      bindings[1].offset = m_offset_1_color;
      bindings[1].size = sizeof(glm::vec4);

      desc.entryCount = bindings.size();
      desc.entries = bindings.data();

      group0 = wgpuDeviceCreateBindGroup(GetDevice(), &desc);

      bindings[0].offset = m_offset_2_matrix;
      bindings[1].offset = m_offset_2_color;

      group1 = wgpuDeviceCreateBindGroup(GetDevice(), &desc);
    }

    // first triangle
    // light blue color with smaller depth value
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, group0, 0, nullptr);
    wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

    // second triangle
    // light green color with larger depth value
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, group1, 0, nullptr);
    wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

    // we should see the second triangle is blocked by first triangle, even it
    // is rendered last

    wgpuBindGroupRelease(group0);
    wgpuBindGroupRelease(group1);
  }

private:
  WGPUBuffer m_vertex_buffer = {};
  WGPUBuffer m_matrix_buffer = {};
  WGPUBindGroupLayout m_group0_layout = {};
  WGPUPipelineLayout m_pipeline_layout = {};
  WGPURenderPipeline m_pipeline = {};
  // texture for depth buffer in render pipeline
  WGPUTextureView m_depth_attachment = {};

  uint32_t m_offset_1_matrix = 0;
  uint32_t m_offset_1_color = 0;
  uint32_t m_offset_2_matrix = 0;
  uint32_t m_offset_2_color = 0;
};

int main(int argc, const char **argv) {
  DepthBuffer app{};

  app.Run();

  return 0;
}
