#include "flu/simulation/solver.hpp"
#include "flu/app/visualization.hpp"
#include "onyx/app/input.hpp"
#include "tkit/profiling/macros.hpp"

namespace Flu
{
template <Dimension D>
static f32 computeKernel(const KernelType p_Kernel, const f32 p_Radius, const f32 p_Distance) noexcept
{
    switch (p_Kernel)
    {
    case KernelType::Spiky2:
        return Kernel<D>::Spiky2(p_Radius, p_Distance);
    case KernelType::Spiky3:
        return Kernel<D>::Spiky3(p_Radius, p_Distance);
    case KernelType::Spiky5:
        return Kernel<D>::Spiky5(p_Radius, p_Distance);
    case KernelType::Poly6:
        return Kernel<D>::Poly6(p_Radius, p_Distance);
    case KernelType::CubicSpline:
        return Kernel<D>::CubicSpline(p_Radius, p_Distance);
    case KernelType::WendlandC2:
        return Kernel<D>::WendlandC2(p_Radius, p_Distance);
    case KernelType::WendlandC4:
        return Kernel<D>::WendlandC4(p_Radius, p_Distance);
    }
}
template <Dimension D>
static f32 computeKernelSlope(const KernelType p_Kernel, const f32 p_Radius, const f32 p_Distance) noexcept
{
    switch (p_Kernel)
    {
    case KernelType::Spiky2:
        return Kernel<D>::Spiky2Slope(p_Radius, p_Distance);
    case KernelType::Spiky3:
        return Kernel<D>::Spiky3Slope(p_Radius, p_Distance);
    case KernelType::Spiky5:
        return Kernel<D>::Spiky5Slope(p_Radius, p_Distance);
    case KernelType::Poly6:
        return Kernel<D>::Poly6Slope(p_Radius, p_Distance);
    case KernelType::CubicSpline:
        return Kernel<D>::CubicSplineSlope(p_Radius, p_Distance);
    case KernelType::WendlandC2:
        return Kernel<D>::WendlandC2Slope(p_Radius, p_Distance);
    case KernelType::WendlandC4:
        return Kernel<D>::WendlandC4Slope(p_Radius, p_Distance);
    }
}

bool SimulationSettings::UsesGrid() const noexcept
{
    return LookupMode == ParticleLookupMode::GridSingleThread || LookupMode == ParticleLookupMode::GridMultiThread;
}
bool SimulationSettings::UsesMultiThread() const noexcept
{
    return LookupMode == ParticleLookupMode::BruteForceMultiThread || LookupMode == ParticleLookupMode::GridMultiThread;
}

template <Dimension D> f32 Solver<D>::getInfluence(const f32 p_Distance) const noexcept
{
    return computeKernel<D>(Settings.KType, Settings.SmoothingRadius, p_Distance);
}
template <Dimension D> f32 Solver<D>::getInfluenceSlope(const f32 p_Distance) const noexcept
{
    return computeKernelSlope<D>(Settings.KType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D> f32 Solver<D>::getNearInfluence(const f32 p_Distance) const noexcept
{
    return computeKernel<D>(Settings.NearKType, Settings.SmoothingRadius, p_Distance);
}
template <Dimension D> f32 Solver<D>::getNearInfluenceSlope(const f32 p_Distance) const noexcept
{
    return computeKernelSlope<D>(Settings.NearKType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D> f32 Solver<D>::getViscosityInfluence(const f32 p_Distance) const noexcept
{
    return computeKernel<D>(Settings.ViscosityKType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D>
fvec<D> Solver<D>::computePairwisePressureGradient(const u32 p_Index1, const u32 p_Index2,
                                                   const f32 p_Distance) const noexcept
{
    const fvec<D> dir = (m_Data.Positions[p_Index1] - m_Data.Positions[p_Index2]) / p_Distance;
    const fvec2 kernels = {getInfluenceSlope(p_Distance), getNearInfluenceSlope(p_Distance)};

    const fvec2 pressures1 = GetPressureFromDensity(m_Data.Densities[p_Index1]);
    const fvec2 pressures2 = GetPressureFromDensity(m_Data.Densities[p_Index2]);

    const fvec2 densities = 0.5f * (m_Data.Densities[p_Index1] + m_Data.Densities[p_Index2]);
    const fvec2 coeffs = 0.5f * (pressures1 + pressures2) * kernels / densities;

    return (Settings.ParticleMass * (coeffs.x + coeffs.y)) * dir;
}

template <Dimension D>
fvec<D> Solver<D>::computePairwiseViscosityTerm(const u32 p_Index1, const u32 p_Index2,
                                                const f32 p_Distance) const noexcept
{
    const fvec<D> diff = m_Data.Velocities[p_Index2] - m_Data.Velocities[p_Index1];
    const f32 kernel = getViscosityInfluence(p_Distance);

    const f32 u = glm::length(diff);
    return ((Settings.ViscLinearTerm + Settings.ViscQuadraticTerm * u) * kernel) * diff;
}

template <Dimension D> void Solver<D>::BeginStep(const f32 p_DeltaTime) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::BeginStep");
    m_Data.StagedPositions.resize(m_Data.Positions.size());

    std::swap(m_Data.Positions, m_Data.StagedPositions);
    for (u32 i = 0; i < m_Data.Positions.size(); ++i)
    {
        m_Data.Positions[i] = m_Data.StagedPositions[i] + m_Data.Velocities[i] * p_DeltaTime;
        m_Data.Densities[i] = fvec2{Settings.ParticleMass};
        m_Data.Accelerations[i] = fvec<D>{0.f};
    }
}
template <Dimension D> void Solver<D>::EndStep() noexcept
{
    std::swap(m_Data.Positions, m_Data.StagedPositions);
}
template <Dimension D> void Solver<D>::ApplyComputedForces(const f32 p_DeltaTime) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::ApplyComputedForces");

    for (u32 i = 0; i < m_Data.Positions.size(); ++i)
    {
        m_Data.Velocities[i].y += Settings.Gravity * p_DeltaTime / Settings.ParticleMass;
        m_Data.Velocities[i] += m_Data.Accelerations[i] * p_DeltaTime;
        m_Data.StagedPositions[i] += m_Data.Velocities[i] * p_DeltaTime;
        encase(i);
    }
}
template <Dimension D> void Solver<D>::AddMouseForce(const fvec<D> &p_MousePos) noexcept
{
    for (u32 i = 0; i < m_Data.Positions.size(); ++i)
    {
        const fvec<D> diff = m_Data.Positions[i] - p_MousePos;
        const f32 distance2 = glm::length2(diff);
        if (distance2 < Settings.MouseRadius * Settings.MouseRadius)
        {
            const f32 distance = glm::sqrt(distance2);
            const f32 factor = 1.f - distance / Settings.MouseRadius;
            m_Data.Accelerations[i] += (factor * Settings.MouseForce / distance) * diff;
        }
    }
}

template <Dimension D> void Solver<D>::mergeDensityArrays() noexcept
{
    Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
        for (u32 i = 0; i < FLU_THREAD_COUNT; ++i)
            for (u32 j = p_Start; j < p_End; ++j)
            {
                m_Data.Densities[j] += m_ThreadDensities[i][j];
                m_ThreadDensities[i][j] = fvec2{0.f};
            }
    });
}
template <Dimension D> void Solver<D>::mergeAccelerationArrays() noexcept
{
    Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
        for (u32 i = 0; i < FLU_THREAD_COUNT; ++i)
            for (u32 j = p_Start; j < p_End; ++j)
            {
                m_Data.Accelerations[j] += m_ThreadAccelerations[i][j];
                m_ThreadAccelerations[i][j] = fvec<D>{0.f};
            }
    });
}

template <Dimension D> void Solver<D>::ComputeDensities() noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::ComputeDensities");

