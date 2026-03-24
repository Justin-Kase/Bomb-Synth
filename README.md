# 💣 Bomb Synth

**Hybrid wavetable synthesizer by [Illbomb](https://illbomb.com)**

VST3 + AU · macOS Universal (arm64 / x86_64) · C++17 · JUCE 8

---

## What It Is

Bomb Synth is a professional-grade hybrid synthesizer with three oscillators, each backed by a **6-bank wavetable engine** with real-time waveform visualization. Each bank contains 8 morphable frames generated via additive synthesis — sweep through them with the MORPH knob and watch the oscilloscope animate live.

Dual filters (Moog Ladder + SVF), dual LFOs, amp and filter envelopes, 16-voice polyphony, and a fully labeled 1100×700 resizable UI.

---

## Download

| Platform | Link |
|----------|------|
| macOS Universal (VST3 + AU) | [BombSynth-macOS.zip](https://github.com/Justin-Kase/Bomb-Synth/releases/latest/download/BombSynth-macOS.zip) |
| Windows / Linux | Build from source (see below) |

**macOS installation:**
1. Extract the zip
2. Copy `Bomb Synth.vst3` → `/Library/Audio/Plug-Ins/VST3/`
3. *(Optional)* Copy `Bomb Synth.component` → `/Library/Audio/Plug-Ins/Components/` (AU)

---

## Wavetable Banks

Each of the 3 oscillators can independently select any bank and morph through its 8 frames:

| Bank | Color | Character |
|------|-------|-----------|
| **ANALOG** | Sky blue | Sine → Soft → Triangle → Saw (8/16/32 harmonics) → Square → Pulse 25% |
| **DIGITAL** | Purple | Hollow → Ringy → Bell → FM-like → Clipped → Prime harmonics → Bright → Max |
| **VOCAL** | Amber | Formant vowel morphing: AH → EH → EE → IH → OH → OO → UH → AA |
| **BRASS** | Orange | Progressive harmonic opening from soft to full blaring brass |
| **PLUCK** | Green | Steep rolloff (muted pluck) → gentle rolloff (resonant string) |
| **ORGAN** | Pink | Hammond drawbar presets: Flute → Jazz I/II → Gospel → Rock → Full |

Banks are generated at load time via **additive synthesis** (sum of sine harmonics). The VOCAL bank uses formant modeling with Gaussian envelopes around F1/F2/F3 formant frequencies.

---

## Signal Flow

```
OSC 1 ──┐
OSC 2 ──┼──► [Mix] ──► [Ladder Filter] ──► [Amp Env] ──► [Master Gain] ──► Out
OSC 3 ──┘              [SVF Filter   ]      ↑
                                         [Filter Env]
         LFO 1 ──► mod targets
         LFO 2 ──► mod targets
```

Each oscillator: **Wavetable bank** + **Morph** + Tune + Fine + Level + FM + Unison (1–8 voices) + Detune

---

## Parameters

### Oscillators (× 3)
| Parameter | Range | Description |
|-----------|-------|-------------|
| WAVEFORM | 6 banks | Bank selector (< > arrows in display) |
| MORPH | 0–1 | Sweeps through 8 frames within the selected bank |
| TUNE | ±24 st | Coarse pitch offset in semitones |
| FINE | ±100 c | Fine pitch offset in cents |
| LEVEL | 0–1 | Oscillator output level |
| FM | 0–1 | FM modulation amount to next oscillator |
| UNISON | 1–8 | Number of unison voices |
| DETUNE | 0–1 | Unison voice spread |

### Filter
| Parameter | Range | Description |
|-----------|-------|-------------|
| TYPE | 5 modes | Ladder LP24, Ladder LP12, Ladder HP24, SVF LP, SVF BP |
| CUTOFF | 20–20k Hz | Filter cutoff frequency |
| RESONANCE | 0–1 | Filter resonance / self-oscillation |
| DRIVE | 1–8× | Pre-filter drive / saturation |
| ENV AMT | −1–+1 | Filter envelope modulation depth |

### Envelopes
| Parameter | Range | Description |
|-----------|-------|-------------|
| ATTACK | 0.1–10,000 ms | Attack time |
| DECAY | 0.1–10,000 ms | Decay time |
| SUSTAIN | 0–1 | Sustain level |
| RELEASE | 0.1–10,000 ms | Release time |
| CURVE *(amp only)* | −1–+1 | Envelope curve shape |

### LFOs (× 2)
| Parameter | Range | Description |
|-----------|-------|-------------|
| SHAPE | 7 shapes | Sine, Triangle, Saw, Rev Saw, Square, S&H, Smooth Rand |
| RATE | 0.01–30 Hz | LFO frequency |
| DEPTH | 0–1 | LFO modulation depth |
| PHASE *(LFO 1)* | 0–1 | LFO start phase offset |

### Master
| Parameter | Range | Description |
|-----------|-------|-------------|
| VOLUME | 0–1 | Master output gain |
| GLIDE | 0–5000 ms | Portamento time |

---

## UI

- **Size:** 1100 × 700 px (resizable)
- **OSCILLATORS section:** 3 strips — each with live waveform oscilloscope + bank nav arrows + 7 labeled knobs
- **FILTER section:** type dropdown + 4 knobs
- **AMP ENVELOPE / FILTER ENVELOPE:** standard ADSR knobs, color-coded green / amber
- **LFO 1 / LFO 2:** shape dropdown + rate/depth/phase knobs
- **MASTER:** volume + glide

The wavetable display updates in real-time as you turn the MORPH knob, drawing an interpolated frame between the two adjacent wavetable frames using the same algorithm as the audio engine.

---

## Build

```bash
# Configure (downloads JUCE automatically via CMake FetchContent)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Output locations
# VST3: build/Source/BombSynth_artefacts/Release/VST3/Bomb Synth.vst3
# AU:   build/Source/BombSynth_artefacts/Release/AU/Bomb Synth.component
```

### Prerequisites
- CMake 3.21+
- C++17 compiler
  - macOS: Xcode Command Line Tools (`xcode-select --install`)
  - Windows: Visual Studio 2019+ with C++ workload
  - Linux: GCC 9+ or Clang 10+
- Linux extra deps:
  ```bash
  sudo apt-get install libasound2-dev libjack-jackd2-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxrandr-dev \
    libfreetype6-dev libwebkit2gtk-4.1-dev
  ```

---

## Architecture

See **[SPEC.md](SPEC.md)** for the full technical specification: signal flow diagrams, class hierarchy, DSP notes (PolyBLEP, Huovilainen ladder model, JUCE oversampling), wavetable generation, LFO shapes, mod matrix design, and CPU optimization strategy.

### Key Classes

| Class | Role |
|-------|------|
| `SynthEngine` | 16-voice polyphony, voice allocation, MIDI dispatch |
| `Voice` | Per-voice: 3× WavetableOscillator + LadderFilter + SVFFilter + 2× ADSR |
| `WavetableBankLibrary` | Singleton: 6 banks × 8 frames × 2048 samples, generated at startup |
| `WavetableOscillator` | Phase accumulator + bilinear frame interpolation |
| `AnalogOscillator` | PolyBLEP bandlimited, 8-voice unison (kept for legacy/aux use) |
| `LadderFilter` | Huovilainen 4-pole Moog, 2× oversampled nonlinear stages |
| `SVFFilter` | State-variable LP/HP/BP/Notch |
| `ADSR` | Exponential attack/decay/release with curve shaping |
| `LFO` | 7 shapes + drawable table + tempo sync + fade-in |
| `ModMatrix` | Block-rate source→destination routing |
| `WavetableDisplay` | JUCE Component: oscilloscope viz with 3-pass glow, bank navigation |

---

## Changelog

### v0.2.0 (March 2026)
- **Wavetable engine:** 6 banks × 8 morphable frames, additive synthesis generation at startup
- **WavetableDisplay:** live oscilloscope visualization per oscillator — glow rendering, frame dots, bank navigation arrows, bank-colored accent
- **All 3 oscillators** now use `WavetableOscillator` (bank + morph + level + tune per voice)
- **Full UI labels:** WAVEFORM header in display, OSC badge redesign, stacked combo labels
- **Parameters added:** `osc{1,2,3}_morph`, `osc{1,2,3}_wave` (bank index 0–5)
- **UI expanded:** 1100×700, resizable, OSC section 220px tall

### v0.1.0 (March 2026)
- Core architecture: `SynthEngine`, 16-voice polyphony, voice stealing
- Full DSP engine: `AnalogOscillator` (PolyBLEP), `LadderFilter` (Huovilainen), `SVFFilter`, `ADSR`, `LFO`, `ModMatrix`, `GranularEngine`
- Initial UI: all sections labeled, color-coded by function
- VST3 + AU, macOS Universal, GitHub Actions CI

---

## License

MIT — see [LICENSE](LICENSE)

---

*Built with [JUCE](https://juce.com) · Made by [Illbomb](https://illbomb.com)*
