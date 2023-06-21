
// vertex buffer
struct VertexInput {
    @location(0) position: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

/// transform matrix, used in vertex stage
@group(0) @binding(0)
var<uniform> transform: mat4x4<f32>;

/// fragment color, used in fragment stage
@group(0) @binding(1)
var<uniform> color: vec4<f32>;


@vertex
fn vs_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = transform * vertex.position;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return color;
}

