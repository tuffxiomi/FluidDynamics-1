#pragma once
#include <pl/Mod.hpp>
#include <pl/Config.hpp>
#include <optional>
#include "FluidSim.hpp"
#include "MinecraftWorldAccessor.hpp"

namespace fluiddyn {

struct PersistedConfig {
    int version = 1;
    bool enabled = true;
    int tickIntervalTicks = 2;
    double flowSpeedMultiplier = 1.0;
    bool diagonalFlow = true;
    bool affectAllFluids = true;
    bool enableFluidMixing = true;
    int maxSpreadDistance = 8;
};

class FluidDynamicsMod {
public:
    static FluidDynamicsMod &instance();

    FluidDynamicsMod();

    [[nodiscard]] ll::mod::NativeMod &getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

private:
    ll::mod::NativeMod &mSelf;
    std::optional<pl::config::ConfigFile<PersistedConfig>> mConfig;
    std::unique_ptr<MinecraftWorldAccessor> mWorldAccessor;
    std::unique_ptr<FluidSim> mSim;

    FluidSimConfig toSimConfig(const PersistedConfig &c) const;
    void onGameTick(); // wired in enable(), see MinecraftWorldAccessor.hpp
};

} // namespace fluiddyn

PL_REGISTER_MOD(fluiddyn::FluidDynamicsMod, fluiddyn::FluidDynamicsMod::instance())
