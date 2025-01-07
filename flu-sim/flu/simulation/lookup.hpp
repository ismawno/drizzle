#pragma once

#include "flu/core/glm.hpp"
#include <array>

namespace Flu
{
struct IndexPair
{
    u32 ParticleIndex;
    u32 CellKey;
};

struct Grid
{
    DynamicArray<IndexPair> CellKeys;
    DynamicArray<u32> StartIndices;
};

template <Dimension D> class Lookup
{
  public:
    Lookup(const DynamicArray<fvec<D>> *p_Positions) noexcept;

    void UpdateBruteForceLookup(f32 p_Radius) noexcept;
    void UpdateGridLookup(f32 p_Radius) noexcept;

    template <typename F> void ForEachPairBruteForce(F &&p_Function) const noexcept
    {
        const f32 r2 = m_Radius * m_Radius;
        for (u32 i = 0; i < m_Positions->size(); ++i)
            for (u32 j = i + 1; j < m_Positions->size(); ++j)
            {
                const f32 distance = glm::distance2(m_Positions->at(i), m_Positions->at(j));
                if (distance < r2)
                    std::forward<F>(p_Function)(i, j, glm::sqrt(distance));
            }
    }

    template <typename F> void ForEachPairGrid(F &&p_Function) const noexcept
    {
        const auto offsets = getGridOffsets();
        const f32 r2 = m_Radius * m_Radius;

        const auto isVisited = [](auto it1, auto it2, const u32 p_CellIndex) {
            for (auto it = it1; it != it2; ++it)
                if (*it == p_CellIndex)
                    return true;
            return false;
        };

        const auto processPair = [this, r2](const u32 p_Index1, const u32 p_Index2, F &&p_Function) {
            const f32 distance = glm::distance2(m_Positions->at(p_Index1), m_Positions->at(p_Index2));
            if (distance < r2)
                std::forward<F>(p_Function)(p_Index1, p_Index2, glm::sqrt(distance));
        };

        auto &keys = m_Grid.CellKeys;
        auto &indices = m_Grid.StartIndices;

        for (u32 i = 0; i < keys.size(); ++i)
        {
            const u32 cellKey1 = keys[i].CellKey;
            const u32 index1 = keys[i].ParticleIndex;
            for (u32 j = i + 1; j < keys.size() && keys[j].CellKey == cellKey1; ++j)
                processPair(index1, keys[j].ParticleIndex, std::forward<F>(p_Function));

            const ivec<D> center = getCellPosition(m_Positions->at(index1));
            std::array<u32, offsets.size()> visited{};
            u32 visitedIndex = 0;
            for (const ivec<D> &offset : offsets)
            {
                const u32 cellKey2 = getCellIndex(center + offset);
                if (cellKey2 <= cellKey1 || isVisited(visited.begin(), visited.begin() + visitedIndex, cellKey2))
                    continue;
                visited[visitedIndex++] = cellKey2;

                const u32 startIndex = indices[cellKey2];
                for (u32 j = startIndex; j < keys.size() && keys[j].CellKey == cellKey2; ++j)
                    processPair(index1, keys[j].ParticleIndex, std::forward<F>(p_Function));
            }
        }
    }

    template <typename F> void ForEachParticleBruteForce(const u32 p_Index, F &&p_Function) const noexcept
    {
        const f32 r2 = m_Radius * m_Radius;
        for (u32 i = 0; i < m_Positions->size(); ++i)
        {
            if (p_Index == i)
                continue;
            const f32 distance = glm::distance2(m_Positions->at(p_Index), m_Positions->at(i));
            if (distance < r2)
                std::forward<F>(p_Function)(i, glm::sqrt(distance));
        }
    }

    template <typename F> void ForEachParticleGrid(const u32 p_Index, F &&p_Function) const noexcept
    {
        if (m_Positions->empty())
            return;
        const f32 r2 = m_Radius * m_Radius;

        std::array<u32, 9> visited{};
        u32 visitedIndex = 0;
        const auto processParticle = [this, p_Index, r2, &visited, &visitedIndex](const ivec<D> &p_Offset,
                                                                                  F &&p_Function) {
            auto &keys = m_Grid.CellKeys;
            auto &indices = m_Grid.StartIndices;

            const fvec<D> &point = m_Positions->at(p_Index);
            const ivec<D> cellPosition = getCellPosition(point) + p_Offset;
            const u32 cellKey = getCellIndex(cellPosition);
            for (u32 i = 0; i < visitedIndex; ++i)
                if (visited[i] == cellKey)
                    return;
            visited[visitedIndex++] = cellKey;

            const u32 startIndex = indices[cellKey];
            for (u32 i = startIndex; i < keys.size() && keys[i].CellKey == cellKey; ++i)
            {
                const u32 particleIndex = keys[i].ParticleIndex;
                if (particleIndex == p_Index)
                    continue;
                const f32 distance = glm::distance2(point, m_Positions->at(particleIndex));
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
    ivec<D> getCellPosition(const fvec<D> &p_Position) const noexcept;
    u32 getCellIndex(const ivec<D> &p_CellPosition) const noexcept;
    std::array<ivec<D>, D * D * D + 2 - D> getGridOffsets() const noexcept;

    const DynamicArray<fvec<D>> *m_Positions;

    Grid m_Grid;
    f32 m_Radius;
};
} // namespace Flu