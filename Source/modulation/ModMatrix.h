#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

// Any modulation source registers itself here and provides a sample-accurate buffer
class ModMatrix {
public:
    using GetValuesFn = std::function<float(int sampleIndex)>;

    struct Connection {
        std::string sourceId;
        std::string destId;
        float       amount   = 0.f;    // -1..1
        bool        bipolar  = true;
        bool        enabled  = true;
    };

    // Register a source — lambda returns the current mod value for a given sample
    void registerSource(const std::string& id, GetValuesFn fn);

    // Add / remove / clear connections
    void addConnection(const Connection& c);
    void removeConnection(const std::string& sourceId, const std::string& destId);
    void clearConnections();

    // Register a target parameter (pointer to the float the synth reads each block)
    void registerTarget(const std::string& id, float* baseValue, float* modValue,
                        float minVal, float maxVal);

    // Call once per block: zeros all modValues, then accumulates all active connections
    void processBlock(int numSamples);

    const std::vector<Connection>& getConnections() const { return connections_; }

private:
    struct Target {
        float* baseValue = nullptr;
        float* modValue  = nullptr;
        float  minVal    = 0.f;
        float  maxVal    = 1.f;
    };

    std::unordered_map<std::string, GetValuesFn> sources_;
    std::unordered_map<std::string, Target>      targets_;
    std::vector<Connection>                      connections_;
};
