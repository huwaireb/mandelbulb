#include "AppDelegate.hh"

AppDelegate::AppDelegate()
    : window_title(nsStringUtf8("App")) { };

AppDelegate::AppDelegate(const char* title)
    : window_title(nsStringUtf8(title)) { };

AppDelegate::~AppDelegate()
{
    this->mtk_view->release();
    this->window->release();
    this->device->release();
}

void AppDelegate::applicationWillFinishLaunching(
    NS::Notification* notification)
{
    NS::Application* app = static_cast<NS::Application*>(notification->object());
    app->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void AppDelegate::applicationDidFinishLaunching(
    NS::Notification* notification)
{
    CGRect frame = {
        .origin = { .x = 100.0, .y = 100.0 },
        .size = { .width = 900.0, .height = 1024.0 },
    };

    this->window = NS::Window::alloc()->init(
        frame, NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled,
        NS::BackingStoreBuffered, false);

    this->device = MTL::CreateSystemDefaultDevice();

    this->mtk_view = MTK::View::alloc()->init(frame, this->device);
    this->mtk_view->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    this->mtk_view->setClearColor(MTL::ClearColor::Make(0.1, 0.1, 0.1, 1.0));
    this->mtk_view->setDepthStencilPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);
    this->mtk_view->setClearDepth(1.0f);

    this->mtk_view->setSampleCount(4);

    auto view_delegate = MTKViewDelegate::init(device);

    if (!view_delegate) {
        std::cerr << std::format("Failed to create MTKViewDelegate: {}", view_delegate.error().message());
        return;
    }

    this->view_delegate = std::move(view_delegate.value());

    this->mtk_view->setDelegate(this->view_delegate.get());

    this->window->setContentView(this->mtk_view);
    this->window->setTitle(this->window_title);

    // Key, as in, show the window.
    this->window->makeKeyAndOrderFront(nullptr);

    NS::Application* app = static_cast<NS::Application*>(notification->object());

    app->activateIgnoringOtherApps(true);
}

bool AppDelegate::applicationShouldTerminateAfterLastWindowClosed(
    NS::Application* _ /*pSender*/)
{
    return true;
}
