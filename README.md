# 💣 Bomb Synth

**Hybrid wavetable synthesizer by [Illbomb](https://illbomb.com)**

VST3 + AU · macOS Universal (arm64 / x86_64) · C++17 · JUCE 8

---

## Download

| Platform | Link |
|----------|------|
| macOS Universal arm64 + x86_64 (VST3 + AU) | [BombSynth-macOS.zip](https://github.com/Justin-Kase/Bomb-Synth/releases/latest/download/BombSynth-macOS.zip) |
| Windows x64 (VST3) | [BombSynth-Windows.zip](https://github.com/Justin-Kase/Bomb-Synth/releases/latest/download/BombSynth-Windows.zip) |
| Linux x64 (VST3) | [BombSynth-Linux.zip](https://github.com/Justin-Kase/Bomb-Synth/releases/latest/download/BombSynth-Linux.zip) |

**macOS:** Extract → copy `Bomb Synth.vst3` → `/Library/Audio/Plug-Ins/VST3/` · *(AU)* copy `Bomb Synth.component` → `/Library/Audio/Plug-Ins/Components/`

**Windows:** Extract → copy `Bomb Synth.vst3` → `C:\Program Files\Common Files\VST3\`

**Linux:** Extract → copy `Bomb Synth.vst3` → `~/.vst3/` or `/usr/lib/vst3/`

---

## What It Is

Bomb Synth is a professional-grade hybrid synthesizer. Three independent oscillators each backed by a **6-bank wavetable engine** with real-time waveform visualization and custom wavetable loading. A full **effects rack** (Reverb, Delay, Chorus). A **polyrhythmic step sequencer + arpeggiator** with probability, microtiming, and scale quantization. 70 factory presets. 16-voice polyphony. Fully resizable 1100×700 UI.

---

## Features at a Glance

| Module | What it does |
|--------|-------------|
| **9 Oscillator Engines** | WT · GR · Sine · Saw · SuperSaw (7-voice) · Square (w/ PW) · Triangle · SawTri · Noise — per oscillator |
| **3× Wavetable Oscillators** | 6 built-in banks × 8 morphable frames. Load your own WAV/AIFF. Phase Bend/Smear/Mirror warp. |
| **Granular Engine** | Per-oscillator WT/GR toggle. Density, size, spray, pitch scatter. Uses wavetable bank as source. |
| **Per-osc Octave** | ±3 octave spinner per oscillator, independent of TUNE semitone knob. |
| **Filter** | Moog Ladder LP/HP, SVF LP/BP, Formant (vowel A–U), Comb. Drive, resonance, live H(f) display. |
| **Effects** | Reverb, Delay, Chorus — post-synth, wet/dry per effect. |
| **Sequencer** | 4 polyrhythmic lanes, per-step probability, microtiming, scale quantization (12 scales). |
| **Arpeggiator** | Per-lane: Up/Down/UpDown/Random/AsPlayed/Converge/Diverge, rate, 1–4 octaves. **[RND]** to randomize any lane. |
| **Modulation** | 2× LFOs (7 shapes) + ModWheel → 8 routing slots → 24 targets (all osc params + filter). |
| **Presets** | 70 factory presets across 9 categories. |

---

## Wavetable Oscillators

### Built-in Banks

| Bank | Color | Character |
|------|-------|-----------|
| **ANALOG** | Sky blue | Sine → Soft → Triangle → Saw (8/16/32h) → Square → Pulse 25% |
| **DIGITAL** | Purple | Hollow → Ringy → Bell → FM-like → Clipped → Primes → Bright → Max |
| **VOCAL** | Amber | Formant vowels: AH → EH → EE → IH → OH → OO → UH → AA |
| **BRASS** | Orange | Progressive harmonic brightness: soft → full blaring brass |
| **PLUCK** | Green | Steep rolloff (muted pluck) → gentle rolloff (string) |
| **ORGAN** | Pink | Hammond drawbars: Flute → Jazz I/II → Gospel → Rock → Full |

### Loading Custom Wavetables

Click the **↑** icon in the top-right of any oscillator display. Supports WAV and AIFF:

- **Single-cycle WAV** — replicated across all 8 morph frames
- **Multi-frame WAV** (Serum/Surge format, 256-cycle files) — sliced into 8 evenly spaced frames
- **Any audio file** — recorded instruments, vocals, field recordings, anything

Up to 26 user banks per session, each with a unique colour.

---

## Sequencer & Arpeggiator

Access via the **SEQUENCER** tab.

### Step Sequencer

- **4 polyrhythmic lanes** — each lane has an independent step count (1–16). Different step counts create natural polyrhythms that loop against each other.
- **Per-step controls:**
  - **Note** — MIDI note value, quantized to the active scale
  - **Velocity** — drag vertically on any step cell to set. Fill height = velocity.
  - **Gate** — click to toggle. Off steps are dark.
  - **Probability** — 0–100%. An orange dot at the bottom shows reduced probability steps.
  - **Microtiming** — ±50% of step duration. A yellow triangle shows the push/pull offset.
  - **Gate length** — per step, as fraction of step duration
- **Per-lane controls:** step count, octave transpose (±3), swing
- **Scale quantization** — 12 scales available. All programmed notes snap to the selected scale.
- **Click a step** to select it. The detail panel shows all step parameters with `< >` spinners.

### Scale Options

Chromatic · Major · Minor · Dorian · Mixolydian · Pentatonic Maj · Pentatonic Min · Blues · Harmonic Minor · Whole Tone · Diminished · Phrygian

### Arpeggiator

- **Modes:** Up / Down / Up+Down / Random / As Played
- **Octave range:** 1–4 octaves
- **Tracks held MIDI notes** from keyboard input
- Toggle **ARP** button in the sequencer control bar

### Transport

- **DAW SYNC** — reads BPM and play state from the DAW transport (Bitwig, Ableton, etc.)
- **FREE** mode — set your own BPM with the numeric display
- Play/Stop buttons with per-lane step position reset

---

## Effects

Access via the **EFFECTS** tab. All effects are post-synth in series: Delay → Chorus → Reverb. Turn any WET knob above 0 to activate.

| Effect | Controls | Notes |
|--------|----------|-------|
| **Reverb** | Room, Damp, Width, Wet | JUCE DSP Reverb |
| **Delay** | Time (1–2000ms), Feedback, Wet | Stereo ring-buffer delay |
| **Chorus** | Rate (0.1–8 Hz), Depth, Wet | JUCE DSP Chorus |

---

## Factory Presets

70 presets in 9 categories, browser in the header bar:

| Category | Count | Examples |
|----------|-------|---------|
| **Bass** | 10 | Dark Saw, Acid, Reese, 808 Sub, Growl, Wobble, FM, Distorted |
| **Lead** | 10 | Super Saw, Screaming, Moog, Vocal, Brass, Saw Stack, Wide, FM |
| **Keys** | 8 | Electric Piano, Rhodes, Vibraphone, Clavinet, Bell Keys, Crystal |
| **Pads** | 8 | String, Choir, Space, Warm, Brass, Glass, Dream |
| **Organ** | 6 | Full, Jazz, Rock, Gospel, Pipe, Drawbar Flute |
| **Pluck** | 6 | Classic, Harp, Nylon Guitar, Koto, Banjo, Bass Guitar |
| **Arp** | 8 | Acid, Digital, Crystal, Brass, Pluck, Vocal, Glassy, Organ |
| **Drums** | 9 | Kick, Snare, Hi-Hat (open+closed), Tom, Clap, Cowbell, Rimshot |
| **Special** | 5 | Theremin, Alien Voice, Drone, Harmonic Series, Goth Bells |

---

## Signal Flow

```
MIDI keyboard ──┐
Sequencer/Arp ──┤──► SynthEngine (16 voices) ──► Delay ──► Chorus ──► Reverb ──► Out
                │         │
                │    OSC 1+2+3 (Wavetable)
                │    Ladder Filter + SVF
                │    Amp Env + Filter Env
                └    LFO 1 + LFO 2
```

---

## Parameters

### Oscillators (× 3)
| Parameter | Range | Description |
|-----------|-------|-------------|
| WAVEFORM | 6+ banks | `< >` to cycle banks, `↑` to load custom WAV/AIFF |
| MORPH | 0–1 | Sweeps through 8 frames within the bank |
| TUNE | ±24 st | Coarse pitch |
| FINE | ±100 c | Fine pitch |
| LEVEL | 0–1 | Oscillator mix level |
| FM | 0–1 | FM into next oscillator |
| UNISON | 1–8 | Unison voice count |
| DETUNE | 0–1 | Unison spread |

### Filter
| Parameter | Range | Description |
|-----------|-------|-------------|
| TYPE | 5 modes | Ladder LP24, LP12, HP24 · SVF LP, BP |
| CUTOFF | 20–20k Hz | Cutoff frequency |
| RESONANCE | 0–1 | Self-oscillation |
| DRIVE | 1–8× | Pre-filter saturation |
| ENV AMT | −1–+1 | Filter envelope depth |

### Envelopes (Amp + Filter)
ADSR with curve shaping. Live ADSR visualizer displays the current shape.

### LFOs (× 2)
7 shapes: Sine · Triangle · Saw · Rev Saw · Square · S&H · Smooth Random

### Effects
Reverb (room/damp/width/wet) · Delay (time/feedback/wet) · Chorus (rate/depth/wet)

---

## UI

Four tabs in the content area:

- **SYNTH** — Oscillators (WT/Granular toggle, warp controls), Filter (with H(f) display), Envelopes, LFOs, Master
- **EFFECTS** — Reverb, Delay, Chorus
- **MOD** — 8 routing slots: source → target → amount
- **SEQUENCER** — Step grid, lane controls, arpeggiator, transport

Header bar: title · preset browser (category + name + `< >`) · version + branding

---

## Build

```bash
# Configure (downloads JUCE via CMake FetchContent)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# VST3: build/Source/BombSynth_artefacts/Release/VST3/Bomb Synth.vst3
# AU:   build/Source/BombSynth_artefacts/Release/AU/Bomb Synth.component
```

### Prerequisites
- CMake 3.21+  ·  C++17 compiler
- macOS: `xcode-select --install`
- Windows: Visual Studio 2019+ with C++ workload
- Linux: `sudo apt-get install libasound2-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxrandr-dev libfreetype6-dev libwebkit2gtk-4.1-dev`

---

## Architecture

See **[SPEC.md](SPEC.md)** for full DSP notes, class hierarchy, and signal flow diagrams.

### Key Classes

| Class | Role |
|-------|------|
| `SynthEngine` | 16-voice polyphony, MIDI dispatch, voice stealing |
| `Voice` | 3× WavetableOscillator + LadderFilter + SVFFilter + 2× ADSR |
| `WavetableBankLibrary` | Singleton: 6 built-in banks + user-loaded banks, thread-safe |
| `WavetableOscillator` | Phase accumulator + bilinear frame interpolation |
| `LadderFilter` | Huovilainen 4-pole Moog, 2× oversampled |
| `SVFFilter` | State-variable LP/HP/BP/Notch |
| `Sequencer` | Polyrhythmic step engine, arp, scale quantization, DAW sync |
| `SequencerPanel` | Full sequencer/arp UI with step grid and detail editor |
| `WavetableDisplay` | Live oscilloscope per oscillator, bank nav, load button |
| `EnvelopeDisplay` | Animated ADSR shape visualizer |
| `PresetManager` | 70 factory presets, category browser, APVTS apply |

---

## Changelog

### v0.7.0 (March 2026)
- **9 oscillator engines** — mode button cycles: WT → GR → Sine → Saw → SuperSaw → Square → Triangle → SawTri → Noise. MORPH knob repurposed: SuperSaw = detune spread, Square = pulse width.
- **Per-oscillator octave** — `< OCT >` spinner (±3 oct) in each oscillator label column, stacks on TUNE
- **Full mod matrix (24 targets)** — LFO1/LFO2/Velocity/ModWheel → Cutoff, Res, Drive, Pitch, Amp, Osc1-3 Morph/Tune/Fine/Level/FM/Detune, LFO2 Rate
- **Per-lane arpeggiator** — each lane has independent ARP on/off, mode (7 modes incl. Converge/Diverge), rate (1/4–1/32), octave spread
- **Pattern randomize** — `[RND]` button per lane generates scale-aware random steps with accent bias, velocity curves, ghost note probabilities, microtiming humanisation

### v0.6.0 (March 2026)
- **Granular engine** — per-oscillator [WT|GR] toggle. MORPH → grain position scan. DENSITY, SIZE, SPRAY, PITCH SCATTER knobs. Uses current wavetable bank as grain source.
- **Spectral warp** — per-oscillator warp mode (None/Phase Bend/Smear/Mirror) + WARP AMT knob. WavetableDisplay shows PB/SM/MR badge.

### v0.5.0 (March 2026)
- **MODULATION tab** — 8 routing slots: LFO1/LFO2/Velocity/ModWheel → Cutoff/Pitch/Amp/Osc Morph/LFO2 Rate
- **LFO engine** — 7 shapes running at block rate, feeding mod matrix
- **FormantFilter** — vowel filter A/E/I/O/U (`filter_type=4`)
- **CombFilter** — feedback comb filter (`filter_type=5`)
- **FilterDisplay** — live H(f) frequency response curve, animated
- **Wavetable path persistence** — user banks reload automatically on next session

### v0.4.0 (March 2026)
- **Step sequencer** — 4 polyrhythmic lanes, per-step probability, microtiming, gate length
- **Arpeggiator** — Per-lane: Up/Down/UpDown/Random/AsPlayed/Converge/Diverge, rate 1/4–1/32, 1–4 octaves. **[RND]** randomizes lane pattern to current scale.
- **Scale quantization** — 12 scales, snaps all sequencer notes
- **SEQUENCER tab** added alongside SYNTH / EFFECTS

### v0.3.0 (March 2026)
- **Effects tab** — Reverb, Delay, Chorus (10 new params)
- **Live ADSR visualizers** in Amp and Filter envelope sections
- **SYNTH / EFFECTS tab bar**

### v0.2.0 (March 2026)
- **Wavetable engine** — 6 banks × 8 morphable frames, additive synthesis
- **Custom wavetable loading** — `↑` button loads any WAV/AIFF
- **70 factory presets** + preset browser in header
- **Oscilloscope display** per oscillator, 3-pass glow

### v0.1.0 (March 2026)
- Core: `SynthEngine`, 16-voice polyphony, PolyBLEP oscillators, Ladder filter, ADSR, LFOs
- VST3 + AU, macOS Universal, GitHub Actions CI

---

## License

MIT — see [LICENSE](LICENSE)

---

*Built with [JUCE](https://juce.com) · Made by [Illbomb](https://illbomb.com)*
