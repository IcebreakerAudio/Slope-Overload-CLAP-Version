#include "SlopeOverloadPlugin.h"

#include <clap/clap.h>
#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hxx>
#include <cmath>
#include <cstddef>
#include <cstdio>

#include "AudioBuffer.h"
#include "ScopedNoDenormals.h"

namespace
{

constexpr const char *kFeatures[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr};

float dbToGain(double db) noexcept { return static_cast<float>(std::pow(10.0, db / 20.0)); }

constexpr uint32_t kStateMagic = 0x31766f53;  // "Sov1"
constexpr uint32_t kStateVersion = 1;

template <typename T>
bool writeAll(const clap_ostream_t *stream, const T &value) noexcept
{
    auto *cursor = reinterpret_cast<const std::byte *>(&value);
    size_t bytesLeft = sizeof(T);
    while (bytesLeft > 0)
    {
        const int64_t written = stream->write(stream, cursor, bytesLeft);
        if (written <= 0)
        {
            return false;
        }
        cursor += written;
        bytesLeft -= static_cast<size_t>(written);
    }
    return true;
}

template <typename T>
bool readAll(const clap_istream_t *stream, T &value) noexcept
{
    auto *cursor = reinterpret_cast<std::byte *>(&value);
    size_t bytesLeft = sizeof(T);
    while (bytesLeft > 0)
    {
        const int64_t readCount = stream->read(stream, cursor, bytesLeft);
        if (readCount <= 0)
        {
            return false;
        }
        cursor += readCount;
        bytesLeft -= static_cast<size_t>(readCount);
    }
    return true;
}

}  // namespace

const clap_plugin_descriptor_t SlopeOverloadPlugin::descriptor = {
    CLAP_VERSION_INIT,
    "com.icebreakeraudio.slopeoverload",
    "Slope Overload",
    "Icebreaker Audio",
    "https://github.com/IcebreakerAudio/Slope-Overload",
    "",
    "",
    "0.1.0",
    "NES/Famicom DPCM delta-modulation emulation",
    &kFeatures[0]};

SlopeOverloadPlugin::SlopeOverloadPlugin(const clap_host_t *host)
    : SlopeOverloadPluginBase(&descriptor, host),
      _params{
          Parameter(ParamIndex::Active, "On/Off", 0.0, 1.0, 1.0,
                     CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_AUTOMATABLE, ParamFormat::Toggle),
          Parameter(ParamIndex::InGain, "Input", -60.0, 24.0, 0.0, CLAP_PARAM_IS_AUTOMATABLE,
                     ParamFormat::Decibels),
          Parameter(ParamIndex::OutGain, "Output", -60.0, 12.0, 0.0, CLAP_PARAM_IS_AUTOMATABLE,
                     ParamFormat::Decibels),
          Parameter(ParamIndex::SRate, "Sample Rate", 0.0, 15.0, 7.0,
                     CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_AUTOMATABLE, ParamFormat::Integer),
          Parameter(ParamIndex::AAFilt, "Pre-Filter", 0.0, 1.0, 1.0,
                     CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_AUTOMATABLE, ParamFormat::Toggle),
          Parameter(ParamIndex::Speaker, "Speaker", 0.0, 2.0, 0.0,
                     CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM | CLAP_PARAM_IS_AUTOMATABLE, ParamFormat::Choice,
                     std::vector<std::string>{"A", "B", "C"})}
{
}

uint32_t SlopeOverloadPlugin::audioPortsCount(bool) const noexcept
{
    return 1;
}

