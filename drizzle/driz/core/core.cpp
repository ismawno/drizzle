#include "driz/core/core.hpp"
#include "onyx/core/core.hpp"
#include "tkit/utils/literals.hpp"

#define DRIZ_MAX_WORKERS (ONYX_MAX_THREADS - 1)

namespace Driz
{
using namespace TKit::Literals;

static TKit::Storage<TKit::ThreadPool> s_ThreadPool;
static TKit::ArenaAllocator s_Arena{1_mb};

static fs::path s_SettingsPath = fs::path(DRIZ_ROOT_PATH) / "saves" / "settings";
static fs::path s_StatePath2 = fs::path(DRIZ_ROOT_PATH) / "saves" / "2D";
static fs::path s_StatePath3 = fs::path(DRIZ_ROOT_PATH) / "saves" / "3D";

void Core::Initialize() noexcept
{
    s_ThreadPool.Construct(DRIZ_MAX_WORKERS);
    Onyx::Core::Initialize(Onyx::Specs{.TaskManager = s_ThreadPool.Get()});

    fs::create_directories(s_SettingsPath);
    fs::create_directories(s_StatePath2);
    fs::create_directories(s_StatePath3);
}
void Core::Terminate() noexcept
{
    Onyx::Core::Terminate();
    s_ThreadPool.Destruct();
}

TKit::ArenaAllocator &Core::GetArena() noexcept
{
    return s_Arena;
}
TKit::ThreadPool &Core::GetThreadPool() noexcept
{
    return *s_ThreadPool.Get();
}

const fs::path &Core::GetSettingsPath() noexcept
{
    return s_SettingsPath;
}
u32 Core::GetThreadIndex() noexcept
{
    thread_local u32 tindex = s_ThreadPool->GetThreadIndex();
    return tindex;
}

template <Dimension D> const fs::path &Core::GetStatePath() noexcept
{
    if constexpr (D == D2)
        return s_StatePath2;
    else
        return s_StatePath3;
}

template const fs::path &Core::GetStatePath<D2>() noexcept;
template const fs::path &Core::GetStatePath<D3>() noexcept;

} // namespace Driz
