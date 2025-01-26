#include "flu/app/inspector.hpp"
#include "tkit/profiling/clock.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D> Inspector<D>::Inspector(const Solver<D> *p_Solver) noexcept : m_Solver(p_Solver)
{
}

template <Dimension D> void Inspector<D>::Render() noexcept
{
    ImGui::Text("The inspector's purpose is to provide detailed information about the simulation to detect "
                "inconsistencies or bugs");
    ImGui::Text(
        "Positions and velocities shown here correspond to their values when they were captured by the "
        "inspector, which is not necessarily (and likely not) the current state of the simulation. This is because the "
        "inspector inspects at the middle of the frame, just after the pressure gradients and viscosities have been "
        "computed");

    if (!m_WantsToInspect)
        m_WantsToInspect = ImGui::Button("Inspect");
    else
        ImGui::Text("Waiting for simulation to un-pause... To prevent it from progressing, use 'Dummy step' instead.");
    if (m_LastInspectionTime > 0.f && ImGui::TreeNode("Inspection results"))
    {
        renderInspectionData();
        ImGui::TreePop();
    }

    renderGridData();
    renderParticleData();
}

template <Dimension D> void Inspector<D>::Inspect() noexcept
{
    m_WantsToInspect = false;
    const Lookup<D> &lookup = m_Solver->GetLookup();

    m_Data = m_Solver->GetData();
    m_Grid = lookup.GetGrid();
    m_LookupRadius = lookup.GetRadius();

    m_PairIterData = InspectionData{};
    m_ParticleIterData = InspectionData{};

    TKit::Clock clock;
    lookup.ForEachPairBruteForce(getPairCollectionIterPair(m_PairIterData.BruteForcePairs));
    lookup.ForEachPairGrid(getPairCollectionIterPair(m_PairIterData.GridPairs));

    for (u32 i = 0; i < m_Solver->GetParticleCount(); ++i)
    {
        lookup.ForEachParticleBruteForce(i, getPairCollectionIterParticle(i, m_ParticleIterData.BruteForcePairs));
        lookup.ForEachParticleGrid(i, getPairCollectionIterParticle(i, m_ParticleIterData.GridPairs));
    }

    for (const auto &pair : m_PairIterData.BruteForcePairs.Pairs)
        if (!m_PairIterData.GridPairs.Pairs.contains(pair))
            m_PairIterData.MissingInGrid.insert(pair);

    for (const auto &pair : m_PairIterData.GridPairs.Pairs)
        if (!m_PairIterData.BruteForcePairs.Pairs.contains(pair))
            m_PairIterData.MissingInBruteForce.insert(pair);

    for (const auto &pair : m_ParticleIterData.BruteForcePairs.Pairs)
        if (!m_ParticleIterData.GridPairs.Pairs.contains(pair))
            m_ParticleIterData.MissingInGrid.insert(pair);

    for (const auto &pair : m_ParticleIterData.GridPairs.Pairs)
        if (!m_ParticleIterData.BruteForcePairs.Pairs.contains(pair))
            m_ParticleIterData.MissingInBruteForce.insert(pair);

    m_LastInspectionTime = clock.GetElapsed().AsMilliseconds();
}

template <Dimension D> bool Inspector<D>::WantsToInspect() const noexcept
{
    return m_WantsToInspect;
}

template <Dimension D> void Inspector<D>::renderParticle(const u32 p_Index) const noexcept
{
    const auto &positions = m_Data.Positions;
    const auto &velocities = m_Data.Velocities;
    const auto &accelerations = m_Data.Accelerations;
    const auto &densities = m_Data.Densities;

    const fvec<D> &pos = positions[p_Index];
    const fvec<D> &vel = velocities[p_Index];
    const fvec<D> &acc = accelerations[p_Index];

    const f32 speed = glm::length(vel);
    const f32 accMag = glm::length(acc);

    const ivec<D> cellPosition = Lookup<D>::GetCellPosition(pos, m_LookupRadius);
    const u32 cellKey = Lookup<D>::GetCellKey(cellPosition, positions.size());

    ImGui::Text("Particle %u", p_Index);
    ImGui::Indent(15.f);
    ImGui::Text("Density: %.2f", densities[p_Index]);
    if constexpr (D == D2)
    {
        ImGui::Text("Position: (%.2f, %.2f)", pos.x, pos.y);
        ImGui::Text("Velocity: (%.2f, %.2f) (%.2f)", vel.x, vel.y, speed);
        ImGui::Text("Acceleration: (%.2f, %.2f) (%.2f)", acc.x, acc.y, accMag);
        ImGui::Text("Cell: (%d, %d) (%u)", cellPosition.x, cellPosition.y, cellKey);
    }
    else
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f) (%.2f)", vel.x, vel.y, vel.z, speed);
        ImGui::Text("Acceleration: (%.2f, %.2f, %.2f) (%.2f)", acc.x, acc.y, acc.z, accMag);
        ImGui::Text("Cell: (%d, %d, %d) (%u)", cellPosition.x, cellPosition.y, cellPosition.z, cellKey);
    }
    ImGui::Unindent(15.f);
}

