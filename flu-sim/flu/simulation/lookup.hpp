#pragma once

#include "flu/core/glm.hpp"

namespace Flu
{
struct IndexPair
{
    u32 ParticleIndex;
    u32 CellKey;
};

struct ImplicitGrid
{
    DynamicArray<IndexPair> CellKeys;
    DynamicArray<u32> StartIndices;
};

template <Dimension D> class Lookup
{
  public:
    Lookup(const DynamicArray<fvec<D>> *p_Positions) noexcept;

    void UpdateBruteForceLookup(f32 p_Radius) noexcept;
    void UpdateImplicitGridLookup(f32 p_Radius) noexcept;

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

    template <typename F> void ForEachParticleImplicitGrid(const u32 p_Index, F &&p_Function) const noexcept
    {
        if (m_Positions->empty())
            return;
        const f32 r2 = m_Radius * m_Radius;
        static DynamicArray<u8> visited;
        visited.resize(m_Positions->size(), 0);
        visited.assign(m_Positions->size(), 0);

        const auto processParticle = [this, p_Index, r2](const ivec<D> &p_Offset, F &&p_Function) {
            auto &keys = m_ImplicitGrid.CellKeys;
            auto &indices = m_ImplicitGrid.StartIndices;

            const fvec<D> &point = m_Positions->at(p_Index);
            const ivec<D> cellPosition = getCellPosition(point) + p_Offset;
            const u32 cellKey = getCellIndex(cellPosition);
            const u32 startIndex = indices[cellKey];
            for (u32 i = startIndex; i < keys.size() && keys[i].CellKey == cellKey; ++i)
            {
                const u32 particleIndex = keys[i].ParticleIndex;
                if (particleIndex == p_Index || visited[particleIndex])
                    continue;
                visited[particleIndex] = 1;
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

    const DynamicArray<fvec<D>> *m_Positions;

    ImplicitGrid m_ImplicitGrid;
    f32 m_Radius;
};
} // namespace Flu