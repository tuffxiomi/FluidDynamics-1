#include "FluidDynamicsMod.hpp"
#include <pl/ModMenu.hpp>
#include <filesystem>

namespace fluiddyn {

FluidDynamicsMod &FluidDynamicsMod::instance() {
    static FluidDynamicsMod inst;
    return inst;
}

FluidDynamicsMod::FluidDynamicsMod() : mSelf(*ll::mod::NativeMod::current()) {}

FluidSimConfig FluidDynamicsMod::toSimConfig(const PersistedConfig &c) const {
    FluidSimConfig sc;
    sc.enabled = c.enabled;
    sc.tickIntervalTicks = c.tickIntervalTicks;
    sc.flowSpeedMultiplier = c.flowSpeedMultiplier;
    sc.diagonalFlow = c.diagonalFlow;
    sc.affectAllFluids = c.affectAllFluids;
    sc.enableFluidMixing = c.enableFluidMixing;
    sc.maxSpreadDistance = c.maxSpreadDistance;
    return sc;
}

bool FluidDynamicsMod::load() {
    auto &self = getSelf();
    std::filesystem::create_directories(self.getConfigDir());

    FluidRegistry::instance().registerVanillaDefaults();
    // Additional fluids (from other add-ons/mods) can be registered here
    // too, e.g.:
    //   FluidRegistry::instance().registerFluid({"other:custom_fluid", ...});

    mConfig.emplace();
    if (!mConfig->load()) {
        self.getLogger().error("FluidDynamics: failed to load config");
        return false;
    }

    self.getLogger().info("FluidDynamics loaded ({} fluids registered)",
                           FluidRegistry::instance().all().size());
    return true;
}

bool FluidDynamicsMod::enable() {
    auto &self = getSelf();
    const auto &cfg = mConfig->value();

    // --- Unverified boundary -------------------------------------------
    // MinecraftWorldAccessor is where this mod would call into the real
    // game via signature-scanned hooks (block read/write, tick callback).
    // Those specific signatures are NOT included here -- see
    // MinecraftWorldAccessor.hpp for exactly what's needed and why I'm not
    // fabricating addresses for it. Until they're filled in,
    // MinecraftWorldAccessor::isReady() returns false and the mod stays
    // loaded but inert rather than crashing the game.
    mWorldAccessor = std::make_unique<MinecraftWorldAccessor>();
    if (!mWorldAccessor->isReady()) {
        self.getLogger().warn(
            "FluidDynamics: game hooks not wired up yet (see "
            "MinecraftWorldAccessor.hpp) -- simulation disabled, mod menu "
            "toggle will show as unavailable.");
    }

    mSim = std::make_unique<FluidSim>(*mWorldAccessor, toSimConfig(cfg));

    return pl::modmenu::ModuleBuilder("fluid_dynamics.core", "Fluid Dynamics")
        .modId(self.getId())
        .description("Flowing water/lava physics for all fluids.")
        .defaultEnabled(cfg.enabled && mWorldAccessor->isReady())
        .config("flowSpeedMultiplier", "Flow Speed", pl::modmenu::ConfigType::SliderFloat,
                std::to_string(cfg.flowSpeedMultiplier), "0.1", "4.0")
        .config("enableFluidMixing", "Fluid Mixing", pl::modmenu::ConfigType::Toggle,
                cfg.enableFluidMixing ? "true" : "false", "", "")
        .config("diagonalFlow", "Diagonal Flow", pl::modmenu::ConfigType::Toggle,
                cfg.diagonalFlow ? "true" : "false", "", "")
        .registerModule();
}

bool FluidDynamicsMod::disable() {
    mSim.reset();
    mWorldAccessor.reset();
    return true;
}

bool FluidDynamicsMod::unload() {
    mConfig.reset();
    return true;
}

} // namespace fluiddyn
