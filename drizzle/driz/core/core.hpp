#pragma once

#include "driz/core/alias.hpp"
#include "driz/core/dimension.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include "onyx/object/primitives.hpp"
#include <filesystem>

// #define DRIZ_ENABLE_INSPECTOR
#define DRIZ_MAX_THREADS ONYX_MAX_THREADS
#define DRIZ_MAX_TASKS (ONYX_MAX_THREADS - 1)

namespace Driz
{
namespace fs = std::filesystem;

template <typename T> using SimArray = TKit::StaticArray<T, 70000>;

struct Core
{
    static void Initialize() noexcept;
    static void Terminate() noexcept;

    static TKit::ArenaAllocator &GetArena() noexcept;
    static TKit::ThreadPool &GetThreadPool() noexcept;
    static void SetWorkerThreadCount(u32 p_ThreadCount) noexcept;

    static const fs::path &GetSettingsPath() noexcept;
    static u32 GetThreadIndex() noexcept;

    template <Dimension D> static const fs::path &GetStatePath() noexcept;

    template <typename F>
    static void ForEach(const u32 p_Start, const u32 p_End, const u32 p_Partitions, F &&p_Function) noexcept
    {
        TKit::ThreadPool &pool = GetThreadPool();
        TKit::Array<TKit::Task<> *, DRIZ_MAX_TASKS> tasks{};

        TKit::BlockingForEach(pool, p_Start, p_End, tasks.begin(), p_Partitions, std::forward<F>(p_Function));
        const u32 tcount = (p_Partitions - 1) >= DRIZ_MAX_TASKS ? DRIZ_MAX_TASKS : (p_Partitions - 1);

        for (u32 i = 0; i < tcount; ++i)
        {
            pool.WaitUntilFinished(tasks[i]);
            pool.DestroyTask(tasks[i]);
        }
    }

    static inline Onyx::Resolution Resolution = Onyx::Resolution::VeryLow;
};
} // namespace Driz
