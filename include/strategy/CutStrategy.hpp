#pragma once
#pragma once

#include "RangeStrategy.hpp"
#include <utility>
#include <stdexcept>

class CutStrategy : public RangeStrategy {
private:
    double startParam;
    double lengthParam;
    bool isSeconds;

    size_t secondsToFrameIndex(const std::vector<std::unique_ptr<AudioFrame>>& frames, double seconds) const {
        double accumulated = 0.0;
        for (size_t i = 0; i < frames.size(); ++i) {
            accumulated += frames[i]->getDurationInSeconds();
            if (accumulated >= seconds) {
                return i;
            }
        }
        return frames.size();
    }

public:
    CutStrategy(double start, double length, bool seconds = false) 
        : startParam(start), lengthParam(length), isSeconds(seconds) {}

    std::pair<size_t, size_t> calculateRange(const std::vector<std::unique_ptr<AudioFrame>>& frames) const override {
        size_t startIdx = isSeconds ? secondsToFrameIndex(frames, startParam) : static_cast<size_t>(startParam);
        size_t endIdx = isSeconds ? secondsToFrameIndex(frames, startParam + lengthParam) : startIdx + static_cast<size_t>(lengthParam);

        if (startIdx > frames.size()) startIdx = frames.size();
        if (endIdx > frames.size()) endIdx = frames.size();

        return {startIdx, endIdx};
    }
};