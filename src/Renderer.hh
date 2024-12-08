#pragma once

#include "base.hh"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

static const int MAX_FRAMES_IN_FLIGHT = 3;

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

    RendererError& operator=(RendererError& other);
};

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

    dispatch_semaphore_t frame_semaphore;

    void buildBuffers();
    void buildDepthStencilStates();

    std::expected<void, RendererError> buildShaders();

    struct VertexData {
        simd::float3 position;
        simd::float2 texcoord;
    };

    struct Uniforms {
        float time;
        simd::float2 resolution;
        simd::float3 camera_position;
        simd::float3 camera_target;
        simd::float3 camera_up;
    };

public:
    static std::expected<std::unique_ptr<Renderer>, RendererError> init(MTL::Device* device);

    ~Renderer();

    void draw(MTK::View* view);
};
