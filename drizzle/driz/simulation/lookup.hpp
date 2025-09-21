#pragma once

#include "driz/core/glm.hpp"
#include "driz/core/core.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/profiling/macros.hpp"

namespace Driz
{
struct GridCell
{
    u32 Key;
    u32 Start;
    u32 End;
};

struct GridData
{
    SimArray<GridCell> Cells;
    SimArray<u32> ParticleIndices;
    SimArray<u32> CellKeyToCellIndex;
};

template <Dimension D> class LookupMethod
{
  public:
    void SetPositions(const SimArray<fvec<D>> *p_Positions);

    void UpdateBruteForceLookup(f32 p_Radius);
    void UpdateGridLookup(f32 p_Radius);

    static ivec<D> GetCellPosition(const fvec<D> &p_Position, f32 p_Radius);
    static u32 GetCellKey(const ivec<D> &p_CellPosition, u32 p_ParticleCount);

    ivec<D> GetCellPosition(const fvec<D> &p_Position) const;
    u32 GetCellKey(const ivec<D> &p_CellPosition) const;

    u32 DrawCells(Onyx::RenderContext<D> *p_Context) const;
    u32 GetCellCount() const;

    template <typename F> void ForEachPair(F &&p_Function, const u32 p_Partitions) const
    {
        const OffsetArray offsets = getGridOffsets();
        Core::ForEach(0, Grid.Cells.GetSize(), p_Partitions,
                      [this, &offsets, &p_Function](const u32 p_Start, const u32 p_End) {
                          TKIT_PROFILE_NSCOPE("Driz::Solver::ForEachPair");
                          const u32 tindex = Core::GetThreadIndex();
                          for (u32 i = p_Start; i < p_End; ++i)
                          {
                              const f32 r2 = Radius * Radius;
                              const auto &positions = *m_Positions;

                              const auto processPair = [r2, tindex, &positions](const u32 p_Index1, const u32 p_Index2,
                                                                                F &&p_Function) {
                                  const f32 distance = glm::distance2(positions[p_Index1], positions[p_Index2]);
                                  if (distance < r2)
                                      std::forward<F>(p_Function)(p_Index1, p_Index2, glm::sqrt(distance), tindex);
                              };

                              const GridCell &cell = Grid.Cells[i];
                              for (u32 i = cell.Start; i < cell.End; ++i)
                              {
                                  const u32 index1 = Grid.ParticleIndices[i];
                                  for (u32 j = i + 1; j < cell.End; ++j)
                                      processPair(index1, Grid.ParticleIndices[j], std::forward<F>(p_Function));

                                  const ivec<D> center = GetCellPosition(positions[index1]);
                                  const u32 cellKey1 = cell.Key;

                                  TKit::Array<u32, s_OffsetCount> visited;
                                  u32 visitedSize = 0;
                                  const auto checkVisited = [&visited, &visitedSize](const u32 p_CellKey) {
                                      for (u32 i = 0; i < visitedSize; ++i)
                                          if (visited[i] == p_CellKey)
                                              return false;
                                      visited[visitedSize++] = p_CellKey;
                                      return true;
                                  };

                                  for (const ivec<D> &offset : offsets)
                                  {
                                      const u32 cellKey2 = GetCellKey(center + offset);
                                      const u32 cellIndex = Grid.CellKeyToCellIndex[cellKey2];
                                      if (cellKey2 > cellKey1 && cellIndex != UINT32_MAX && checkVisited(cellKey2))
                                      {
                                          const GridCell &cell2 = Grid.Cells[cellIndex];
                                          for (u32 j = cell2.Start; j < cell2.End; ++j)
                                              processPair(index1, Grid.ParticleIndices[j], std::forward<F>(p_Function));
                                      }
                                  }
                              }
                          }
                      });
    }

    GridData Grid;
    f32 Radius;

  private:
    static constexpr u32 s_OffsetCount = D * D * D + 2 - D;
    using OffsetArray = TKit::Array<ivec<D>, s_OffsetCount>;

    OffsetArray getGridOffsets() const;

    const SimArray<fvec<D>> *m_Positions = nullptr;
};
} // namespace Driz
