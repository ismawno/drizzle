#pragma once

#include "flu/core/alias.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"

// #define FLU_ENABLE_INSPECTOR

namespace Flu
{
template <typename T> using SimArray = TKit::StaticArray<T, 70000>;

struct Core
{
    static void Initialize() noexcept;
    static void Terminate() noexcept;

    static TKit::ArenaAllocator &GetArena() noexcept;
    static TKit::ThreadPool &GetThreadPool() noexcept;
    static void SetWorkerThreadCount(u32 p_ThreadCount) noexcept;

    template <typename F> static void ForEach(const u32 p_Start, const u32 p_End, F &&p_Function) noexcept
    {
        TKit::ThreadPool &pool = GetThreadPool();
        const u32 partitions = pool.GetThreadCount() + 1;
        TKit::Array<TKit::Ref<TKit::Task<void>>, TKIT_THREAD_POOL_MAX_THREADS> tasks;

        TKit::ForEachMainThreadLead(pool, p_Start, p_End, tasks.begin(), partitions, std::forward<F>(p_Function));
        for (u32 i = 0; i < partitions - 1; ++i)
            tasks[i]->WaitUntilFinished();
    }
};
} // namespace Flu