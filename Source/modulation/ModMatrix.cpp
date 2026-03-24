#include "ModMatrix.h"

void ModMatrix::registerSource(const std::string& id, GetValuesFn fn) {
    sources_[id] = std::move(fn);
}

void ModMatrix::registerTarget(const std::string& id, float* baseValue, float* modValue,
                                float minVal, float maxVal) {
    targets_[id] = { baseValue, modValue, minVal, maxVal };
}

void ModMatrix::addConnection(const Connection& c) {
    // Replace if same src→dest already exists
    for (auto& conn : connections_)
        if (conn.sourceId == c.sourceId && conn.destId == c.destId) { conn = c; return; }
    connections_.push_back(c);
}

void ModMatrix::removeConnection(const std::string& srcId, const std::string& destId) {
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
        [&](const Connection& c) { return c.sourceId == srcId && c.destId == destId; }),
        connections_.end());
}

void ModMatrix::clearConnections() { connections_.clear(); }

void ModMatrix::processBlock(int numSamples) {
    // Zero all mod values
    for (auto& [id, tgt] : targets_)
        if (tgt.modValue) *tgt.modValue = 0.f;

    // Accumulate at block-rate (use first sample of block; per-sample is optional)
    for (auto& conn : connections_) {
        if (!conn.enabled) continue;
        auto srcIt = sources_.find(conn.sourceId);
        auto dstIt = targets_.find(conn.destId);
        if (srcIt == sources_.end() || dstIt == targets_.end()) continue;

        float srcVal = srcIt->second(0);   // block-rate: sample 0
        float mod    = srcVal * conn.amount;

        auto& tgt = dstIt->second;
        if (tgt.modValue) {
            *tgt.modValue += mod * (tgt.maxVal - tgt.minVal);
            *tgt.modValue  = juce::jlimit(tgt.minVal - *tgt.baseValue,
                                           tgt.maxVal - *tgt.baseValue,
                                           *tgt.modValue);
        }
    }
}
