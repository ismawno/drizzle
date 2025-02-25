#include "flu/app/intro_layer.hpp"
#include "flu/app/sim_layer.hpp"
#include "flu/app/argparse.hpp"
#include "onyx/app/app.hpp"

void SetIntroLayer(Onyx::Application &p_App, const Flu::ParseResult &p_Result) noexcept
{
    if (p_Result.State2)
        p_App.SetUserLayer<Flu::IntroLayer>(&p_App, p_Result.Settings, *p_Result.State2);
    else if (p_Result.State3)
        p_App.SetUserLayer<Flu::IntroLayer>(&p_App, p_Result.Settings, *p_Result.State3);
    else
        p_App.SetUserLayer<Flu::IntroLayer>(&p_App, p_Result.Settings, p_Result.Dim);
}

int main(int argc, char **argv)
{
    Flu::Core::Initialize();
    {
        Onyx::Window::Specs specs{};
        specs.Name = "Fluid simulator";
        specs.PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        Onyx::Application app{specs};

        const Flu::ParseResult result = Flu::ParseArgs(argc, argv);
        if (result.Intro)
            SetIntroLayer(app, result);
        else if (result.Dim == Flu::D2)
            app.SetUserLayer<Flu::SimLayer<Flu::D2>>(&app, result.Settings, *result.State2);
        else
            app.SetUserLayer<Flu::SimLayer<Flu::D3>>(&app, result.Settings, *result.State3);

        if (result.HasRunTime)
        {
            app.Startup();
            TKit::Clock frameClock{};
            TKit::Clock runtimeClock{};
            while (runtimeClock.GetElapsed().AsSeconds() < result.RunTime && app.NextFrame(frameClock))
                ;
            app.Shutdown();
        }
        else
            app.Run();
    }
    Flu::Core::Terminate();
}