#include "flu/solver/particle.hpp"

namespace FLU
{
template <Dimension D> void Particle<D>::Draw(Onyx::RenderContext<D> *p_Context) const noexcept
{
    p_Context->Push();

    const f32 speed = glm::min(FastSpeed, glm::length(Velocity));
    static const std::array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};
    const Onyx::Gradient gradient{Gradient};
    const Onyx::Color color = gradient.Evaluate(speed / FastSpeed);

    p_Context->Fill(color);
    p_Context->Scale(2.f * Radius);
    p_Context->Translate(Position);
    p_Context->Circle();

    p_Context->Pop();
}

template struct Particle<Dimension::D2>;
template struct Particle<Dimension::D3>;
} // namespace FLU