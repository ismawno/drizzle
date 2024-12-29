#include "flu/solver/solver.hpp"
#include "tkit/container/hashable_tuple.hpp"
#include "tkit/utilities/math.hpp"

namespace Flu
{
template <Dimension D> static f32 paperKernel(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    if (q >= 1.f)
    {
        const f32 a = 2.f - q;
        const f32 value = 0.25f * glm::one_over_pi<f32>() * a * a * a;
        if constexpr (D == D2)
            return value / (p_Radius * p_Radius);
        else
            return value / (p_Radius * p_Radius * p_Radius);
    }
    const f32 value = glm::one_over_pi<f32>() * (1.f - 1.5f * q * q + 0.75f * q * q * q);
    if constexpr (D == D2)
        return value / (p_Radius * p_Radius);
    else
        return value / (p_Radius * p_Radius * p_Radius);
}

template <Dimension D> static f32 p3Kernel(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return 10.f * val * val * val / (glm::pi<f32>() * bigR);
    else
        return 15.f * val * val * val / (glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 p3KernelSlope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return -30.f * val * val / (glm::pi<f32>() * bigR);
    else
        return -45.f * val * val / (glm::pi<f32>() * bigR * p_Radius);
}

template <Dimension D> f32 Solver<D>::getInfluence(const f32 p_Distance) const noexcept
{
    return p3Kernel<D>(SmoothingRadius, p_Distance);
}
template <Dimension D> f32 Solver<D>::getInfluenceSlope(const f32 p_Distance) const noexcept
{
    return p3KernelSlope<D>(SmoothingRadius, p_Distance);
}

template <Dimension D> ivec<D> Solver<D>::getCellPosition(const fvec<D> &p_Position) const noexcept
{
    ivec<D> cellPosition{0};
    for (u32 i = 0; i < D; ++i)
        cellPosition[i] = static_cast<i32>(p_Position[i] / SmoothingRadius);
    return cellPosition;
}
template <Dimension D> u32 Solver<D>::getCellIndex(const ivec<D> &p_CellPosition) const noexcept
{
    const u32 particles = static_cast<u32>(m_Positions.size());
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

template <Dimension D> void Solver<D>::Step(const f32 p_DeltaTime) noexcept
{
    UpdateGrid();
    for (usize i = 0; i < m_Positions.size(); ++i)
        m_Densities[i] = ComputeDensityAtPoint(m_Positions[i]);

    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        const fvec<D> pressureGradient = ComputePressureGradient(i);
        m_Velocities[i] -= pressureGradient * p_DeltaTime / m_Densities[i];
        m_Velocities[i].y += Gravity * p_DeltaTime / ParticleMass;
        m_Positions[i] += m_Velocities[i] * p_DeltaTime;
    }
    encase();
}

template <Dimension D> void Solver<D>::UpdateGrid() noexcept
{
    m_SpatialLookup.clear();
    for (u32 i = 0; i < m_Positions.size(); ++i)
    {
        const ivec<D> cellPosition = getCellPosition(m_Positions[i]);
        const u32 index = getCellIndex(cellPosition);
        m_SpatialLookup.push_back({i, index});
    }
    std::sort(m_SpatialLookup.begin(), m_SpatialLookup.end(),
              [](const IndexPair &a, const IndexPair &b) { return a.CellIndex < b.CellIndex; });

    u32 prevIndex = 0;
    m_StartIndices.resize(m_Positions.size(), UINT32_MAX);
    for (u32 i = 0; i < m_SpatialLookup.size(); ++i)
    {
        if (i == 0 || m_SpatialLookup[i].CellIndex != prevIndex)
            m_StartIndices[m_SpatialLookup[i].CellIndex] = i;

        prevIndex = m_SpatialLookup[i].CellIndex;
    }
}

template <Dimension D> f32 Solver<D>::ComputeDensityAtPoint(const fvec<D> &p_Point) const noexcept
{
    f32 density = 0.f;
    ForEachPointWithinSmoothingRadius(p_Point, [this, &density](const u32, const f32 p_Distance) {
        density += ParticleMass * getInfluence(p_Distance);
    });
    return density;
}
template <Dimension D>
static fvec<D> getDirection(const fvec<D> &p_P1, const fvec<D> &p_P2, const f32 p_Distance) noexcept
{
    fvec<D> dir;
    if constexpr (D == D2)
        dir = {1.f, 0.f};
    else
        dir = {1.f, 0.f, 0.f};
    if (TKit::ApproachesZero(p_Distance))
        return dir;
    return (p_P1 - p_P2) / p_Distance;
}
template <Dimension D> fvec<D> Solver<D>::ComputePressureGradient(const u32 p_Index) const noexcept
{
    fvec<D> gradient{0.f};
    ForEachPointWithinSmoothingRadius(
        m_Positions[p_Index], [this, &gradient, p_Index](const u32 i, const f32 distance) {
            if (i == p_Index)
                return;
            const fvec<D> dir = getDirection<D>(m_Positions[p_Index], m_Positions[i], distance);
            const f32 kernelGradient = getInfluenceSlope(distance);
            const f32 p1 = GetPressureFromDensity(m_Densities[p_Index]);
            const f32 p2 = GetPressureFromDensity(m_Densities[i]);
            gradient += (0.5f * (p1 + p2) * ParticleMass * kernelGradient / m_Densities[i]) * dir;
        });
    return gradient;
}

template <Dimension D> f32 Solver<D>::GetPressureFromDensity(const f32 p_Density) const noexcept
{
    return PressureStiffness * (p_Density - TargetDensity);
}

template <Dimension D> void Solver<D>::AddParticle(const fvec<D> &p_Position) noexcept
{
    m_Positions.push_back(p_Position);
    m_Velocities.push_back(fvec<D>{0.f});
    m_Densities.resize(m_Positions.size(), 0.f);
}

template <Dimension D> void Solver<D>::encase() noexcept
{
    for (usize i = 0; i < m_Positions.size(); ++i)
        for (u32 j = 0; j < D; ++j)
        {
            if (m_Positions[i][j] - ParticleRadius < m_BoundingBox.Min[j])
            {
                m_Positions[i][j] = m_BoundingBox.Min[j] + ParticleRadius;
                m_Velocities[i][j] = -EncaseFriction * m_Velocities[i][j];
            }
            else if (m_Positions[i][j] + ParticleRadius > m_BoundingBox.Max[j])
            {
                m_Positions[i][j] = m_BoundingBox.Max[j] - ParticleRadius;
                m_Velocities[i][j] = -EncaseFriction * m_Velocities[i][j];
            }
        }
}

template <Dimension D> void Solver<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept
{
    p_Context->Push();

    p_Context->Fill(false);
    p_Context->Outline(Onyx::Color::WHITE);
    p_Context->OutlineWidth(0.02f);

    const fvec<D> center = 0.5f * (m_BoundingBox.Min + m_BoundingBox.Max);
    const fvec<D> size = m_BoundingBox.Max - m_BoundingBox.Min;

    p_Context->Scale(size);
    p_Context->Translate(center);
    p_Context->Square();

    p_Context->Pop();
}
template <Dimension D> void Solver<D>::DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept
{
    const f32 particleSize = 2.f * ParticleRadius;
    const std::array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};
    const Onyx::Gradient gradient{Gradient};
    for (usize i = 0; i < m_Positions.size(); ++i)
    {
        const fvec<D> &pos = m_Positions[i];
        const fvec<D> &vel = m_Velocities[i];

        const f32 speed = glm::min(FastSpeed, glm::length(vel));
        const Onyx::Color color = gradient.Evaluate(speed / FastSpeed);

        p_Context->Fill(color);
        p_Context->Push();

        p_Context->Scale(particleSize);
        p_Context->Translate(pos);
        p_Context->Circle();

        // p_Context->Pop();
        // p_Context->Push();

        // p_Context->Scale(SmoothingRadius);
        // p_Context->Translate(pos);
        // p_Context->Alpha(0.4f);
        // p_Context->Circle(0.f, 1.f);

        p_Context->Pop();
    }
}

template <Dimension D> usize Solver<D>::GetParticleCount() const noexcept
{
    return m_Positions.size();
}

template class Solver<Dimension::D2>;
template class Solver<Dimension::D3>;

} // namespace Flu