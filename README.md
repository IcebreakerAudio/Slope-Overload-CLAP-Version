# Slope Overload (CLAP Edition)
__A JUCE-free rebuild of the DPCM LoFi plugin effect__

## About The Effect

This is a rough emulation of the NES (Famicom) sample channel, which uses 1-bit delta modulation to store and reconstruct audio. Rather than storing each sample amplitude as a number, delta modulation only cares if the sample is higher (1) or lower (0) than the previous sample, and this is how we get away with only using 1-bit.

This method of digital sampling can actually yield high quality audio with high enough sample rates (a variation on this technique was used for SACD). Naturally the NES had fairly low sample rates (with the highest around 33.1kHz). The audio chip supported 16 different sample rate settings for playback.

This form of digital lofi is slightly different from the usual PCM bitcrushers as it introduces a unique artifact:
```Slope Overload```

Because of how the encoding works, curves in the waveform get distorted into straight lines. So a sine wave would become a triangle wave.

## The Plugin

This is a ground-up rebuild of the [original JUCE-based Slope Overload](https://github.com/IcebreakerAudio/Slope-Overload) on a native CLAP stack, with no JUCE anywhere in the dependency tree.

The UI was built using [Visage](https://github.com/VitalAudio/visage) and uses 100% vector graphics so it can be resized freely.

In order to make the effect more usable, pre and post filters were added, as well as a gate which stops playback if the input audio is below the encoding threshold (without this you get a constant tone at the nyquist limit which is not unlike tinnitus).

There are also two speaker impulse responses for added retro lofi nostalgia.

## Library Replacements

This project was built primarily to learn CLAP and Visage, and to produce a small but public-release-quality plugin — while improving on the original implementation where reasonable. Every JUCE-coupled piece of the original was replaced with a JUCE-free equivalent:

| Concern | Original | This project |
|---|---|---|
| Plugin format/ABI | JUCE + clap-juce-extensions | CLAP, native |
| UI | JUCE | Visage |
| DSP utilities | IADSP + JUCE | IADSP + FFTConvolver |

Visage did have a bug when rendering certain SVG gradients, so this project uses a fork that fixes that bug. Hopefully this fix will be merged into the main Visage repository soon.

## Compatibility

Despite the library swap, effort went into keeping this version a drop-in replacement where it matters most:

- **Parameters, automation, and host recall are compatible.** All parameters keep the same IDs, ranges, and defaults as the original, so automation lanes and DAW parameter recall behave equivalently between versions.
- **Preset/session files are not interchangeable.** The CLAP `state` extension uses a new binary format — presets saved from the original JUCE build won't load here, and vice versa.
- **Performance is roughly equivalent** between the two versions.

## Differences

While audio processing is the same with little difference in performance, there are some noticeable updates:
- The UI feels smoother, especially when resizing.
- The binary sizes are about half the size of the binaries produced by JUCE.

## Dependencies

The effect is built entirely on CLAP-native tooling, added to the git project as submodules:

- [clap](https://github.com/free-audio/clap) / [clap-helpers](https://github.com/free-audio/clap-helpers) — the CLAP plugin ABI and C++ helpers for building on top of it
- [clap-wrapper](https://github.com/free-audio/clap-wrapper) — wraps the CLAP plugin to also ship as VST3 / AU / standalone
- [IADSP](https://github.com/IcebreakerAudio/IADSP) — Icebreaker Audio's shared DSP library (filters, synth basics, utilities, waveshaping)
- [Visage](https://github.com/VitalAudio/visage) — GPU-accelerated cross-platform UI library, replacing JUCE's UI layer
- [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) — partitioned FFT convolution, replacing `juce::dsp::Convolution` for the speaker-IR stage

The UI uses two fonts which both have OFL licenses.

## Build

__Slope Overload__ can be built using CMake (3.25+, C++20-capable compiler).

Submodules are already checked out in a clone from source control; for a fresh clone:

```
git submodule update --init --recursive
```

Configure and build:

```
cmake -B build -S .
cmake --build build --config Debug
```

The first configure downloads the VST3 SDK, rtaudio, rtmidi, and wil via CPM, so it's slow the first time.

Personally I use [Visual Studio Code](https://code.visualstudio.com/) for working on and building the project, but you can also build from the terminal if you have CMake installed and set up for that.

## Install

Out of the box __Slope Overload__ supports CLAP, VST3, and AU (Apple-only). A pre-built standalone version is not supplied, but can be built from source if you want it.

Note that Apple have a very heavy-handed security system that will probably block the plugins from being used. You will need to update the MacOS security features to either allow unsigned files, or to exclude the plugin files (the method for how to do this changes now and again, so Google for the latest technique).

If you build the plugins yourself then you won't need to deal with the security stuff since you create the plugin binaries.
