#include "flu/solver/solver.hpp"

namespace FLU
{
template <Dimension D> void Solver<D>::Step(f32 p_DeltaTime) noexcept
{
    for (Particle<D> &particle : Particles)
    {
        particle.Velocity.y += Gravity * p_DeltaTime;
        particle.Position += particle.Velocity * p_DeltaTime;
    }
    Encase();
}
template <Dimension D> void Solver<D>::Encase() noexcept
{
    for (Particle<D> &particle : Particles)
    {
        for (u32 i = 0; i < D; ++i)
        {
            if (particle.Position[i] - Particle<D>::Radius < BoundingBox.Min[i])
            {
                particle.Position[i] = BoundingBox.Min[i] + Particle<D>::Radius;
                particle.Velocity[i] = -EncaseFriction * particle.Velocity[i];
            }
            else if (particle.Position[i] + Particle<D>::Radius > BoundingBox.Max[i])
            {
                particle.Position[i] = BoundingBox.Max[i] - Particle<D>::Radius;
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

template struct Solver<Dimension::D2>;
template struct Solver<Dimension::D3>;

} // namespace FLU