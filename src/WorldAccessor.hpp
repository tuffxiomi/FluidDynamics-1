#pragma once
#include <cstdint>
#include <string>

namespace fluiddyn {

struct BlockPos {
    int x = 0, y = 0, z = 0;
    bool operator==(const BlockPos &o) const { return x == o.x && y == o.y && z == o.z; }
};
struct BlockPosHash {
    size_t operator()(const BlockPos &p) const {
        return (std::hash<int>()(p.x) * 73856093) ^
               (std::hash<int>()(p.y) * 19349663) ^
               (std::hash<int>()(p.z) * 83492791);
    }
};

struct BlockState {
    std::string blockId;  // "minecraft:air", "minecraft:water", ...
    int level = 0;        // fluid level, meaningless for non-fluids
    bool isSourceFlag = false;
};

// Pure interface. FluidSim only talks to the world through this, so the
// simulation logic itself has zero dependency on any specific game binary
// and can be compiled/tested standalone (see tests/mock_world_test.cpp).
//
// A real MinecraftWorldAccessor implementation needs three things wired to
// actual game internals, none of which I can verify from this environment:
//   1. read block id + fluid level at a position
//   2. write block id + fluid level at a position
//   3. a per-tick callback fired by the game's simulation loop
// Those three touch points are exactly what the SDK's Signature/Hook/Patch
// APIs (api/signature, api/hook, api/patch in the LeviLauncher docs) exist
// to wire up -- but the concrete byte signatures depend on your exact
// Minecraft build and have to come from actually disassembling it
// (Ghidra/IDA), not from BedrockTools (which has no fluid-related code to
// borrow signatures from) or from guesswork on my end.
class WorldAccessor {
public:
    virtual ~WorldAccessor() = default;
    virtual BlockState getBlock(const BlockPos &pos) const = 0;
    virtual void setBlock(const BlockPos &pos, const BlockState &state) = 0;
};

} // namespace fluiddyn