    const auto pairWiseST = [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
        const fvec2 densities = Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};
        m_Data.Densities[p_Index1] += densities;
        m_Data.Densities[p_Index2] += densities;
    };
    const auto pairWiseMT = [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance,
                                   const u32 p_ThreadIndex) {
        const fvec2 densities = Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};

        m_ThreadDensities[p_ThreadIndex][p_Index1] += densities;
        m_ThreadDensities[p_ThreadIndex][p_Index2] += densities;
    };

    const auto bruteForcePairWiseST = [this, pairWiseST]() { m_Lookup.ForEachPairBruteForceST(pairWiseST); };
    const auto bruteForcePairWiseMT = [this, pairWiseMT]() {
        m_Lookup.ForEachPairBruteForceMT(pairWiseMT);
        mergeDensityArrays();
    };

    const auto gridPairWiseST = [this, pairWiseST]() { m_Lookup.ForEachPairGridST(pairWiseST); };
    const auto gridPairWiseMT = [this, pairWiseMT]() {
        m_Lookup.ForEachPairGridMT(pairWiseMT);
        mergeDensityArrays();
    };

    const auto bruteForceParticleWiseST = [this]() {
        for (u32 i = 0; i < m_Data.Positions.size(); ++i)
        {
            fvec2 densities{Settings.ParticleMass};
            m_Lookup.ForEachParticleBruteForce(i, [this, &densities](const u32, const f32 p_Distance) {
                densities += Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};
            });
            m_Data.Densities[i] = densities;
        }
    };
    const auto bruteForceParticleWiseMT = [this]() {
        Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
            for (u32 i = p_Start; i < p_End; ++i)
            {
                fvec2 densities{Settings.ParticleMass};
                m_Lookup.ForEachParticleBruteForce(i, [this, &densities](const u32, const f32 p_Distance) {
                    densities += Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};
                });
                m_Data.Densities[i] = densities;
            }
        });
    };

    const auto gridParticleWiseST = [this]() {
        for (u32 i = 0; i < m_Data.Positions.size(); ++i)
        {
            fvec2 densities{Settings.ParticleMass};
            m_Lookup.ForEachParticleGrid(i, [this, &densities](const u32, const f32 p_Distance) {
                densities += Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};
            });
            m_Data.Densities[i] = densities;
        }
    };
    const auto gridParticleWiseMT = [this]() {
        Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
            for (u32 i = p_Start; i < p_End; ++i)
            {
                fvec2 densities{Settings.ParticleMass};
                m_Lookup.ForEachParticleGrid(i, [this, &densities](const u32, const f32 p_Distance) {
                    densities += Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};
                });
                m_Data.Densities[i] = densities;
            }
        });
    };

    ForEachWithinSmoothingRadius(bruteForcePairWiseST, bruteForcePairWiseMT, gridPairWiseST, gridPairWiseMT,
                                 bruteForceParticleWiseST, bruteForceParticleWiseMT, gridParticleWiseST,
                                 gridParticleWiseMT);
}
template <Dimension D> void Solver<D>::AddPressureAndViscosity() noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::PressureAndViscosity");
    const auto computeAccelerations = [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
        const fvec<D> gradient = computePairwisePressureGradient(p_Index1, p_Index2, p_Distance);
        const fvec<D> term = computePairwiseViscosityTerm(p_Index1, p_Index2, p_Distance);

        const fvec<D> acc1 = term - gradient / m_Data.Densities[p_Index1].x;
        const fvec<D> acc2 = term - gradient / m_Data.Densities[p_Index2].x;

        return std::make_pair(acc1, acc2);
    };

    const auto pairWiseST = [this, computeAccelerations](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
        const auto [acc1, acc2] = computeAccelerations(p_Index1, p_Index2, p_Distance);
        m_Data.Accelerations[p_Index1] += acc1;
        m_Data.Accelerations[p_Index2] -= acc2;
    };
    const auto pairWiseMT = [this, computeAccelerations](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance,
                                                         const u32 p_ThreadIndex) {
        const auto [acc1, acc2] = computeAccelerations(p_Index1, p_Index2, p_Distance);
        m_ThreadAccelerations[p_ThreadIndex][p_Index1] += acc1;
        m_ThreadAccelerations[p_ThreadIndex][p_Index2] -= acc2;
    };

    const auto bruteForcePairWiseST = [this, pairWiseST]() { m_Lookup.ForEachPairBruteForceST(pairWiseST); };
    const auto bruteForcePairWiseMT = [this, pairWiseMT]() {
        m_Lookup.ForEachPairBruteForceMT(pairWiseMT);
        mergeAccelerationArrays();
    };

    const auto gridPairWiseST = [this, pairWiseST]() { m_Lookup.ForEachPairGridST(pairWiseST); };
    const auto gridPairWiseMT = [this, pairWiseMT]() {
        m_Lookup.ForEachPairGridMT(pairWiseMT);
        mergeAccelerationArrays();
    };

    const auto bruteForceParticleWiseST = [this]() {
        for (u32 i = 0; i < m_Data.Positions.size(); ++i)
        {
            fvec<D> gradient{0.f};
            fvec<D> vterm{0.f};
            m_Lookup.ForEachParticleBruteForce(
                i, [this, i, &gradient, &vterm](const u32 p_Index, const f32 p_Distance) {
                    const fvec<D> g = computePairwisePressureGradient(i, p_Index, p_Distance);
                    const fvec<D> t = computePairwiseViscosityTerm(i, p_Index, p_Distance);
                    gradient += g;
                    vterm += t;
                });
            m_Data.Accelerations[i] = vterm - gradient / m_Data.Densities[i].x;
        }
    };
    const auto bruteForceParticleWiseMT = [this]() {
        Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
            for (u32 i = p_Start; i < p_End; ++i)
            {
                fvec<D> gradient{0.f};
                fvec<D> vterm{0.f};
                m_Lookup.ForEachParticleBruteForce(
                    i, [this, i, &gradient, &vterm](const u32 p_Index, const f32 p_Distance) {
                        const fvec<D> g = computePairwisePressureGradient(i, p_Index, p_Distance);
                        const fvec<D> t = computePairwiseViscosityTerm(i, p_Index, p_Distance);
                        gradient += g;
                        vterm += t;
                    });
                m_Data.Accelerations[i] = vterm - gradient / m_Data.Densities[i].x;
            }
        });
    };

    const auto gridParticleWiseST = [this]() {
        for (u32 i = 0; i < m_Data.Positions.size(); ++i)
        {
            fvec<D> gradient{0.f};
            fvec<D> vterm{0.f};
            m_Lookup.ForEachParticleGrid(i, [this, i, &gradient, &vterm](const u32 p_Index, const f32 p_Distance) {
                const fvec<D> g = computePairwisePressureGradient(i, p_Index, p_Distance);
                const fvec<D> t = computePairwiseViscosityTerm(i, p_Index, p_Distance);
                gradient += g;
                vterm += t;
            });
            m_Data.Accelerations[i] = vterm - gradient / m_Data.Densities[i].x;
        }
    };
    const auto gridParticleWiseMT = [this]() {
        Core::ForEach(0, m_Data.Positions.size(), [this](const u32 p_Start, const u32 p_End, const u32) {
            for (u32 i = p_Start; i < p_End; ++i)
            {
                fvec<D> gradient{0.f};
                fvec<D> vterm{0.f};
                m_Lookup.ForEachParticleGrid(i, [this, i, &gradient, &vterm](const u32 p_Index, const f32 p_Distance) {
                    const fvec<D> g = computePairwisePressureGradient(i, p_Index, p_Distance);
                    const fvec<D> t = computePairwiseViscosityTerm(i, p_Index, p_Distance);
                    gradient += g;
                    vterm += t;
                });
                m_Data.Accelerations[i] = vterm - gradient / m_Data.Densities[i].x;
            }
        });
    };
    ForEachWithinSmoothingRadius(bruteForcePairWiseST, bruteForcePairWiseMT, gridPairWiseST, gridPairWiseMT,
                                 bruteForceParticleWiseST, bruteForceParticleWiseMT, gridParticleWiseST,
                                 gridParticleWiseMT);
}

