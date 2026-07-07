#pragma once

#include <array>
#include <clap/helpers/plugin.hh>
#include <memory>

#include "DeltaModulation.h"
#include "IA_Utilities/CrossfadeMixer.hpp"
#include "IA_Utilities/FiFo.hpp"
#include "ParamAttachment.h"
#include "Parameter.h"
#include "ScopeAttachment.h"
#include "Speaker.h"
#include "SlopeOverloadEditor.h"

using SlopeOverloadPluginBase =
    clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>;

class SlopeOverloadPlugin : public SlopeOverloadPluginBase
{
public:
    static const clap_plugin_descriptor_t descriptor;

    explicit SlopeOverloadPlugin(const clap_host_t *host);

#ifdef __linux__
    bool implementsPosixFdSupport() const noexcept override { return true; }
    void onPosixFd(int fd, clap_posix_fd_flags_t flags) noexcept override;
#endif

protected:
    // clap_plugin_audio_ports
    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info_t *info) const noexcept override;

    // clap_plugin_audio_ports_config
    bool implementsAudioPortsConfig() const noexcept override { return true; }
    uint32_t audioPortsConfigCount() const noexcept override;
    bool audioPortsGetConfig(uint32_t index, clap_audio_ports_config_t *config) const noexcept override;
    bool audioPortsSetConfig(clap_id configId) noexcept override;

    // clap_plugin_params
    bool implementsParams() const noexcept override { return true; }
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(uint32_t paramIndex, clap_param_info_t *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;
    bool paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept override;
    bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;
    void paramsFlush(const clap_input_events_t *in, const clap_output_events_t *out) noexcept override;

    // clap_plugin_state
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream_t *stream) noexcept override;
    bool stateLoad(const clap_istream_t *stream) noexcept override;

    // clap_plugin_latency
    bool implementsLatency() const noexcept override { return true; }
    uint32_t latencyGet() const noexcept override { return static_cast<uint32_t>(dpcm.getLatencySamples()); }

    // clap_plugin_gui
    bool implementsGui() const noexcept override { return true; }
    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override;
    bool guiCreate(const char *api, bool isFloating) noexcept override;
    void guiDestroy() noexcept override;
    bool guiSetParent(const clap_window_t *window) noexcept override;
    bool guiSetScale(double scale) noexcept override { return false; }
    bool guiCanResize() const noexcept override { return true; }
    bool guiGetResizeHints(clap_gui_resize_hints_t *hints) noexcept override;
    bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept override;
    bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override;

    // clap_plugin (lifecycle)
    bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
    void deactivate() noexcept override;

    clap_process_status process(const clap_process_t *process) noexcept override;

private:
    enum ParamIndex : uint32_t
    {
        Active = 0,
        InGain,
        OutGain,
        SRate,
        AAFilt,
        Speaker,
        ParamCount
    };

    enum class PortConfig : clap_id
    {
        Stereo = 0,
        Mono = 1
    };

    void drainParamEvents(const clap_input_events_t *in) noexcept;
    void drainOutboundParamEvents(const clap_output_events_t *out) noexcept;
    void requestParamFlush() noexcept;
    const Parameter *findParam(clap_id paramId) const noexcept;
    Parameter *findParam(clap_id paramId) noexcept;

    int pluginWidth() const noexcept;
    int pluginHeight() const noexcept;
    void setPluginDimensions(int width, int height) noexcept;

    std::array<Parameter, ParamCount> _params;
    std::array<PendingParamChange, ParamCount> pendingChanges;
    std::array<ParamAttachment, ParamCount> paramAttachments;
    PortConfig _portConfig = PortConfig::Stereo;
    DeltaModulation dpcm;
    ::Speaker speaker;
    IADSP::CrossfadeMixer<float> mixer;
    Fifo<float> scopeFifo;
    ScopeAttachment scopeAttachment;
    std::unique_ptr<SlopeOverloadEditor> editor;
};
