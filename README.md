# FluidDynamics

A "Flowing Fluids"-style content mod scaffold for LeviLauncher, built on the
documented Preloader SDK (`api/mod`, `api/config`, `api/mod-menu`,
`api/signature`, `api/hook`).

## What's real and tested right now

`src/FluidSim.hpp` + `src/FluidRegistry.hpp` + `src/WorldAccessor.hpp` are a
complete, working fluid-flow simulation:

- Data-driven fluid registry, so it supports **any** fluid (vanilla water
  and lava are registered by default, and any other add-on's fluid can be
  added with one `registerFluid()` call, no code changes) -- see the
  "custom/modded fluid" test.
- Outward spread with level decay, gravity, diagonal flow, and
  fluid-mixing rules (lava + water -> obsidian/cobblestone), same spirit as
  the Flowing Fluids mod.
- Bounded and safe: `maxSpreadDistance` and a hard `maxBlocksPerTick` cap
  guarantee `processTick()` always returns, even against a misbehaving
  world accessor.

Run `tests/mock_world_test.cpp` yourself (plain g++, no NDK/SDK/game
needed):

```
g++ -std=c++17 -Wall -Wextra -o mock_world_test tests/mock_world_test.cpp
./mock_world_test
```

All 4 tests pass, including one that used to hang until it caught a real
bug (diagonal flow into an unbounded mock void) -- left in as a demonstration
that the test actually exercises the logic rather than rubber-stamping it.

## What's intentionally not done, and why

`src/MinecraftWorldAccessor.hpp` is a stub. Wiring this mod to the *real*
game requires three real hook points into `libminecraftpe.so` (block read,
block write, a per-tick callback), which in turn requires real signatures
for your specific Minecraft build.

I didn't fabricate those, for two concrete reasons, not just caution:

1. The provided `libminecraftpe.so` is a **stripped** ARM64 binary (no
   symbol table) -- there's nothing to derive real signatures from without
   an actual disassembler (Ghidra/IDA) and manual analysis, which isn't
   something this environment can do.
2. The provided `BedrockTools-main.zip` turned out to be a PvP
   cheat/utility client (ESP, x-ray, click-rate spoofing, HUD overlays for
   combat) with **no fluid or block-read/write code at all** -- there was
   nothing relevant in it to port over even if that were the goal. No files
   or code from it are used anywhere in this project.

## Filling in the real hooks

Once you (or someone with a disassembler) has verified signatures for your
Minecraft build, follow the pattern in `MinecraftWorldAccessor.hpp`'s
comments, using the SDK's own `pl::memory::Signature` / `pl::memory::Hook`
APIs (see the LeviLauncher docs' Signature API and Hook API pages) to
resolve and call them. Until `isReady()` returns true there, the mod loads
and registers its Mod Menu entry but stays inert rather than touching game
memory it can't verify.

## Layout

```
manifest.json                 -- LeviLauncher mod manifest
assets/icon.png                -- mod icon
config/config.json            -- default settings
config/config.schema.json     -- settings schema
src/FluidDynamicsMod.hpp/.cpp -- PL_REGISTER_MOD lifecycle, Mod Menu entry
src/FluidRegistry.hpp         -- data-driven fluid definitions (all fluids)
src/FluidSim.hpp              -- the actual flow/mix/spread algorithm
src/WorldAccessor.hpp         -- interface boundary to the game
src/MinecraftWorldAccessor.hpp-- real-game stub (see above)
tests/mock_world_test.cpp     -- standalone tests, no NDK required
```

## Building the real .so

This needs the actual `preloader-android` SDK and Android NDK, neither of
which are reachable from this sandbox (no network access). Once you have
them, this follows the same CMake/xmake + `FetchContent` pattern as the
`full-cpp-mod` example in the LeviLauncher docs: link `src/*.cpp` against
the `preloader` target, target `arm64-v8a`, and package the output
`.so` + `manifest.json` + `config/` into a `.levipack` the same way.
