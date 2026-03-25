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
    enum { Up=0, Down, UpDown, Random, AsPlayed, Converge, Diverge, Count };
    static constexpr const char* kNames[] = {
        "Up","Down","Up/Down","Random","As Played","Converge","Diverge"
    };
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

    // Per-lane arpeggiator
    bool arpEnabled  = false;
    int  arpMode     = ArpMode::Up;
    int  arpRateDiv  = 16;      // 4=1/4, 8=1/8, 16=1/16, 32=1/32
    int  arpOctaves  = 1;       // 1-4 octave spread

    // Per-lane held notes & arp state (updated live)
    std::vector<int> heldNotes;
    std::vector<int> arpSequence;
    int  arpStep     = 0;

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

    // (global arp moved to per-lane)

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

            // Per-lane step duration (arp uses own rate div; seq uses global 1/16)
            const double laneStepDur = lane.arpEnabled ? arpStepDur(lane) : stepDurSamples;

            double pos = stepPos_;
            double end = pos + numSamples;

            while (pos < end) {
                double swingFactor = ((lane.currentStep % 2 == 0) && lane.swing > 0.f)
                    ? (1.0 + lane.swing * 0.33)
                    : (1.0 - lane.swing * 0.33);
                double thisStepDur = laneStepDur * swingFactor;
                double stepEndPos  = std::floor(pos / thisStepDur) * thisStepDur + thisStepDur;
                double stepStartPos= stepEndPos - thisStepDur;

                if (stepStartPos >= pos && stepStartPos < end) {
                    // Send pending note-off if it falls in this buffer
                    if (lane.pendingNoteOff >= 0 && lane.pendingNoteOffPos >= stepPos_
                        && lane.pendingNoteOffPos < stepPos_ + numSamples) {
                        int offSmp = juce::jlimit(0, numSamples - 1, (int)(lane.pendingNoteOffPos - stepPos_));
                        midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff), offSmp);
                        lane.pendingNoteOff = -1;
                    }

                    if (lane.arpEnabled) {
                        // ── ARP mode: fire next arp note ──────────────────────
                        if (!lane.arpSequence.empty()) {
                            int sampleOffset = juce::jlimit(0, numSamples - 1, (int)(stepStartPos - stepPos_));
                            int rawNote = lane.arpSequence[lane.arpStep % (int)lane.arpSequence.size()];
                            int note    = SeqScale::quantize(juce::jlimit(0, 127, rawNote), rootNote, scaleIdx);
                            int vel     = 100;

                            if (lane.arpMode == ArpMode::Random)
                                lane.arpStep = rng_.nextInt((int)lane.arpSequence.size());
                            else
                                lane.arpStep = (lane.arpStep + 1) % (int)lane.arpSequence.size();

                            if (lane.pendingNoteOff >= 0)
                                midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff), sampleOffset);
                            midi.addEvent(juce::MidiMessage::noteOn(lane.midiChannel, note, (uint8_t)vel), sampleOffset);

                            lane.pendingNoteOff    = note;
                            lane.pendingNoteOffPos = stepPos_ + sampleOffset + 0.85 * thisStepDur;
                        }
                        lane.currentStep = (lane.currentStep + 1) % lane.numSteps;
                    } else {
                        // ── SEQ mode: fire step pattern ───────────────────────
                        const auto& step = lane.steps[lane.currentStep];
                        double utimeSamples = step.utime * thisStepDur;
                        double triggerPos   = stepStartPos + utimeSamples;
                        int    sampleOffset = juce::jlimit(0, numSamples - 1, (int)(triggerPos - stepPos_));

                        if (step.gate && rng_.nextFloat() <= step.prob) {
                            int raw  = step.note + lane.octave * 12;
                            int note = SeqScale::quantize(juce::jlimit(0, 127, raw), rootNote, scaleIdx);
                            int vel  = juce::jlimit(1, 127, (int)(step.vel * 127.f));

                            if (lane.pendingNoteOff >= 0)
                                midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff), sampleOffset);
                            midi.addEvent(juce::MidiMessage::noteOn(lane.midiChannel, note, (uint8_t)vel), sampleOffset);

                            lane.pendingNoteOff    = note;
                            lane.pendingNoteOffPos = stepPos_ + sampleOffset + step.gateLen * thisStepDur;
                        }
                        lane.currentStep = (lane.currentStep + 1) % lane.numSteps;
                    }
                }

                pos = stepEndPos;
            }

            // Handle note-off that falls in this buffer
            if (lane.pendingNoteOff >= 0
                && lane.pendingNoteOffPos >= stepPos_
                && lane.pendingNoteOffPos < stepPos_ + numSamples) {
                int offSmp = juce::jlimit(0, numSamples - 1, (int)(lane.pendingNoteOffPos - stepPos_));
                midi.addEvent(juce::MidiMessage::noteOff(lane.midiChannel, lane.pendingNoteOff), offSmp);
                lane.pendingNoteOff = -1;
            }
        }

        stepPos_ += numSamples;
    }

    // ── Arpeggiator: track held notes (per-lane) ─────────────────────────────
    void arpNoteOn(int note, int laneIdx = -1) {
        auto applyTo = [&](SequencerLane& lane) {
            if (std::find(lane.heldNotes.begin(), lane.heldNotes.end(), note) == lane.heldNotes.end())
                lane.heldNotes.push_back(note);
            rebuildArpSequence(lane);
        };
        if (laneIdx < 0) { for (auto& l : lanes) if (l.arpEnabled) applyTo(l); }
        else if (laneIdx < kMaxLanes) applyTo(lanes[laneIdx]);
    }
    void arpNoteOff(int note, int laneIdx = -1) {
        auto applyTo = [&](SequencerLane& lane) {
            lane.heldNotes.erase(std::remove(lane.heldNotes.begin(), lane.heldNotes.end(), note), lane.heldNotes.end());
            rebuildArpSequence(lane);
        };
        if (laneIdx < 0) { for (auto& l : lanes) if (l.arpEnabled) applyTo(l); }
        else if (laneIdx < kMaxLanes) applyTo(lanes[laneIdx]);
    }

    // ── Randomize a lane's step pattern ───────────────────────────────────────
    // Generates musically coherent random steps using current scale/root.
    void randomizeLane(int laneIdx) {
        if (laneIdx < 0 || laneIdx >= kMaxLanes) return;
        auto& lane = lanes[laneIdx];
        const int n = lane.numSteps;

        // Build pool of available scale notes across 2 octaves
        std::vector<int> pool;
        for (int oct = 0; oct < 2; ++oct)
            for (int i = 0; i < 12; ++i) {
                int candidate = rootNote + oct * 12 + i;
                int q = SeqScale::quantize(candidate, rootNote, scaleIdx);
                if (q == candidate && candidate >= 36 && candidate <= 96)
                    pool.push_back(candidate);
            }
        if (pool.empty()) { pool = {60,62,64,67,69}; }  // fallback C Major penta

        // Random gate density: 50-85%
        const float density = 0.50f + rng_.nextFloat() * 0.35f;

        for (int s = 0; s < n; ++s) {
            auto& step = lane.steps[s];

            // Gate: accented beats always on, others by density
            bool accentBeat = (s % 4 == 0);
            step.gate  = accentBeat || (rng_.nextFloat() < density);

            // Note: pull from scale pool, bias toward root on beat 1
            if (s == 0)
                step.note = rootNote + lane.octave * 12;
            else
                step.note = pool[rng_.nextInt((int)pool.size())] + lane.octave * 12;
            step.note = juce::jlimit(0, 127, step.note);

            // Velocity: accented beats louder, with variation
            if (accentBeat)
                step.vel = 0.75f + rng_.nextFloat() * 0.25f;
            else
                step.vel = 0.45f + rng_.nextFloat() * 0.40f;

            // Probability: 90-100% usually, occasional 40-60% ghost note
            step.prob = (rng_.nextFloat() < 0.15f) ? (0.4f + rng_.nextFloat() * 0.25f) : 1.0f;

            // Microtiming: subtle humanisation on non-accents
            step.utime = accentBeat ? 0.0f : (rng_.nextFloat() - 0.5f) * 0.08f;

            // Gate length: short on some steps for staccato, long on accents
            step.gateLen = accentBeat ? (0.7f + rng_.nextFloat() * 0.25f)
                                      : (0.3f + rng_.nextFloat() * 0.55f);
        }
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
        // (arp settings now per-lane)

        for (int l = 0; l < kMaxLanes; ++l) {
            auto* lx = new juce::XmlElement("Lane");
            lx->setAttribute("idx",        l);
            lx->setAttribute("numSteps",  lanes[l].numSteps);
            lx->setAttribute("active",    lanes[l].active);
            lx->setAttribute("octave",    lanes[l].octave);
            lx->setAttribute("swing",     lanes[l].swing);
            lx->setAttribute("arpEn",     lanes[l].arpEnabled);
            lx->setAttribute("arpMode",   lanes[l].arpMode);
            lx->setAttribute("arpRate",   lanes[l].arpRateDiv);
            lx->setAttribute("arpOct",    lanes[l].arpOctaves);
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

        for (auto* lx : xml->getChildWithTagNameIterator("Lane")) {
            int l = lx->getIntAttribute("idx", 0);
            if (l < 0 || l >= kMaxLanes) continue;
            lanes[l].numSteps   = lx->getIntAttribute("numSteps", 8);
            lanes[l].active     = lx->getBoolAttribute("active", l < 2);
            lanes[l].octave     = lx->getIntAttribute("octave", 0);
            lanes[l].swing      = (float)lx->getDoubleAttribute("swing", 0.0);
            lanes[l].arpEnabled = lx->getBoolAttribute("arpEn",   false);
            lanes[l].arpMode    = lx->getIntAttribute("arpMode",  ArpMode::Up);
            lanes[l].arpRateDiv = lx->getIntAttribute("arpRate",  16);
            lanes[l].arpOctaves = lx->getIntAttribute("arpOct",   1);
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
    double samplesPerStep_= 0.0;   // base 1/16 step
    double stepPos_       = 0.0;   // absolute sample counter
    juce::Random rng_;

    void updateTiming() {
        samplesPerStep_ = (60.0 / bpm) * sampleRate_ / 4.0;  // 1/16 note
    }

    // Samples per step for a lane's arp rate division
    double arpStepDur(const SequencerLane& lane) const {
        // arpRateDiv: 4=quarter, 8=eighth, 16=sixteenth, 32=thirty-second
        return (60.0 / bpm) * sampleRate_ * 4.0 / lane.arpRateDiv;
    }

    void rebuildArpSequence(SequencerLane& lane) {
        lane.arpSequence.clear();
        if (lane.heldNotes.empty()) return;

        auto notes = lane.heldNotes;
        std::sort(notes.begin(), notes.end());

        switch (lane.arpMode) {
            case ArpMode::Up:
                lane.arpSequence = notes; break;
            case ArpMode::Down:
                lane.arpSequence = { notes.rbegin(), notes.rend() }; break;
            case ArpMode::UpDown:
                lane.arpSequence = notes;
                if (notes.size() > 2)
                    for (int i = (int)notes.size() - 2; i > 0; --i)
                        lane.arpSequence.push_back(notes[i]);
                break;
            case ArpMode::Random:
                lane.arpSequence = notes; break;
            case ArpMode::AsPlayed:
                lane.arpSequence = lane.heldNotes; break;
            case ArpMode::Converge: {
                // Outer→inner: hi, lo, hi-1, lo+1, ...
                std::vector<int> lo = notes, hi = notes;
                int l = 0, h = (int)notes.size() - 1;
                while (l <= h) {
                    if (h != l) { lane.arpSequence.push_back(notes[h]); lane.arpSequence.push_back(notes[l]); }
                    else        { lane.arpSequence.push_back(notes[l]); }
                    ++l; --h;
                }
                break;
            }
            case ArpMode::Diverge: {
                // Inner→outer: middle notes first
                int mid = (int)notes.size() / 2;
                int lo = mid, hi = mid;
                while (lo >= 0 || hi < (int)notes.size()) {
                    if (lo >= 0 && hi < (int)notes.size() && lo != hi) {
                        lane.arpSequence.push_back(notes[lo--]);
                        lane.arpSequence.push_back(notes[hi++]);
                    } else if (lo >= 0) { lane.arpSequence.push_back(notes[lo--]); }
                    else                { lane.arpSequence.push_back(notes[hi++]); }
                }
                break;
            }
        }

        // Expand across octaves
        auto base = lane.arpSequence;
        for (int o = 1; o < lane.arpOctaves; ++o)
            for (int note : base)
                lane.arpSequence.push_back(note + o * 12);

        if (lane.arpMode == ArpMode::Random)
            for (int i = (int)lane.arpSequence.size() - 1; i > 0; --i)
                std::swap(lane.arpSequence[i], lane.arpSequence[rng_.nextInt(i + 1)]);

        if (lane.arpStep >= (int)lane.arpSequence.size()) lane.arpStep = 0;
    }
};
