#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>

// ─── Scale quantization tables ────────────────────────────────────────────────
namespace SeqScale {
    // Intervals from root (semitones), terminated by -1
    static constexpr int kData[][13] = {
        {0,1,2,3,4,5,6,7,8,9,10,11,-1}, // 0 Chromatic
        {0,2,4,5,7,9,11,-1},             // 1 Major
        {0,2,3,5,7,8,10,-1},             // 2 Natural Minor
        {0,2,3,5,7,9,10,-1},             // 3 Dorian
        {0,2,4,5,7,9,10,-1},             // 4 Mixolydian
        {0,2,4,7,9,-1},                  // 5 Pentatonic Maj
        {0,3,5,7,10,-1},                 // 6 Pentatonic Min
        {0,3,5,6,7,10,-1},               // 7 Blues
        {0,2,3,5,7,8,11,-1},             // 8 Harmonic Minor
        {0,2,4,6,8,10,-1},               // 9 Whole Tone
        {0,2,3,5,6,8,9,11,-1},           // 10 Diminished
        {0,1,3,5,7,8,10,-1},             // 11 Phrygian
    };
    static constexpr const char* kNames[] = {
        "Chromatic","Major","Minor","Dorian","Mixolydian",
        "Penta Maj","Penta Min","Blues","Harm Minor","Whole Tone","Diminished","Phrygian"
    };
    static constexpr int kCount = 12;

    inline int quantize(int midiNote, int rootNote, int scaleIdx) {
        if (scaleIdx <= 0) return midiNote;
        int octave = midiNote / 12;
        int pc     = midiNote % 12;
        int root   = rootNote % 12;
        // Normalise to root
        int rel = ((pc - root) + 12) % 12;
        // Find nearest scale degree
        const int* intervals = kData[scaleIdx];
        int best = 0, bestDist = 99;
        for (int i = 0; intervals[i] != -1; ++i) {
            int d = std::abs(rel - intervals[i]);
            if (d < bestDist) { bestDist = d; best = intervals[i]; }
            // Also check wrapping (e.g. rel=11 nearest to 0)
            d = std::abs(rel - (intervals[i] + 12));
            if (d < bestDist) { bestDist = d; best = intervals[i]; }
        }
        return octave * 12 + ((root + best) % 12);
    }
}

// ─── Arpeggiator ─────────────────────────────────────────────────────────────
namespace ArpMode {
    enum { Up=0, Down, UpDown, Random, AsPlayed, Count };
    static constexpr const char* kNames[] = { "Up","Down","Up/Down","Random","As Played" };
}

// ─── Step data ────────────────────────────────────────────────────────────────
struct SequencerStep {
    int   note    = 60;    // MIDI note 0-127
    float vel     = 0.8f;  // 0-1
    bool  gate    = true;
    float prob    = 1.0f;  // 0-1 probability
    float utime   = 0.0f;  // microtiming: -0.5 to +0.5 (fraction of step duration)
    float gateLen = 0.9f;  // gate length as fraction of step duration
};

// ─── Lane ────────────────────────────────────────────────────────────────────
struct SequencerLane {
    static constexpr int kMaxSteps = 16;
    std::array<SequencerStep, kMaxSteps> steps;
    int  numSteps    = 8;
    int  currentStep = 0;
    bool active      = true;
    int  octave      = 0;       // ±3 octave transpose
    int  midiChannel = 1;       // 1-16
    float swing      = 0.0f;   // 0-1 swing amount

    // Pending note-off tracking
    int    pendingNoteOff     = -1;
    double pendingNoteOffPos  = -1.0;  // in absolute samples
};

// ─── Sequencer engine ─────────────────────────────────────────────────────────
class Sequencer {
public:
    static constexpr int kMaxLanes = 4;

    // State
    std::array<SequencerLane, kMaxLanes> lanes;
    int   numActiveLanes = 2;
    int   rootNote   = 60;    // C4 root
    int   scaleIdx   = 1;     // Major
    bool  playing    = false;
    float bpm        = 120.f;
    bool  syncDAW    = true;

