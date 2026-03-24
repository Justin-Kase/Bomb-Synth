# 💣 Bomb Synth

**Hybrid wavetable synthesizer by [Illbomb](https://illbomb.com)**

VST3 + AU · macOS Universal (arm64 / x86_64) · C++17 · JUCE 8

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

## What It Is

Bomb Synth is a professional-grade hybrid synthesizer with three independent oscillators, each backed by a **6-bank wavetable engine** with real-time waveform visualization. Each bank contains 8 morphable frames generated via additive synthesis — sweep through them with the MORPH knob and watch the oscilloscope animate live.

You can also **load your own wavetable files** (WAV or AIFF) from any source — single-cycle waveforms, Serum/Surge exports, recorded samples, anything — and they slot straight into the oscillator alongside the built-in banks.

70 factory presets across 9 categories. Dual filters (Moog Ladder + SVF), dual LFOs, amp and filter envelopes, 16-voice polyphony, fully resizable 1100×700 UI.

---

## Wavetable Banks

Each of the 3 oscillators independently selects a bank and morphs through its 8 frames:

| Bank | Color | Character |
|------|-------|-----------|
| **ANALOG** | Sky blue | Sine → Soft → Triangle → Saw (8/16/32 harmonics) → Square → Pulse 25% |
| **DIGITAL** | Purple | Hollow → Ringy → Bell → FM-like → Clipped → Prime harmonics → Bright → Max |
| **VOCAL** | Amber | Formant vowel morphing: AH → EH → EE → IH → OH → OO → UH → AA |
| **BRASS** | Orange | Progressive harmonic opening from soft to full blaring brass |
| **PLUCK** | Green | Steep rolloff (muted pluck) → gentle rolloff (resonant string) |
| **ORGAN** | Pink | Hammond drawbar presets: Flute → Jazz I/II → Gospel → Rock → Full |

Banks are generated at load time via **additive synthesis** (sum of sine harmonics).

### Loading Your Own Wavetables

Click the **↑** icon in the top-right corner of any oscillator display to open a file browser. Supports WAV and AIFF:

- **Single-cycle WAV** (e.g. Adventure Kid AKWF, Surge factory banks) — replicated across all 8 morph frames
- **Multi-frame wavetable** (e.g. Serum `.wav` exports, 256-cycle files) — sliced into 8 evenly spaced frames you can morph through
- **Any WAV/AIFF** — recorded instruments, vocals, synths, field recordings

Loaded banks appear immediately in the `< >` navigation alongside built-in banks, each assigned a unique colour. Up to 26 user banks per session.

---

## Factory Presets

70 presets organized into 9 categories, accessible from the browser at the top of the UI:

| Category | Count | Highlights |
|----------|-------|------------|
| **Bass** | 10 | Dark Saw, Acid, Reese, Pluck, 808 Sub, Growl, Wobble, FM, Distorted, Brass |
| **Lead** | 10 | Super Saw, Screaming, Moog, Digital Stab, Vocal, Brass, Pluck, Saw Stack, Wide, FM |
| **Keys** | 8 | Electric Piano, Rhodes, Vibraphone, Clavinet, Wurlitzer, Bell Keys, Crystal, Marimba |
| **Pads** | 8 | String, Choir, Space, Warm, Brass, Organ, Glass, Dream |
| **Organ** | 6 | Full, Jazz, Rock, Gospel, Pipe, Drawbar Flute |
| **Pluck** | 6 | Classic, Harp, Nylon Guitar, Koto, Banjo, Bass Guitar |
| **Arp** | 8 | Acid, Digital, Crystal, Brass, Pluck, Vocal, Glassy, Organ |
| **Drums** | 9 | Kick, Snare, Hi-Hat (open+closed), Tom, Clap, Cowbell, Rimshot, Perc Block |
| **Special** | 5 | Theremin, Alien Voice, Formant Sweep, Drone, Harmonic Series, Goth Bells, Sweep Pad |

`<` / `>` arrows navigate through all 70 presets globally.

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
| WAVEFORM | 6+ banks | Bank selector — `< >` arrows or `↑` to load a custom WAV |
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
| DRIVE | 1–8× | Pre-filter saturation |
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
| DEPTH | 0–1 | Modulation depth |
| PHASE *(LFO 1)* | 0–1 | LFO start phase offset |

### Master
| Parameter | Range | Description |
|-----------|-------|-------------|
| VOLUME | 0–1 | Master output gain |
| GLIDE | 0–5000 ms | Portamento time |

---

## UI

- **Size:** 1100 × 700 px (resizable)
- **Header bar:** title + preset browser (category + name dropdowns + `<` `>` navigation)
- **OSCILLATORS section:** 3 strips — each with live waveform oscilloscope + bank nav + `↑` load button + 7 labeled knobs
- **FILTER section:** type dropdown + 4 knobs
- **AMP ENVELOPE / FILTER ENVELOPE:** ADSR knobs, color-coded green / amber
- **LFO 1 / LFO 2:** shape dropdown + rate/depth/phase knobs
- **MASTER:** volume + glide

The wavetable display updates in real-time as you turn the MORPH knob, rendering the interpolated frame with a 3-pass glow effect using the same algorithm as the audio engine.

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

See **[SPEC.md](SPEC.md)** for the full technical specification.

### Key Classes

| Class | Role |
|-------|------|
| `SynthEngine` | 16-voice polyphony, voice allocation, MIDI dispatch |
| `Voice` | Per-voice: 3× WavetableOscillator + LadderFilter + SVFFilter + 2× ADSR |
| `WavetableBankLibrary` | Singleton: built-in banks + user-loaded banks, thread-safe addUserBank() |
| `WavetableOscillator` | Phase accumulator + bilinear frame interpolation |
| `AnalogOscillator` | PolyBLEP bandlimited (kept for legacy/aux use) |
| `LadderFilter` | Huovilainen 4-pole Moog, 2× oversampled nonlinear stages |
| `SVFFilter` | State-variable LP/HP/BP/Notch |
| `ADSR` | Exponential attack/decay/release with curve shaping |
| `LFO` | 7 shapes + tempo sync + fade-in |
| `ModMatrix` | Block-rate source→destination routing |
| `WavetableDisplay` | JUCE Component: oscilloscope viz, bank nav, load button |
| `PresetManager` | 70 factory presets, category/name lookup, APVTS apply |

---

## Changelog

### v0.3.0 (March 2026)
- **User wavetable loading** — `↑` button on each oscillator display opens a file browser; supports WAV and AIFF; single-cycle and multi-frame formats; up to 26 user banks per session
- **70 factory presets** — Bass, Lead, Keys, Pads, Organ, Pluck, Arp, Drums, Special
- **Preset browser** — category + name dropdowns + `< >` global navigation in header bar
- **Bug fix** — oscilloscope display was hidden by z-order issue (section panel painted on top of oscillator strips)

### v0.2.0 (March 2026)
- **Wavetable engine:** 6 banks × 8 morphable frames, additive synthesis generation at startup
- **WavetableDisplay:** live oscilloscope per oscillator — glow rendering, frame dots, bank navigation
- **Full UI labels:** WAVEFORM header, OSC badge redesign, stacked combo labels
- **UI expanded:** 1100×700, resizable

### v0.1.0 (March 2026)
- Core architecture: `SynthEngine`, 16-voice polyphony, voice stealing
- Full DSP: `AnalogOscillator` (PolyBLEP), `LadderFilter` (Huovilainen), `SVFFilter`, `ADSR`, `LFO`, `ModMatrix`
- Initial labeled UI, VST3 + AU, macOS Universal, GitHub Actions CI

---

## License

MIT — see [LICENSE](LICENSE)

---

*Built with [JUCE](https://juce.com) · Made by [Illbomb](https://illbomb.com)*
