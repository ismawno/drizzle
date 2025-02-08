#include "flu/core/core.hpp"
#include "onyx/core/core.hpp"
#include "tkit/utils/literals.hpp"

namespace Flu
{
using namespace TKit::Literals;

static TKit::Storage<TKit::ThreadPool> s_ThreadPool;
static TKit::ArenaAllocator s_Arena{1_mb};

void Core::Initialize() noexcept
{
    s_ThreadPool.Construct(7);
    Onyx::Core::Initialize(s_ThreadPool.Get());
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
void Core::SetWorkerThreadCount(const u32 p_ThreadCount) noexcept
{
    s_ThreadPool.Destruct();
    s_ThreadPool.Construct(p_ThreadCount);
}

} // namespace Flu