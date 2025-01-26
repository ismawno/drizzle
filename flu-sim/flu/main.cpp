#include "flu/app/intro_layer.hpp"
#include "onyx/app/app.hpp"

int main()
{
    Flu::Core::Initialize();
    {
        Onyx::Window::Specs specs{};
        specs.Name = "Fluid simulator";

        Onyx::Application app{specs};
        app.SetUserLayer<Flu::IntroLayer>(&app);
        app.Run();
    }
    Flu::Core::Terminate();
}