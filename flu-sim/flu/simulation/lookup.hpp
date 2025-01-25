#pragma once

#include "flu/core/glm.hpp"
#include "flu/core/core.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/core/literals.hpp"
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
    SimArray<u32> ParticleIndices;
    SimArray<u32> CellKeyToIndex;
    SimArray<GridCell> Cells;
};

template <Dimension D> class Lookup
{
  public:
    void SetPositions(const SimArray<fvec<D>> *p_Positions) noexcept;

    void UpdateBruteForceLookup(f32 p_Radius) noexcept;
    void UpdateGridLookup(f32 p_Radius) noexcept;

    ivec<D> GetCellPosition(const fvec<D> &p_Position) const noexcept;
    u32 GetCellKey(const ivec<D> &p_CellPosition) const noexcept;

    u32 DrawCells(Onyx::RenderContext<D> *p_Context) const noexcept;

    u32 GetCellCount() const noexcept;

    template <typename F> void ForEachPairBruteForce(F &&p_Function) const noexcept
    {
        const auto &positions = *m_Positions;
        const f32 r2 = m_Radius * m_Radius;
        for (u32 i = 0; i < m_Positions->size(); ++i)
            for (u32 j = i + 1; j < m_Positions->size(); ++j)
            {
                const f32 distance = glm::distance2(positions[i], positions[j]);
                if (distance < r2)
                    std::forward<F>(p_Function)(i, j, glm::sqrt(distance));
            }
    }

    template <typename F> void ForEachPairGrid(F &&p_Function) const noexcept
    {
        const auto offsets = getGridOffsets();
        const f32 r2 = m_Radius * m_Radius;

        const auto isVisited = [](const auto it1, const auto it2, const u32 p_CellKey) {
            for (auto it = it1; it != it2; ++it)
                if (*it == p_CellKey)
                    return true;
            return false;
        };

        const auto &positions = *m_Positions;
        const auto processPair = [r2, &positions](const u32 p_Index1, const u32 p_Index2, F &&p_Function) {
            const f32 distance = glm::distance2(positions[p_Index1], positions[p_Index2]);
            TKIT_ASSERT(p_Index1 != p_Index2, "Invalid pair");
            if (distance < r2)
                std::forward<F>(p_Function)(p_Index1, p_Index2, glm::sqrt(distance));
        };

        const auto &particleIndices = m_Grid.ParticleIndices;
        const auto &cellMap = m_Grid.CellKeyToIndex;
        const auto &cells = m_Grid.Cells;

        for (const GridCell &cell1 : cells)
            for (u32 i = cell1.Start; i < cell1.End; ++i)
            {
                const u32 index1 = particleIndices[i];
                for (u32 j = i + 1; j < cell1.End; ++j)
                    processPair(index1, particleIndices[j], std::forward<F>(p_Function));

                const ivec<D> center = GetCellPosition(positions[index1]);
                const u32 cellKey1 = cell1.Key;

                TKit::Array<u32, offsets.size()> visited{};
                u32 visitedSize = 0;
                for (const ivec<D> &offset : offsets)
                {
                    const u32 cellKey2 = GetCellKey(center + offset);
                    const u32 cellIndex = cellMap[cellKey2];
                    if (cellKey2 > cellKey1 && cellIndex != UINT32_MAX &&
                        !isVisited(visited.begin(), visited.begin() + visitedSize, cellKey2))
                    {
                        visited[visitedSize++] = cellKey2;
                        const GridCell &cell2 = cells[cellIndex];
                        for (u32 j = cell2.Start; j < cell2.End; ++j)
                            processPair(index1, particleIndices[j], std::forward<F>(p_Function));
                    }
                }
            }
    }

    template <typename F> void ForEachParticleBruteForce(const u32 p_Index, F &&p_Function) const noexcept
    {
        const auto &positions = *m_Positions;
        const f32 r2 = m_Radius * m_Radius;
        for (u32 i = 0; i < m_Positions->size(); ++i)
        {
            if (p_Index == i)
                continue;
            const f32 distance = glm::distance2(positions[p_Index], positions[i]);
            if (distance < r2)
                std::forward<F>(p_Function)(i, glm::sqrt(distance));
        }
    }

    template <typename F> void ForEachParticleGrid(const u32 p_Index, F &&p_Function) const noexcept
    {
        if (m_Positions->empty())
            return;
        const f32 r2 = m_Radius * m_Radius;

        TKit::Array<u32, 9> visited{};
        u32 visitedSize = 0;
        const auto processParticle = [this, p_Index, r2, &visited, &visitedSize](const ivec<D> &p_Offset,
                                                                                 F &&p_Function) {
            const auto &particleIndices = m_Grid.ParticleIndices;
            const auto &cellMap = m_Grid.CellKeyToIndex;
            const auto &cells = m_Grid.Cells;
            const auto &positions = *m_Positions;

            const fvec<D> &point = positions[p_Index];
            const ivec<D> cellPosition = GetCellPosition(point) + p_Offset;
            const u32 cellKey = GetCellKey(cellPosition);

            const u32 cellIndex = cellMap[cellKey];
            if (cellIndex == UINT32_MAX)
                return;

            for (u32 i = 0; i < visitedSize; ++i)
                if (visited[i] == cellKey)
                    return;
            visited[visitedSize++] = cellKey;

            const GridCell &cell = cells[cellIndex];
            for (u32 i = cell.Start; i < cell.End; ++i)
            {
                const u32 particleIndex = particleIndices[i];
                if (particleIndex == p_Index)
                    continue;
                const f32 distance = glm::distance2(point, positions[particleIndex]);
                if (distance < r2)
                    std::forward<F>(p_Function)(particleIndex, glm::sqrt(distance));
            }
        };

        for (i32 offsetX = -1; offsetX <= 1; ++offsetX)
            for (i32 offsetY = -1; offsetY <= 1; ++offsetY)
                if constexpr (D == D2)
                {
                    const ivec<D> offset = {offsetX, offsetY};
                    processParticle(offset, std::forward<F>(p_Function));
                }
                else
                    for (i32 offsetZ = -1; offsetZ <= 1; ++offsetZ)
                    {
                        const ivec<D> offset = {offsetX, offsetY, offsetZ};
                        processParticle(offset, std::forward<F>(p_Function));
                    }
    }

  private:
    TKit::Array<ivec<D>, D * D * D + 2 - D> getGridOffsets() const noexcept;

    const SimArray<fvec<D>> *m_Positions = nullptr;

    Grid m_Grid;
    f32 m_Radius;
};
} // namespace Flu