bool SlopeOverloadPlugin::audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info_t *info) const noexcept
{
    if (index > 0)
    {
        return false;
    }

    const bool isMono = _portConfig == PortConfig::Mono;
    info->id = 0;
    std::snprintf(info->name, sizeof(info->name), "%s", isInput ? "Input" : "Output");
    info->channel_count = isMono ? 1 : 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = isMono ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

uint32_t SlopeOverloadPlugin::audioPortsConfigCount() const noexcept
{
    return 2;
}

bool SlopeOverloadPlugin::audioPortsGetConfig(uint32_t index, clap_audio_ports_config_t *config) const noexcept
{
    if (index > 1)
    {
        return false;
    }

    const auto config_ = static_cast<PortConfig>(index);
    const bool isMono = config_ == PortConfig::Mono;

    config->id = static_cast<clap_id>(config_);
    std::snprintf(config->name, sizeof(config->name), "%s", isMono ? "Mono" : "Stereo");
    config->input_port_count = 1;
    config->output_port_count = 1;
    config->has_main_input = true;
    config->main_input_channel_count = isMono ? 1 : 2;
    config->main_input_port_type = isMono ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
    config->has_main_output = true;
    config->main_output_channel_count = isMono ? 1 : 2;
    config->main_output_port_type = isMono ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
    return true;
}

bool SlopeOverloadPlugin::audioPortsSetConfig(clap_id configId) noexcept
{
    if (isActive() || configId > static_cast<clap_id>(PortConfig::Mono))
    {
        return false;
    }

    _portConfig = static_cast<PortConfig>(configId);
    return true;
}

uint32_t SlopeOverloadPlugin::paramsCount() const noexcept
{
    return ParamCount;
}

bool SlopeOverloadPlugin::paramsInfo(uint32_t paramIndex, clap_param_info_t *info) const noexcept
{
    if (paramIndex >= ParamCount)
    {
        return false;
    }

    _params[paramIndex].info(info);
    return true;
}

bool SlopeOverloadPlugin::paramsValue(clap_id paramId, double *value) noexcept
{
    const auto *param = findParam(paramId);
    if (param == nullptr)
    {
        return false;
    }

    *value = param->value();
    return true;
}

bool SlopeOverloadPlugin::paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept
{
    const auto *param = findParam(paramId);
    return param != nullptr && param->valueToText(value, display, size);
}

bool SlopeOverloadPlugin::paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept
{
    const auto *param = findParam(paramId);
    return param != nullptr && param->textToValue(display, value);
}

void SlopeOverloadPlugin::paramsFlush(const clap_input_events_t *in, const clap_output_events_t *) noexcept
{
    drainParamEvents(in);
}

bool SlopeOverloadPlugin::stateSave(const clap_ostream_t *stream) noexcept
{
    if (!writeAll(stream, kStateMagic) || !writeAll(stream, kStateVersion) ||
        !writeAll(stream, static_cast<uint32_t>(ParamCount)))
    {
        return false;
    }

    for (const auto &param : _params)
    {
        if (!writeAll(stream, param.value()))
        {
            return false;
        }
    }

    return true;
}

bool SlopeOverloadPlugin::stateLoad(const clap_istream_t *stream) noexcept
{
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t count = 0;
    if (!readAll(stream, magic) || magic != kStateMagic || !readAll(stream, version) ||
        version != kStateVersion || !readAll(stream, count) || count != ParamCount)
    {
        return false;
    }

    for (auto &param : _params)
    {
        double value = 0.0;
        if (!readAll(stream, value))
        {
            return false;
        }
        param.setValue(value);
    }

    return true;
}

bool SlopeOverloadPlugin::activate(double sampleRate, uint32_t, uint32_t maxFrameCount) noexcept
{
    const int numChannels = _portConfig == PortConfig::Mono ? 1 : 2;
    dpcm.initialize(sampleRate, static_cast<int>(maxFrameCount), numChannels);
    speaker.initialize(sampleRate, static_cast<int>(maxFrameCount), numChannels);

    mixer.prepare(sampleRate, static_cast<int>(maxFrameCount), numChannels);
    mixer.setLatencyCompensation(dpcm.getLatencySamples());
    if (_host.canUseLatency())
    {
        _host.latencyChanged();
    }

    return true;
}

void SlopeOverloadPlugin::deactivate() noexcept
{
    dpcm.reset();
    mixer.reset();
}

clap_process_status SlopeOverloadPlugin::process(const clap_process_t *process) noexcept
{
    const ScopedNoDenormals noDenormals;

    drainParamEvents(process->in_events);

    const AudioBuffer input(process->audio_inputs[0].data32, process->audio_inputs[0].channel_count,
                             process->frames_count);
    AudioBuffer output(process->audio_outputs[0].data32, process->audio_outputs[0].channel_count,
                        process->frames_count);
    output.copyFrom(input);

    mixer.pushFirstSignal(output.data(), static_cast<int>(output.numFrames()));

    dpcm.setAntiAliasing(findParam(ParamIndex::AAFilt)->value() >= 0.5);
    dpcm.setSampleRateIndex(static_cast<int>(std::lround(findParam(ParamIndex::SRate)->value())));

    const auto inGain = dbToGain(findParam(ParamIndex::InGain)->value());
    const auto outGain = dbToGain(findParam(ParamIndex::OutGain)->value());

    for (uint32_t ch = 0; ch < output.numChannels(); ++ch)
    {
        for (auto &s : output.channel(ch))
        {
            s *= inGain;
        }
    }

    dpcm.process(output);

    const auto speakerChoice = static_cast<int>(std::lround(findParam(ParamIndex::Speaker)->value())) - 1;
    speaker.setSpeaker(speakerChoice);
    speaker.process(output);

    for (uint32_t ch = 0; ch < output.numChannels(); ++ch)
    {
        for (auto &s : output.channel(ch))
        {
            s *= outGain;
        }
    }

    mixer.setMix(findParam(ParamIndex::Active)->value() >= 0.5 ? 1.0f : 0.0f);
    mixer.mixSecondSignal(output.data(), static_cast<int>(output.numFrames()));

    return CLAP_PROCESS_CONTINUE;
}

void SlopeOverloadPlugin::drainParamEvents(const clap_input_events_t *in) noexcept
{
    if (in == nullptr)
    {
        return;
    }

    const uint32_t count = in->size(in);
    for (uint32_t i = 0; i < count; ++i)
    {
        const auto *hdr = in->get(in, i);
        if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID || hdr->type != CLAP_EVENT_PARAM_VALUE)
        {
            continue;
        }

        const auto *ev = reinterpret_cast<const clap_event_param_value_t *>(hdr);
        if (auto *param = findParam(ev->param_id))
        {
            param->setValue(ev->value);
        }
    }
}

const Parameter *SlopeOverloadPlugin::findParam(clap_id paramId) const noexcept
{
    if (paramId >= ParamCount)
    {
        return nullptr;
    }
    return &_params[paramId];
}

Parameter *SlopeOverloadPlugin::findParam(clap_id paramId) noexcept
{
    if (paramId >= ParamCount)
    {
        return nullptr;
    }
    return &_params[paramId];
}
