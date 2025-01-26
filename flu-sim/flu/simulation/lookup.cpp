#include "flu/simulation/lookup.hpp"
#include "flu/app/visualization.hpp"
#include "tkit/utilities/hash.hpp"
#include "tkit/profiling/macros.hpp"

namespace Flu
{
template <Dimension D> void Lookup<D>::SetPositions(const SimArray<fvec<D>> *p_Positions) noexcept
{
    m_Positions = p_Positions;
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

    IndexPair *keys = Core::GetArena().Allocate<IndexPair>(particles);

    const auto &positions = *m_Positions;
    for (u32 i = 0; i < particles; ++i)
    {
        const ivec<D> cellPosition = GetCellPosition(positions[i]);
        const u32 key = GetCellKey(cellPosition);
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
    Core::GetArena().Reset();
}

template <Dimension D> u32 Lookup<D>::DrawCells(Onyx::RenderContext<D> *p_Context) const noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Lookup::DrawCells");
    const auto isUnique = [](const auto it1, const auto it2, const ivec<D> &p_Position) {
        for (auto it = it1; it != it2; ++it)
            if (*it == p_Position)
                return false;
        return true;
    };

    const auto &positions = *m_Positions;

    u32 cellClashes = 0;
    for (const GridCell &cell : m_Grid.Cells)
    {
        TKit::Array<ivec<D>, 8> uniquePositions;
        u32 uniqueSize = 0;
        for (u32 i = cell.Start; i < cell.End; ++i)
        {
            const u32 index = m_Grid.ParticleIndices[i];
            const ivec<D> cellPosition = GetCellPosition(positions[index]);
            if (isUnique(uniquePositions.begin(), uniquePositions.begin() + uniqueSize, cellPosition))
                uniquePositions[uniqueSize++] = cellPosition;
        }

        const Onyx::Color color = uniqueSize == 1 ? Onyx::Color::WHITE : Onyx::Color::RED;
        Visualization<D>::DrawCell(p_Context, uniquePositions[0], m_Radius, color, 0.04f);
        cellClashes += uniqueSize - 1;

        for (u32 i = 1; i < uniqueSize; ++i)
        {
            Visualization<D>::DrawCell(p_Context, uniquePositions[i], m_Radius, color, 0.04f);
            const fvec<D> pos1 = fvec<D>{uniquePositions[i - 1]} + 0.5f * m_Radius;
            const fvec<D> pos2 = fvec<D>{uniquePositions[i]} + 0.5f * m_Radius;

            p_Context->Fill(Onyx::Color::YELLOW);
            p_Context->Line(pos1, pos2, 0.08f);
        }
    }
    return cellClashes;
}

template <Dimension D> ivec<D> Lookup<D>::GetCellPosition(const fvec<D> &p_Position, const f32 p_Radius) noexcept
{
    ivec<D> cellPosition{0};
    for (u32 i = 0; i < D; ++i)
        cellPosition[i] = static_cast<i32>(p_Position[i] / p_Radius) - (p_Position[i] < 0.f);
    return cellPosition;
}
template <Dimension D> u32 Lookup<D>::GetCellKey(const ivec<D> &p_CellPosition, const u32 p_ParticleCount) noexcept
{
    return TKit::Hash(p_CellPosition) % p_ParticleCount;
}

template <Dimension D> ivec<D> Lookup<D>::GetCellPosition(const fvec<D> &p_Position) const noexcept
{
    return GetCellPosition(p_Position, m_Radius);
}
template <Dimension D> u32 Lookup<D>::GetCellKey(const ivec<D> &p_CellPosition) const noexcept
{
    return GetCellKey(p_CellPosition, m_Positions->size());
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

template <Dimension D> const Grid &Lookup<D>::GetGrid() const noexcept
{
    return m_Grid;
}
template <Dimension D> f32 Lookup<D>::GetRadius() const noexcept
{
    return m_Radius;
}

template class Lookup<D2>;
template class Lookup<D3>;

} // namespace Flu
