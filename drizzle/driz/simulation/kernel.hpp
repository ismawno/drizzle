#pragma once

#include "driz/core/alias.hpp"
#include "driz/core/dimension.hpp"

namespace Driz
{
enum class KernelType
{
    Spiky2 = 0,
    Spiky3,
    Spiky5,
    Poly6,
    CubicSpline,
    WendlandC2,
    WendlandC4
};
// Kernels expect the distance to be inferior to the radius
template <Dimension D> struct Kernel
{
    static f32 Spiky2(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 Spiky2Slope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 Spiky3(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 Spiky3Slope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 Spiky5(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 Spiky5Slope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 Poly6(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 Poly6Slope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 CubicSpline(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 CubicSplineSlope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 WendlandC2(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 WendlandC2Slope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 WendlandC4(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 WendlandC4Slope(f32 p_Radius, f32 p_Distance) noexcept;
};
} // namespace Driz