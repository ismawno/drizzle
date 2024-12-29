#pragma once

#include "tkit/utilities/glm.hpp"
#include "flu/core/alias.hpp"
#include "flu/core/dimension.hpp"

namespace Flu
{
using fvec2 = glm::vec<2, f32>;
using fvec3 = glm::vec<3, f32>;
using fvec4 = glm::vec<4, f32>;

template <Dimension D> using fvec = glm::vec<D, f32>;

using ivec2 = glm::vec<2, i32>;
using ivec3 = glm::vec<3, i32>;
using ivec4 = glm::vec<4, i32>;

template <Dimension D> using ivec = glm::vec<D, i32>;

} // namespace Flu