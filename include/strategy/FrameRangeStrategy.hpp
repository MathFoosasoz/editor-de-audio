#pragma once

#include "RangeStrategy.hpp"
#include <stdexcept>

class FrameRangeStrategy : public RangeStrategy {
public:
    enum class Mode { TrimStart, TrimEnd, Extract };

private:
    Mode mode;
    size_t param1;
    size_t param2;

public:
    FrameRangeStrategy(Mode m, size_t p1, size_t p2 = 0) : mode(m), param1(p1), param2(p2) {}

    std::pair<size_t, size_t> calculateRange(const std::vector<std::unique_ptr<AudioFrame>>& frames) const override {
        size_t total = frames.size();
        if (total == 0) return {0, 0};

        switch (mode) {
            case Mode::TrimStart:
                if (param1 >= total) return {total, total};
                return {param1, total};

            case Mode::TrimEnd:
                if (param1 >= total) return {0, 0};
                return {0, total - param1};

            case Mode::Extract:
                if (param1 >= total) return {total, total};
                {
                    size_t end = param1 + param2;
                    if (end > total) end = total;
                    return {param1, end};
                }
        }
        return {0, total};
    }
};