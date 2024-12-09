#pragma once

#include "Renderer.hh"
#include <MetalKit/MetalKit.hpp>

// Methods for responding to a MetalKit view’s drawing and resizing events.
class MTKViewDelegate : public MTK::ViewDelegate {
    std::unique_ptr<Renderer> renderer;

public:
    static std::expected<std::unique_ptr<MTKViewDelegate>, RendererError>
    init(MTL::Device* device);

    virtual ~MTKViewDelegate() override = default;
    virtual void drawInMTKView(MTK::View* view) override;

private:
    explicit MTKViewDelegate(std::unique_ptr<Renderer> r);
};
