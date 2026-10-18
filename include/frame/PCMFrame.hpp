#pragma once

#include "AudioFrame.hpp"
#include <vector>
#include <cstdint>
#include <fstream>

// Frame de PCM
// Vale notar que isso é um encoding ligeiramente mais avançado do que o PCM, visto que ele tem uma sample rate acoplada com os seus frames
class PCMFrame : public AudioFrame {
private:
    std::vector<int16_t> samples;
    int sampleRate;
    int channels;

public:
    PCMFrame(std::vector<int16_t> s, int sr, int ch) 
        : samples(std::move(s)), sampleRate(sr), channels(ch) {}

    const std::vector<int16_t>& getSamples() const { return samples; }
    std::vector<int16_t>& getSamples() { return samples; }
    
    int getSampleRate() const { return sampleRate; }
    int getChannels() const { return channels; }

    void parse(std::ifstream& file) override {
        // implemented here for polymorphism
    }

    size_t getSize() const override {
        return samples.size() * sizeof(int16_t);
    }

    double getDurationInSeconds() const override {
    if (sampleRate <= 0 || channels <= 0) return 0.0;
    return static_cast<double>(samples.size()) / (channels * sampleRate);
}
};