# TASKS.md

Build roadmap for porting Slope Overload from JUCE (`../Slope-Overload`) to this CLAP/Visage/IADSP/FFTConvolver stack. See `CLAUDE.md` for project context and architecture reference. Check items off as they land; this file is expected to evolve as work progresses.

## Phase 0 — Build & Toolchain Bootstrap

- [x] Root `CMakeLists.txt`:
  - `cmake_minimum_required(VERSION 3.25)` (driven by IADSP's requirement)
  - `project(SlopeOverload VERSION 0.1.0 LANGUAGES C CXX)`
  - C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`)
  - `set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "..." FORCE)` *before* adding clap-wrapper, so VST3/AudioUnit SDKs auto-fetch via CPM (resolves Open Items #1)
  - `add_subdirectory()` for 5 of the 6 submodules: `clap`, `clap-helpers`, `clap-wrapper`, `IADSP`, `visage`
  - **`FFTConvolver` is deliberately NOT wired here** — it ships with no `CMakeLists.txt` and isn't used until Phase 2's convolution stage. A small `cmake/FFTConvolver.cmake` module (hand-written in this repo, not the submodule) will define its static-library target when Phase 2 needs it (tracked there, see Phase 2's first bullet)
  - `add_subdirectory(Plugin)` for the passthrough plugin below
  - **Discovered while implementing:** MSVC needs `CMAKE_MSVC_RUNTIME_LIBRARY` pinned at the root, before any `add_subdirectory()`, or the VST3 wrapper link fails with `LNK2038` (static/dynamic CRT mismatch) — clap-wrapper's own `CMakeLists.txt` only sets this within its own subdirectory scope, which doesn't reach sibling targets
- [x] Add `.gitignore` (generic CMake/IDE ignores, adapted from `../Slope-Overload/.gitignore` minus JUCE-specific entries), `LICENSE` (MIT, copied from the original, copyright year bumped), `VERSION` (`0.1.0`)
- [x] `Plugin/` folder: a trivial passthrough CLAP plugin against the raw CLAP C ABI, modeled directly on `third_party/clap-wrapper/tests/clap-first-example/` (`distortion_clap.cpp` + `distortion_clap_entry.cpp`/`.h`), stripped down to:
  - stereo in → stereo out, direct copy, no DSP
  - `audio-ports` extension only (no params, no state, no track-info — those land for real in Phase 1)
  - plugin id `com.icebreakeraudio.slopeoverload`, name "Slope Overload"
  - **Note:** this is intentionally *not* the final plugin shell. Phase 1 rewrites `Plugin/passthrough_clap.cpp` and its entry point in place to use `clap::helpers::Plugin<>` — this folder is not throwaway, but its raw-ABI contents are.
- [x] `Plugin/CMakeLists.txt`: static `impl` library + one `make_clapfirst_plugins()` call —
  - `TARGET_NAME SlopeOverload`, `IMPL_TARGET SlopeOverload-impl`
  - `OUTPUT_NAME "Slope Overload"`
  - `BUNDLE_IDENTIFIER "com.icebreakeraudio.slopeoverload"`
  - `PLUGIN_FORMATS CLAP VST3 AUV2` (AUV2 is a safe no-op off-Apple — guarded internally by `if (APPLE AND ...)`)
  - `AUV2_MANUFACTURER_CODE "IceB"`, `AUV2_SUBTYPE_CODE "IASO"` (reused from the original JUCE project's `PLUGIN_MANUFACTURER_CODE`/`PLUGIN_CODE`)
  - `STANDALONE_CONFIGURATIONS standalone "Slope Overload" com.icebreakeraudio.slopeoverload`
  - `ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/SlopeOverload_assets`
  - **Discovered while implementing:** `make_clapfirst.cmake` itself has a variable-name typo — it declares/parses `BUNDLE_IDENTIFIER` correctly for the CLAP target, but references the misspelled `C1ST_BUNDLE_IDENTIFER` (missing "I") for the VST3/AUv2 bundle IDs, so those two always get an empty-prefixed bundle ID (e.g. `.vst3` instead of `com.icebreakeraudio.slopeoverload.vst3`) regardless of what's passed in. This is an upstream bug in `third_party/clap-wrapper`, not something to fix here; it doesn't block building or loading, only cosmetic bundle-ID correctness for VST3/AU.
- [x] Configure + build (`cmake -B build -S .` / `cmake --build build`); confirm `.clap`/`.vst3`/standalone artifacts land under `build/SlopeOverload_assets` — this is the real toolchain proof (CMake → CLAP → VST3/standalone wrapping). Done: `Slope Overload.clap`, `Slope Overload.vst3`, and the `Slope Overload.exe` standalone all build successfully; the standalone binary was smoke-tested (launches and stays running without crashing)
- [ ] Confirm the passthrough plugin loads and passes audio through in at least one DAW/host — **manual step, not yet done** (needs a real DAW; nothing in this sandboxed dev environment can do this)

## Phase 1 — CLAP Plugin Shell

- [x] Plugin class via `clap::helpers::Plugin<>` (clap-helpers' C++ base, avoids hand-rolling the raw C ABI) — `Plugin/SlopeOverloadPlugin.h/.cpp`, renamed in place from Phase 0's `passthrough_clap.*`/`passthrough_clap_entry.*`. Requires `#include <clap/helpers/plugin.hxx>` (and `host-proxy.hxx`) in exactly the TU defining the class — `plugin.hh` only declares the template, `plugin.hxx` has the method bodies that need instantiating for our `<Terminate, Maximal>` specialization, otherwise the linker can't find them.
- [x] `audio-ports` extension — supports both mono (1-in/1-out) and stereo (2-in/2-out) via the `audio-ports-config` extension (2 configs, host-selectable while inactive); the original's mono→stereo expansion case was intentionally dropped, only matching in/out channel counts are supported
- [x] Internal `Parameter` abstraction (`Plugin/Parameter.h/.cpp`) + `params` extension covering the original's 6 parameters with matching IDs/ranges/defaults:
  - `active` (bool, default true)
  - `inGain` (float, −60..+24 dB, default 0)
  - `outGain` (float, −60..+12 dB, default 0)
  - `sRate` (int, 0–15, default 7)
  - `aaFilt` (bool, default true)
  - `speaker` (choice {A, B, C}, default A)
