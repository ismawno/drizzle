#include "flu/simulation/lookup.hpp"
#include "tkit/container/hashable_tuple.hpp"
#include "tkit/profiling/macros.hpp"

namespace Flu
{
template <Dimension D>
Lookup<D>::Lookup(const TKit::DynamicArray<fvec<D>> *p_Positions) noexcept : m_Positions(p_Positions)
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
    const u32 particles = m_Positions->size();

    struct IndexPair
    {
        u32 ParticleIndex;
        u32 CellKey;
    };

    auto &cellMap = m_Grid.CellKeyToIndex;
    auto &particleIndices = m_Grid.ParticleIndices;
    auto &cells = m_Grid.Cells;

    cellMap.resize(particles);
    particleIndices.resize(particles);
    cells.clear();

    IndexPair *keys = m_Allocator.Push<IndexPair>(particles);

    auto &positions = *m_Positions;
    for (u32 i = 0; i < particles; ++i)
    {
        const ivec<D> cellPosition = getCellPosition(positions[i]);
        const u32 key = getCellKey(cellPosition);
        keys[i] = IndexPair{i, key};
        cellMap[i] = UINT32_MAX;
    }
    std::sort(keys, keys + particles, [](const IndexPair &a, const IndexPair &b) {
        if (a.CellKey == b.CellKey)
            return a.ParticleIndex < b.ParticleIndex;
        return a.CellKey < b.CellKey;
    });

    u32 prevKey = keys[0].CellKey;
    GridCell cell{prevKey, 0, 0};
    for (u32 i = 0; i < particles; ++i)
    {
        if (keys[i].CellKey != prevKey)
        {
            cell.End = i;
            cells.push_back(cell);

            cellMap[keys[i].CellKey] = cells.size();
            cell.Key = keys[i].CellKey;
            cell.Start = i;
        }
        particleIndices[i] = keys[i].ParticleIndex;
        prevKey = keys[i].CellKey;
    }
    cell.End = particles;
    cells.push_back(cell);
    m_Allocator.Reset();
}

template <Dimension D> void Lookup<D>::DrawCells(Onyx::RenderContext<D> *p_Context) const noexcept
{
    TKit::HashMap<u32, ivec<D>> keyToPosition;
    for (const fvec<D> &pos : *m_Positions)
    {
        p_Context->Fill(Onyx::Color::WHITE);

        const ivec<D> cellPosition = getCellPosition(pos);
        const u32 key = getCellKey(cellPosition);
        const auto find = keyToPosition.find(key);
        if (find == keyToPosition.end())
            keyToPosition.emplace(key, cellPosition);
        else if (find->second != cellPosition)
            p_Context->Fill(Onyx::Color::RED);

        if constexpr (D == D2)
        {
            const ivec2 br = cellPosition + ivec2{m_Radius, 0};
            const ivec2 tl = cellPosition + ivec2{0, m_Radius};
            p_Context->Line(cellPosition, br, 0.04f);
            p_Context->Line(cellPosition, tl, 0.04f);

            p_Context->Line(br, br + ivec2{0, m_Radius}, 0.04f);
            p_Context->Line(tl, tl + ivec2{m_Radius, 0}, 0.04f);
        }
    }
}

template <Dimension D> ivec<D> Lookup<D>::getCellPosition(const fvec<D> &p_Position) const noexcept
{
    ivec<D> cellPosition{0};
    for (u32 i = 0; i < D; ++i)
        cellPosition[i] = static_cast<i32>(p_Position[i] / m_Radius) - (p_Position[i] < 0.f);
    return cellPosition;
}
template <Dimension D> u32 Lookup<D>::getCellKey(const ivec<D> &p_CellPosition) const noexcept
{
    const u32 particles = m_Positions->size();
    if constexpr (D == D2)
    {
        TKit::HashableTuple<u32, u32> key{p_CellPosition.x, p_CellPosition.y};
        return key() % particles;
    }
    else
    {
        TKit::HashableTuple<u32, u32, u32> key{p_CellPosition.x, p_CellPosition.y, p_CellPosition.z};
        return key() % particles;
    }
}

template <Dimension D> TKit::Array<ivec<D>, D * D * D + 2 - D> Lookup<D>::getGridOffsets() const noexcept
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

template <Dimension D> u32 Lookup<D>::GetCellCount() const noexcept
{
    return m_Grid.Cells.size();
}

template class Lookup<D2>;
template class Lookup<D3>;

} // namespace Flu
