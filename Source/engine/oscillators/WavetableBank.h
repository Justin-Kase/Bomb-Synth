#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>
#include <functional>
#include <cstdint>

// ── Constants ─────────────────────────────────────────────────────────────────
static constexpr int kWTSize   = 2048;
static constexpr int kWTFrames = 8;
static constexpr int kWTBanks  = 6;

// ── WavetableFrame ─────────────────────────────────────────────────────────────
struct WavetableFrame {
    std::array<float, kWTSize> data {};

    inline float getSample(float phase01) const noexcept {
        float idx = phase01 * kWTSize;
        int   i0  = (int)idx & (kWTSize - 1);
        int   i1  = (i0 + 1) & (kWTSize - 1);
        float t   = idx - (float)(int)idx;
        return data[i0] * (1.f - t) + data[i1] * t;
    }
};

// ── WavetableBank ──────────────────────────────────────────────────────────────
struct WavetableBank {
    juce::String  name;
    uint32_t      colourArgb { 0xFF4FC3F7 };   // ARGB — avoids juce_graphics dep
    bool          isUser     { false };
    juce::String  filePath;                     // set for user-loaded banks
    std::array<WavetableFrame, kWTFrames> frames;

    inline float getSample(float phase01, float morph01) const noexcept {
        float fpos = juce::jlimit(0.f, (float)(kWTFrames - 1), morph01 * (kWTFrames - 1));
        int   f0   = (int)fpos;
        int   f1   = juce::jmin(f0 + 1, kWTFrames - 1);
        float t    = fpos - f0;
        return frames[f0].getSample(phase01) * (1.f - t)
             + frames[f1].getSample(phase01) * t;
    }
};

// ── WavetableBankLibrary (singleton, generated at startup) ─────────────────────
class WavetableBankLibrary {
public:
    static WavetableBankLibrary& get() {
        static WavetableBankLibrary inst;
        return inst;
    }

    int numBanks()        const { return (int)banks_.size(); }
    int numBuiltinBanks() const { return kWTBanks; }

    const WavetableBank& bank(int idx) const {
        return banks_[juce::jlimit(0, (int)banks_.size() - 1, idx)];
    }

    // ── Load a wavetable from raw mono float samples ───────────────────────────
    //  Slices totalSamples into kWTFrames equal segments, resampling each to
    //  kWTSize via linear interpolation.  Returns the new bank index, or -1 on
    //  error.  Thread-safe: banks_ is pre-reserved so no reallocation occurs.
    int addUserBank(const juce::String& name, uint32_t colourArgb,
                    const float* rawSamples, int totalSamples,
                    const juce::String& filePath = {}) {
        if (!rawSamples || totalSamples < 2) return -1;

        // Normalise source
        float peak = 0.f;
        for (int i = 0; i < totalSamples; ++i) peak = std::max(peak, std::abs(rawSamples[i]));
        const float gain = (peak > 1e-6f) ? 1.f / peak : 1.f;

        WavetableBank b;
        b.name       = name;
        b.colourArgb = colourArgb;
        b.isUser     = true;
        b.filePath   = filePath;

        for (int fr = 0; fr < kWTFrames; ++fr) {
            // Determine source segment for this frame
            int startSmp, segLen;
            if (totalSamples <= kWTSize) {
                // Single-cycle: use the whole buffer for every frame
                startSmp = 0;
                segLen   = totalSamples;
            } else {
                startSmp = (int)((long long)fr       * totalSamples / kWTFrames);
                int end  = (int)((long long)(fr + 1) * totalSamples / kWTFrames);
                segLen   = std::max(1, end - startSmp);
            }

            auto& frame = b.frames[fr];
            for (int i = 0; i < kWTSize; ++i) {
                float pos = (float)i * segLen / kWTSize;
                int   s0  = (int)pos % segLen;
                int   s1  = (s0 + 1) % segLen;
                float t   = pos - (float)(int)pos;
                frame.data[i] = gain * (rawSamples[startSmp + s0] * (1.f - t)
                                      + rawSamples[startSmp + s1] * t);
            }
        }

        // Pre-reserved — push_back won't reallocate, safe from audio thread
        juce::ScopedLock sl(addLock_);
        banks_.push_back(std::move(b));
        return (int)banks_.size() - 1;
    }

private:
    std::vector<WavetableBank> banks_;
    juce::CriticalSection     addLock_;  // guards addUserBank only

    // ── Helpers ────────────────────────────────────────────────────────────────
    static void normalize(WavetableFrame& f) {
        float mx = 0.f;
        for (float v : f.data) mx = std::max(mx, std::abs(v));
        if (mx > 1e-6f) for (float& v : f.data) v /= mx;
    }