template <Dimension D> fvec2 Solver<D>::GetPressureFromDensity(const Density &p_Density) const noexcept
{
    const f32 p1 = Settings.PressureStiffness * (p_Density.x - Settings.TargetDensity);
    const f32 p2 = Settings.NearPressureStiffness * p_Density.y;
    return {p1, p2};
}

template <Dimension D> void Solver<D>::UpdateLookup() noexcept
{
    m_Lookup.SetPositions(&m_Data.Positions);
    switch (Settings.LookupMode)
    {
    case ParticleLookupMode::BruteForceMultiThread:
    case ParticleLookupMode::BruteForceSingleThread:
        m_Lookup.UpdateBruteForceLookup(Settings.SmoothingRadius);
        break;
    case ParticleLookupMode::GridMultiThread:
    case ParticleLookupMode::GridSingleThread:
        m_Lookup.UpdateGridLookup(Settings.SmoothingRadius);
        break;
    }
}

template <Dimension D> void Solver<D>::UpdateAllLookups() noexcept
{
    m_Lookup.SetPositions(&m_Data.Positions);
    m_Lookup.UpdateBruteForceLookup(Settings.SmoothingRadius);
    m_Lookup.UpdateGridLookup(Settings.SmoothingRadius);
}

template <Dimension D> void Solver<D>::AddParticle(const fvec<D> &p_Position) noexcept
{
    m_Data.Positions.push_back(p_Position);
    m_Data.Velocities.push_back(fvec<D>{0.f});
    m_Data.Accelerations.push_back(fvec<D>{0.f});
    m_Data.Densities.push_back(fvec2{Settings.ParticleMass});
    for (auto &densities : m_ThreadDensities)
        densities.push_back(fvec2{0.f});
    for (auto &accelerations : m_ThreadAccelerations)
        accelerations.push_back(fvec<D>{0.f});
}

