#pragma once

#include "base.hh"

#include "MTKViewDelegate.hh"

#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

import std;

class AppDelegate : public NS::ApplicationDelegate {
    NS::Window* window;
    NS::String* window_title = nsStringUtf8("Unset Title");

    MTK::View* mtk_view;
    MTL::Device* device;
    std::unique_ptr<MTKViewDelegate> view_delegate;

public:
    AppDelegate();
    AppDelegate(const char* title);
    ~AppDelegate();

    virtual void
    applicationWillFinishLaunching(NS::Notification* pNotification) override;
    virtual void
    applicationDidFinishLaunching(NS::Notification* pNotification) override;

    virtual bool applicationShouldTerminateAfterLastWindowClosed(
        NS::Application* pSender) override;
};
