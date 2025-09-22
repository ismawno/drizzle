#include "driz/simulation/solver.hpp"
#include "driz/app/visualization.hpp"
#include "onyx/app/input.hpp"
#include "tkit/profiling/macros.hpp"

namespace Driz
{
template <Dimension D> static f32 computeKernel(const KernelType p_Kernel, const f32 p_Radius, const f32 p_Distance)
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
    return 0.f;
}
template <Dimension D>
static f32 computeKernelSlope(const KernelType p_Kernel, const f32 p_Radius, const f32 p_Distance)
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
    return 0.f;
}

template <Dimension D> f32 Solver<D>::getInfluence(const f32 p_Distance) const
{
    return computeKernel<D>(Settings.KType, Settings.SmoothingRadius, p_Distance);
}
template <Dimension D> f32 Solver<D>::getInfluenceSlope(const f32 p_Distance) const
{
    return computeKernelSlope<D>(Settings.KType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D> f32 Solver<D>::getNearInfluence(const f32 p_Distance) const
{
    return computeKernel<D>(Settings.NearKType, Settings.SmoothingRadius, p_Distance);
}
template <Dimension D> f32 Solver<D>::getNearInfluenceSlope(const f32 p_Distance) const
{
    return computeKernelSlope<D>(Settings.NearKType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D> f32 Solver<D>::getViscosityInfluence(const f32 p_Distance) const
{
    return computeKernel<D>(Settings.ViscosityKType, Settings.SmoothingRadius, p_Distance);
}

template <Dimension D>
Solver<D>::Solver(const SimulationSettings &p_Settings, const SimulationState<D> &p_State) : Settings(p_Settings)
{
    Data.State = p_State;
    Data.Accelerations.Resize(p_State.Positions.GetSize(), fvec<D>{0.f});
    Data.Densities.Resize(p_State.Positions.GetSize(), fvec2{Settings.ParticleMass});
    Data.StagedPositions.Resize(p_State.Positions.GetSize());
    for (auto &densities : m_ThreadDensities)
        densities.Resize(p_State.Positions.GetSize(), fvec2{0.f});
    for (auto &accelerations : m_ThreadAccelerations)
        accelerations.Resize(p_State.Positions.GetSize(), fvec<D>{0.f});
    if constexpr (D == D3)
        Data.UnderMouseInfluence.Resize(p_State.Positions.GetSize(), u8{0});
}

template <Dimension D> void Solver<D>::BeginStep(const f32 p_DeltaTime)
{
    TKIT_PROFILE_NSCOPE("Driz::Solver::BeginStep");
    Data.StagedPositions.Resize(Data.State.Positions.GetSize());

    std::swap(Data.State.Positions, Data.StagedPositions);
    for (u32 i = 0; i < Data.State.Positions.GetSize(); ++i)
    {
        Data.State.Positions[i] = Data.StagedPositions[i] + Data.State.Velocities[i] * p_DeltaTime;
        Data.Densities[i] = fvec2{Settings.ParticleMass};
        Data.Accelerations[i] = fvec<D>{0.f};
        if constexpr (D == D3)
            Data.UnderMouseInfluence[i] = 0;
    }
}
template <Dimension D> void Solver<D>::EndStep()
{
    std::swap(Data.State.Positions, Data.StagedPositions);
}
template <Dimension D> void Solver<D>::ApplyComputedForces(const f32 p_DeltaTime)
{
    TKIT_PROFILE_NSCOPE("Driz::Solver::ApplyComputedForces");

    for (u32 i = 0; i < Data.State.Positions.GetSize(); ++i)
    {
        Data.State.Velocities[i].y += Settings.Gravity * p_DeltaTime / Settings.ParticleMass;
        Data.State.Velocities[i] += Data.Accelerations[i] * p_DeltaTime;
        Data.StagedPositions[i] += Data.State.Velocities[i] * p_DeltaTime;
        encase(i);
    }
}
template <Dimension D> void Solver<D>::AddMouseForce(const fvec<D> &p_MousePos)
{
    for (u32 i = 0; i < Data.State.Positions.GetSize(); ++i)
    {
        const fvec<D> diff = Data.State.Positions[i] - p_MousePos;
        const f32 distance2 = glm::length2(diff);
        if (distance2 < Settings.MouseRadius * Settings.MouseRadius)
        {
            const f32 distance = glm::sqrt(distance2);
            const f32 factor = 1.f - distance / Settings.MouseRadius;
            Data.Accelerations[i] += (factor * Settings.MouseForce / distance) * diff;
            if constexpr (D == D3)
                Data.UnderMouseInfluence[i] = 1;
        }
    }
}

template <Dimension D> void Solver<D>::mergeDensityArrays()
{
    Core::ForEach(0, Data.State.Positions.GetSize(), Settings.Partitions, [this](const u32 p_Start, const u32 p_End) {
        TKIT_PROFILE_NSCOPE("Driz::Solver::MergeDensityArrays");
        for (u32 i = 0; i < DRIZ_MAX_THREADS; ++i)
            for (u32 j = p_Start; j < p_End; ++j)
            {
                Data.Densities[j] += m_ThreadDensities[i][j];
                m_ThreadDensities[i][j] = fvec2{0.f};
            }
    });
}
template <Dimension D> void Solver<D>::mergeAccelerationArrays()
{
    Core::ForEach(0, Data.State.Positions.GetSize(), Settings.Partitions, [this](const u32 p_Start, const u32 p_End) {
        TKIT_PROFILE_NSCOPE("Driz::Solver::MergeAccelerationArrays");
        for (u32 i = 0; i < DRIZ_MAX_THREADS; ++i)
            for (u32 j = p_Start; j < p_End; ++j)
            {
                Data.Accelerations[j] += m_ThreadAccelerations[i][j];
                m_ThreadAccelerations[i][j] = fvec<D>{0.f};
            }
    });
}

template <Dimension D> void Solver<D>::ComputeDensities()
{
    TKIT_PROFILE_NSCOPE("Driz::Solver::ComputeDensities");

    const auto fn = [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance, const u32 p_ThreadIndex) {
        const fvec2 densities = Settings.ParticleMass * fvec2{getInfluence(p_Distance), getNearInfluence(p_Distance)};

        m_ThreadDensities[p_ThreadIndex][p_Index1] += densities;
        m_ThreadDensities[p_ThreadIndex][p_Index2] += densities;
    };
    Lookup.ForEachPair(fn, Settings.Partitions);
    mergeDensityArrays();
}
template <Dimension D> void Solver<D>::AddPressureAndViscosity()
{
    TKIT_PROFILE_NSCOPE("Driz::Solver::AddPressureAndViscosity");
    const auto computeAccelerations = [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
        // Gradient
        const fvec<D> dir = (Data.State.Positions[p_Index1] - Data.State.Positions[p_Index2]) / p_Distance;
        const fvec2 kernels = {getInfluenceSlope(p_Distance), getNearInfluenceSlope(p_Distance)};

        const fvec2 pressures1 = getPressureFromDensity(Data.Densities[p_Index1]);
        const fvec2 pressures2 = getPressureFromDensity(Data.Densities[p_Index2]);

        const fvec2 d1 = Data.Densities[p_Index1];
        const fvec2 d2 = Data.Densities[p_Index2];

        const fvec2 densities = d1 + d2;
        const fvec2 coeffs = (pressures1 + pressures2) * kernels / densities;

        const fvec<D> gradient = (Settings.ParticleMass * (coeffs.x + coeffs.y)) * dir;

        // Viscosity
        const fvec<D> diff = Data.State.Velocities[p_Index2] - Data.State.Velocities[p_Index1];
        const f32 kernel = getViscosityInfluence(p_Distance);

        const f32 u = glm::length(diff);

        const fvec<D> vterm = ((Settings.ViscLinearTerm + Settings.ViscQuadraticTerm * u) * kernel) * diff;

        // Elasticity
        const f32 factor = Settings.ElasticityStrength * (1.f - Settings.ElasticityLength / Settings.SmoothingRadius) *
                           (Settings.ElasticityLength - p_Distance);
        const fvec<D> eterm = factor * dir;
        const fvec<D> acc = eterm + vterm - gradient;

        return std::make_pair(acc / d1.x, acc / d2.x);
    };

    const auto fn = [this, &computeAccelerations](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance,
                                                  const u32 p_ThreadIndex) {
        const auto [acc1, acc2] = computeAccelerations(p_Index1, p_Index2, p_Distance);
        m_ThreadAccelerations[p_ThreadIndex][p_Index1] += acc1;
        m_ThreadAccelerations[p_ThreadIndex][p_Index2] -= acc2;
    };

    Lookup.ForEachPair(fn, Settings.Partitions);
    mergeAccelerationArrays();
}

template <Dimension D> fvec2 Solver<D>::getPressureFromDensity(const Density &p_Density) const
{
    const f32 p1 = Settings.PressureStiffness * (p_Density.x - Settings.TargetDensity);
    const f32 p2 = Settings.NearPressureStiffness * p_Density.y;
    return {p1, p2};
}

template <Dimension D> void Solver<D>::UpdateLookup()
{
    Lookup.SetPositions(&Data.State.Positions);
    Lookup.UpdateGridLookup(Settings.SmoothingRadius);
}

template <Dimension D> void Solver<D>::UpdateAllLookups()
{
    Lookup.SetPositions(&Data.State.Positions);
    Lookup.UpdateBruteForceLookup(Settings.SmoothingRadius);
    Lookup.UpdateGridLookup(Settings.SmoothingRadius);
}

template <Dimension D> void Solver<D>::AddParticle(const fvec<D> &p_Position)
{
    Data.State.Positions.Append(p_Position);
    Data.State.Velocities.Append(fvec<D>{0.f});
    Data.Accelerations.Append(fvec<D>{0.f});
    Data.Densities.Append(fvec2{Settings.ParticleMass});
    for (auto &densities : m_ThreadDensities)
        densities.Append(fvec2{0.f});
    for (auto &accelerations : m_ThreadAccelerations)
        accelerations.Append(fvec<D>{0.f});
    if constexpr (D == D3)
        Data.UnderMouseInfluence.Append(u8{0});
}

template <Dimension D> void Solver<D>::encase(const u32 p_Index)
{
    const f32 factor = 1.f - Settings.EncaseFriction;
    for (u32 j = 0; j < D; ++j)
    {
        if (Data.StagedPositions[p_Index][j] - Settings.ParticleRadius < Data.State.Min[j])
        {
            Data.StagedPositions[p_Index][j] = Data.State.Min[j] + Settings.ParticleRadius;
            Data.State.Velocities[p_Index][j] = -factor * Data.State.Velocities[p_Index][j];
        }
        else if (Data.StagedPositions[p_Index][j] + Settings.ParticleRadius > Data.State.Max[j])
        {
            Data.StagedPositions[p_Index][j] = Data.State.Max[j] - Settings.ParticleRadius;
            Data.State.Velocities[p_Index][j] = -factor * Data.State.Velocities[p_Index][j];
        }
    }
}

template <Dimension D> void Solver<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const
{
    Visualization<D>::DrawBoundingBox(p_Context, Data.State.Min, Data.State.Max,
                                      Onyx::Color::FromHexadecimal("A6B1E1"));
}
template <Dimension D> void Solver<D>::DrawParticles(Onyx::RenderContext<D> *p_Context) const
{
    if constexpr (D == D2)
        Visualization<D2>::DrawParticles(p_Context, Settings, Data.State);
    else
        Visualization<D3>::DrawParticles(p_Context, Settings, Data, Onyx::Color::GREEN, Onyx::Color::ORANGE);
}

template <Dimension D> u32 Solver<D>::GetParticleCount() const
{
    return Data.State.Positions.GetSize();
}

template class Solver<Dimension::D2>;
template class Solver<Dimension::D3>;

} // namespace Driz
