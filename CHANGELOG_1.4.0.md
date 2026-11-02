# BvM 1.4.0 — Performance, portability and validation

- Avoids YIN pitch tracking unless pitch correction or harmonies are active.
- Bypasses reverb DSP when its wet mix is zero and resets its state on deactivation.
- Adds a DSP-only CMake configuration that does not download JUCE.
- Expands regression tests to cover pitch-detector gating, pitch activation, 44.1–192 kHz processing and irregular block sizes.
- Makes the build target explicit: 64-bit VST3 on Windows/Linux, VST3 + AU on macOS, Standalone on all three.
- Documents real compatibility limits and avoids claiming universal DAW/CPU support.
