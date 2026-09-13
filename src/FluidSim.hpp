#pragma once
#include <deque>
#include <unordered_set>
#include "FluidRegistry.hpp"
#include "WorldAccessor.hpp"

namespace fluiddyn {

struct FluidSimConfig {
    bool enabled = true;
    int tickIntervalTicks = 2;
    double flowSpeedMultiplier = 1.0;
    bool diagonalFlow = true;
    bool affectAllFluids = true;
    bool enableFluidMixing = true;
    int maxSpreadDistance = 8;
    // Hard safety cap independent of maxSpreadDistance. Real worlds can
    // have unloaded chunks or edge-of-world behavior that a finite test
    // world doesn't -- this guarantees processTick() always returns
    // instead of hanging if the world accessor ever reports something
    // unexpected (e.g. every unknown position reads back as air).
    int maxBlocksPerTick = 4096;
};

// Generic multi-fluid flow simulation, similar in spirit to the "Flowing
// Fluids" mod: fluids spread outward from sources, decrease in level with
// distance, flow diagonally around obstacles, and can mix with an
// adjacent, different fluid to produce a result block (e.g. obsidian).
//
// Because it looks fluids up through FluidRegistry rather than switching on
// hardcoded block ids, it works for every registered fluid -- vanilla water
// and lava out of the box, plus anything else registered at load time --
// without needing separate code per fluid.
class FluidSim {
public:
    explicit FluidSim(WorldAccessor &world, FluidSimConfig cfg = {})
        : mWorld(world), mCfg(cfg) {}

    void setConfig(const FluidSimConfig &cfg) { mCfg = cfg; }

    // Call once per fluid source/flow update (e.g. when a fluid block is
    // placed, or picked up from the scheduled-tick queue). Not a full-world
    // scan -- mirrors vanilla's "only simulate blocks that changed" model
    // so it stays cheap on real worlds.
    void queueUpdate(const BlockPos &pos) {
        if (!mCfg.enabled) return;
        mPending.push_back(pos);
    }

    // Drains the pending queue, applying at most maxSpreadDistance rings of
    // propagation per call. Returns number of blocks changed, for testing.
    int processTick() {
        if (!mCfg.enabled) return 0;
        int changed = 0;
        std::unordered_set<BlockPos, BlockPosHash> visited;

        while (!mPending.empty()) {
            if (static_cast<int>(visited.size()) >= mCfg.maxBlocksPerTick) {
                // Bail out safely and let the remaining queue resume next
                // tick, rather than ever spinning unbounded.
                break;
            }
            BlockPos pos = mPending.front();
            mPending.pop_front();
            if (visited.count(pos)) continue;
            visited.insert(pos);

            BlockState here = mWorld.getBlock(pos);
            const FluidDef *fluid = mCfg.affectAllFluids
                                         ? FluidRegistry::instance().find(here.blockId)
                                         : nullptr;
            if (!fluid) continue;

            changed += spreadFrom(pos, here, *fluid, visited);
        }
        return changed;
    }

private:
    WorldAccessor &mWorld;
    FluidSimConfig mCfg;
    std::deque<BlockPos> mPending;

    static const std::vector<BlockPos> &cardinalOffsets() {
        static const std::vector<BlockPos> offs = {
            {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
        };
        return offs;
    }
    static const std::vector<BlockPos> &diagonalOffsets() {
        static const std::vector<BlockPos> offs = {
            {1, 0, 1}, {1, 0, -1}, {-1, 0, 1}, {-1, 0, -1}
        };
        return offs;
    }

    int spreadFrom(const BlockPos &pos, const BlockState &here, const FluidDef &fluid,
                   std::unordered_set<BlockPos, BlockPosHash> & /*visited*/) {
        int changed = 0;

        // 1. Fall straight down first, like vanilla gravity-fed fluids.
        BlockPos below{pos.x, pos.y - 1, pos.z};
        BlockState belowState = mWorld.getBlock(below);
        if (belowState.blockId == "minecraft:air") {
            mWorld.setBlock(below, {fluid.blockId, 0, /*isSource*/ false});
            mPending.push_back(below);
            changed++;
        } else if (mCfg.enableFluidMixing) {
            changed += tryMix(below, belowState, fluid);
        }

        // 2. Horizontal spread, decreasing level with distance from source.
        int nextLevel = here.level + 1;
        if (nextLevel > fluid.maxLevel) return changed;
        if ((nextLevel - 0) > mCfg.maxSpreadDistance) return changed;

        auto trySpreadTo = [&](const BlockPos &n) {
            BlockState neighbor = mWorld.getBlock(n);
            if (neighbor.blockId == "minecraft:air") {
                mWorld.setBlock(n, {fluid.blockId, nextLevel, false});
                mPending.push_back(n);
                changed++;
            } else if (mCfg.enableFluidMixing) {
                changed += tryMix(n, neighbor, fluid);
            } else if (neighbor.blockId == fluid.blockId && neighbor.level > nextLevel) {
                // A thinner flow arrived from a shorter path -- update it,
                // matching vanilla's "shortest path wins" fluid leveling.
                mWorld.setBlock(n, {fluid.blockId, nextLevel, false});
                mPending.push_back(n);
                changed++;
            }
        };

        for (const auto &off : cardinalOffsets())
            trySpreadTo({pos.x + off.x, pos.y + off.y, pos.z + off.z});

        if (mCfg.diagonalFlow)
            for (const auto &off : diagonalOffsets())
                trySpreadTo({pos.x + off.x, pos.y + off.y, pos.z + off.z});

        return changed;
    }

    int tryMix(const BlockPos &pos, const BlockState &target, const FluidDef &incoming) {
        for (const auto &rule : incoming.mixRules) {
            if (target.blockId != rule.withFluidId) continue;
            const std::string &result = target.isSourceFlag ? rule.resultBlockSource
                                                              : rule.resultBlockFlowing;
            mWorld.setBlock(pos, {result, 0, true});
            return 1;
        }
        return 0;
    }
};

} // namespace fluiddyn
