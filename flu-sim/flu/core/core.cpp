#include "flu/core/core.hpp"
#include "tkit/core/literals.hpp"

namespace Flu
{
using namespace TKit::Literals;

static TKit::ArenaAllocator s_Arena{1_mb};

TKit::ArenaAllocator &Core::GetArena() noexcept
{
    return s_Arena;
}
} // namespace Flu