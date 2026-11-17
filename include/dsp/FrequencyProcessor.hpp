#pragma once

#include "FastFourierTransform.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>

class FrequencyEffect;

// Aplica um efeito em janelas sobrepostas e volta às amostras PCM intercaladas.
class FrequencyProcessor {
private:
    size_t windowSize;
    FastFourierTransform fft;
    std::vector<double> window;
    std::vector<double> gains;

public:
    FrequencyProcessor(int sampleRate, const FrequencyEffect& effect);
    void apply(std::vector<int16_t>& samples, size_t channels) const;
};
