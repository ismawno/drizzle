#include "flu/layer.hpp"

namespace FLU
{
Layer::Layer() noexcept : ONYX::Layer("FLU Layer")
{
}

void Layer::OnUpdate() noexcept
{
}

void Layer::OnRender(const VkCommandBuffer) noexcept
{
}
} // namespace FLU