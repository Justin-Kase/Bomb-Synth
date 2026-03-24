# Bomb Synth 💣

Professional hybrid VST3/AU synthesizer by **Illbomb** — analog, wavetable, and granular engines with a full modulation system, dual filters, 8-slot effects rack, and polyrhythmic sequencer.

## Download

**Pre-built binaries:**
- [macOS (Universal)](../../releases/latest/download/BombSynth-macOS.zip)
- [Windows (x64)](../../releases/latest/download/BombSynth-Windows.zip)
- [Linux (x64)](../../releases/latest/download/BombSynth-Linux.zip)

### Installation
- **macOS:** Extract → copy `Bomb Synth.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
- **Windows:** Extract → copy `Bomb Synth.vst3` to `C:\Program Files\Common Files\VST3\`
- **Linux:** Extract → copy `Bomb Synth.vst3` to `~/.vst3/`

## Architecture

See [SPEC.md](SPEC.md) for the full technical specification including signal flow, class architecture, DSP design notes, and CPU optimization strategy.

### Sound Engines
| Engine | Description |
|--------|-------------|
| **Analog** | PolyBLEP bandlimited. Waveforms: Sine, Saw, Square, Triangle, S+T. FM/PM, drift, 8-voice unison |
| **Wavetable** | Frame morphing. Spectral warp modes: Phase Bend, Smear, Mirror |
| **Granular** | Position, density, size, spray, pitch scatter, time stretch |

### Modulation
- 8× LFOs (drawable shapes, tempo-sync, fade-in)
- 8× Multi-stage DAHDSR envelopes with curves
- 8× Macro controls
- Random generators: S&H, Chaos, Turing machine
- Drag-and-drop ModMatrix with unlimited routings

### Filters
- Moog Ladder (LP12/24, HP12/24, BP) with 2× oversampling
- State Variable Filter (LP/HP/BP/Notch)
- Formant, Comb — serial/parallel routing

### Effects (8 slots)
Reverb · Delay · Distortion · Chorus · Phaser · Flanger · Bitcrusher · Compressor

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Prerequisites
- CMake 3.21+
- Xcode CLI (macOS) / MSVC (Windows) / GCC or Clang (Linux)
- Linux: `sudo apt-get install libasound2-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxcursor-dev libxrandr-dev libfreetype6-dev libwebkit2gtk-4.1-dev`

## Changelog

### v0.1.0 (March 2026)
- Core architecture: SynthEngine, 16-voice polyphony, voice stealing
- AnalogOscillator: PolyBLEP saw/square/triangle/sine/SawTri + FM + drift + 8-voice unison
- WavetableOscillator: frame morphing stub
- GranularEngine: grain scheduler with Hann-windowed grains
- LadderFilter: Huovilainen 4-pole Moog with 2× oversampling
- SVFFilter: state-variable LP/HP/BP/Notch
- ADSR: attack/decay/sustain/release with curve control
- LFO: 7 shapes + drawable + tempo-sync + fade-in
- ModMatrix: source/destination routing with amounts
- Initial UI: envelope + filter + LFO + master knobs

## Tech Stack
- JUCE 8.x (CMake FetchContent)
- C++17
- VST3 + AU

## License
MIT
