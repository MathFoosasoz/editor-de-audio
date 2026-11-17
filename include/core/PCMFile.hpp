#pragma once

#include "AudioFile.hpp"
#include <vector>
#include <memory>
#include "../frame/PCMFrame.hpp"
#include "../strategy/RangeStrategy.hpp"

class MP3File;

// Arquivo WAV e representação PCM usada para misturar formatos e processar amostras.
class PCMFile : public AudioFile {
private:
    int sampleRate = 44100;
    int channels = 2;

    void addSamples(const std::vector<int16_t>& samples);
    std::vector<int16_t> convertSamples(int targetSampleRate, int targetChannels) const;

public:
    explicit PCMFile(const std::string& path);
    PCMFile(int sr, int ch);

    void parse() override;
    void save(const std::string& outputPath) const override;
    size_t getFrameCount() const override;
    void applyRange(const RangeStrategy& strategy) override;
    void applyCut(const RangeStrategy& strategy) override;
    void merge(const std::string& path) override;
    void merge(const PCMFile& other);
    void addFrame(std::unique_ptr<AudioFrame> frame) override;
    std::unique_ptr<PCMFile> toPCM() const override;

    void applyBassBoost(float gainFactor) override;

    std::unique_ptr<MP3File> toMP3() const;
};
