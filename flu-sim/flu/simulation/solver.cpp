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

template <Dimension D> void Solver<D>::BeginStep(const f32 p_DeltaTime) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::BeginStep");
    m_PredictedPositions.resize(m_Positions.size());
    ApplyExternal(p_DeltaTime);
    std::swap(m_Positions, m_PredictedPositions);

    UpdateLookup();
    ComputeDensities();

    ApplyPressureAndViscosity();
}
template <Dimension D> void Solver<D>::EndStep(const f32 p_DeltaTime) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::EndStep");
    std::swap(m_Positions, m_PredictedPositions);
    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        m_Velocities[i] += m_Accelerations[i] * p_DeltaTime;
        m_Positions[i] += m_Velocities[i] * p_DeltaTime;
        encase(i);
    }
}
template <Dimension D> void Solver<D>::ApplyMouseForce(const fvec<D> &p_MousePos) noexcept
{
    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        const fvec<D> diff = m_Positions[i] - p_MousePos;
        const f32 distance = glm::length(diff);
        if (distance < Settings.MouseRadius)
        {
            const f32 factor = 1.f - distance / Settings.MouseRadius;
            m_Accelerations[i] += (factor * Settings.MouseForce / distance) * diff;
        }
    }
}
template <Dimension D> void Solver<D>::ApplyExternal(const f32 p_DeltaTime) noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::ApplyExternal");
    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        m_Velocities[i].y += Settings.Gravity * p_DeltaTime / Settings.ParticleMass;
        m_PredictedPositions[i] = m_Positions[i] + m_Velocities[i] * p_DeltaTime;
        m_Densities[i] = Settings.ParticleMass;
        m_NearDensities[i] = Settings.ParticleMass;
        m_Accelerations[i] = fvec<D>{0.f};
    }
}
template <Dimension D> void Solver<D>::ComputeDensities() noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::ComputeDensities");
    if (Settings.IterateOverPairs)
        ForEachPairWithinSmoothingRadius([this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
            const f32 density = Settings.ParticleMass * getInfluence(p_Distance);
            const f32 nearDensity = Settings.ParticleMass * getNearInfluence(p_Distance);

            m_Densities[p_Index1] += density;
            m_NearDensities[p_Index1] += nearDensity;

            m_Densities[p_Index2] += density;
            m_NearDensities[p_Index2] += nearDensity;
        });
    else
        for (u32 i = 0; i < m_Positions.size(); ++i)
        {
            f32 density = 0.f;
            f32 nearDensity = 0.f;
            ForEachParticleWithinSmoothingRadius(i, [this, &density, &nearDensity](const u32, const f32 p_Distance) {
                density += Settings.ParticleMass * getInfluence(p_Distance);
                nearDensity += Settings.ParticleMass * getNearInfluence(p_Distance);
            });
            m_Densities[i] = density;
            m_NearDensities[i] = nearDensity;
        }
}
template <Dimension D> void Solver<D>::ApplyPressureAndViscosity() noexcept
{
    TKIT_PROFILE_NSCOPE("Flu::Solver::PressureAndViscosity");
    const auto pairwisePressureGradient = getPairwisePressureGradientComputation();
    const auto pairwiseViscosityTerm = getPairwiseViscosityTermComputation();
    if (Settings.IterateOverPairs)
    {
        ForEachPairWithinSmoothingRadius([this, &pairwisePressureGradient, &pairwiseViscosityTerm](
                                             const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
            const fvec<D> gradient = pairwisePressureGradient(p_Index1, p_Index2, p_Distance);
            const fvec<D> term = pairwiseViscosityTerm(p_Index1, p_Index2, p_Distance);

            const fvec<D> dv1 = term - gradient / m_Densities[p_Index1];
            const fvec<D> dv2 = term - gradient / m_Densities[p_Index2];

            m_Accelerations[p_Index1] += dv1;
            m_Accelerations[p_Index2] -= dv2;
        });
    }
    else
        for (u32 i = 0; i < m_Positions.size(); ++i)
        {
            fvec<D> gradient{0.f};
            fvec<D> vterm{0.f};
            ForEachParticleWithinSmoothingRadius(i, [&gradient, &vterm, i, &pairwisePressureGradient,
                                                     &pairwiseViscosityTerm](const u32 p_Index2, const f32 p_Distance) {
                gradient += pairwisePressureGradient(i, p_Index2, p_Distance);
                vterm += pairwiseViscosityTerm(i, p_Index2, p_Distance);
            });
            m_Accelerations[i] = vterm - gradient / m_Densities[i];
        }
}

