#pragma once

#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"

// #define FLU_ENABLE_INSPECTOR

#ifndef FLU_THREAD_COUNT
#    define FLU_THREAD_COUNT 8
#endif

namespace Flu
{
template <typename T> using SimArray = TKit::StaticArray<T, 16000>;

struct Core
{
    static void Initialize() noexcept;
    static void Terminate() noexcept;

    static TKit::ArenaAllocator &GetArena() noexcept;
    static TKit::ThreadPool &GetThreadPool() noexcept;
};
} // namespace Flu