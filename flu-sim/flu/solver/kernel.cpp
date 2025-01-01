#include "flu/solver/kernel.hpp"
#include "flu/core/glm.hpp"

namespace Flu
{
template <Dimension D> static f32 spikySigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return 10.f / (glm::pi<f32>() * bigR);
    else
        return 15.f / (glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 cubicSigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius;
    if constexpr (D == D2)
        return 10.f / (7.f * glm::pi<f32>() * bigR);
    else
        return 1.f / (glm::pi<f32>() * bigR * p_Radius);
}

template <Dimension D> f32 Kernel<D>::Spiky(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return spikySigma<D>(p_Radius) * val * val * val;
}
template <Dimension D> f32 Kernel<D>::SpikySlope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return -3.f * spikySigma<D>(p_Radius) * val * val;
}

template <Dimension D> f32 Kernel<D>::CubicSpline(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    if (q <= 1.f)
        return cubicSigma<D>(p_Radius) * (1.f - 1.5f * q * q + 0.75f * q * q * q);

    const f32 q2 = 2.f - q;
    return 0.25f * cubicSigma<D>(p_Radius) * q2 * q2 * q2;
}
template <Dimension D> f32 Kernel<D>::CubicSplineSlope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    if (q <= 1.f)
        return 3.f * cubicSigma<D>(p_Radius) * q * (0.75f * q - 1.f);

    const f32 q2 = 2.f - q;
    return -0.75f * cubicSigma<D>(p_Radius) * q2 * q2;
}

template struct Kernel<Dimension::D2>;
template struct Kernel<Dimension::D3>;

} // namespace Flu