    static WavetableFrame additive(int maxH,
                                    std::function<float(int)> ampFn,
                                    std::function<float(int)> phaseFn = nullptr) {
        WavetableFrame f;
        f.data.fill(0.f);
        constexpr float twoPi = 6.28318530718f;
        for (int i = 0; i < kWTSize; ++i) {
            float ph = twoPi * i / kWTSize;
            float s  = 0.f;
            for (int h = 1; h <= maxH; ++h) {
                float a   = ampFn(h);
                float phi = phaseFn ? phaseFn(h) : 0.f;
                s += a * std::sin(h * ph + phi);
            }
            f.data[i] = s;
        }
        normalize(f);
        return f;
    }

    static float gauss(float x, float mu, float sig) {
        float d = (x - mu) / sig;
        return std::exp(-0.5f * d * d);
    }

    // ── ANALOG  ────────────────────────────────────────────────────────────────
    static WavetableBank makeAnalog() {
        WavetableBank b;
        b.name   = "ANALOG";
        b.colourArgb = 0xFF4FC3F7u;

        // 0: pure sine
        b.frames[0] = additive(1, [](int h) { return h == 1 ? 1.f : 0.f; });

        // 1: soft (1/n²)
        b.frames[1] = additive(8, [](int h) { return 1.f / (float)(h * h); });

        // 2: triangle  (odd, 1/n², alt phase)
        b.frames[2] = additive(32, [](int h) -> float {
            return (h & 1) ? 1.f / (float)(h * h) : 0.f;
        }, [](int h) -> float {
            return ((h / 2) & 1) ? 3.14159265f : 0.f;
        });

        // 3: saw 8 harmonics
        b.frames[3] = additive(8,  [](int h) { return 1.f / h; });

        // 4: saw 16 harmonics
        b.frames[4] = additive(16, [](int h) { return 1.f / h; });

        // 5: full saw 32
        b.frames[5] = additive(32, [](int h) { return 1.f / h; });

        // 6: square  (odd, 1/n)
        b.frames[6] = additive(32, [](int h) -> float {
            return (h & 1) ? 1.f / h : 0.f;
        });

        // 7: pulse 25%
        b.frames[7] = additive(32, [](int h) -> float {
            return std::sin(h * 0.78539816f) / h;   // sin(n·π/4)/n
        });

        return b;
    }

    // ── DIGITAL ────────────────────────────────────────────────────────────────
    static WavetableBank makeDigital() {
        WavetableBank b;
        b.name   = "DIGITAL";
        b.colourArgb = 0xFFCE93D8u;

        // 0: hollow (odd only)
        b.frames[0] = additive(32, [](int h) -> float {
            return (h & 1) ? 1.f / h : 0.f;
        });
        // 1: ringy (even boosted)
        b.frames[1] = additive(32, [](int h) -> float {
            return (h & 1) ? 0.3f / h : 1.2f / h;
        });
        // 2: bell (every 3rd)
        b.frames[2] = additive(32, [](int h) -> float {
            return (h % 3 == 0) ? 1.5f / h : 0.1f / h;
        });
        // 3: FM-like (exponential)
        b.frames[3] = additive(48, [](int h) -> float {
            return std::exp(-0.12f * h);
        });
        // 4: hard clipped sim (strong low odds)
        b.frames[4] = additive(32, [](int h) -> float {
            if (!(h & 1)) return 0.f;
            return (h <= 5) ? 1.5f / h : 0.4f / h;
        });
        // 5: prime harmonics
        b.frames[5] = additive(48, [](int h) -> float {
            static const int primes[] = {1,2,3,5,7,11,13,17,19,23,29,31,37,41,43,47};
            for (int p : primes) if (p == h) return 1.f / h;
            return 0.f;
        });
        // 6: spectral tilt (boosted highs)
        b.frames[6] = additive(48, [](int h) -> float {
            return std::pow((float)h, -0.6f);
        });
        // 7: equal amplitude all
        b.frames[7] = additive(32, [](int h) -> float {
            return std::pow((float)h, -0.25f);
        });

        return b;
    }

