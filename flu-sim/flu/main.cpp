#include "flu/layer.hpp"
#include "onyx/app/app.hpp"
#include "kit/memory/stack_allocator.hpp"
#include "kit/multiprocessing/thread_pool.hpp"
#include "kit/core/literals.hpp"

using namespace KIT::Literals;

int main()
{
    KIT::StackAllocator allocator{10_kb};
    KIT::ThreadPool<std::mutex> pool{7};

    ONYX::Core::Initialize(&allocator, &pool);

    ONYX::Window::Specs specs{};
    specs.Name = "Fluid simulator";

    ONYX::Application app{specs};
    app.Layers.Push<FLU::Layer>();
    app.Run();

    ONYX::Core::Terminate();
}