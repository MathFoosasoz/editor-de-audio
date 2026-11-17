#pragma once

#include "FrequencyEffect.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

// Filtros com uma transição suave em torno da frequência de corte.
class FrequencyFilter : public FrequencyEffect {
public:
    enum class Mode { BassBoost, LowPass, HighPass };

    static constexpr double bassCutoffHz = 200.0;

private:
    Mode mode;
    double cutoffHz;
    double gainFactor;

public:
    FrequencyFilter(Mode mode, double cutoffHz, double gainFactor = 1.0)
        : mode(mode), cutoffHz(cutoffHz), gainFactor(gainFactor) {
        if (!std::isfinite(cutoffHz) || cutoffHz <= 0.0) {
            throw std::invalid_argument("A frequência de corte deve ser positiva e finita.");
        }
        if (!std::isfinite(gainFactor) || gainFactor < 0.0) {
            throw std::invalid_argument("O ganho deve ser finito e maior ou igual a zero.");
        }
    }

    void validateSampleRate(int sampleRate) const override {
        if (mode != Mode::BassBoost && cutoffHz >= sampleRate / 2.0) {
            throw std::invalid_argument("A frequência de corte deve ser menor que metade da taxa de amostragem.");
        }
    }

    double getGain(double frequencyHz) const override {
        // A transição ocupa a faixa entre 80% e 120% da frequência de corte.
        const double position = std::clamp((frequencyHz / cutoffHz - 0.8) / 0.4, 0.0, 1.0);
        const double lowPassGain = (1.0 + std::cos(std::numbers::pi * position)) / 2.0;

        if (mode == Mode::LowPass) return lowPassGain;
        if (mode == Mode::HighPass) return 1.0 - lowPassGain;
        return std::lerp(1.0, gainFactor, lowPassGain);
    }
};
