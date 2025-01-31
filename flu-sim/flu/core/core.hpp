#pragma once

#include "flu/core/alias.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"

// #define FLU_ENABLE_INSPECTOR

#ifndef FLU_THREAD_COUNT
#    define FLU_THREAD_COUNT 8
#endif

namespace Flu
{
template <typename T> using SimArray = TKit::StaticArray<T, 70000>;

struct Core
{
    static void Initialize() noexcept;
    static void Terminate() noexcept;

    static TKit::ArenaAllocator &GetArena() noexcept;
    static TKit::ThreadPool &GetThreadPool() noexcept;

    template <typename F> static void ForEach(const u32 p_Start, const u32 p_End, F &&p_Function) noexcept
    {
        TKit::ThreadPool &pool = GetThreadPool();
        TKit::ForEachMainThreadLead(pool, p_Start, p_End, pool.GetThreadCount(), std::forward<F>(p_Function));
        pool.AwaitPendingTasks();
    }
};
} // namespace Flu