#include "driz/app/intro_layer.hpp"
#include "driz/app/sim_layer.hpp"
#include "driz/app/argparse.hpp"
#include "onyx/app/app.hpp"

void SetIntroLayer(Onyx::Application &p_App, const Driz::ParseResult &p_Result) noexcept
{
    if (p_Result.State2)
        p_App.SetUserLayer<Driz::IntroLayer>(&p_App, p_Result.Settings, *p_Result.State2);
    else if (p_Result.State3)
        p_App.SetUserLayer<Driz::IntroLayer>(&p_App, p_Result.Settings, *p_Result.State3);
    else
        p_App.SetUserLayer<Driz::IntroLayer>(&p_App, p_Result.Settings, p_Result.Dim);
}

int main(int argc, char **argv)
{
    Driz::Core::Initialize();
    {
        Onyx::Window::Specs specs{};
        specs.Name = "Drizzle";
        specs.PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        Onyx::Application app{specs};

        const Driz::ParseResult result = Driz::ParseArgs(argc, argv);
        if (result.Intro)
            SetIntroLayer(app, result);
        else if (result.Dim == Driz::D2)
            app.SetUserLayer<Driz::SimLayer<Driz::D2>>(&app, result.Settings, *result.State2);
        else
            app.SetUserLayer<Driz::SimLayer<Driz::D3>>(&app, result.Settings, *result.State3);

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
    Driz::Core::Terminate();
}