#pragma once

#include "onyx/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/alias.hpp"

namespace FLU
{
template <Dimension D> struct LayerData
{
    ONYX::RenderContext<D> *Context;
};

class Layer final : public ONYX::Layer
{
  public:
    Layer(ONYX::Application *p_Application) noexcept;

    void OnStart() noexcept override;
    void OnUpdate() noexcept override;
    void OnRender(VkCommandBuffer) noexcept override;

  private:
    template <Dimension D> void onUpdate(const LayerData<D> &p_Data) noexcept;
    template <Dimension D> void onRender(const LayerData<D> &p_Data) noexcept;

    ONYX::Application *m_Application;
    ONYX::Window *m_Window;
    LayerData<D2> m_Data2;
};
} // namespace FLU