- [x] `state` extension with a new, simple serialization format (magic/version/count header + fixed-order parameter values; no preset compatibility with the original — see Open Items #4)
- [x] `process()`: drain parameter-change events (block-granularity, not sample-accurate — nothing yet depends on per-sample timing since no DSP consumes the values), pure passthrough audio (Phase 1 intentionally does not apply gain/bypass to the signal — that lands with the real DSP in Phase 2), handle mono/stereo port config
- [x] `latency` extension (explicit 0 for now). `tail` extension skipped — meaningless before Phase 2's convolution/oversampling exist to report a real tail
- [ ] Load-test the Phase 1 shell in an actual DAW/host (parameter automation, state save/reload round-trip) — **manual step, not yet done**, same sandboxed-environment limitation as Phase 0's outstanding DAW check. `free-audio/clap-validator` was not readily available in this environment either (would need a Rust toolchain fetch/build); vendoring it is still tracked as Phase 5 work

## Phase 2 — Audio Engine Core (headless, no UI)

- [ ] Wire up `FFTConvolver` (deferred from Phase 0 — it has no `CMakeLists.txt`): author `cmake/FFTConvolver.cmake` declaring a static library from its 4 loose sources (`AudioFFT`, `FFTConvolver`, `TwoStageFFTConvolver`, `Utilities` `.cpp`/`.h`), `include()`'d from the root `CMakeLists.txt`
- [ ] Define an internal audio-buffer abstraction (`float**` + channel/frame counts) replacing JUCE's `AudioBlock`/`ProcessContext`
- [ ] Create Envelope Follower class in IADSP library
- [ ] Port `DeltaModulation`:
  - `IADSP::Oversampler` (direct swap for `juce::dsp::Oversampling`)
  - `IADSP::FirstOrderFilter` (DC pre/post filters)
  - existing `IADSP::OnePoleEQFilter` (high-boost — no change needed)
  - `IADSP::SecondOrderFilter` cascade for anti-aliasing (use SecondOrderFilter in place of JUCE TPTFilter)
  - new custom envelope follower for the gate (see Open Items #2 and task above)
  - the core delta-quantization loop (clock-phase accumulator / 7-bit depth / threshold) — pure math, no JUCE deps, ports directly
- [ ] Port the speaker/convolution stage:
  - two `fftconvolver::FFTConvolver` instances (mono-only API, one per channel) for the two real IRs
  - reuse `IADSP::BasicClippers::cubicSoftClip` post-convolution as-is
  - real-time-safe IR swap on speaker change (see Open Items #5)
  - the third `speaker` choice is an intentional bypass/off state (no convolution run, not a missing IR) — no extra asset needed
  - IR loading/resampling is done (`DSP/IRLoader.h`, see Open Items #8/#9) — still open: calling `resampleSpeakerIR()` at `prepareToPlay`/`activate()` time with the host's real sample rate and feeding the result into `FFTConvolver::init()`
- [ ] Custom dry/wet mixer in IADSP libaray (replaces `juce::dsp::DryWetMixer`)
- [ ] Custom bypass delay line (replaces `juce::dsp::DelayLine`)
- [ ] Wire latency reporting (oversampler latency; FFTConvolver reportedly adds none — confirm)

## Phase 3 — GUI with Visage

- [ ] Adapt `third_party/visage/examples/ClapPlugin/clap_plugin.cpp` as the `gui` extension integration template (`guiCreate`/`guiSetParent`/`guiSetSize`/resize hints) — this exists already and de-risks CLAP↔Visage embedding entirely
- [ ] Custom slider widget with digital text readout (replaces `TextSlider`) for `inGain`/`outGain`/`sRate`
- [ ] Custom grouped-toggle selector (replaces `RadioButtonComponent`) for `speaker`
- [ ] Reuse a plain `ToggleButton` for `aaFilt`
- [ ] Custom oscilloscope `Frame` (replaces `PixelScope`), fed by a new lock-free SPSC ring buffer (see Open Items #4) written on the audio thread, polled on a UI timer (~50ms, matching the original's refresh rate)
- [ ] Power button via `ToggleIconButton` using the existing `PowerButton_On.svg`/`PowerButton_Off.svg`
- [ ] Resizable vector background via `SvgFrame` + `Background.svg`
- [ ] Port assets from `../Slope-Overload/assets` (2 fonts, 3 SVGs) into this project, embedded via `visage_file_embed` — the 2 WAV IRs are already handled (pulled forward into `DSP/IR/`, see Open Items #8; not via `visage_file_embed`, see below)
- [ ] Adapt or create some kind of parameter attachment class or way of managing connections between the audio engine and the UI

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

2. **No JUCE-free ballistics/envelope follower.** `juce::dsp::BallisticsFilter` (used for the RMS/attack-release gate) has no IADSP counterpart.
   → Implement a small one-pole attack/release envelope follower; good candidate to upstream into `IADSP::IA_SynthBasics` since IADSP is our own library.

3. **No lock-free FIFO usable without JUCE.** `IADSP::Fifo` wraps `juce::AbstractFifo`; needed for audio-thread→UI-thread oscilloscope data.
   → Write a small SPSC ring buffer; check `visage_utils`'s threading utilities first in case something reusable already exists there.

4. **Parameter management & state serialization need a full redesign.** JUCE's `AudioProcessorValueTreeState`/XML state has no direct analog in CLAP.
   → Small internal `Parameter` struct wired to the `params` extension; simple custom binary/JSON format for the `state` extension.

5. **Real-time-unsafe IR swap risk.** Naively calling `FFTConvolver::init()` (which allocates) on the audio thread when `speaker` changes would break real-time safety.
   → Build the new convolver off the audio thread, swap in a ready pointer (double-buffer pattern). Note: only 2 IRs are ever needed — the `speaker` parameter's third choice is an intentional bypass/off state, not a missing asset.

6. **LV2 format dropped, by choice.** `clap-wrapper` doesn't produce LV2. The original only included LV2 because JUCE made it free to add, not because it mattered to the project.
   → Not pursued. No action needed.

7. **No plugin-validation tooling vendored.** The original had no test suite either (manual DAW testing only), but CLAP plugins lose JUCE's relatively forgiving host-wrapper safety net.
   → Recommend adding `free-audio/clap-validator` as an external dev tool.

8. **~~No WAV decoding capability anywhere in the dependency tree.~~ Resolved.** `FFTConvolver` only accepts raw `float*` sample arrays (no file I/O at all), and neither `IADSP` nor `visage` contain a WAV/RIFF decoder; the original got this for free via JUCE's `AudioFormatManager` inside `juce::dsp::Convolution::loadImpulseResponse()`.
   → Chose the "pre-decode + embed" option: `tools/convert_ir_wav.py` (stdlib-only Python, hand-rolled RIFF chunk walk — asserts mono only, decodes whatever PCM/float bit depth the `fmt ` chunk actually reports) converts each source WAV in `Assets/` into a generated C++ header under `DSP/IR/` (`constexpr std::array<float, N>` + a `SpeakerIRData{ sampleRate, std::span<const float> }` instance, see `DSP/IRData.h`). No WAV parser ships in the plugin binary; no new runtime dependency. `DSP/IRLoader.h` exposes `getSpeakerIRData(SpeakerIR)` and `resampleSpeakerIR(SpeakerIR, targetSampleRate)`. Rerun the script only if the source IR assets change.

9. **~~IR sample-rate mismatch.~~ Partially resolved.** The shipped IR WAVs are fixed at 48kHz; the host/plugin may run at a different sample rate. The IR must be resampled to match the plugin's running sample rate at load time — not the other way around (never resample the live audio signal to match the IR).
   → `DSP/IRLoader::resampleSpeakerIR()` wraps `IADSP::IA_Utilities/ResamplingFilter.hpp` and does this now. Still open: actually calling it at `prepareToPlay`/`activate()` time with the host's live sample rate and feeding the result into `FFTConvolver::init()` — that lands with the rest of the Phase 2 convolution stage.
