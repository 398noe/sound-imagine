# Sound Imagine 2.0.0

This release brings together the development of Sound Imagine since its initial 2024 version. The original plug-in introduced FFT-based stereo analysis and a live 3D visualization. Version 2.0 replaces that early implementation with a redesigned analyzer, a more complete stereo measurement workflow, a rebuilt interface, and a new build and verification setup.

## Audio analysis

- Rebuilt the FFT analysis pipeline around a Hann window, correct FFT work-buffer sizing, and window-energy normalization.
- Added selectable FFT sizes of 2,048, 4,096, 8,192, 16,384, and 32,768 points, with the corresponding window length and frequency-bin spacing shown in settings.
- Added 32, 64, or 128 logarithmically spaced bands from 20 Hz to the lower of 20 kHz and the Nyquist frequency. Band power is apportioned across bin boundaries to reduce discontinuities.
- Added selectable 50, 250, or 1,000 ms power averaging. Power and cross-power are averaged before deriving levels and stereo measurements.
- Added integrated band RMS in dBFS and power spectral density (PSD) in dBFS/Hz.
- Added frequency-band measurements for Mid and Side power, Side fraction, left/right correlation, left/right balance, and relative mono-sum level.
- Added handling for mono input, digital silence, non-finite samples, and bands where correlation cannot be defined.
- Added configurable display thresholds and a no-recent-audio indicator that preserves the last measurement when the host stops sending audio.

## Visualization and workflow

- Reworked the display as a software-rendered, interactive 3D spectrum: frequency, Side fraction, and band level are shown together without requiring OpenGL.
- Added frequency and level guides, Side-percentage markers, correlation-based color, and band details on hover.
- Added free camera rotation, pan, zoom, reset, and X/Y/Z axis-aligned views.
- Added a four-view layout combining the free camera with the X, Y, and Z views. In the four-view layout, camera gestures affect the free-view panel.
- Added a separate all-bands window for reading every measurement, including bands below the graph threshold. It can be opened without resizing the graph.
- Added a shared freeze control for the graph and band list. Audio pass-through and analysis continue while the displayed measurements are held.
- Added a help view explaining the measurements and how to interpret the different projections.
- Added English and Japanese interface languages, including labels, status text, help, and the band list.
- Added saving and restoring of analysis settings, threshold, language, camera rotation, pan, zoom, and view layout with the DAW project.

## Audio path and reliability

- Rebuilt audio processing so the input passes through unchanged, with no added plug-in latency.
- Moved FFT work off the audio callback to an analysis thread fed by a fixed-capacity single-producer/single-consumer queue.
- Kept allocation, FFT work, GUI access, and mutex waits out of the audio callback. If analysis falls behind, analysis samples are skipped and the analysis window is rebuilt; audio output remains unaffected.
- Added automated checks for stereo measurements across 32–192 kHz, normalization, PSD calibration, band coverage, silence, invalid values, reinitialization, queue overload and recovery, state save/restore, freeze behavior, rendering, and audio pass-through.
- Added validation of the built VST3 bundle in a JUCE test host, including loading, editor reconnection, state restoration, and pass-through checks.

## Build and packaging

- Replaced the earlier generated project/build setup with a CMake-based C++20 project and Windows x64 build presets.
- Pinned JUCE to 8.0.15 and added PowerShell scripts for configure/build/test, launching the Standalone build, and installing the VST3 bundle.
- Added Debug and Release builds for the VST3 plug-in and Standalone application, plus recorded verification results and analysis documentation.
- Retained the existing VST3 manufacturer and plug-in codes (`Nyao` / `Imag`).

## Compatibility and limitations

- Existing projects load with default settings for the new interface. The previous version did not persist plug-in parameters, so there are no saved settings to migrate.
- The analyzer reports band RMS and PSD; it does not measure Sample Peak, True Peak, LUFS, phase angle, or coherence.
- Verified on Windows x64. Individual DAW behavior and builds on other operating systems have not been confirmed.
