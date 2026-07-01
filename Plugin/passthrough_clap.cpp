// Phase 0 toolchain bootstrap: a pure stereo passthrough written directly against the raw
// CLAP C ABI, modeled on clap-wrapper's tests/clap-first-example. Phase 1 replaces this with
// a clap::helpers::Plugin<>-based shell - this file is not the final plugin architecture.

#include <clap/clap.h>
#include <cstdio>
#include <cstring>

#include "passthrough_clap_entry.h"

namespace
{

const char *features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr};

const clap_plugin_descriptor_t pluginDescriptor = {
    CLAP_VERSION_INIT,
    "com.icebreakeraudio.slopeoverload",
    "Slope Overload",
    "Icebreaker Audio",
    "https://github.com/IcebreakerAudio/Slope-Overload",
    "",
    "",
    "0.1.0",
    "NES/Famicom DPCM delta-modulation emulation (toolchain bootstrap passthrough)",
    &features[0]};

struct PassthroughPlugin
{
    clap_plugin_t plugin;
    const clap_host_t *host;
};

uint32_t audioPortsCount(const clap_plugin_t *, bool)
{
    return 1;
}

bool audioPortsGet(const clap_plugin_t *, uint32_t index, bool isInput, clap_audio_port_info_t *info)
{
    if (index > 0)
    {
        return false;
    }

    info->id = 0;
    std::snprintf(info->name, sizeof(info->name), "%s", isInput ? "Stereo In" : "Stereo Out");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

const clap_plugin_audio_ports_t audioPortsExtension = {audioPortsCount, audioPortsGet};

bool pluginInit(const clap_plugin_t *)
{
    return true;
}

void pluginDestroy(const clap_plugin_t *plugin)
{
    delete static_cast<PassthroughPlugin *>(plugin->plugin_data);
}

bool pluginActivate(const clap_plugin_t *, double, uint32_t, uint32_t)
{
    return true;
}

void pluginDeactivate(const clap_plugin_t *) {}

bool pluginStartProcessing(const clap_plugin_t *)
{
    return true;
}

void pluginStopProcessing(const clap_plugin_t *) {}

void pluginReset(const clap_plugin_t *) {}

clap_process_status pluginProcess(const clap_plugin_t *, const clap_process_t *process)
{
    for (uint32_t channel = 0; channel < process->audio_outputs[0].channel_count; ++channel)
    {
        std::memcpy(process->audio_outputs[0].data32[channel], process->audio_inputs[0].data32[channel],
                    sizeof(float) * process->frames_count);
    }

    return CLAP_PROCESS_CONTINUE;
}

const void *pluginGetExtension(const clap_plugin_t *, const char *id)
{
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
    {
        return &audioPortsExtension;
    }

    return nullptr;
}

void pluginOnMainThread(const clap_plugin_t *) {}

const clap_plugin_t *createPlugin(const clap_plugin_factory_t *, const clap_host_t *host, const char *pluginId)
{
    if (!clap_version_is_compatible(host->clap_version))
    {
        return nullptr;
    }

    if (std::strcmp(pluginId, pluginDescriptor.id) != 0)
    {
        return nullptr;
    }

    auto *plug = new PassthroughPlugin{};
    plug->host = host;
    plug->plugin.desc = &pluginDescriptor;
    plug->plugin.plugin_data = plug;
    plug->plugin.init = pluginInit;
    plug->plugin.destroy = pluginDestroy;
    plug->plugin.activate = pluginActivate;
    plug->plugin.deactivate = pluginDeactivate;
    plug->plugin.start_processing = pluginStartProcessing;
    plug->plugin.stop_processing = pluginStopProcessing;
    plug->plugin.reset = pluginReset;
    plug->plugin.process = pluginProcess;
    plug->plugin.get_extension = pluginGetExtension;
    plug->plugin.on_main_thread = pluginOnMainThread;

    return &plug->plugin;
}

uint32_t factoryGetPluginCount(const clap_plugin_factory_t *)
{
    return 1;
}

const clap_plugin_descriptor_t *factoryGetPluginDescriptor(const clap_plugin_factory_t *, uint32_t index)
{
    return index == 0 ? &pluginDescriptor : nullptr;
}

const clap_plugin_factory_t pluginFactory = {factoryGetPluginCount, factoryGetPluginDescriptor, createPlugin};

}  // namespace

bool passthrough_entry_init(const char *)
{
    return true;
}

void passthrough_entry_deinit() {}

const void *passthrough_entry_get_factory(const char *factoryId)
{
    if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0)
    {
        return &pluginFactory;
    }

    return nullptr;
}
