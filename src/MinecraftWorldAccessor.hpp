#pragma once
#include "WorldAccessor.hpp"

// This file is deliberately incomplete, and that's the honest state of it.
//
// To back FluidDynamics with the real game world, three things need to be
// wired to actual libminecraftpe.so internals for your specific Minecraft
// build:
//
//   1. A function/vtable slot that reads a block id + fluid level at a
//      BlockPos (something like BlockSource::getBlock / Block::getLegacyBlock
//      in various historical Bedrock builds).
//   2. A function/vtable slot that writes a block at a BlockPos
//      (BlockSource::setBlock or equivalent).
//   3. A per-tick hook fired from the world simulation loop, to drive
//      FluidSim::processTick() on a schedule instead of a full re-scan.
//
// None of those are things I can respectably give you as concrete byte
// signatures or offsets:
//   - libminecraftpe.so as provided is a stripped ARM64 binary with no
//     symbol table (verified: `file` reports "stripped"), so there is
//     nothing to resolve these against except a real disassembler
//     (Ghidra/IDA) and manual analysis against your exact build -- work I
//     can't do reliably from a text chat, and getting it wrong doesn't
//     "mostly work," it segfaults the game.
//   - BedrockTools has no fluid/block-read/write code at all to draw
//     from -- every one of its modules is a player/visual/HUD/misc combat
//     utility, so there was nothing fluid-related in it to port over.
//
// Once you (or someone with a disassembler) have real signatures for the
// three items above, this is exactly the shape the SDK's Signature/Hook
// APIs expect -- see api/signature and api/hook in the LeviLauncher docs.
// The sketch would look like:
//
//   #include <pl/memory/Signature.hpp>
//   #include <pl/memory/Hook.hpp>
//
//   auto addr = pl::memory::Signature::find("<verified pattern>");
//   if (addr) {
//       mGetBlockFn = reinterpret_cast<GetBlockFn>(addr);
//       ...
//   }
//
// Until then, isReady() reports false so the rest of the mod stays safe
// and inert instead of dereferencing a null/garbage pointer.

namespace fluiddyn {

class MinecraftWorldAccessor : public WorldAccessor {
public:
    MinecraftWorldAccessor() = default;

    bool isReady() const { return mReady; }

    BlockState getBlock(const BlockPos &pos) const override {
        (void)pos;
        // TODO: call the real signature-resolved read function here once
        // it exists. Returning air keeps this safe as a no-op until then.
        return BlockState{"minecraft:air", 0, false};
    }

    void setBlock(const BlockPos &pos, const BlockState &state) override {
        (void)pos;
        (void)state;
        // TODO: call the real signature-resolved write function here.
        // Intentionally a no-op until mReady is actually true.
    }

private:
    bool mReady = false; // flips true only once real hooks are wired in
};

} // namespace fluiddyn
