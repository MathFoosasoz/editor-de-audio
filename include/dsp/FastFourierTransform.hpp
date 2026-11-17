#pragma once

#include <complex>
#include <cstddef>
#include <vector>

// FFT radix-2. As tabelas são reaproveitadas nas transformadas de mesmo tamanho.
class FastFourierTransform {
private:
    std::vector<size_t> reversedIndices;
    std::vector<std::complex<double>> phaseFactors;

    void transform(std::vector<std::complex<double>>& values, bool inverse) const;

public:
    explicit FastFourierTransform(size_t size);
    void forward(std::vector<std::complex<double>>& values) const;
    void inverse(std::vector<std::complex<double>>& values) const;
};
