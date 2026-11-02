# BvM 1.3.0 — Futuristic Audio Engine Update

## DSP correction
- Rebuilt the look-ahead limiter detector around a preallocated monotonic peak deque.
- The detector now evaluates the complete linked-stereo look-ahead window, rather than reacting only to the current sample. This prevents a transient already inside the delay line from escaping before gain reduction is applied.
- The peak deque and audio delay buffers are allocated only in `prepare()`. Processing remains allocation-free.
- Added a transient timing/ceiling regression test in addition to the existing finite-value, stereo-link and signal-path smoke tests.

## Validation boundary
- The standalone DSP test target is independent of JUCE and can be built with CMake/CTest.
- The complete JUCE plugin still requires a machine with JUCE 8.0.4 available and must be validated in real hosts (VST3/AU/Standalone) before release.
- The limiter is a sample-peak limiter. It is **not** advertised as a true-peak limiter; true-peak certification requires oversampled reconstruction and dedicated inter-sample test vectors.
- “AI” controls in this version are adaptive DSP heuristics, not a trained neural network or cloud AI service.
