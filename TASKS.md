# TASKS.md

Build roadmap for porting Slope Overload from JUCE (`../Slope-Overload`) to this CLAP/Visage/IADSP/FFTConvolver stack. See `CLAUDE.md` for project context and architecture reference. Check items off as they land; this file is expected to evolve as work progresses.

## Phase 0 — Build & Toolchain Bootstrap (complete)

- [x] Root `CMakeLists.txt`: C++20, `cmake_minimum_required(VERSION 3.25)` (IADSP's requirement), `CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON` set before adding clap-wrapper so VST3/AudioUnit SDKs auto-fetch via CPM (Open Items #1), `add_subdirectory()` for `clap`/`clap-helpers`/`clap-wrapper`/`IADSP`/`visage` + `Plugin/`. `FFTConvolver` deliberately not wired here (no upstream `CMakeLists.txt`; picked up in Phase 2). **Gotcha:** `CMAKE_MSVC_RUNTIME_LIBRARY` must be pinned at the root, before any `add_subdirectory()`, or the VST3 wrapper link fails with `LNK2038` — clap-wrapper only sets it within its own subdirectory scope.
- [x] `.gitignore`, `LICENSE` (MIT), `VERSION` (`0.1.0`)
- [x] `Plugin/` passthrough CLAP plugin (raw C ABI, stereo passthrough, `audio-ports` only) proved the toolchain end-to-end; superseded in place by Phase 1's `clap::helpers::Plugin<>` rewrite.
- [x] `Plugin/CMakeLists.txt`: `make_clapfirst_plugins()` — id `com.icebreakeraudio.slopeoverload`, `PLUGIN_FORMATS CLAP VST3 AUV2` (AUV2 a safe no-op off-Apple), `AUV2_MANUFACTURER_CODE "IceB"` / `AUV2_SUBTYPE_CODE "IASO"` (reused from the original), standalone config included. **Gotcha:** `make_clapfirst.cmake` has an upstream typo (references `C1ST_BUNDLE_IDENTIFER`, missing "I") that leaves the VST3/AUv2 bundle ID empty-prefixed (e.g. `.vst3` not `com.icebreakeraudio.slopeoverload.vst3`) regardless of what's passed — cosmetic only, not fixable here.
- [x] Confirmed `.clap`/`.vst3`/standalone artifacts build under `build/SlopeOverload_assets`; standalone smoke-tested (launches, stays running).
- [ ] Load the plugin in an actual DAW/host — no DAW available in this dev environment; folded into Phase 5's manual DAW load-testing pass.

## Phase 1 — CLAP Plugin Shell (complete)

- [x] Plugin class via `clap::helpers::Plugin<>` — `Plugin/SlopeOverloadPlugin.h/.cpp` (renamed in place from Phase 0's `passthrough_clap.*`). **Gotcha:** the TU defining the class must `#include <clap/helpers/plugin.hxx>` and `host-proxy.hxx` (not just `plugin.hh`) or the linker can't find the `<Terminate, Maximal>` template instantiation.
- [x] `audio-ports` extension — mono (1-in/1-out) and stereo (2-in/2-out) via `audio-ports-config` (host-selectable while inactive); the original's mono→stereo expansion case was dropped.
- [x] Internal `Parameter` abstraction (`Plugin/Parameter.h/.cpp`) + `params` extension, matching the original's 6 parameters (IDs/ranges/defaults): `active` (bool, default true), `inGain` (float, −60..+24 dB), `outGain` (float, −60..+12 dB), `sRate` (int 0–15, default 7), `aaFilt` (bool, default true), `speaker` (choice {A, B, C}, default A).
- [x] `state` extension — new simple binary format (magic/version/count header + fixed-order values); no preset compatibility with the original (Open Items #4).
- [x] `process()`: block-granularity parameter drain, passthrough audio (Phase 1 applies no DSP — that's Phase 2), mono/stereo port handling.
- [x] `latency` extension (hardcoded 0 until Phase 2). `tail` skipped as meaningless pre-Phase 2.
- [ ] Load-test in a DAW (parameter automation, state round-trip) — same DAW-availability gap as Phase 0, folded into Phase 5. `clap-validator` wasn't readily runnable here either (needs a Rust toolchain fetch) — tracked as its own Phase 5 item.

## Phase 2 — Audio Engine Core (headless, no UI) (complete)

- [x] Envelope follower added to IADSP — `IADSP::EnvelopeFollower` (peak/RMS ballistics, ported from `juce::dsp::BallisticsFilter`'s exact exponential coefficients).
- [x] Wired up `FFTConvolver` via hand-written `cmake/FFTConvolver.cmake` (static lib from its 4 loose sources, no upstream `CMakeLists.txt`), `include()`'d from the root.
- [x] Internal audio-buffer abstraction — `AudioBuffer` (`third_party/IADSP/IA_Utilities/AudioBuffer.hpp`; moved into the IADSP submodule from this repo's `DSP/` once IADSP's own `Fifo` needed the same type), a non-owning `float**` + channel/frame-count view replacing JUCE's `AudioBlock`/`ProcessContext` (no chain-uniformity need for a fixed hand-written pipeline). Wired into `SlopeOverloadPlugin::process()`.
- [x] Port `DeltaModulation` (`DSP/DeltaModulation.h/.cpp`) — concrete, `float`-only, non-template port of the original's chain: `IADSP::Oversampler` (swap for `juce::dsp::Oversampling`), `IADSP::FirstOrderFilter` (DC pre/post), `IADSP::OnePoleEQFilter` (high-boost), `IADSP::SecondOrderFilter` cascade (anti-aliasing, swap for JUCE `TPTFilter`), `IADSP::EnvelopeFollower` (RMS + peak, `rmsFollower`/`peakFollower`) driving the delta-quantization gate, and the core delta-quantization loop (clock-phase accumulator / 7-bit depth / threshold — pure math, ported directly). `activate()`/`deactivate()` call `prepare()`/`reset()`; `sRate`/`aaFilt` drive `setSampleRateIndex`/`setAntiAliasing` per block. Added `DSP/ScopedNoDenormals.h` (FTZ/DAZ RAII guard, replaces `juce::ScopedNoDenormals`).
- [x] Port the speaker/convolution stage (`DSP/Speaker.h/.cpp`): two `fftconvolver::FFTConvolver` instances (mono-only API, one per channel) per real IR, `IADSP::BasicClippers::cubicSoftClip` post-convolution. Since only 2 IRs are ever needed (`speaker`'s third choice is an intentional bypass, not a missing asset), both convolvers are built once in `prepare()` off the audio thread (called from `activate()`) — runtime `speaker` switches just pick which pre-built convolver's output to use, crossfaded over ~20ms (including to/from bypass), with a silence-drain burst fed to the convolver being faded away from so its overlap-add tail can't resurface as a ghost echo on reselect. Zero added latency (per FFTConvolver's own docs, given `init()`'s blockSize matches the host's max block size). `speaker` mapping: A(0)=bypass, B(1)=`HS200Close`, C(2)=`VL1Edge`. IR loading/resampling via `DSP/IRLoader.h` (Open Items #8/#9).
- [x] Custom dry/wet mixer + bypass delay line added to IADSP (replace `juce::dsp::DryWetMixer`/`DelayLine`): `IADSP::DelayLine` (single-channel, integer-sample, mirrored-buffer trick) and `IADSP::CrossfadeMixer` built on top of it (click-free two-source blend, `IADSP::LinearSmoother`-driven 50ms ramp on mix changes). Both take raw `Type**` (matching `Oversampler`/`LoudnessMeter`), not this project's `AudioBuffer`, so IADSP stays free of a dependency on it.
- [x] Wire latency reporting. Wired into `SlopeOverloadPlugin`: `dpcm`/`speaker`/gain now always run every block (matching the original's design/CPU-cost tradeoff) and `active` just drives `mixer.setMix()` — a deliberate deviation from an earlier interim hard-gate-on-`active` approach. `activate()` sets `mixer.setLatencyCompensation(dpcm.getLatencySamples())` and calls `_host.latencyChanged()` (only legal during `activate()`, per `clap/ext/latency.h`); `latencyGet()` returns `dpcm.getLatencySamples()` (FFTConvolver/`Speaker` itself contributes none).

## Phase 3 — GUI with Visage (next up)

- [x] Adapt `third_party/visage/examples/ClapPlugin/clap_plugin.cpp` as the `gui` extension integration template (`guiCreate`/`guiSetParent`/`guiSetSize`/resize hints) — this exists already and de-risks CLAP↔Visage embedding entirely
- [x] Resizable vector background via `SvgFrame` + `Background.svg`
- [x] Port assets from `../Slope-Overload/assets` (2 fonts, 3 SVGs) into this project, embedded via `visage_file_embed` — the 2 WAV IRs are already handled (pre-decoded into `DSP/IRData/`, see Open Items #8; not via `visage_file_embed`)
- [x] Custom slider widget with digital text readout (replaces `TextSlider`) for `inGain`/`outGain`/`sRate`
- [x] Custom grouped-toggle selector (replaces `RadioButtonComponent`) for `speaker`
- [x] Reuse a plain `ToggleButton` for `aaFilt`
- [x] Power button via `ToggleIconButton` using the existing `PowerButton_On.svg`/`PowerButton_Off.svg`
- [ ] Custom oscilloscope `Frame` (replaces `PixelScope`), fed by `IADSP::Fifo` (see Open Items #3) written on the audio thread, polled on a UI timer (~50ms, matching the original's refresh rate)
- [x] Adapt or create some kind of parameter attachment class or way of managing connections between the audio engine and the UI

## Phase 4 — Packaging & Distribution

- [ ] Configure `make_clapfirst_plugins()` for CLAP + VST3 + AUv2 + Standalone with real bundle metadata (bundle ID, manufacturer/subtype codes) — LV2 intentionally excluded (see Open Items #6)
- [ ] Finalize VST3/AudioUnit SDK sourcing for release builds (Open Items #1)
- [ ] Revisit the macOS Gatekeeper/notarization friction the original README already flags, before any public release

## Phase 5 — Testing & Validation

- [ ] Add `free-audio/clap-validator` as an external dev-time tool to sanity-check the CLAP implementation (not vendored as a submodule — just a CLI run manually/in CI; see Open Items #7)
- [ ] Manual DAW load-testing pass (the original also had no automated test suite — testing was always manual)

## Open Items (missing pieces + proposed solutions)

1. **VST3 SDK & AudioUnit SDK aren't vendored.** clap-wrapper needs them for VST3/AU builds.
   → Use `CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES=ON` (CPM auto-fetch) for early bring-up; consider vendoring as submodules later for reproducible/offline builds. Note the VST3 SDK's GPLv3/commercial dual license — the original project already accepts this tradeoff by shipping VST3 builds.

2. ~~**No JUCE-free ballistics/envelope follower.**~~ Done — `IADSP::EnvelopeFollower` (`third_party/IADSP/IA_Utilities/EnvelopeFollower.{hpp,cpp}`) is a JUCE-free port of `juce::dsp::BallisticsFilter`'s attack/release ballistics math (peak/RMS modes, exact exponential coefficients), wired into `DeltaModulation`'s gate as `rmsFollower`/`peakFollower`.

3. ~~**No lock-free FIFO usable without JUCE.**~~ Done — `IADSP::Fifo` (`third_party/IADSP/IA_Utilities/FiFo.hpp`) was rewritten as a lock-free SPSC ring buffer over `std::atomic` (acquire/release on the read/write positions); a `juce::AudioBuffer` convenience overload compiles in automatically only if JUCE is reachable on the include path, but there's no hard dependency anymore. **Not yet committed in the IADSP submodule itself** — do that before relying on it from a fresh clone. Still open: actually wiring it into the audio-thread→UI oscilloscope path (Phase 3).

4. ~~**Parameter management & state serialization need a full redesign.**~~ Done — internal `Parameter` struct (`Plugin/Parameter.h/.cpp`) wired to the `params` extension; custom binary format (magic/version/count header) for the `state` extension. No preset compatibility with the original, by choice.

5. ~~**Real-time-unsafe IR swap risk.**~~ Done — since only 2 IRs are ever needed (the `speaker` parameter's third choice is an intentional bypass/off state, not a missing asset), both convolvers are built once in `Speaker::prepare()` off the audio thread; a runtime `speaker` switch just picks which already-built convolver's output to use, crossfaded (~20ms) to avoid clicks.

6. **LV2 format dropped, by choice.** `clap-wrapper` doesn't produce LV2. The original only included LV2 because JUCE made it free to add, not because it mattered to the project.
   → Not pursued. No action needed.

7. **No plugin-validation tooling vendored.** The original had no test suite either (manual DAW testing only), but CLAP plugins lose JUCE's relatively forgiving host-wrapper safety net.
   → Recommend adding `free-audio/clap-validator` as an external dev tool.

8. **~~No WAV decoding capability anywhere in the dependency tree.~~ Resolved.** `FFTConvolver` only accepts raw `float*` sample arrays (no file I/O at all), and neither `IADSP` nor `visage` contain a WAV/RIFF decoder; the original got this for free via JUCE's `AudioFormatManager` inside `juce::dsp::Convolution::loadImpulseResponse()`.
   → Chose the "pre-decode + embed" option: `tools/convert_ir_wav.py` (stdlib-only Python, hand-rolled RIFF chunk walk — asserts mono only, decodes whatever PCM/float bit depth the `fmt ` chunk actually reports) converts each source WAV in `Assets/` into a generated C++ header under `DSP/IRData/` (`constexpr std::array<float, N>` + a `SpeakerIRData{ sampleRate, std::span<const float> }` instance, see `DSP/IRData/IRData.h`). No WAV parser ships in the plugin binary; no new runtime dependency. `DSP/IRLoader.h` exposes `getSpeakerIRData(SpeakerIR)` and `resampleSpeakerIR(SpeakerIR, targetSampleRate)`. Rerun the script only if the source IR assets change.

9. ~~**IR sample-rate mismatch.**~~ Done — the shipped IR WAVs are fixed at 48kHz; `DSP/IRLoader::resampleSpeakerIR()` (wrapping `IADSP::IA_Utilities/ResamplingFilter.hpp`) is called from `Speaker::prepare()` with the host's live sample rate before `FFTConvolver::init()`, so the IR is resampled to match the plugin's running rate, never the other way around.
