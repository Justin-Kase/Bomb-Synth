# Bomb Synth — Full Technical Specification v0.1.0

**Publisher:** Illbomb  
**Format:** VST3 / AU  
**Engine:** JUCE 8 + C++17  
**Polyphony:** Up to 32 voices (auto-steal)  
**Sample Rates:** 44.1 kHz – 192 kHz

---

## 1. Feature Specification

### 1.1 Sound Engines (per voice, selectable/layerable)

| Engine | Description |
|--------|-------------|
| **Analog** | PolyBLEP bandlimited oscillator. Waveforms: Sine, Saw, Square, Triangle, S+T. Pulse width, FM/PM, oscillator drift/detune, unison up to 8 voices |
| **Wavetable** | Frame-based wavetable playback with morphing. Import .wav/.wt. Spectral warp modes: Phase Distortion, Bend, Smear, Mirror |
| **Granular** | Sample playback with position, density, size, pitch scatter, time stretch, spray |

All three engines can run simultaneously and are mixed at voice level.

### 1.2 Modulation System

| Source | Notes |
|--------|-------|
| **LFO 1–8** | Drawable shape editor. Rates: 0.01 Hz – 100 Hz or tempo-sync. Modes: Free, Retrig, OneShot, S&H |
| **Envelope 1–8** | Multi-stage DAHDSR with per-stage curves (lin/exp/custom). Loop mode |
| **Macro 1–8** | User-named knobs, each maps to ≤32 targets with individual amounts |
| **Random / Turing** | S&H, Chaos (logistic map), Turing machine (probabilistic bit-shift) |
| **MIDI** | Velocity, Aftertouch (poly + channel), Pitch Bend, CC 1–127 |
| **ModMatrix** | Any source → any parameter. Unlimited slots. Bipolar amounts |

### 1.3 Filter Section

| Filter | Model |
|--------|-------|
| **Ladder** | Huovilainen 4-pole Moog. Modes: LP12/24, HP12/24, BP |
| **OTA** | CEM3320-style. Modes: LP/HP/BP 12/24 |
| **SEM** | Oberheim SEM 2-pole state-variable |
| **Formant** | Pair of bandpass peaks tracking vowel formants (A E I O U) |
| **Comb** | Feedback comb, positive/negative, resonance to self-oscillation |

Dual filter slots — routing: Serial, Parallel, Split (L/R).  
Filter FM input from oscillators. Drive (pre-filter saturation).

### 1.4 Effects Rack (8 slots, any order)

| Effect | Key Parameters |
|--------|---------------|
| Reverb | Size, Damp, Width, Pre-delay, Mod |
| Delay | Time (sync), Feedback, Spread, Filter, Ping-pong |
| Distortion | Drive, Mode (Soft/Hard/Fold/Wrap/Tube), Tone |
| Chorus | Voices, Rate, Depth, Mix, Spread |
| Phaser | Stages, Rate, Feedback, Mix |
| Flanger | Rate, Depth, Feedback, Delay offset |
| Bitcrusher | Bit depth, Sample rate reduction, Dither |
| Compressor | Threshold, Ratio, Attack, Release, Makeup, Sidechain |

### 1.5 Sequencer + Arpeggiator

**Sequencer:**
- 1–32 steps per lane, up to 4 polyrhythmic lanes
- Per-step: pitch, velocity, gate, probability (0–100 %), microtiming offset
- Scale quantization (all 10 Bomb Synth scales)
- Direction: Forward, Reverse, Bounce, Random, Shuffle

**Arpeggiator:**
- Modes: Up, Down, Up/Down, Order, Chord, Random
- Octave range 1–4, gate, swing, rate (tempo-sync)

### 1.6 UI/UX

- Window size: 1400 × 900 (resizable 50–200 %)
- Tabs: SYNTH · MODULATION · EFFECTS · SEQUENCER · SETTINGS
- Real-time visualizers: envelope shape, LFO waveform, wavetable frame, filter freq response
- Drag-and-drop modulation: drag any source to any knob → mod ring appears
- Color-coded mod rings on every parameter
- GPU-accelerated rendering via JUCE OpenGL

---

## 2. Signal Flow

```
MIDI In
  │
  ▼
Voice Manager (steal, legato, glide)
  │
  ├─► Voice × N
  │     │
  │     ├─ Osc 1 (Analog / Wavetable / Granular)
  │     ├─ Osc 2 (Analog / Wavetable / Granular)
  │     ├─ Osc 3 (Analog / Wavetable / Granular)
  │     │     └─ Ring Mod / FM between oscs
  │     │
  │     ├─ Pre-filter Drive / Saturation
  │     │
  │     ├─ Filter A ──┬── Serial ──► Filter B ──►
  │     │             ├── Parallel ──────────────►  Sum
  │     │             └── Split (L/R) ───────────►
  │     │
  │     └─ Amp Envelope → Pan → VCA
  │
  ▼
Voice Sum (stereo)
  │
  ▼
Effects Rack (global, 8 slots)
  │
  ▼
Master Volume / Limiter
  │
  ▼
Audio Out (stereo)

Modulation Bus (parallel to all):
  LFO 1-8 ──┐
  ENV 1-8 ──┤
  Macro 1-8 ┤──► ModMatrix ──► Parameter targets
  Rand/Tur ─┤
  MIDI CC ──┘
```

