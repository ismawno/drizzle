#pragma once

#include "flu/simulation/solver.hpp"

namespace Flu
{
using ParticlePair = std::pair<u32, u32>;

struct LookupPairs
{
    TKit::TreeSet<ParticlePair> Pairs;
    TKit::TreeSet<ParticlePair> DuplicatePairs;
};

struct InspectionData
{
    LookupPairs BruteForcePairs;
    LookupPairs GridPairs;

    TKit::TreeSet<ParticlePair> MissingInBruteForce;
    TKit::TreeSet<ParticlePair> MissingInGrid;
};

template <Dimension D> class Inspector
{
  public:
    explicit Inspector(const Solver<D> *p_Solver) noexcept;

    void Render() noexcept;
    void Inspect() noexcept;

    bool WantsToInspect() const noexcept;

  private:
    void renderInspectionData() const noexcept;
    void renderGridData() const noexcept;
    void renderParticleData() const noexcept;

    void renderParticle(const u32 p_Index) const noexcept;
    void renderPairs(const TKit::TreeSet<ParticlePair> &p_Pairs, const u32 p_Selected) const noexcept;
    void renderPairData(const InspectionData &p_Data, const u32 p_Selected) const noexcept;

    static auto getPairCollectionIterPair(LookupPairs &p_Pairs) noexcept
    {
        return [&p_Pairs](const u32 p_Index1, const u32 p_Index2, const f32) {
            const u32 index1 = glm::min(p_Index1, p_Index2);
            const u32 index2 = glm::max(p_Index1, p_Index2);

            if (!p_Pairs.Pairs.insert({index1, index2}).second)
                p_Pairs.DuplicatePairs.insert({index1, index2});
        };
    }
    static auto getPairCollectionIterParticle(const u32 p_Index1, LookupPairs &p_Pairs) noexcept
    {
        return [p_Index1, &p_Pairs](const u32 p_Index2, const f32) {
            const u32 index1 = glm::min(p_Index1, p_Index2);
            const u32 index2 = glm::max(p_Index1, p_Index2);

            if (!p_Pairs.Pairs.insert({index1, index2}).second)
                p_Pairs.DuplicatePairs.insert({index1, index2});
        };
    }

    const Solver<D> *m_Solver;
    SimulationData<D> m_Data;
    Grid m_Grid;
    f32 m_LookupRadius;

    InspectionData m_PairIterData{};
    InspectionData m_ParticleIterData{};
    f32 m_LastInspectionTime = 0.f;
    bool m_WantsToInspect = false;
};
} // namespace Flu