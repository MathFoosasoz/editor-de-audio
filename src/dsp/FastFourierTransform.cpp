#include "../../include/dsp/FastFourierTransform.hpp"
#include <bit>
#include <numbers>
#include <stdexcept>
#include <utility>

FastFourierTransform::FastFourierTransform(size_t size) {
    if (!std::has_single_bit(size)) {
        throw std::invalid_argument("O tamanho da FFT deve ser uma potência de dois.");
    }

    reversedIndices.resize(size);
    for (size_t i = 1; i < size; ++i) {
        reversedIndices[i] = (reversedIndices[i / 2] >> 1) | ((i % 2) * (size / 2));
    }

    phaseFactors.resize(size / 2);
    for (size_t i = 0; i < phaseFactors.size(); ++i) {
        const double angle = -2.0 * std::numbers::pi * i / size;
        phaseFactors[i] = std::polar(1.0, angle);
    }
}

void FastFourierTransform::forward(std::vector<std::complex<double>>& values) const {
    transform(values, false);
}

void FastFourierTransform::inverse(std::vector<std::complex<double>>& values) const {
    transform(values, true);
}

void FastFourierTransform::transform(std::vector<std::complex<double>>& values, bool inverse) const {
    const size_t size = reversedIndices.size();
    if (values.size() != size) throw std::invalid_argument("Tamanho do vetor incompatível com a FFT.");

    // A inversão dos bits coloca as entradas na ordem usada pelas borboletas radix-2.
    for (size_t i = 0; i < size; ++i) {
        if (i < reversedIndices[i]) std::swap(values[i], values[reversedIndices[i]]);
    }

    for (size_t halfWidth = 1; halfWidth < size; halfWidth *= 2) {
        const size_t width = 2 * halfWidth;
        const size_t step = size / width;
        for (size_t start = 0; start < size; start += width) {
            for (size_t i = 0; i < halfWidth; ++i) {
                const auto factor = phaseFactors[i * step];
                const auto even = values[start + i];
                const auto odd = values[start + i + halfWidth] * (inverse ? std::conj(factor) : factor);
                values[start + i] = even + odd;
                values[start + i + halfWidth] = even - odd;
            }
        }
    }

    if (inverse) {
        for (auto& value : values) value /= static_cast<double>(size);
    }
}