---

## 3. Class Architecture

### Core Plugin

```cpp
class BombSynthAudioProcessor    : public juce::AudioProcessor
class BombSynthAudioProcessorEditor : public juce::AudioProcessorEditor
class BombSynthLookAndFeel       : public juce::LookAndFeel_V4
```

### Engine

```cpp
class SynthEngine       // Voice manager: note-on/off, steal, render
class Voice             // Single polyphonic voice; owns oscs + filters + envelopes

// Oscillators
class OscillatorBase    // Pure interface: prepare(), render(block)
class AnalogOscillator  : public OscillatorBase   // PolyBLEP, FM, drift
class WavetableOscillator : public OscillatorBase  // Frame morphing, spectral warp
class GranularEngine    : public OscillatorBase    // Grain scheduler, sample buffer

// Filters
class FilterBase        // Interface: process(block), setCutoff/Resonance
class LadderFilter      : public FilterBase   // Huovilainen 4-pole Moog
class SVFFilter         : public FilterBase   // State-variable (SEM/OTA modes)
class FormantFilter     : public FilterBase   // Vowel formant pair
class CombFilter        : public FilterBase   // Comb with feedback

class DualFilterSection // Holds FilterA + FilterB, routing enum

// Envelopes
class ADSR              // Classic 4-stage with curves
class MultiStageEnvelope // DAHDSR + loop, per-stage bezier curves
```

### Modulation

```cpp
class ModMatrix         // Source/dest routing table; apply() per block
class ModSource         // Base: getValues(numSamples) → float*
class LFO       : public ModSource   // Drawable LFO, tempo-sync
class EnvelopeMod : public ModSource // Wraps MultiStageEnvelope
class MacroControl : public ModSource
class RandomGen  : public ModSource  // S&H, chaos, Turing machine
class ModConnection { ModSource* src; ParameterID dest; float amount; bool bipolar; }
```

### Effects

```cpp
class EffectBase        // process(AudioBuffer), setParameter(id, val)
class EffectsChain      // Ordered rack of 8 EffectBase slots
class ReverbEffect      : public EffectBase
class DelayEffect       : public EffectBase
class DistortionEffect  : public EffectBase
class ChorusEffect      : public EffectBase
class PhaserEffect      : public EffectBase
class FlangerEffect     : public EffectBase
class BitcrusherEffect  : public EffectBase
class CompressorEffect  : public EffectBase
```

### Sequencer

```cpp
class StepSequencer     // 4 polyrhythmic lanes, per-step params
class Arpeggiator       // Mode, octave, swing, gate
class SequencerClock    // Derives ticks from host or internal BPM
```

### UI

```cpp
class MainPanel         : public juce::Component  // Tab host
class SynthTab          : public juce::Component  // Oscs + filters
class ModulationTab     : public juce::Component  // LFOs, envs, macros, matrix
class EffectsTab        : public juce::Component  // Rack
class SequencerTab      : public juce::Component
class OscillatorPanel   : public juce::Component
class FilterPanel       : public juce::Component
class ModMatrixPanel    : public juce::Component
class EnvelopeVisualizer : public juce::Component  // Bezier envelope display
class LFOVisualizer      : public juce::Component  // Drawable LFO
class WavetableVisualizer : public juce::Component // 3D frame display
class FilterVisualizer   : public juce::Component  // Freq response curve
```

---

## 4. DSP Design Notes

### 4.1 Analog Oscillator (PolyBLEP)

Bandlimited synthesis using polynomial bandlimited step (PolyBLEP) for saw and square waves:

```
saw(t)   = 2t - 1   + polyBLEP(t, dt)
square(t) = sign(saw(t<pw)) - polyBLEP(t,dt) + polyBLEP(fmod(t+1-pw,1),dt)
```

Oscillator drift: per-voice random walk on frequency ± 0.5 cents, smoothed at 10 Hz.  
FM: operator → carrier with FM index parameter. Linear FM preserves harmonic series.  
Unison: N voices detune-spread ± detune semitones, panned by spread amount.

### 4.2 Wavetable Engine

Wavetable: N frames × 2048 samples (band-limited per octave using mipmapping).  
Morphing: linear interpolation between adjacent frames, position driven by mod.  
Spectral warp: per-frame FFT → manipulate phase/magnitude → IFFT → output.

### 4.3 Granular Engine

Grain scheduler: Poisson process with density (grains/sec) + jitter.  
Per grain: position in sample buffer, size, pitch transpose, envelope (Hann window), pan.  
Time stretch: independent position and pitch control via grain playback rate vs. playhead speed.