    // Arpeggiator
    bool arpEnabled  = false;
    int  arpMode     = ArpMode::Up;
    int  arpOctaves  = 1;
    int  arpRateDiv  = 4;     // denominator: 4=1/4, 8=1/8, 16=1/16, 32=1/32

    // ── Init ──────────────────────────────────────────────────────────────────
    Sequencer() {
        // Default patterns
        for (int l = 0; l < kMaxLanes; ++l) {
            int nSteps = (l == 0) ? 16 : (l == 1) ? 12 : 8;
            lanes[l].numSteps = nSteps;
            lanes[l].active   = (l < 2);
            lanes[l].midiChannel = l + 1;
            lanes[l].octave   = -l;  // lane 0=oct0, lane 1=oct-1, etc.
            for (int s = 0; s < nSteps; ++s) {
                auto& step = lanes[l].steps[s];
                // Spread notes across a triad-like pattern
                static const int kPattern[][16] = {
                    {60,60,63,65,67,67,70,72, 60,60,63,65,67,67,70,72},
                    {48,48,51,53, 55,55,58,60, 48,51,53,55},
                    {36,36,39,41, 55,58,60,63},
                    {72,74,76,79, 72,74,76,79},
                };
                step.note  = kPattern[l][s];
                step.gate  = (s % 2 == 0) || (s % 3 == 0);
                step.vel   = 0.65f + 0.35f * ((s % 4 == 0) ? 1.f : 0.5f);
                step.prob  = (s % 7 == 6) ? 0.5f : 1.0f;    // every 7th step is 50%
                step.utime = (s % 5 == 3) ? 0.05f : 0.0f;   // subtle push on some steps
            }
        }
    }

    void prepare(double sr) {
        sampleRate_ = sr;
        updateTiming();
    }

    void setPlaying(bool p) {
        if (p && !playing) {
            // Reset all lane positions
            for (auto& lane : lanes) {
                lane.currentStep     = 0;
                lane.pendingNoteOff  = -1;
                lane.pendingNoteOffPos = -1.0;
            }
            stepPos_ = 0.0;
        }
        playing = p;
    }

