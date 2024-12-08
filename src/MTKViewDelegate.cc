#include "MTKViewDelegate.hh"

std::expected<std::unique_ptr<MTKViewDelegate>, RendererError>
MTKViewDelegate::init(MTL::Device* device)
{
    return Renderer::init(device)
        .transform([](std::unique_ptr<Renderer> r) {
            return std::unique_ptr<MTKViewDelegate>(
                new MTKViewDelegate(std::move(r)));
        });
}

void MTKViewDelegate::drawInMTKView(MTK::View* view)
{
    renderer->draw(view);
}
