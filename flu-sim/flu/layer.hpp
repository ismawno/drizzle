#pragma once

#include "onyx/app/layer.hpp"

namespace FLU
{
class Layer final : public ONYX::Layer
{
  public:
    Layer() noexcept;

    void OnUpdate() noexcept override;
    void OnRender(VkCommandBuffer) noexcept override;

  private:
};
} // namespace FLU