### 4.4 Ladder Filter (Huovilainen Model)

```
// 4 one-pole stages with nonlinear tanh saturation
// Feedback around all 4 stages implements resonance
stage[n] = stage[n-1] + dt * (tanh(in - resonance * fb) - tanh(stage[n]))
output   = stage[3]
```

Cutoff range: 20 Hz – 20 kHz. Resonance self-oscillates at ~1.0.  
Oversampled 2× internally to reduce aliasing from nonlinear stages.

### 4.5 Multi-Stage Envelope

Stages: Delay, Attack, Hold, Decay, Sustain (level), Release.  
Per-stage curve: linear (0), exponential (>0), logarithmic (<0), or bezier control points.  
Time range: 0.1 ms – 30 s.  
Loop mode: loop between Attack and Release with optional ping-pong.

### 4.6 LFO

Waveforms: Sine, Triangle, Saw, Reverse Saw, Square, S&H, Smooth Random.  
Drawable mode: 512-point user table with tension control per segment.  
Tempo-sync: 1/128T – 8 bars. Phase offset per voice in poly mode.  
Fade-in: 0–30 s ramp on note start.

### 4.7 Modulation Matrix

Pull-based: each audio block, ModMatrix calls getValues() on all active sources, multiplies by amount, adds to target parameter.  
Targets are normalised 0-1 values; ModMatrix adds ±amount to pre-clamped target.  
MIDI sources (velocity, aftertouch) are per-voice; LFO/ENV can be per-voice or global.

---

## 5. UI Layout Wireframe

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  💣 BOMB SYNTH                          PRESET: Init ▼   [SAVE] [LOAD]  🔒  │
├──────┬───────────────────────────────────────────────────────────────────────┤
│ SYNTH│ MODULATION │ EFFECTS │ SEQUENCER │ SETTINGS                           │
├──────┴────────────────────────────────────────┬───────────────────────────────┤
│  OSC 1        OSC 2        OSC 3              │  ENV 1    ╭─────────────╮    │
│ [Analog ▼]  [Wavetable▼] [Granular▼]         │           │   ╱╲        │    │
│ ┌────────┐  ┌──────────┐  ┌─────────┐        │      A D S R            │    │
│ │  ∿∿∿   │  │ ≋≋≋≋≋≋≋  │  │ ·:·:·:· │        │           ╰─────────────╯    │
│ │ TUNE   │  │ POSITION │  │ DENSITY │        │  LFO 1   ~~~  RATE  DEPTH    │
│ │ FM     │  │ MORPH    │  │ SIZE    │        │  LFO 2   ~~~                  │
│ └────────┘  └──────────┘  └─────────┘        │  MACRO   [1][2][3][4]        │
├───────────────────────────────────────────────┤                               │
│  FILTER A              FILTER B              │  MOD MATRIX                  │
│  [Ladder ▼] CUT RES    [SEM ▼]  CUT RES      │  SRC ──────────────► DEST   │
│  ────────── ○   ○      ──────── ○   ○        │  [LFO1][0.5][Cutoff A]  [-]  │
│  DRIVE [○]  [Serial ▼]                        │  [ENV1][1.0][Amp]       [-]  │
└───────────────────────────────────────────────┴───────────────────────────────┘
```

---

## 6. Preset System

- Presets stored as XML via JUCE `ValueTree` serialisation
- Preset browser: category tags, search, favorites
- Factory bank: 128 presets (Init + categories: Bass, Lead, Pad, Pluck, FX, Keys)
- User bank: unlimited, stored in `~/Library/Application Support/Illbomb/BombSynth/`
- A/B comparison: snapshot current state into slot A or B, toggle
- Randomize: full random + "mutate" (±20 % variation from current)

---

## 7. CPU Optimization Strategy

| Strategy | Detail |
|----------|--------|
| **SIMD** | Use `juce::FloatVectorOperations` and manual SSE2/NEON for oscillator/filter inner loops |
| **Block processing** | All DSP processes 64/128-sample blocks; no per-sample branching |
| **Voice culling** | Voices below –80 dB RMS for >50 ms are released; steal oldest-lowest |
| **Oversampling** | Only filter nonlinear stages oversample (2×); oscillators use PolyBLEP |
| **Wavetable mip-mapping** | Pre-computed band-limited frames per octave — zero runtime AA cost |
| **Mod matrix batching** | ModMatrix applies all mod in one pass per block; no per-sample routing |
| **Effects lazy bypass** | Effects with wet=0 are short-circuited (buffer copy skipped) |
| **Thread model** | Audio thread: DSP only. UI thread: repaints only. Shared: lock-free ring buffer |

---

## 8. Core DSP Code Stubs

See `Source/engine/` for implemented stubs. Each file compiles standalone.  
Implemented in v0.1.0: AnalogOscillator (PolyBLEP), LadderFilter, ADSR, LFO, ModMatrix.  
Wavetable, Granular, full Effects in v0.2.0.
