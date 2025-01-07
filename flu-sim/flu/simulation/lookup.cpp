#include "flu/simulation/lookup.hpp"
#include "tkit/container/hashable_tuple.hpp"
#include "tkit/profiling/macros.hpp"

namespace Flu
{
template <Dimension D> Lookup<D>::Lookup(const DynamicArray<fvec<D>> *p_Positions) noexcept : m_Positions(p_Positions)
{
}

template <Dimension D> void Lookup<D>::UpdateBruteForceLookup(const f32 p_Radius) noexcept
{
    m_Radius = p_Radius;
}

template <Dimension D> void Lookup<D>::UpdateGridLookup(const f32 p_Radius) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Lookup::UpdateGrid");
    if (m_Positions->empty())
        return;
    m_Radius = p_Radius;

    auto &keys = m_Grid.CellKeys;
    keys.clear();
    for (u32 i = 0; i < m_Positions->size(); ++i)
    {
        const ivec<D> cellPosition = getCellPosition((*m_Positions)[i]);
        const u32 index = getCellIndex(cellPosition);
        keys.push_back({i, index});
    }
    std::sort(keys.begin(), keys.end(), [](const IndexPair &a, const IndexPair &b) {
        if (a.CellKey == b.CellKey)
            return a.ParticleIndex < b.ParticleIndex;
        return a.CellKey < b.CellKey;
    });

    auto &indices = m_Grid.StartIndices;
    indices.resize(keys.size(), UINT32_MAX);

    u32 prevIndex = keys[0].CellKey;
    indices[prevIndex] = 0;
    for (u32 i = 1; i < keys.size(); ++i)
    {
        if (keys[i].CellKey != prevIndex)
            indices[keys[i].CellKey] = i;

        prevIndex = keys[i].CellKey;
    }
}

template <Dimension D> ivec<D> Lookup<D>::getCellPosition(const fvec<D> &p_Position) const noexcept
{
    ivec<D> cellPosition{0};
    for (u32 i = 0; i < D; ++i)
        cellPosition[i] = static_cast<i32>(p_Position[i] / m_Radius);
    return cellPosition;
}
template <Dimension D> u32 Lookup<D>::getCellIndex(const ivec<D> &p_CellPosition) const noexcept
{
    const u32 particles = static_cast<u32>(m_Positions->size());
    if constexpr (D == D2)
    {
        TKit::HashableTuple<u32, u32> key{p_CellPosition.x, p_CellPosition.y};
        return static_cast<u32>(key() % particles);
    }
    else
    {
        TKit::HashableTuple<u32, u32, u32> key{p_CellPosition.x, p_CellPosition.y, p_CellPosition.z};
        return static_cast<u32>(key() % particles);
    }
}

template <Dimension D> std::array<ivec<D>, D * D * D + 2 - D> Lookup<D>::getGridOffsets() const noexcept
{
    if constexpr (D == D2)
        return {ivec<D>{-1, -1}, ivec<D>{-1, 0}, ivec<D>{-1, 1}, ivec<D>{0, -1},
                ivec<D>{0, 1},   ivec<D>{1, -1}, ivec<D>{1, 0},  ivec<D>{1, 1}};
    else
        return {ivec<D>{-1, -1, -1}, ivec<D>{-1, -1, 0}, ivec<D>{-1, -1, 1}, ivec<D>{-1, 0, -1}, ivec<D>{-1, 0, 0},
                ivec<D>{-1, 0, 1},   ivec<D>{-1, 1, -1}, ivec<D>{-1, 1, 0},  ivec<D>{-1, 1, 1},  ivec<D>{0, -1, -1},
                ivec<D>{0, -1, 0},   ivec<D>{0, -1, 1},  ivec<D>{0, 0, -1},  ivec<D>{0, 0, 1},   ivec<D>{0, 1, -1},
                ivec<D>{0, 1, 0},    ivec<D>{0, 1, 1},   ivec<D>{1, -1, -1}, ivec<D>{1, -1, 0},  ivec<D>{1, -1, 1},
                ivec<D>{1, 0, -1},   ivec<D>{1, 0, 0},   ivec<D>{1, 0, 1},   ivec<D>{1, 1, -1},  ivec<D>{1, 1, 0},
                ivec<D>{1, 1, 1}};
}

template class Lookup<D2>;
template class Lookup<D3>;

} // namespace Flu