    void stop(juce::MidiBuffer& midi) {
        playing = false;
        // Send note-off for any held notes
        for (auto& lane : lanes) {
            if (lane.pendingNoteOff >= 0) {
                midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff), 0);
                lane.pendingNoteOff = -1;
            }
        }
    }

    // ── Called from processBlock ───────────────────────────────────────────────
    void processBlock(juce::MidiBuffer& midi,
                      int numSamples,
                      juce::AudioPlayHead* playHead)
    {
        // Sync BPM from DAW if available
        if (syncDAW && playHead) {
            if (auto pos = playHead->getPosition()) {
                if (pos->getBpm().hasValue())
                    bpm = (float)*pos->getBpm();
                if (!pos->getIsPlaying()) {
                    if (playing) stop(midi);
                    return;
                }
                if (!playing) setPlaying(true);
            }
        }

        if (!playing) return;
        updateTiming();

        const double stepDurSamples = samplesPerStep_;

        for (int laneIdx = 0; laneIdx < numActiveLanes; ++laneIdx) {
            auto& lane = lanes[laneIdx];
            if (!lane.active) continue;

            // Calculate effective step duration (with swing for even steps)
            double pos = stepPos_;
            double end = pos + numSamples;

            while (pos < end) {
                // Swing: even steps are slightly longer, odd steps shorter
                double swingFactor = ((lane.currentStep % 2 == 0) && lane.swing > 0.f)
                    ? (1.0 + lane.swing * 0.33)
                    : (1.0 - lane.swing * 0.33);
                double thisStepDur = stepDurSamples * swingFactor;

                // When does current step end?
                double stepEndPos = std::floor(pos / thisStepDur) * thisStepDur + thisStepDur;

                // Check if this step fires in this buffer
                double stepStartPos = stepEndPos - thisStepDur;

                if (stepStartPos >= pos && stepStartPos < end) {
                    const auto& step = lane.steps[lane.currentStep];

                    // Microtiming: offset trigger point
                    double utimeSamples = step.utime * thisStepDur;
                    double triggerPos = stepStartPos + utimeSamples;
                    int    sampleOffset = juce::jlimit(0, numSamples - 1,
                                                       (int)(triggerPos - stepPos_));

                    // Send pending note-off if it falls in this buffer
                    if (lane.pendingNoteOff >= 0 && lane.pendingNoteOffPos >= stepPos_
                        && lane.pendingNoteOffPos < stepPos_ + numSamples) {
                        int offSmp = (int)(lane.pendingNoteOffPos - stepPos_);
                        midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff),
                                      offSmp);
                        lane.pendingNoteOff = -1;
                    }

                    // Fire step if gate on and probability check passes
                    if (step.gate && rng_.nextFloat() <= step.prob) {
                        // Quantize note to scale
                        int raw  = step.note + lane.octave * 12;
                        int note = SeqScale::quantize(juce::jlimit(0, 127, raw), rootNote, scaleIdx);
                        int vel  = juce::jlimit(1, 127, (int)(step.vel * 127.f));

                        // Note-off previous note
                        if (lane.pendingNoteOff >= 0) {
                            midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff),
                                          sampleOffset);
                        }
                        midi.addEvent(juce::MidiMessage::noteOn(lane.midiChannel, note, (uint8_t)vel),
                                      sampleOffset);

                        lane.pendingNoteOff    = note;
                        lane.pendingNoteOffPos = stepPos_ + sampleOffset + step.gateLen * thisStepDur;
                    }

                    // Advance to next step
                    lane.currentStep = (lane.currentStep + 1) % lane.numSteps;
                }

                pos = stepEndPos;
            }

            // Handle note-off that falls in this buffer
            if (lane.pendingNoteOff >= 0
                && lane.pendingNoteOffPos >= stepPos_
                && lane.pendingNoteOffPos < stepPos_ + numSamples) {
                int offSmp = juce::jlimit(0, numSamples - 1,
                                          (int)(lane.pendingNoteOffPos - stepPos_));
                midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff),
                              offSmp);
                lane.pendingNoteOff = -1;
            }
        }

        stepPos_ += numSamples;
    }

    // ── Arpeggiator: track held notes ─────────────────────────────────────────
    void arpNoteOn(int note)  {
        if (std::find(heldNotes_.begin(), heldNotes_.end(), note) == heldNotes_.end())
            heldNotes_.push_back(note);
        rebuildArpSequence();
    }
    void arpNoteOff(int note) {
        heldNotes_.erase(std::remove(heldNotes_.begin(), heldNotes_.end(), note), heldNotes_.end());
        rebuildArpSequence();
    }

    // ── State ─────────────────────────────────────────────────────────────────
    int getCurrentStep(int lane) const { return lanes[lane].currentStep; }

    // ── XML ───────────────────────────────────────────────────────────────────
    juce::XmlElement* createXml() const {
        auto* xml = new juce::XmlElement("Sequencer");
        xml->setAttribute("numLanes", numActiveLanes);
        xml->setAttribute("root",     rootNote);
        xml->setAttribute("scale",    scaleIdx);
        xml->setAttribute("bpm",      bpm);
        xml->setAttribute("syncDAW",  syncDAW);
        xml->setAttribute("arpEnabled", arpEnabled);
        xml->setAttribute("arpMode",  arpMode);
        xml->setAttribute("arpOct",   arpOctaves);
        xml->setAttribute("arpRate",  arpRateDiv);

        for (int l = 0; l < kMaxLanes; ++l) {
            auto* lx = new juce::XmlElement("Lane");
            lx->setAttribute("idx",      l);
            lx->setAttribute("numSteps", lanes[l].numSteps);
            lx->setAttribute("active",   lanes[l].active);
            lx->setAttribute("octave",   lanes[l].octave);
            lx->setAttribute("swing",    lanes[l].swing);
            for (int s = 0; s < SequencerLane::kMaxSteps; ++s) {
                auto* sx = new juce::XmlElement("Step");
                const auto& step = lanes[l].steps[s];
                sx->setAttribute("note",  step.note);
                sx->setAttribute("vel",   step.vel);
                sx->setAttribute("gate",  step.gate);
                sx->setAttribute("prob",  step.prob);
                sx->setAttribute("utime", step.utime);
                sx->setAttribute("glen",  step.gateLen);
                lx->addChildElement(sx);
            }
            xml->addChildElement(lx);
        }
        return xml;
    }

    void loadFromXml(const juce::XmlElement* xml) {
        if (!xml) return;
        numActiveLanes = xml->getIntAttribute("numLanes", 2);
        rootNote   = xml->getIntAttribute("root",    60);
        scaleIdx   = xml->getIntAttribute("scale",    1);
        bpm        = (float)xml->getDoubleAttribute("bpm", 120.0);
        syncDAW    = xml->getBoolAttribute("syncDAW", true);
        arpEnabled = xml->getBoolAttribute("arpEnabled", false);
        arpMode    = xml->getIntAttribute("arpMode",  0);
        arpOctaves = xml->getIntAttribute("arpOct",   1);
        arpRateDiv = xml->getIntAttribute("arpRate",  16);

        for (auto* lx : xml->getChildWithTagNameIterator("Lane")) {
            int l = lx->getIntAttribute("idx", 0);
            if (l < 0 || l >= kMaxLanes) continue;
            lanes[l].numSteps = lx->getIntAttribute("numSteps", 8);
            lanes[l].active   = lx->getBoolAttribute("active", l < 2);
            lanes[l].octave   = lx->getIntAttribute("octave", 0);
            lanes[l].swing    = (float)lx->getDoubleAttribute("swing", 0.0);
            int s = 0;
            for (auto* sx : lx->getChildWithTagNameIterator("Step")) {
                if (s >= SequencerLane::kMaxSteps) break;
                auto& step  = lanes[l].steps[s++];
                step.note   = sx->getIntAttribute("note", 60);
                step.vel    = (float)sx->getDoubleAttribute("vel",  0.8);
                step.gate   = sx->getBoolAttribute("gate",  true);
                step.prob   = (float)sx->getDoubleAttribute("prob", 1.0);
                step.utime  = (float)sx->getDoubleAttribute("utime",0.0);
                step.gateLen= (float)sx->getDoubleAttribute("glen", 0.9);
            }
        }
    }

