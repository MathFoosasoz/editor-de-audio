#include "../../include/frame/AudioFrame.hpp"

std::vector<int16_t>& AudioFrame::getSamples() {
    static std::vector<int16_t> empty;
    return empty;
}

const std::vector<int16_t>& AudioFrame::getSamples() const {
    static const std::vector<int16_t> empty;
    return empty;
}