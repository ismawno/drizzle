#pragma once

#include "flu/simulation/solver.hpp"
#ifdef FLU_ENABLE_INSPECTOR

namespace Flu
{
using ParticlePair = std::pair<u32, u32>;

struct LookupPairs
{
    TKit::TreeSet<ParticlePair> Pairs;
    TKit::TreeMap<ParticlePair, u32> DuplicatePairs;
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

    void renderParticle(u32 p_Index) const noexcept;
    void renderPairs(const TKit::TreeSet<ParticlePair> &p_Pairs, u32 p_Selected) const noexcept;
    void renderDuplicatePairs(const TKit::TreeMap<ParticlePair, u32> &p_Pairs, u32 p_Selected) const noexcept;
    void renderPairData(const InspectionData &p_Data, u32 p_Selected) const noexcept;

    auto getPairWiseCollectionST(LookupPairs &p_Pairs) const noexcept
    {
        return [&p_Pairs](const u32 p_Index1, const u32 p_Index2, const f32) {
            const u32 index1 = glm::min(p_Index1, p_Index2);
            const u32 index2 = glm::max(p_Index1, p_Index2);
            const ParticlePair pair{index1, index2};

            if (!p_Pairs.Pairs.insert(pair).second)
            {
                if (p_Pairs.DuplicatePairs.contains(pair))
                    ++p_Pairs.DuplicatePairs[pair];
                else
                    p_Pairs.DuplicatePairs[pair] = 2;
            }
        };
    }
    auto getPairWiseCollectionMT(LookupPairs &p_Pairs) const noexcept
    {
        return [this, &p_Pairs](const u32 p_Index1, const u32 p_Index2, const f32, const u32) {
            const u32 index1 = glm::min(p_Index1, p_Index2);
            const u32 index2 = glm::max(p_Index1, p_Index2);
            const ParticlePair pair{index1, index2};

            std::scoped_lock lock(m_Mutex);
            if (!p_Pairs.Pairs.insert(pair).second)
            {
                if (p_Pairs.DuplicatePairs.contains(pair))
                    ++p_Pairs.DuplicatePairs[pair];
                else
                    p_Pairs.DuplicatePairs[pair] = 2;
            }
        };
    }
    auto getParticleWiseCollection(const u32 p_Index1, LookupPairs &p_Pairs) const noexcept
    {
        return [p_Index1, &p_Pairs](const u32 p_Index2, const f32) {
            const u32 index1 = glm::min(p_Index1, p_Index2);
            const u32 index2 = glm::max(p_Index1, p_Index2);
            const ParticlePair pair{index1, index2};

            if (!p_Pairs.Pairs.insert(pair).second)
            {
                if (p_Pairs.DuplicatePairs.contains(pair))
                    ++p_Pairs.DuplicatePairs[pair];
                else
                    p_Pairs.DuplicatePairs[pair] = 2;
            }
        };
    }

    const Solver<D> *m_Solver;
    SimulationData<D> m_Data;
    Grid m_Grid;
    f32 m_LookupRadius;

    InspectionData m_PairWiseST{};
    InspectionData m_PairWiseMT{};
    InspectionData m_ParticleWise{};

    f32 m_LastInspectionTime = 0.f;
    bool m_WantsToInspect = false;

    mutable std::mutex m_Mutex;
};
} // namespace Flu
#endif