template <Dimension D> void Solver<D>::encase(const u32 p_Index) noexcept
{
    const f32 factor = 1.f - Settings.EncaseFriction;
    for (u32 j = 0; j < D; ++j)
    {
        if (m_Data.StagedPositions[p_Index][j] - Settings.ParticleRadius < Bounds.Min[j])
        {
            m_Data.StagedPositions[p_Index][j] = Bounds.Min[j] + Settings.ParticleRadius;
            m_Data.Velocities[p_Index][j] = -factor * m_Data.Velocities[p_Index][j];
        }
        else if (m_Data.StagedPositions[p_Index][j] + Settings.ParticleRadius > Bounds.Max[j])
        {
            m_Data.StagedPositions[p_Index][j] = Bounds.Max[j] - Settings.ParticleRadius;
            m_Data.Velocities[p_Index][j] = -factor * m_Data.Velocities[p_Index][j];
        }
    }
}

template <Dimension D> void Solver<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept
{
    Visualization<D>::DrawBoundingBox(p_Context, Bounds.Min, Bounds.Max, Onyx::Color::FromHexadecimal("A6B1E1", false));
}
template <Dimension D> void Solver<D>::DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept
{
    const f32 particleSize = 2.f * Settings.ParticleRadius;

    const Onyx::Gradient gradient{Settings.Gradient};
    for (u32 i = 0; i < m_Data.Positions.size(); ++i)
    {
        const fvec<D> &pos = m_Data.Positions[i];
        const fvec<D> &vel = m_Data.Velocities[i];

        const f32 speed = glm::min(Settings.FastSpeed, glm::length(vel));
        const Onyx::Color color = gradient.Evaluate(speed / Settings.FastSpeed);
        Visualization<D>::DrawParticle(p_Context, pos, particleSize, color);
    }
}

template <Dimension D> u32 Solver<D>::GetParticleCount() const noexcept
{
    return m_Data.Positions.size();
}

template <Dimension D> const Lookup<D> &Solver<D>::GetLookup() const noexcept
{
    return m_Lookup;
}
template <Dimension D> const SimulationData<D> &Solver<D>::GetData() const noexcept
{
    return m_Data;
}

template class Solver<Dimension::D2>;
template class Solver<Dimension::D3>;

} // namespace Flu