template <Dimension D>
void Inspector<D>::renderPairs(const TKit::TreeSet<ParticlePair> &p_Pairs, const u32 p_Selected) const noexcept
{
    for (const auto &pair : p_Pairs)
    {
        const bool isSelected =
            p_Selected == TKit::Limits<u32>::max() || pair.first == p_Selected || pair.second == p_Selected;
        if (isSelected && ImGui::TreeNode(&pair, "Pair: %u, %u", pair.first, pair.second))
        {
            renderParticle(pair.first);
            renderParticle(pair.second);
            ImGui::TreePop();
        }
    }
}

template <Dimension D>
void Inspector<D>::renderPairData(const InspectionData &p_Data, const u32 p_Selected) const noexcept
{
    ImGui::Columns(2, "Inspection data", true);
    ImGui::BeginChild("Brute force", {0, 250}, true);
    if (ImGui::TreeNode(&p_Data.BruteForcePairs.Pairs, "Brute force pairs: %u", p_Data.BruteForcePairs.Pairs.size()))
    {
        renderPairs(p_Data.BruteForcePairs.Pairs, p_Selected);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(&p_Data.BruteForcePairs.DuplicatePairs, "Duplicate pairs: %u",
                        p_Data.BruteForcePairs.DuplicatePairs.size()))
    {
        renderPairs(p_Data.BruteForcePairs.DuplicatePairs, p_Selected);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(&p_Data.MissingInGrid, "Missing in grid: %u", p_Data.MissingInGrid.size()))
    {
        renderPairs(p_Data.MissingInGrid, p_Selected);
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("Grid", {0, 250}, true);
    if (ImGui::TreeNode(&p_Data.GridPairs.Pairs, "Grid pairs: %u", p_Data.GridPairs.Pairs.size()))
    {
        renderPairs(p_Data.GridPairs.Pairs, p_Selected);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(&p_Data.GridPairs.DuplicatePairs, "Duplicate pairs: %u",
                        p_Data.GridPairs.DuplicatePairs.size()))
    {
        renderPairs(p_Data.GridPairs.DuplicatePairs, p_Selected);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(&p_Data.MissingInBruteForce, "Missing in brute force: %u", p_Data.MissingInBruteForce.size()))
    {
        renderPairs(p_Data.MissingInBruteForce, p_Selected);
        ImGui::TreePop();
    }
    ImGui::EndChild();
    ImGui::Columns(1);
}

template <Dimension D> void Inspector<D>::renderInspectionData() const noexcept
{
    ImGui::Text("Last inspection took %.2f ms", m_LastInspectionTime);

    static u32 selected = TKit::Limits<u32>::max();
    static bool search = false;
    ImGui::Checkbox("Enable search##Inspection", &search);

    if (search)
        ImGui::InputInt("Search by index##Inspection", (int *)&selected);
    else
        selected = TKit::Limits<u32>::max();

    if (ImGui::TreeNode("Iteration per pair lookup"))
    {
        renderPairData(m_PairIterData, selected);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Iteration per particle lookup"))
    {
        renderPairData(m_ParticleIterData, selected);
        ImGui::TreePop();
    }
}

template <Dimension D> void Inspector<D>::renderGridData() const noexcept
{
    static u32 selected = TKit::Limits<u32>::max();
    static bool search = false;
    ImGui::Checkbox("Enable search##Grid", &search);

    if (search)
        ImGui::InputInt("Search by index##Grid", (int *)&selected);
    else
        selected = TKit::Limits<u32>::max();

    const u32 cellCount = m_Grid.Cells.size();
    if (ImGui::TreeNode(this, "Unique cells in grid: %u", cellCount))
    {
        ImGui::BeginChild("Grid", {0, 250}, true);

        for (const GridCell &cell : m_Grid.Cells)
        {
            bool present = selected == TKit::Limits<u32>::max();
            if (!present)
                for (u32 i = cell.Start; i < cell.End; ++i)
                    if (m_Grid.ParticleIndices[i] == selected)
                    {
                        present = true;
                        break;
                    }

            if (present && ImGui::TreeNode(&cell, "Cell key: %u, Particles: %u", cell.Key, cell.End - cell.Start))
            {
                for (u32 i = cell.Start; i < cell.End; ++i)
                    renderParticle(m_Grid.ParticleIndices[i]);
                ImGui::TreePop();
            }
        }

        ImGui::EndChild();
        ImGui::TreePop();
    }
}

template <Dimension D> void Inspector<D>::renderParticleData() const noexcept
{
    const u32 pcount = m_Data.Positions.size();
    if (ImGui::TreeNode(m_Solver, "Particles: %u", pcount))
    {
        ImGui::BeginChild("Particles", {0, 250}, true);

        static u32 selectedParticle = 0;
        static u32 start = 0;
        static u32 end = pcount;

        ImGui::InputInt("Search by index", (i32 *)&selectedParticle);
        selectedParticle = glm::min(selectedParticle, pcount - 1);

        renderParticle(selectedParticle);
        ImGui::Spacing();

        ImGui::SliderInt("Start", (i32 *)&start, 0, end);
        ImGui::SliderInt("End", (i32 *)&end, start, pcount);

        for (u32 i = start; i < end; ++i)
            renderParticle(i);
        ImGui::EndChild();
        ImGui::TreePop();
    }
}

template class Inspector<D2>;
template class Inspector<D3>;

} // namespace Flu