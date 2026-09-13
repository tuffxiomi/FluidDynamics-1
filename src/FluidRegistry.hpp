#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace fluiddyn {

// Generic description of a fluid type. New fluids (vanilla or from other
// mods) are supported by adding an entry here -- no code changes needed.
// This is what makes "support all fluids" possible instead of hardcoding
// water/lava checks throughout the simulation.
struct FluidDef {
    std::string blockId;       // e.g. "minecraft:water", "minecraft:lava"
    int maxLevel = 7;          // vanilla water/lava use levels 0-7 (0 = source)
    int viscosityTicks = 5;    // lower = flows faster (lava-like = higher)
    bool isSource(int level) const { return level == 0; }

    // Optional: what this fluid turns into when it meets another fluid.
    // e.g. lava + water -> obsidian (source) / cobblestone (flowing)
    struct MixResult {
        std::string withFluidId;
        std::string resultBlockSource;
        std::string resultBlockFlowing;
    };
    std::vector<MixResult> mixRules;
};

class FluidRegistry {
public:
    static FluidRegistry &instance() {
        static FluidRegistry reg;
        return reg;
    }

    void registerFluid(FluidDef def) {
        mFluids[def.blockId] = std::move(def);
    }

    const FluidDef *find(const std::string &blockId) const {
        auto it = mFluids.find(blockId);
        return it == mFluids.end() ? nullptr : &it->second;
    }

    bool isFluid(const std::string &blockId) const {
        return mFluids.count(blockId) != 0;
    }

    const std::unordered_map<std::string, FluidDef> &all() const { return mFluids; }

    // Registers the vanilla fluids. Call once at mod load. Anything else
    // (custom fluids from other add-ons) can be added later via
    // registerFluid() without touching FluidSim at all.
    void registerVanillaDefaults() {
        FluidDef water;
        water.blockId = "minecraft:water";
        water.maxLevel = 7;
        water.viscosityTicks = 5;
        water.mixRules.push_back({"minecraft:lava", "minecraft:obsidian", "minecraft:cobblestone"});
        registerFluid(water);

        FluidDef lava;
        lava.blockId = "minecraft:lava";
        lava.maxLevel = 7;
        lava.viscosityTicks = 30; // lava flows slower than water
        lava.mixRules.push_back({"minecraft:water", "minecraft:obsidian", "minecraft:cobblestone"});
        registerFluid(lava);
    }

private:
    std::unordered_map<std::string, FluidDef> mFluids;
};

} // namespace fluiddyn
