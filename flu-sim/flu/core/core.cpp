#include "flu/core/core.hpp"
#include "onyx/core/core.hpp"
#include "tkit/core/literals.hpp"

namespace Flu
{
using namespace TKit::Literals;

static TKit::ThreadPool s_ThreadPool{FLU_THREAD_COUNT - 1};
static TKit::ArenaAllocator s_Arena{1_mb};

void Core::Initialize() noexcept
{
    Onyx::Core::Initialize(&s_ThreadPool);
}
void Core::Terminate() noexcept
{
    Onyx::Core::Terminate();
}

TKit::ArenaAllocator &Core::GetArena() noexcept
{
    return s_Arena;
}
TKit::ThreadPool &Core::GetThreadPool() noexcept
{
    return s_ThreadPool;
}
} // namespace Flu