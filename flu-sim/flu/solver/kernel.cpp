#include "flu/solver/kernel.hpp"
#include "flu/core/glm.hpp"

namespace Flu
{
template <Dimension D> static f32 spiky2Sigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return 6.f / (glm::pi<f32>() * bigR);
    else
        return 15.f / (2.f * glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 spiky3Sigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return 10.f / (glm::pi<f32>() * bigR);
    else
        return 15.f / (glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 spiky5Sigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius * p_Radius * p_Radius * p_Radius * p_Radius * p_Radius;
    if constexpr (D == D2)
        return 21.f / (glm::pi<f32>() * bigR);
    else
        return 42.f / (glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 cubicSigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius;
    if constexpr (D == D2)
        return 10.f / (7.f * glm::pi<f32>() * bigR);
    else
        return 1.f / (glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 wendlandC2Sigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius;
    if constexpr (D == D2)
        return 7.f / (4.f * glm::pi<f32>() * bigR);
    else
        return 21.f / (16.f * glm::pi<f32>() * bigR * p_Radius);
}
template <Dimension D> static f32 wendlandC4Sigma(const f32 p_Radius) noexcept
{
    const f32 bigR = p_Radius * p_Radius;
    if constexpr (D == D2)
        return 9.f / (4.f * glm::pi<f32>() * bigR);
    else
        return 495.f / (256.f * glm::pi<f32>() * bigR * p_Radius);
}

template <Dimension D> f32 Kernel<D>::Spiky2(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return spiky2Sigma<D>(p_Radius) * val * val;
}
template <Dimension D> f32 Kernel<D>::Spiky2Slope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return -2.f * spiky2Sigma<D>(p_Radius) * val;
}

template <Dimension D> f32 Kernel<D>::Spiky3(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return spiky3Sigma<D>(p_Radius) * val * val * val;
}
template <Dimension D> f32 Kernel<D>::Spiky3Slope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return -3.f * spiky3Sigma<D>(p_Radius) * val * val;
}

template <Dimension D> f32 Kernel<D>::Spiky5(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return spiky5Sigma<D>(p_Radius) * val * val * val * val * val;
}
template <Dimension D> f32 Kernel<D>::Spiky5Slope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 val = p_Radius - p_Distance;
    return -5.f * spiky5Sigma<D>(p_Radius) * val * val * val * val;
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

template <Dimension D> f32 Kernel<D>::WendlandC2(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    const f32 q2 = 1.f - 0.5f * q;
    return wendlandC2Sigma<D>(p_Radius) * q2 * q2 * q2 * q2 * (2.f * q + 1.f);
}
template <Dimension D> f32 Kernel<D>::WendlandC2Slope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    const f32 q2 = 1.f - 0.5f * q;
    return -5.f * q * q2 * q2 * q2 * wendlandC2Sigma<D>(p_Radius);
}

template <Dimension D> f32 Kernel<D>::WendlandC4(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    const f32 q2 = 1.f - 0.5f * q;
    return wendlandC4Sigma<D>(p_Radius) * q2 * q2 * q2 * q2 * q2 * q2 * (35.f * q * q / 12.f + 3 * q + 1.f);
}
template <Dimension D> f32 Kernel<D>::WendlandC4Slope(const f32 p_Radius, const f32 p_Distance) noexcept
{
    const f32 q = 2.f * p_Distance / p_Radius;
    const f32 q2 = 1.f - 0.5f * q;
    return -wendlandC4Sigma<D>(p_Radius) * 7.f * q2 * q2 * q2 * q2 * q2 * q * (5 * q + 2.f) / 3.f;
}

template struct Kernel<Dimension::D2>;
template struct Kernel<Dimension::D3>;

} // namespace Flu