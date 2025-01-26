#pragma once

#include "flu/core/glm.hpp"
#include "flu/core/core.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/core/literals.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include <array>

namespace Flu
{
struct GridCell
{
    u32 Key;
    u32 Start;
    u32 End;
};

struct Grid
{
    SimArray<GridCell> Cells;
    SimArray<u32> ParticleIndices;
    SimArray<u32> CellKeyToIndex;
};

template <Dimension D> class Lookup
{
  public:
    void SetPositions(const SimArray<fvec<D>> *p_Positions) noexcept;

    void UpdateBruteForceLookup(f32 p_Radius) noexcept;
    void UpdateGridLookup(f32 p_Radius) noexcept;

    static ivec<D> GetCellPosition(const fvec<D> &p_Position, f32 p_Radius) noexcept;
    static u32 GetCellKey(const ivec<D> &p_CellPosition, u32 p_ParticleCount) noexcept;

    ivec<D> GetCellPosition(const fvec<D> &p_Position) const noexcept;
    u32 GetCellKey(const ivec<D> &p_CellPosition) const noexcept;

    u32 DrawCells(Onyx::RenderContext<D> *p_Context) const noexcept;
    u32 GetCellCount() const noexcept;

    const Grid &GetGrid() const noexcept;
    f32 GetRadius() const noexcept;

    template <typename F> void ForEachPairBruteForceST(F &&p_Function) const noexcept
    {
        const f32 r2 = m_Radius * m_Radius;
        for (u32 i = 0; i < m_Positions->size(); ++i)
            processPass(i, r2, std::forward<F>(p_Function));
    }

    template <typename F> void ForEachPairBruteForceMT(F &&p_Function) const noexcept
    {
        const auto &positions = *m_Positions;
        const f32 r2 = m_Radius * m_Radius;

        TKit::ThreadPool &pool = Flu::Core::GetThreadPool();
        TKit::ForEachMainThreadLead(
            pool, 0, positions.size(), pool.GetThreadCount(),
            [this, r2, &positions, &p_Function](const u32 p_Start, const u32 p_End, const u32 p_ThreadIndex) {
                for (u32 i = p_Start; i < p_End; ++i)
                    processPass(i, r2, std::forward<F>(p_Function), p_ThreadIndex);
            });
        pool.AwaitPendingTasks();
    }

    template <typename F> void ForEachPairGridST(F &&p_Function) const noexcept
    {
        const OffsetArray offsets = getGridOffsets();
        for (const GridCell &cell : m_Grid.Cells)
            processCell(cell, offsets, std::forward<F>(p_Function));
    }

    template <typename F> void ForEachPairGridMT(F &&p_Function) const noexcept
    {
        const OffsetArray offsets = getGridOffsets();
        TKit::ThreadPool &pool = Flu::Core::GetThreadPool();

        TKit::ForEachMainThreadLead(
            pool, 0, m_Grid.Cells.size(), pool.GetThreadCount(),
            [this, &offsets, &p_Function](const u32 p_Start, const u32 p_End, const u32 p_ThreadIndex) {
                for (u32 i = p_Start; i < p_End; ++i)
                    processCell(m_Grid.Cells[i], offsets, std::forward<F>(p_Function), p_ThreadIndex);
            });
        pool.AwaitPendingTasks();
    }

  private:
    static constexpr u32 s_OffsetCount = D * D * D + 2 - D;
    using OffsetArray = TKit::Array<ivec<D>, s_OffsetCount>;

    template <typename F, typename... Args>
    void processPass(const u32 p_Index, const f32 p_Radius2, F &&p_Function, Args &&...p_Args) const noexcept
    {
        const auto &positions = *m_Positions;
        for (u32 j = p_Index + 1; j < positions.size(); ++j)
        {
            const f32 distance = glm::distance2(positions[p_Index], positions[j]);
            if (distance < p_Radius2)
                std::forward<F>(p_Function)(p_Index, j, glm::sqrt(distance), std::forward<Args>(p_Args)...);
        }
    }

    template <typename F, typename... Args>
    void processCell(const GridCell &p_Cell, const OffsetArray &p_Offsets, F &&p_Function,
                     Args &&...p_Args) const noexcept
    {
        const f32 r2 = m_Radius * m_Radius;
        const auto &positions = *m_Positions;
        const auto &particleIndices = m_Grid.ParticleIndices;
        const auto &cellMap = m_Grid.CellKeyToIndex;
        const auto &cells = m_Grid.Cells;

        const auto isVisited = [](const auto it1, const auto it2, const u32 p_CellKey) {
            for (auto it = it1; it != it2; ++it)
                if (*it == p_CellKey)
                    return true;
            return false;
        };

        const auto processPair = [r2, &positions](const u32 p_Index1, const u32 p_Index2, F &&p_Function,
                                                  Args &&...p_Args) {
            const f32 distance = glm::distance2(positions[p_Index1], positions[p_Index2]);
            if (distance < r2)
                std::forward<F>(p_Function)(p_Index1, p_Index2, glm::sqrt(distance), std::forward<Args>(p_Args)...);
        };

        for (u32 i = p_Cell.Start; i < p_Cell.End; ++i)
        {
            const u32 index1 = particleIndices[i];
            for (u32 j = i + 1; j < p_Cell.End; ++j)
                processPair(index1, particleIndices[j], std::forward<F>(p_Function));

            const ivec<D> center = GetCellPosition(positions[index1]);
            const u32 cellKey1 = p_Cell.Key;

            TKit::Array<u32, s_OffsetCount> visited{};
            u32 visitedSize = 0;
            for (const ivec<D> &offset : p_Offsets)
            {
                const u32 cellKey2 = GetCellKey(center + offset);
                const u32 cellIndex = cellMap[cellKey2];
                if (cellKey2 > cellKey1 && cellIndex != UINT32_MAX &&
                    !isVisited(visited.begin(), visited.begin() + visitedSize, cellKey2))
                {
                    visited[visitedSize++] = cellKey2;
                    const GridCell &cell2 = cells[cellIndex];
                    for (u32 j = cell2.Start; j < cell2.End; ++j)
                        processPair(index1, particleIndices[j], std::forward<F>(p_Function),
                                    std::forward<Args>(p_Args)...);
                }
            }
        }
    }

    OffsetArray getGridOffsets() const noexcept;

    const SimArray<fvec<D>> *m_Positions = nullptr;

    Grid m_Grid;
    f32 m_Radius;
};
} // namespace Flu