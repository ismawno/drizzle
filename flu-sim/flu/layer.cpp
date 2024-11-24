#include "flu/layer.hpp"

namespace FLU
{
Layer::Layer(ONYX::Application *p_Application) noexcept : ONYX::Layer("FLU Layer"), m_Application(p_Application)
{
}

void Layer::OnStart() noexcept
{
    m_Window = m_Application->GetMainWindow();
    m_Data2.Context = m_Window->GetRenderContext<D2>();
}

void Layer::OnUpdate() noexcept
{
}

void Layer::OnRender(const VkCommandBuffer) noexcept
{
    onRender(m_Data2);
}

template <Dimension D> void Layer::onRender(const LayerData<D> &p_Data) noexcept
{
    p_Data.Context->Flush();

    const f32 step = 1.5f * m_Application->GetDeltaTime().AsSeconds();
    p_Data.Context->ApplyCameraLikeMovementControls(step, step);
    p_Data.Context->Square();
}

} // namespace FLU