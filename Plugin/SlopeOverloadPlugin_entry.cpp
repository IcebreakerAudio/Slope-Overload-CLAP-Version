#include <clap/clap.h>
#include <cstring>

#include "SlopeOverloadPlugin.h"
#include "clapwrapper/vst3.h"

namespace
{

uint32_t factoryGetPluginCount(const clap_plugin_factory_t *)
{
    return 1;
}

const clap_plugin_descriptor_t *factoryGetPluginDescriptor(const clap_plugin_factory_t *, uint32_t index)
{
    return index == 0 ? &SlopeOverloadPlugin::descriptor : nullptr;
}

const clap_plugin_t *createPlugin(const clap_plugin_factory_t *, const clap_host_t *host, const char *pluginId)
{
    if (!clap_version_is_compatible(host->clap_version))
    {
        return nullptr;
    }

    if (std::strcmp(pluginId, SlopeOverloadPlugin::descriptor.id) != 0)
    {
        return nullptr;
    }

    auto *plugin = new SlopeOverloadPlugin(host);
    return plugin->clapPlugin();
}

const clap_plugin_factory_t pluginFactory = {factoryGetPluginCount, factoryGetPluginDescriptor, createPlugin};

// Pins the VST3 build's component class ID to the one the original JUCE-based Slope-Overload
// plugin registered (read from its built moduleinfo.json: CID "ABCDEF019182FAEB496365424941534F",
// split into 8-hex-digit groups per clapwrapper/vst3.h's migration recipe). Without this, hosts
// see the new VST3 as a different plugin and can't recall settings saved with the old one.
constexpr array_of_16_bytes legacyVst3ComponentId =
    COMPONENT_ID(0xABCDEF01, 0x9182FAEB, 0x49636542, 0x4941534F);

const clap_plugin_info_as_vst3_t legacyVst3Info = {"Icebreaker Audio", &legacyVst3ComponentId, "Fx"};

const clap_plugin_info_as_vst3_t *getVst3Info(const clap_plugin_factory_as_vst3 *, uint32_t index)
{
    return index == 0 ? &legacyVst3Info : nullptr;
}

const clap_plugin_factory_as_vst3_t vst3FactoryInfo = {"Icebreaker Audio", "", "", getVst3Info};

bool entryInit(const char *)
{
    return true;
}

void entryDeinit() {}

const void *entryGetFactory(const char *factoryId)
{
    if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0)
    {
        return &pluginFactory;
    }

    if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_INFO_VST3) == 0)
    {
        return &vst3FactoryInfo;
    }

    return nullptr;
}

}  // namespace

extern "C"
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {CLAP_VERSION, entryInit, entryDeinit, entryGetFactory};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
