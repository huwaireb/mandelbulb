#include "Renderer.hh"

import std;

std::expected<std::unique_ptr<Renderer>, RendererError>
Renderer::init(MTL::Device* device)
{
    auto renderer = std::unique_ptr<Renderer>(new Renderer());

    renderer->device = device->retain();
    renderer->command_queue = renderer->device->newCommandQueue();
    renderer->angle = 0.0f;
    renderer->frame = 0;
    renderer->current_time = 0.0f;

    if (!renderer->command_queue) {
        return std::unexpected(RendererError(
            RendererError::Kind::DeviceError,
            nullptr));
    }

    return renderer->buildShaders()
        .transform([r = std::move(renderer)]() mutable {
            r->buildBuffers();
            r->buildDepthStencilStates();
            r->frame_semaphore = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
            return std::move(r);
        });
}

Renderer::~Renderer()
{
    this->vertex_data_buffer->release();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        this->uniform_buffer[i]->release();
    }

    this->shader_lib->release();
    this->depth_stencil_state->release();
    this->pipeline_state->release();
    this->command_queue->release();
    this->device->release();
}

std::expected<void, RendererError>
Renderer::buildShaders()
{
    NS::Error* error = nullptr;

    MTL::Library* mtl_lib = this->device->newLibrary(nsStringUtf8(SHADER_SRC), nullptr, &error);
    if (!mtl_lib) {
        return std::unexpected(RendererError(
            RendererError::Kind::ShaderCompilationFailed,
            error));
    }

    MTL::Function* vertex_fn = mtl_lib->newFunction(nsStringUtf8("vertexMain"));
    MTL::Function* frag_fn = mtl_lib->newFunction(nsStringUtf8("fragmentMain"));

    if (!vertex_fn || !frag_fn) {
        return std::unexpected(RendererError(
            RendererError::Kind::EntrypointNotFound,
            error));
    }

    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vertex_fn);
    desc->setFragmentFunction(frag_fn);
    desc->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    desc->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth32Float);

    this->pipeline_state = this->device->newRenderPipelineState(desc, &error);
    if (!this->pipeline_state) {
        return std::unexpected(RendererError(
            RendererError::Kind::PipelineCreationFailed,
            error));
    }

    this->shader_lib = mtl_lib;

    vertex_fn->release();
    frag_fn->release();
    desc->release();

    return {};
}

void Renderer::buildBuffers()
{
    const usize vertex_data_size = sizeof(this->vertices);
    this->vertex_data_buffer = this->device->newBuffer(
        vertex_data_size,
        MTL::ResourceStorageModeManaged);

    memcpy(this->vertex_data_buffer->contents(), this->vertices, vertex_data_size);
    this->vertex_data_buffer->didModifyRange(NS::Range::Make(0, vertex_data_size));

    // Create uniform buffers for each frame
    const usize uniform_buffer_size = sizeof(Uniforms);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        this->uniform_buffer[i] = this->device->newBuffer(
            uniform_buffer_size,
            MTL::ResourceStorageModeManaged);
    }
}

void Renderer::buildDepthStencilStates()
{
    MTL::DepthStencilDescriptor* desc = MTL::DepthStencilDescriptor::alloc()->init();

    desc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    desc->setDepthWriteEnabled(true);

    this->depth_stencil_state = this->device->newDepthStencilState(desc);
    desc->release();
}

void Renderer::draw(MTK::View* view)
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;

    // Update uniforms
    Uniforms* uniforms = static_cast<Uniforms*>(
        this->uniform_buffer[frame]->contents());

    current_time += 0.016f;

    uniforms->time = current_time;
    uniforms->resolution = {
        static_cast<float>(view->drawableSize().width),
        static_cast<float>(view->drawableSize().height)
    };

    uniforms->camera_position = { 0.0f, 0.0f, -3.0f };
    uniforms->camera_target = { 0.0f, 0.0f, 0.0f };
    uniforms->camera_up = { 0.0f, 1.0f, 0.0f };

    this->uniform_buffer[frame]->didModifyRange(NS::Range::Make(0, sizeof(Uniforms)));

    MTL::CommandBuffer* cmd_buffer = this->command_queue->commandBuffer();

    dispatch_semaphore_wait(this->frame_semaphore, DISPATCH_TIME_FOREVER);

    Renderer* renderer = this;
    cmd_buffer->addCompletedHandler(^void(MTL::CommandBuffer* buffer) {
        dispatch_semaphore_signal(renderer->frame_semaphore);
    });

    MTL::RenderPassDescriptor* rpd = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* encoder = cmd_buffer->renderCommandEncoder(rpd);

    encoder->setRenderPipelineState(this->pipeline_state);
    encoder->setDepthStencilState(this->depth_stencil_state);

    encoder->setVertexBuffer(this->vertex_data_buffer, 0, 0);
    encoder->setVertexBuffer(this->uniform_buffer[frame], 0, 1);
    encoder->setFragmentBuffer(this->uniform_buffer[frame], 0, 0);

    encoder->drawPrimitives(
        MTL::PrimitiveType::PrimitiveTypeTriangle,
        /* vertexStart */ NS::UInteger(0), // vertex_id starting point
        /* vertexCount */ NS::UInteger(6)); // 6 Vertices 2 Triangles

    encoder->endEncoding();
    cmd_buffer->presentDrawable(view->currentDrawable());
    cmd_buffer->commit();

    angle += 0.01f;

    pool->release();
}

RendererError::RendererError(Kind kind)
    : kind(kind)
    , details(std::nullopt) { };

RendererError::RendererError(Kind kind, NS::Error* error)
    : kind(kind)
    , details(error->localizedDescription()->retain())
{
}

RendererError::~RendererError()
{
    if (details)
        details.value()->release();
}

constexpr std::string_view RendererError::baseMessage(Kind kind)
{
    switch (kind) {
    case Kind::ShaderCompilationFailed:
        return "Shader compilation failed";
    case Kind::EntrypointNotFound:
        return "Shader entrypoint not found";
    case Kind::PipelineCreationFailed:
        return "Pipeline creation failed";
    case Kind::DeviceError:
        return "Metal device error";
    }
}

std::string RendererError::message() const
{
    std::string_view base_msg = baseMessage(this->kind);

    return this->details
        .transform([base_msg](auto* details) {
            return std::format("{}: {}",
                base_msg,
                details->utf8String());
        })
        .value_or(std::string(base_msg));
}
