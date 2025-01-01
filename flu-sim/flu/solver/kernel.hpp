#pragma once

#include "flu/core/alias.hpp"
#include "flu/core/dimension.hpp"

namespace Flu
{
enum class KernelType
{
    Spiky = 0,
    CubicSpline
};
// Kernels expect the distance to be inferior to the radius
template <Dimension D> struct Kernel
{

    static f32 Spiky(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 SpikySlope(f32 p_Radius, f32 p_Distance) noexcept;

    static f32 CubicSpline(f32 p_Radius, f32 p_Distance) noexcept;
    static f32 CubicSplineSlope(f32 p_Radius, f32 p_Distance) noexcept;
};
} // namespace Flu