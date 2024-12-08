#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "AppDelegate.hh"
#include <AppKit/AppKit.hpp>

int main(/*int argc, char *argv[]*/)
{
    NS::AutoreleasePool* auto_release_pool = NS::AutoreleasePool::alloc()->init();

    AppDelegate del("Mandelbulb");

    NS::Application* shared_application = NS::Application::sharedApplication();

    shared_application->setDelegate(&del);
    shared_application->run();

    auto_release_pool->release();

    return 0;
}