template <Dimension D>
std::pair<f32, f32> Solver<D>::GetPressureFromDensity(const f32 p_Density, const f32 p_NearDensity) const noexcept
{
    const f32 p1 = Settings.PressureStiffness * (p_Density - Settings.TargetDensity);
    const f32 p2 = Settings.NearPressureStiffness * p_NearDensity;
    return {p1, p2};
}

template <Dimension D> void Solver<D>::UpdateLookup() noexcept
{
    switch (Settings.SearchMethod)
    {
    case NeighborSearch::BruteForce:
        m_Lookup.UpdateBruteForceLookup(Settings.SmoothingRadius);
        break;
    case NeighborSearch::Grid:
        m_Lookup.UpdateGridLookup(Settings.SmoothingRadius);
        break;
    }
}

template <Dimension D> void Solver<D>::AddParticle(const fvec<D> &p_Position) noexcept
{
    m_Positions.push_back(p_Position);
    m_Velocities.push_back(fvec<D>{0.f});
    m_Accelerations.push_back(fvec<D>{0.f});
    m_Densities.resize(m_Positions.size(), 0.f);
    m_NearDensities.resize(m_Positions.size(), 0.f);
}

template <Dimension D> void Solver<D>::encase(const u32 p_Index) noexcept
{
    const f32 factor = 1.f - Settings.EncaseFriction;
    for (u32 j = 0; j < D; ++j)
    {
        if (m_Positions[p_Index][j] - Settings.ParticleRadius < BoundingBox.Min[j])
        {
            m_Positions[p_Index][j] = BoundingBox.Min[j] + Settings.ParticleRadius;
            m_Velocities[p_Index][j] = -factor * m_Velocities[p_Index][j];
        }
        else if (m_Positions[p_Index][j] + Settings.ParticleRadius > BoundingBox.Max[j])
        {
            m_Positions[p_Index][j] = BoundingBox.Max[j] - Settings.ParticleRadius;
            m_Velocities[p_Index][j] = -factor * m_Velocities[p_Index][j];
        }
    }
}

template <Dimension D> void Solver<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept
{
    Visualization<D>::DrawBoundingBox(p_Context, BoundingBox.Min, BoundingBox.Max,
                                      Onyx::Color::FromHexadecimal("A6B1E1", false));
}
template <Dimension D> void Solver<D>::DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept
{
    const f32 particleSize = 2.f * Settings.ParticleRadius;

    const Onyx::Gradient gradient{Settings.Gradient};
    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        const fvec<D> &pos = m_Positions[i];
        const fvec<D> &vel = m_Velocities[i];

        const f32 speed = glm::min(Settings.FastSpeed, glm::length(vel));
        const Onyx::Color color = gradient.Evaluate(speed / Settings.FastSpeed);
        Visualization<D>::DrawParticle(p_Context, pos, particleSize, color);
    }
}

template <Dimension D> u32 Solver<D>::GetParticleCount() const noexcept
{
    return m_Positions.size();
}

template <Dimension D> const Lookup<D> &Solver<D>::GetLookup() const noexcept
{
    return m_Lookup;
}

template class Solver<Dimension::D2>;
template class Solver<Dimension::D3>;

} // namespace Flu