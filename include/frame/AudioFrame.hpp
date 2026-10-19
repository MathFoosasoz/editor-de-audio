#pragma once
#include <fstream>
#include <cstddef>
#include <vector>
#include <cstdint>

// Classe abstrata para um "frame" de aúdio
// O que é um frame depende muito de encoding. No mp3, é uma estrutura com header, bit reservoir, etc
// No PCM, são só N valores (1 por channel) indicando as amplitudes
class AudioFrame {
public:
    virtual ~AudioFrame() = default;
    virtual void parse(std::ifstream& file) = 0;
    virtual size_t getSize() const = 0;
    virtual double getDurationInSeconds() const = 0;
    virtual std::vector<int16_t>& getSamples();
    virtual const std::vector<int16_t>& getSamples() const;
};