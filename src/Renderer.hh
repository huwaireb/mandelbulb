#pragma once

#include "base.hh"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

/// Triple buffering
static const int MAX_FRAMES_IN_FLIGHT = 3;

/// Shader source code embedded in the program (REQ. C23 or C++26)
static constexpr char SHADER_SRC[] = {
#embed "Shader.metal"
    , '\0'
};

struct RendererError {
    enum class Kind {
        ShaderCompilationFailed,
        EntrypointNotFound,
        PipelineCreationFailed,
        DeviceError
    };

    Kind kind;
    std::optional<NS::String*> details;

    RendererError(Kind kind);
    RendererError(Kind kind, NS::Error* error = nullptr);

    ~RendererError();

    static constexpr std::string_view baseMessage(Kind kind);
    std::string message() const;
};

/// The Renderer class is responsible for rendering the Mandelbulb fractal.
class Renderer {
    MTL::Device* device;
    MTL::CommandQueue* command_queue;
    MTL::RenderPipelineState* pipeline_state;

    MTL::Library* shader_lib;

    MTL::Buffer* vertex_data_buffer;
    MTL::Buffer* uniform_buffer[MAX_FRAMES_IN_FLIGHT];
    MTL::DepthStencilState* depth_stencil_state;

    float current_time;
    float angle;
    int frame;

    /// Semaphore to synchronize CPU and GPU, ensure frames in flight
    dispatch_semaphore_t frame_semaphore;

    /** Creates necessary GPU buffers:
     * - Vertex buffer containing a full-screen quad (two triangles)
     * - Multiple uniform buffers (one for each frame in flight) that store
     *   rendering parameters like time, resolution, and camera settings **/
    void buildBuffers();

    /** Sets up depth testing configuration:
     * - Enables depth testing with "less than" comparison
     * - Enables depth write
     * - Creates depth stencil state object used during rendering **/
    void buildDepthStencilStates();

    /** Compiles and sets up the shader pipeline:
     * - Loads and compiles Metal shader code
     * - Configures render pipeline (pvertex/fragment functions, pixel format)
     * - Creates pipeline state object
     * @return error if shader compilation or pipeline creation fails **/
    std::expected<void, RendererError> buildShaders();

    struct VertexData {
        simd::float3 position;
        simd::float2 texcoord; // texture coordinates
    };

    struct Uniforms {
        float time;
        simd::float2 resolution; // width, height
        simd::float3 camera_position;
        simd::float3 camera_target;
        simd::float3 camera_up;
    };

    /// Full-screen quad vertices
    static constexpr VertexData vertices[] = {
        // First triangle
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, // bottom left
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }, // bottom right
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }, // top left

        // Second triangle
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }, // bottom right
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } }, // top right
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } } // top left
    };

public:
    // The only way to return an error from a constructor is
    // to throw an exception, which is not ideal.
    static std::expected<std::unique_ptr<Renderer>, RendererError>
    init(MTL::Device* device);

    ~Renderer();

    void draw(MTK::View* view);
};
