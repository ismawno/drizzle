#include "flu/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/core/literals.hpp"

using namespace TKit::Literals;

int main()
{
    TKit::ThreadPool<std::mutex> pool{7};

    Onyx::Core::Initialize(&pool);

    Onyx::Window::Specs specs{};
    specs.Name = "Fluid simulator";

    Onyx::Application app{specs};
    app.Layers.Push<FLU::Layer<TKit::D2>>(&app);
    app.Run();

    Onyx::Core::Terminate();
}