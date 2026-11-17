#include "../../include/dsp/FrequencyProcessor.hpp"
#include "../../include/strategy/FrequencyEffect.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <thread>

static size_t getWindowSize(int sampleRate) {
    if (sampleRate <= 0) throw std::invalid_argument("A taxa de amostragem deve ser positiva.");

    constexpr size_t minimumWindow = 1024;
    constexpr size_t maximumWindow = 32768;
    constexpr size_t targetBinWidthHz = 12;
    const size_t targetSize = (static_cast<size_t>(sampleRate) + targetBinWidthHz - 1) / targetBinWidthHz;
    return std::bit_ceil(std::clamp(targetSize, minimumWindow, maximumWindow));
}

FrequencyProcessor::FrequencyProcessor(int sampleRate, const FrequencyEffect& effect)
    : windowSize(getWindowSize(sampleRate)), fft(windowSize), window(windowSize), gains(windowSize) {
    effect.validateSampleRate(sampleRate);

    // A raiz da janela de Hann é aplicada na ida e na volta. Com 50% de sobreposição,
    // a soma dos pesos das duas janelas reconstrói o sinal quando o ganho é unitário.
    for (size_t i = 0; i < windowSize; ++i) {
        window[i] = std::sin(std::numbers::pi * i / windowSize);
    }

    // Limita o ganho para caber nas somas intermediárias mesmo antes da normalização da inversa.
    const double maxSampleMagnitude = -static_cast<double>(std::numeric_limits<int16_t>::min());
    const double maxGain = std::numeric_limits<double>::max() / (2.0 * maxSampleMagnitude * windowSize * windowSize);
    // Os bins espelhados recebem o mesmo ganho para a inversa continuar real.
    for (size_t bin = 0; bin <= windowSize / 2; ++bin) {
        const double frequencyHz = static_cast<double>(bin) * sampleRate / windowSize;
        const double gain = effect.getGain(frequencyHz);
        if (!std::isfinite(gain) || gain < 0.0 || gain > maxGain) {
            throw std::invalid_argument("Ganho inválido no efeito de frequência.");
        }
        gains[bin] = gain;
        if (bin > 0) gains[windowSize - bin] = gain;
    }
}

void FrequencyProcessor::apply(std::vector<int16_t>& samples, size_t channels) const {
    if (channels == 0 || samples.size() % channels != 0) {
        throw std::invalid_argument("Amostras PCM incompatíveis com o número de canais.");
    }
    if (samples.empty()) return;
    if (std::all_of(gains.begin(), gains.end(), [](double gain) { return gain == 1.0; })) return;

    const size_t hopSize = windowSize / 2;
    const size_t samplesPerChannel = samples.size() / channels;
    const size_t hopCount = (samplesPerChannel - 1) / hopSize + 1;
    std::vector<int16_t> processed(samples.size());

    auto transformWindow = [&](size_t windowIndex, size_t channel, std::vector<std::complex<double>>& spectrum) {
        const size_t first = windowIndex == 0 ? hopSize : 0;
        const size_t sampleStart = windowIndex == 0 ? 0 : (windowIndex - 1) * hopSize;
        const size_t count = std::min(windowSize - first, samplesPerChannel - sampleStart);

        // Completa com zeros nas bordas e lê um canal por vez para não misturar as amostras.
        std::fill(spectrum.begin(), spectrum.end(), 0.0);
        for (size_t i = 0; i < count; ++i) {
            spectrum[first + i] = samples[(sampleStart + i) * channels + channel] * window[first + i];
        }

        fft.forward(spectrum);
        for (size_t i = 0; i < windowSize; ++i) spectrum[i] *= gains[i];
        fft.inverse(spectrum);
    };

    const size_t availableThreads = std::max(1u, std::thread::hardware_concurrency());
    constexpr size_t minimumHopsPerTask = 8;
    const size_t hopsPerTask = std::max(minimumHopsPerTask, (hopCount * channels - 1) / availableThreads + 1);
    const size_t tasksPerChannel = (hopCount - 1) / hopsPerTask + 1;
    const size_t taskCount = tasksPerChannel * channels;
    const size_t threadCount = std::min(availableThreads, taskCount);

    auto processTasks = [&](size_t threadIndex, std::vector<std::complex<double>> previous,
                            std::vector<std::complex<double>> current) {
        for (size_t task = threadIndex; task < taskCount; task += threadCount) {
            const size_t channel = task / tasksPerChannel;
            const size_t firstHop = (task % tasksPerChannel) * hopsPerTask;
            const size_t endHop = firstHop + std::min(hopsPerTask, hopCount - firstHop);

            // Cada tarefa escreve uma faixa do canal. A janela da borda também é
            // calculada pela tarefa vizinha para que ambas possam trabalhar em paralelo.
            transformWindow(firstHop, channel, previous);
            for (size_t hop = firstHop; hop < endHop; ++hop) {
                transformWindow(hop + 1, channel, current);
                const size_t start = hop * hopSize;
                const size_t count = std::min(hopSize, samplesPerChannel - start);
                for (size_t i = 0; i < count; ++i) {
                    const double value = previous[hopSize + i].real() * window[hopSize + i] + current[i].real() * window[i];
                    const double clipped = std::clamp(value, static_cast<double>(std::numeric_limits<int16_t>::min()),
                                                     static_cast<double>(std::numeric_limits<int16_t>::max()));
                    processed[(start + i) * channels + channel] = static_cast<int16_t>(std::lround(clipped));
                }
                previous.swap(current);
            }
        }
    };

    std::vector<std::jthread> workers;
    workers.reserve(threadCount - 1);
    for (size_t i = 1; i < threadCount; ++i) {
        workers.emplace_back(processTasks, i, std::vector<std::complex<double>>(windowSize),
                             std::vector<std::complex<double>>(windowSize));
    }
    processTasks(0, std::vector<std::complex<double>>(windowSize), std::vector<std::complex<double>>(windowSize));
    for (auto& worker : workers) worker.join();
    samples.swap(processed);
}
