#pragma once

#include "tkit/memory/arena_allocator.hpp"

namespace Flu
{
struct Core
{
    static TKit::ArenaAllocator &GetArena() noexcept;
};
} // namespace Flu