#pragma once

#include "RangeStrategy.hpp"

class SecondRangeStrategy : public RangeStrategy {
public:
    enum class Mode { TrimStart, TrimEnd, Extract };

private:
    Mode mode;
    double param1;
    double param2;

    size_t secondsToFrameIndex(const std::vector<std::unique_ptr<AudioFrame>>& frames, double seconds, bool roundUp) const {
        double accumulated = 0.0;
        for (size_t i = 0; i < frames.size(); ++i) {
            accumulated += frames[i]->getDurationInSeconds();
            if (accumulated >= seconds) {
                return roundUp ? i + 1 : i;
            }
        }
        return frames.size();
    }

public:
    SecondRangeStrategy(Mode m, double p1, double p2 = 0.0) : mode(m), param1(p1), param2(p2) {}

    std::pair<size_t, size_t> calculateRange(const std::vector<std::unique_ptr<AudioFrame>>& frames) const override {
        size_t total = frames.size();
        if (total == 0) return {0, 0};

        switch (mode) {
            case Mode::TrimStart: {
                size_t startIdx = secondsToFrameIndex(frames, param1, false);
                return {startIdx, total};
            }
            case Mode::TrimEnd: {
                size_t endIdx = secondsToFrameIndex(frames, param1, true);
                if (endIdx >= total) return {0, 0};
                return {0, total - endIdx};
            }
            case Mode::Extract: {
                size_t startIdx = secondsToFrameIndex(frames, param1, false);
                size_t endIdx = secondsToFrameIndex(frames, param1 + param2, true);
                if (startIdx > total) startIdx = total;
                if (endIdx > total) endIdx = total;
                return {startIdx, endIdx};
            }
        }
        return {0, total};
    }
};