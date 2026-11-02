# BvM — Badi-badia Vocal Mix

BvM is a 64-bit JUCE vocal-processing plug-in with a standalone C++17 DSP core.
Version **1.4.0 — Performance & portability pass**.

## What's improved in this build

- Pitch tracking (YIN) is completely skipped unless Auto-Tune or a harmony voice is enabled. This is the largest CPU saving for ordinary vocal mixing sessions.
- Reverb processing is skipped when its wet mix is zero; its state is reset on deactivation. Delay remains fed so its feedback memory stays continuous when its wet mix is changed.
- The real-time DSP path uses preallocated buffers and does not allocate memory in `processBlock`.
- The stereo-linked look-ahead sample-peak limiter uses a preallocated monotonic peak deque.
- Non-finite input samples are sanitized, denormal handling is enabled by the JUCE processor, and latency is reported to the host.
- DSP-only regression tests can now be configured without fetching JUCE or requiring internet access.

## Compatibility target

- **Windows:** 64-bit Windows 10/11, VST3 and Standalone.
- **macOS:** Intel x86_64 and Apple Silicon arm64 universal build, VST3, AU and Standalone.
- **Linux:** 64-bit Linux, VST3 and Standalone (distribution audio/GUI dependencies apply).
- **DAWs:** any host that correctly supports the VST3 format; AU hosts on macOS. Standalone mode is also provided.

A plug-in cannot honestly be promised to work in *every* DAW or on *every* computer: 32-bit hosts, AAX-only hosts (for example Pro Tools without VST3 hosting), unsupported operating systems, and machines below the host's own requirements are outside this build target. CLAP and AAX are not included in this JUCE configuration.

## DSP tests — no JUCE/network required

```bash
cmake -S . -B build-dsp -DBVM_BUILD_PLUGIN=OFF -DBVM_BUILD_TESTS=ON
cmake --build build-dsp --config Release
ctest --test-dir build-dsp -C Release --output-on-failure
```

## Build the complete plug-in

Requires CMake 3.22+, a C++17 compiler, Git/network access (unless `BVM_JUCE_SOURCE_DIR` points to a local JUCE checkout), and platform audio/GUI development libraries.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBVM_BUILD_PLUGIN=ON
cmake --build build --config Release --parallel
```

To use a local JUCE checkout:

```bash
cmake -S . -B build -DBVM_JUCE_SOURCE_DIR=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
```

Build outputs are under `build/BvM_artefacts/Release/`.

## CPU guidance

For lowest CPU use, leave Auto-Tune and harmonies off when they are not needed, and set Delay/Reverb mix to zero when not needed. The YIN pitch detector is intentionally dormant in that configuration. Higher sample rates, smaller host buffers, and enabling pitch shifting/harmonies increase CPU demand. No fixed CPU percentage can be guaranteed across all processors and DAWs.

## Release / validation status

This is a **development build**, not a certified commercial release. The standalone DSP tests cover finite output, non-finite input protection, pitch-detector bypass, pitch-analysis activation, common sample rates, limiter ceiling and look-ahead behavior. The full JUCE plug-in still needs to be built on Windows, macOS and Linux and tested in actual host applications. The limiter is sample-peak, not true-peak. The adaptive voice section is heuristic DSP, not a neural network; it must not be advertised as neural AI.
