#pragma once

#include "tkit/utils/glm.hpp"
#include "driz/core/alias.hpp"
#include "driz/core/dimension.hpp"

namespace Driz
{
using fvec2 = glm::vec<2, f32>;
using fvec3 = glm::vec<3, f32>;
using fvec4 = glm::vec<4, f32>;

template <Dimension D> using fvec = glm::vec<D, f32>;

using ivec2 = glm::vec<2, i32>;
using ivec3 = glm::vec<3, i32>;
using ivec4 = glm::vec<4, i32>;

template <Dimension D> using ivec = glm::vec<D, i32>;

using uvec2 = glm::vec<2, u32>;
using uvec3 = glm::vec<3, u32>;
using uvec4 = glm::vec<4, u32>;

template <Dimension D> using uvec = glm::vec<D, u32>;

} // namespace Driz