#include "driz/simulation/lookup.hpp"
#include "driz/app/visualization.hpp"
#include "tkit/utils/hash.hpp"
#include "tkit/profiling/macros.hpp"

namespace Driz
{
template <Dimension D> void LookupMethod<D>::SetPositions(const SimArray<f32v<D>> *p_Positions)
{
    m_Positions = p_Positions;
}

template <Dimension D> void LookupMethod<D>::UpdateBruteForceLookup(const f32 p_Radius)
{
    Radius = p_Radius;
}

struct IndexPair
{
    u32 ParticleIndex;
    u32 CellKey;
};

enum class RadixSort : u32
{
    Base8 = 8,
    Base16 = 16
};

template <RadixSort Base> IndexPair *radixSort(IndexPair *p_Keys, const u32 p_Count)
{
    constexpr u32 base = static_cast<u32>(Base);
    constexpr u32 bcount = 1 << base;
    constexpr u32 passes = 32 / base;
    constexpr u32 mask = bcount - 1;

    TKit::Array<u32, bcount> buckets{};
    TKit::ArenaAllocator &arena = Core::GetArena();

    IndexPair *sorted = arena.Allocate<IndexPair>(p_Count);

    for (u32 i = 0; i < passes; ++i)
    {
        for (u32 j = 0; j < bcount; ++j)
            buckets[j] = 0;

        const u32 shift = base * i;
        for (u32 j = 0; j < p_Count; ++j)
        {
            const u32 digit = (p_Keys[j].CellKey >> shift) & mask;
            ++buckets[digit];
        }

        for (u32 j = 1; j < bcount; ++j)
            buckets[j] += buckets[j - 1];

        for (u32 j = p_Count - 1; j < p_Count; --j)
        {
            const u32 digit = (p_Keys[j].CellKey >> shift) & mask;
            sorted[--buckets[digit]] = p_Keys[j];
        }
        std::swap(p_Keys, sorted);
    }
    return sorted;
}

template <Dimension D> void LookupMethod<D>::UpdateGridLookup(const f32 p_Radius)
{
    TKIT_PROFILE_NSCOPE("Driz::LookupMethod::UpdateGridLookup");
    if (m_Positions->IsEmpty())
        return;
    Radius = p_Radius;
    const u32 particles = m_Positions->GetSize();

    Grid.CellKeyToCellIndex.Resize(particles);
    Grid.ParticleIndices.Resize(particles);
    Grid.Cells.Clear();

    TKit::ArenaAllocator &arena = Core::GetArena();
    IndexPair *keys = arena.Allocate<IndexPair>(particles);

    const auto &positions = *m_Positions;
    for (u32 i = 0; i < particles; ++i)
    {
        const i32v<D> cellPosition = getCellPosition(positions[i]);
        const u32 key = getCellKey(cellPosition);
        keys[i] = IndexPair{i, key};
        Grid.CellKeyToCellIndex[i] = UINT32_MAX;
    }

    IndexPair *sortedKeys;
    {
        TKIT_PROFILE_NSCOPE("Driz::LookupMethod::CellKeySorting");
        sortedKeys = radixSort<RadixSort::Base16>(keys, particles);
    }

    u32 prevKey = sortedKeys[0].CellKey;
    GridCell cell{prevKey, 0, 0};
    Grid.CellKeyToCellIndex[prevKey] = 0;

    for (u32 i = 0; i < particles; ++i)
    {
        const IndexPair pair = sortedKeys[i];
        if (pair.CellKey != prevKey)
        {
            cell.End = i;
            Grid.Cells.Append(cell);

            Grid.CellKeyToCellIndex[pair.CellKey] = Grid.Cells.GetSize();
            cell.Key = pair.CellKey;
            cell.Start = i;
        }
        Grid.ParticleIndices[i] = pair.ParticleIndex;
        prevKey = pair.CellKey;
    }

    cell.End = particles;
    Grid.Cells.Append(cell);
    arena.Reset();
}

template <Dimension D> u32 LookupMethod<D>::DrawCells(Onyx::RenderContext<D> *p_Context) const
{
    TKIT_PROFILE_NSCOPE("Driz::LookupMethod::DrawCells");
    const auto isUnique = [](const auto it1, const auto it2, const i32v<D> &p_Position) {
        for (auto it = it1; it != it2; ++it)
            if (*it == p_Position)
                return false;
        return true;
    };

    const auto &positions = *m_Positions;

    u32 cellClashes = 0;
    for (const GridCell &cell : Grid.Cells)
    {
        TKit::Array<i32v<D>, 16> uniquePositions;
        u32 uniqueSize = 0;
        for (u32 i = cell.Start; i < cell.End; ++i)
        {
            const u32 index = Grid.ParticleIndices[i];
            const i32v<D> cellPosition = getCellPosition(positions[index]);
            if (isUnique(uniquePositions.begin(), uniquePositions.begin() + uniqueSize, cellPosition))
                uniquePositions[uniqueSize++] = cellPosition;
        }

        const Onyx::Color color = uniqueSize == 1 ? Onyx::Color::WHITE : Onyx::Color::RED;
        Visualization<D>::DrawCell(p_Context, uniquePositions[0], Radius, color, 0.1f);
        cellClashes += uniqueSize - 1;

        for (u32 i = 1; i < uniqueSize; ++i)
        {
            Visualization<D>::DrawCell(p_Context, uniquePositions[i], Radius, color, 0.1f);
            const f32v<D> pos1 = f32v<D>{uniquePositions[i - 1]} + 0.5f * Radius;
            const f32v<D> pos2 = f32v<D>{uniquePositions[i]} + 0.5f * Radius;

            p_Context->Fill(Onyx::Color::YELLOW);
            if constexpr (D == D2)
                p_Context->Line(pos1, pos2, 0.1f);
            else
                p_Context->Line(pos1, pos2, {.Thickness = 0.1f, .Resolution = Core::Resolution});
        }
    }
    return cellClashes;
}

template <Dimension D> i32v<D> LookupMethod<D>::getCellPosition(const f32v<D> &p_Position) const
{
    i32v<D> cellPosition{0};
    for (u32 i = 0; i < D; ++i)
        cellPosition[i] = static_cast<i32>(p_Position[i] / Radius) - (p_Position[i] < 0.f);
    return cellPosition;
}
template <Dimension D> u32 LookupMethod<D>::getCellKey(const i32v<D> &p_CellPosition) const
{
    return TKit::Hash(p_CellPosition) % m_Positions->GetSize();
}

template <Dimension D> LookupMethod<D>::OffsetArray LookupMethod<D>::getGridOffsets() const
{
    if constexpr (D == D2)
        return {i32v<D>{-1, -1}, i32v<D>{-1, 0}, i32v<D>{-1, 1}, i32v<D>{0, -1},
                i32v<D>{0, 1},   i32v<D>{1, -1}, i32v<D>{1, 0},  i32v<D>{1, 1}};
    else
        return {i32v<D>{-1, -1, -1}, i32v<D>{-1, -1, 0}, i32v<D>{-1, -1, 1}, i32v<D>{-1, 0, -1}, i32v<D>{-1, 0, 0},
                i32v<D>{-1, 0, 1},   i32v<D>{-1, 1, -1}, i32v<D>{-1, 1, 0},  i32v<D>{-1, 1, 1},  i32v<D>{0, -1, -1},
                i32v<D>{0, -1, 0},   i32v<D>{0, -1, 1},  i32v<D>{0, 0, -1},  i32v<D>{0, 0, 1},   i32v<D>{0, 1, -1},
                i32v<D>{0, 1, 0},    i32v<D>{0, 1, 1},   i32v<D>{1, -1, -1}, i32v<D>{1, -1, 0},  i32v<D>{1, -1, 1},
                i32v<D>{1, 0, -1},   i32v<D>{1, 0, 0},   i32v<D>{1, 0, 1},   i32v<D>{1, 1, -1},  i32v<D>{1, 1, 0},
                i32v<D>{1, 1, 1}};
}

template class LookupMethod<D2>;
template class LookupMethod<D3>;

} // namespace Driz
