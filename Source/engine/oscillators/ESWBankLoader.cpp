/**
 * ESWBankLoader.cpp
 *
 * Scans the Bomb Synth wavetables folder at runtime and loads each subfolder
 * as a named bank. Each WAV file in a subfolder = one frame in that bank.
 *
 * Wavetable search paths (in priority order):
 *   macOS : ~/Library/Application Support/Bomb Synth/Wavetables/
 *   Windows: %APPDATA%\Bomb Synth\Wavetables\
 *   Linux  : ~/.local/share/Bomb Synth/Wavetables/
 */
#include "WavetableBank.h"
#include <juce_audio_formats/juce_audio_formats.h>

// Colours for named category folders (matched by lowercase prefix)
static uint32_t categoryColour(const juce::String& folderName)
{
    juce::String lo = folderName.toLowerCase();
    if (lo.startsWith("analog"))    return 0xFFFF7043u;
    if (lo.startsWith("basic"))     return 0xFF4FC3F7u;
    if (lo.startsWith("chill"))     return 0xFF80CBC4u;
    if (lo.startsWith("digital"))   return 0xFFCE93D8u;
    if (lo.startsWith("fm"))        return 0xFFFFD740u;
    if (lo.startsWith("growl"))     return 0xFFEF5350u;
    if (lo.startsWith("instr"))     return 0xFF66BB6Au;
    if (lo.startsWith("metal"))     return 0xFFB0BEC5u;
    if (lo.startsWith("spec"))      return 0xFF40C4FFu;
    if (lo.startsWith("synth"))     return 0xFFFF80ABu;
    if (lo.startsWith("vocal"))     return 0xFFFFA726u;
    // Fallback: cycle through palette
    static const uint32_t kPalette[] = {
        0xFFE040FBu, 0xFF00BCD4u, 0xFF8BC34Au, 0xFFFF5722u,
        0xFF607D8Bu, 0xFFFF4081u, 0xFF69F0AEu, 0xFFFF6D00u,
    };
    static int idx = 0;
    return kPalette[(idx++) % 8];
}

// Resample raw samples to kWTSize using linear interpolation + normalise
static WavetableFrame resampleToFrame(const float* src, int srcLen)
{
    WavetableFrame frame;
    float peak = 0.f;

    for (int i = 0; i < kWTSize; ++i) {
        float pos = (float)i * srcLen / kWTSize;
        int   i0  = (int)pos % srcLen;
        int   i1  = (i0 + 1) % srcLen;
        float t   = pos - (float)(int)pos;
        frame.data[i] = src[i0] * (1.f - t) + src[i1] * t;
        peak = std::max(peak, std::abs(frame.data[i]));
    }
    if (peak > 1e-6f)
        for (auto& s : frame.data) s /= peak;

    return frame;
}

void WavetableBankLibrary::loadESWBanks()
{
    // ── Determine wavetables folder ──────────────────────────────────────────
    juce::File wtRoot;

#if JUCE_MAC
    wtRoot = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Bomb Synth/Wavetables");
#elif JUCE_WINDOWS
    wtRoot = juce::File::getSpecialLocation(juce::File::windowsLocalAppData)
                .getChildFile("Bomb Synth\\Wavetables");
#else
    wtRoot = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                .getChildFile(".local/share/Bomb Synth/Wavetables");
#endif

    if (!wtRoot.isDirectory()) return;

    juce::AudioFormatManager formatMgr;
    formatMgr.registerBasicFormats();   // WAV + AIFF

    // ── Iterate category subfolders (sorted) ─────────────────────────────────
    juce::Array<juce::File> subfolders;
    wtRoot.findChildFiles(subfolders, juce::File::findDirectories, false);
    subfolders.sort();

    for (const auto& folder : subfolders)
    {
        juce::Array<juce::File> wavFiles;
        folder.findChildFiles(wavFiles, juce::File::findFiles, false, "*.wav;*.WAV;*.aif;*.aiff");
        wavFiles.sort();

        if (wavFiles.isEmpty()) continue;

        const juce::String folderName = folder.getFileName();
        const uint32_t     colour     = categoryColour(folderName);

        // Split into banks of up to 16 frames
        constexpr int kMaxFrames = 16;
        const int     numBanks   = (wavFiles.size() + kMaxFrames - 1) / kMaxFrames;

        for (int bankIdx = 0; bankIdx < numBanks; ++bankIdx)
        {
            const int start = bankIdx * kMaxFrames;
            const int end   = std::min(start + kMaxFrames, wavFiles.size());

            WavetableBank bank;
            bank.name        = numBanks == 1 ? folderName
                                             : (folderName + " " + juce::String(bankIdx + 1));
            bank.colourArgb  = colour;
            bank.isUser      = false;

            for (int fi = start; fi < end; ++fi)
            {
                std::unique_ptr<juce::AudioFormatReader> reader(
                    formatMgr.createReaderFor(wavFiles[fi]));

                if (!reader) continue;

                const int numSamples = (int)reader->lengthInSamples;
                if (numSamples < 8) continue;

                // Read mono (mix channels if stereo)
                juce::AudioBuffer<float> buf(1, numSamples);
                buf.clear();
                {
                    juce::AudioBuffer<float> tmp((int)reader->numChannels, numSamples);
                    reader->read(&tmp, 0, numSamples, 0, true, true);
                    for (int ch = 0; ch < (int)reader->numChannels; ++ch)
                        buf.addFrom(0, 0, tmp, ch, 0, numSamples,
                                    1.f / (float)reader->numChannels);
                }

                bank.frames.push_back(resampleToFrame(buf.getReadPointer(0), numSamples));
            }

            if (!bank.frames.empty())
                banks_.push_back(std::move(bank));
        }
    }
}
