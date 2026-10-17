#pragma once

#include <vector>
#include <memory>
#include <utility>
#include "../frame/AudioFrame.hpp"

class RangeStrategy {
public:
    virtual ~RangeStrategy() = default;
    virtual std::pair<size_t, size_t> calculateRange(const std::vector<std::unique_ptr<AudioFrame>>& frames) const = 0;
};