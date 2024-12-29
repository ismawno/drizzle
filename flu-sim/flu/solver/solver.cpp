#include "flu/solver/solver.hpp"

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
template <Dimension D> static f32 p3KernelGradient(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return -30.f * val * val / (glm::pi<f32>() * bigR);
    else
        return -45.f * val * val / (glm::pi<f32>() * bigR * p_Radius);
}

template <Dimension D> void Solver<D>::Step(const f32 p_DeltaTime) noexcept
{
    for (usize i = 0; i < Particles.size(); ++i)
        Densities[i] = ComputeDensityAtPoint(Particles[i].Position);

    for (usize i = 0; i < Particles.size(); ++i)
    {
        Particle<D> &particle = Particles[i];
        const vec<D> pressureGradient = ComputePressureGradient(i);
        particle.Velocity -= pressureGradient * p_DeltaTime / Densities[i];
        particle.Velocity.y += Gravity * p_DeltaTime / ParticleMass;
        particle.Position += particle.Velocity * p_DeltaTime;
    }
    Encase();
}

template <Dimension D> f32 Solver<D>::ComputeDensityAtPoint(const vec<D> &p_Point) const noexcept
{
    f32 density = 0.f;
    for (const Particle<D> &particle : Particles)
    {
        const f32 distance = glm::distance(p_Point, particle.Position);
        if (distance < SmoothingRadius)
            density += ParticleMass * p3Kernel<D>(SmoothingRadius, distance);
    }
    return density;
}
template <Dimension D> vec<D> Solver<D>::ComputePressureGradient(const usize p_Index) const noexcept
{
    vec<D> gradient{0.f};
    for (usize i = 0; i < Particles.size(); ++i)
    {
        if (i == p_Index)
            continue;
        const vec<D> diff = Particles[p_Index].Position - Particles[i].Position;
        const f32 distance = glm::length(diff);
        if (distance < SmoothingRadius)
        {
            const f32 kernelGradient = p3KernelGradient<D>(SmoothingRadius, distance);
            const f32 p1 = GetPressureFromDensity(Densities[p_Index]);
            const f32 p2 = GetPressureFromDensity(Densities[i]);
            gradient += (0.5f * (p1 + p2) * ParticleMass * kernelGradient / Densities[i]) * glm::normalize(diff);
        }
    }
    return gradient;
}

template <Dimension D> f32 Solver<D>::GetPressureFromDensity(const f32 p_Density) const noexcept
{
    return PressureStiffness * (p_Density - TargetDensity);
}

template <Dimension D> void Solver<D>::AddParticle(const vec<D> &p_Position) noexcept
{
    Particle<D> particle{};
    particle.Position = p_Position;
    Particles.push_back(particle);
    Densities.resize(Particles.size(), 0.f);
}

template <Dimension D> void Solver<D>::Encase() noexcept
{
    for (Particle<D> &particle : Particles)
    {
        for (u32 i = 0; i < D; ++i)
        {
            if (particle.Position[i] - ParticleRadius < BoundingBox.Min[i])
            {
                particle.Position[i] = BoundingBox.Min[i] + ParticleRadius;
                particle.Velocity[i] = -EncaseFriction * particle.Velocity[i];
            }
            else if (particle.Position[i] + ParticleRadius > BoundingBox.Max[i])
            {
                particle.Position[i] = BoundingBox.Max[i] - ParticleRadius;
                particle.Velocity[i] = -EncaseFriction * particle.Velocity[i];
            }
        }
    }
}

template <Dimension D> void Solver<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept
{
    p_Context->Push();

    p_Context->Fill(false);
    p_Context->Outline(Onyx::Color::WHITE);
    p_Context->OutlineWidth(0.02f);

    const vec<D> center = 0.5f * (BoundingBox.Min + BoundingBox.Max);
    const vec<D> size = BoundingBox.Max - BoundingBox.Min;

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
    for (const Particle<D> &p : Particles)
    {
        const f32 speed = glm::min(FastSpeed, glm::length(p.Velocity));
        const Onyx::Color color = gradient.Evaluate(speed / FastSpeed);

        p_Context->Fill(color);
        p_Context->Push();

        p_Context->Scale(particleSize);
        p_Context->Translate(p.Position);
        p_Context->Circle();

        // p_Context->Pop();
        // p_Context->Push();

        // p_Context->Scale(SmoothingRadius);
        // p_Context->Translate(p.Position);
        // p_Context->Alpha(0.4f);
        // p_Context->Circle(0.f, 1.f);

        p_Context->Pop();
    }
}

template struct Solver<Dimension::D2>;
template struct Solver<Dimension::D3>;

} // namespace Flu