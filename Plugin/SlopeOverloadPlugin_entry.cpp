#include <clap/clap.h>
#include <cstring>

#include "SlopeOverloadPlugin.h"

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
