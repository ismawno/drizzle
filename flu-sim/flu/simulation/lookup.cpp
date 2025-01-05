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

template <Dimension D> void Lookup<D>::UpdateImplicitGridLookup(const f32 p_Radius) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Lookup::UpdateGrid");
    auto &keys = m_ImplicitGrid.CellKeys;
    m_Radius = p_Radius;

    keys.clear();
    for (u32 i = 0; i < m_Positions->size(); ++i)
    {
        const ivec<D> cellPosition = getCellPosition((*m_Positions)[i]);
        const u32 index = getCellIndex(cellPosition);
        keys.push_back({i, index});
    }
    std::sort(keys.begin(), keys.end(), [](const IndexPair &a, const IndexPair &b) { return a.CellKey < b.CellKey; });

    u32 prevIndex = 0;
    auto &indices = m_ImplicitGrid.StartIndices;
    indices.resize(keys.size(), UINT32_MAX);
    for (u32 i = 0; i < keys.size(); ++i)
    {
        if (i == 0 || keys[i].CellKey != prevIndex)
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

template class Lookup<D2>;
template class Lookup<D3>;

} // namespace Flu