private:
    double sampleRate_    = 44100.0;
    double samplesPerStep_= 0.0;
    double stepPos_       = 0.0;   // absolute sample counter
    juce::Random rng_;
    std::vector<int> heldNotes_;
    std::vector<int> arpSequence_;
    int arpStep_ = 0;

    void updateTiming() {
        // 1/16 note = 1 beat / 4
        samplesPerStep_ = (60.0 / bpm) * sampleRate_ / 4.0;
    }

    void rebuildArpSequence() {
        arpSequence_.clear();
        if (heldNotes_.empty()) return;

        auto notes = heldNotes_;
        std::sort(notes.begin(), notes.end());

        switch (arpMode) {
            case ArpMode::Up:     arpSequence_ = notes; break;
            case ArpMode::Down:   arpSequence_ = { notes.rbegin(), notes.rend() }; break;
            case ArpMode::UpDown:
                arpSequence_ = notes;
                if (notes.size() > 2)
                    for (int i = (int)notes.size() - 2; i > 0; --i)
                        arpSequence_.push_back(notes[i]);
                break;
            case ArpMode::Random:
                arpSequence_ = notes; break;
            case ArpMode::AsPlayed:
                arpSequence_ = heldNotes_; break;
        }

        // Expand across arpOctaves
        auto base = arpSequence_;
        for (int o = 1; o < arpOctaves; ++o)
            for (int n : base)
                arpSequence_.push_back(n + o * 12);

        if (arpMode == ArpMode::Random)
            for (int i = (int)arpSequence_.size() - 1; i > 0; --i)
                std::swap(arpSequence_[i], arpSequence_[rng_.nextInt(i + 1)]);

        if (arpStep_ >= (int)arpSequence_.size()) arpStep_ = 0;
    }
};