    // ── VOCAL ──────────────────────────────────────────────────────────────────
    static WavetableBank makeVocal() {
        WavetableBank b;
        b.name   = "VOCAL";
        b.colourArgb = 0xFFFFD740u;

        // Formant data: F1, F2, F3, B1, B2, B3
        struct VF { float F1, F2, F3, B1, B2, B3; };
        static const VF vowels[kWTFrames] = {
            {800.f, 1200.f, 2500.f, 100.f, 120.f, 200.f},   // AH
            {600.f, 1700.f, 2600.f, 100.f, 120.f, 200.f},   // EH
            {270.f, 2200.f, 3100.f,  80.f, 130.f, 200.f},   // EE
            {380.f, 1900.f, 2600.f,  90.f, 120.f, 200.f},   // IH
            {570.f,  840.f, 2800.f, 100.f, 110.f, 200.f},   // OH
            {300.f,  870.f, 2900.f,  80.f, 100.f, 200.f},   // OO
            {600.f, 1000.f, 2500.f, 100.f, 120.f, 200.f},   // UH
            {650.f, 1080.f, 2650.f, 100.f, 120.f, 200.f},   // AA
        };

        for (int fi = 0; fi < kWTFrames; ++fi) {
            const VF vf = vowels[fi];
            b.frames[fi] = additive(48, [vf](int h) -> float {
                float freq = h * 100.f;          // notional 100 Hz fundamental
                float src  = 1.f / h;            // sawtooth source
                float fm   = gauss(freq, vf.F1, vf.B1)
                           + 0.7f * gauss(freq, vf.F2, vf.B2)
                           + 0.4f * gauss(freq, vf.F3, vf.B3);
                return src * (0.3f + fm * 2.5f);
            });
        }
        return b;
    }

    // ── BRASS ──────────────────────────────────────────────────────────────────
    static WavetableBank makeBrass() {
        WavetableBank b;
        b.name   = "BRASS";
        b.colourArgb = 0xFFFF9800u;

        for (int fi = 0; fi < kWTFrames; ++fi) {
            float brightness = (fi + 1) / (float)kWTFrames;
            int   maxH = 4 + (int)(brightness * 28);
            float b2   = 0.5f + brightness * 1.5f;
            float b3   = 0.4f + brightness * 1.0f;
            b.frames[fi] = additive(maxH, [b2, b3](int h) -> float {
                if (h == 1) return 1.f;
                if (h == 2) return b2 * 0.7f;
                if (h == 3) return b3 * 0.6f;
                return 0.4f / h;
            });
        }
        return b;
    }

    // ── PLUCK ──────────────────────────────────────────────────────────────────
    static WavetableBank makePluck() {
        WavetableBank b;
        b.name   = "PLUCK";
        b.colourArgb = 0xFF00E676u;

        for (int fi = 0; fi < kWTFrames; ++fi) {
            // rolloff exponent: steep (pluck) → gentle (sustained string)
            float ro = 0.25f + fi * 0.35f;
            b.frames[fi] = additive(48, [ro](int h) -> float {
                return std::pow((float)h, -ro);
            });
        }
        return b;
    }

    // ── ORGAN ──────────────────────────────────────────────────────────────────
    static WavetableBank makeOrgan() {
        WavetableBank b;
        b.name   = "ORGAN";
        b.colourArgb = 0xFFFF80ABu;

        // Hammond drawbar levels (0-8) for 8 presets
        // Drawbars: sub(0.5x), 8'(1x), 5⅓'(1.5x), 4'(2x), 2⅔'(3x), 2'(4x), 1⅗'(5x), 1⅓'(6x), 1'(8x)
        static const float harms[9]      = {0.5f, 1.f, 1.5f, 2.f, 3.f, 4.f, 5.f, 6.f, 8.f};
        static const float presets[8][9] = {
            {0,8,0,0,0,0,0,0,0},   // pure flute
            {0,8,8,0,0,0,0,0,0},   // 8' + 5⅓'
            {0,8,8,8,0,0,0,0,0},   // lows full
            {0,8,6,8,0,8,0,0,0},   // jazz I
            {0,8,6,8,6,6,0,4,0},   // jazz II
            {8,8,8,8,6,4,2,2,0},   // gospel
            {0,8,8,8,8,4,0,2,0},   // rock
            {0,8,8,8,8,8,8,8,8},   // all drawbars
        };

        constexpr float twoPi = 6.28318530718f;
        for (int fi = 0; fi < kWTFrames; ++fi) {
            WavetableFrame f;
            f.data.fill(0.f);
            for (int i = 0; i < kWTSize; ++i) {
                float ph = twoPi * i / kWTSize;
                float s  = 0.f;
                for (int d = 0; d < 9; ++d)
                    if (presets[fi][d] > 0.f)
                        s += (presets[fi][d] / 8.f) * std::sin(harms[d] * ph);
                f.data[i] = s;
            }
            normalize(f);
            b.frames[fi] = f;
        }
        return b;
    }

    // ── Constructor ────────────────────────────────────────────────────────────
    WavetableBankLibrary() {
        banks_.reserve(32);  // pre-allocate — prevents reallocation when user banks are added
        banks_.push_back(makeAnalog());
        banks_.push_back(makeDigital());
        banks_.push_back(makeVocal());
        banks_.push_back(makeBrass());
        banks_.push_back(makePluck());
        banks_.push_back(makeOrgan());
    }

    JUCE_DECLARE_NON_COPYABLE(WavetableBankLibrary)
};
