#pragma once

#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/static_array.hpp"

namespace Flu
{
template <typename T> using SimArray = TKit::StaticArray<T, 16000>;

struct Core
{
    static TKit::ArenaAllocator &GetArena() noexcept;
};
} // namespace Flu