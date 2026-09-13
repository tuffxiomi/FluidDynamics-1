// Standalone test: compiles and runs with plain g++, no NDK/SDK/game
// binary needed. Proves FluidSim's logic (the part I can actually verify)
// works: water spreads outward and decays with distance, lava+water mixes
// to obsidian/cobblestone, and it terminates instead of looping forever.
//
// Build:  g++ -std=c++17 -Wall -Wextra -o mock_world_test mock_world_test.cpp
// Run:    ./mock_world_test
#include <cassert>
#include <iostream>
#include <unordered_map>
#include "../src/FluidSim.hpp"

using namespace fluiddyn;

class MockWorld : public WorldAccessor {
public:
    BlockState getBlock(const BlockPos &pos) const override {
        auto it = mBlocks.find(pos);
        if (it == mBlocks.end()) return BlockState{"minecraft:air", 0, false};
        return it->second;
    }
    void setBlock(const BlockPos &pos, const BlockState &state) override {
        mBlocks[pos] = state;
    }
    void place(const BlockPos &pos, const BlockState &state) { mBlocks[pos] = state; }

private:
    std::unordered_map<BlockPos, BlockState, BlockPosHash> mBlocks;
};

int main() {
    FluidRegistry::instance().registerVanillaDefaults();

    // --- Test 1: water spreads outward from a source and decays with level
    {
        MockWorld world;
        world.place({0, 0, 0}, {"minecraft:water", 0, true}); // source
        for (int x = -8; x <= 8; x++)
            for (int z = -8; z <= 8; z++)
                if (!(x == 0 && z == 0))
                    world.place({x, 0, z}, {"minecraft:air", 0, false});
        // solid floor so it doesn't just fall forever
        for (int x = -8; x <= 8; x++)
            for (int z = -8; z <= 8; z++)
                world.place({x, -1, z}, {"minecraft:bedrock", 0, false});
        world.place({0, -1, 0}, {"minecraft:bedrock", 0, false});

        FluidSim sim(world, FluidSimConfig{});
        sim.queueUpdate({0, 0, 0});
        int changed = sim.processTick();

        assert(changed > 0);
        BlockState neighbor = world.getBlock({1, 0, 0});
        assert(neighbor.blockId == "minecraft:water");
        assert(neighbor.level == 1); // decayed one step from the source
        std::cout << "[PASS] water spreads and decays (" << changed << " blocks changed)\n";
    }

    // --- Test 2: lava + water mixes to obsidian
    {
        MockWorld world;
        world.place({0, 0, 0}, {"minecraft:lava", 0, true});
        world.place({1, 0, 0}, {"minecraft:water", 0, true});
        world.place({0, -1, 0}, {"minecraft:bedrock", 0, false});
        world.place({1, -1, 0}, {"minecraft:bedrock", 0, false});

        FluidSim sim(world, FluidSimConfig{});
        sim.queueUpdate({0, 0, 0});
        sim.processTick();

        BlockState result = world.getBlock({1, 0, 0});
        assert(result.blockId == "minecraft:obsidian");
        std::cout << "[PASS] lava + water source mixes to obsidian\n";
    }

    // --- Test 3: respects maxSpreadDistance instead of running forever
    {
        MockWorld world;
        for (int x = -20; x <= 20; x++) {
            world.place({x, 0, 0}, {"minecraft:air", 0, false});
            world.place({x, -1, 0}, {"minecraft:bedrock", 0, false});
        }
        world.place({0, 0, 0}, {"minecraft:water", 0, true});

        FluidSimConfig cfg;
        cfg.maxSpreadDistance = 3;
        cfg.diagonalFlow = false; // this test only sets up a 1D line of blocks
        FluidSim sim(world, cfg);
        sim.queueUpdate({0, 0, 0});
        sim.processTick();

        assert(world.getBlock({3, 0, 0}).blockId == "minecraft:water");
        assert(world.getBlock({5, 0, 0}).blockId == "minecraft:air");
        std::cout << "[PASS] spread distance is bounded (terminates)\n";
    }

    // --- Test 4: a custom (non-vanilla) fluid works with zero extra code
    {
        FluidDef custom;
        custom.blockId = "example:slime_fluid";
        custom.maxLevel = 4;
        FluidRegistry::instance().registerFluid(custom);

        MockWorld world;
        world.place({0, 0, 0}, {"example:slime_fluid", 0, true});
        world.place({1, 0, 0}, {"minecraft:air", 0, false});
        world.place({0, -1, 0}, {"minecraft:bedrock", 0, false});
        world.place({1, -1, 0}, {"minecraft:bedrock", 0, false});

        FluidSim sim(world, FluidSimConfig{});
        sim.queueUpdate({0, 0, 0});
        sim.processTick();

        assert(world.getBlock({1, 0, 0}).blockId == "example:slime_fluid");
        std::cout << "[PASS] custom/modded fluid works via registry, no FluidSim changes needed\n";
    }

    std::cout << "\nAll tests passed.\n";
    return 0;
}
