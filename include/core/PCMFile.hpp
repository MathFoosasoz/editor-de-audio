#pragma once

#include "AudioFile.hpp"
#include <vector>
#include <memory>
#include "../frame/PCMFrame.hpp"
#include "../strategy/RangeStrategy.hpp"

class MP3File;

class PCMFile : public AudioFile {
private:
    std::vector<std::unique_ptr<PCMFrame>> frames;
    int sampleRate = 44100;
    int channels = 2;

public:
    explicit PCMFile(const std::string& path);
    PCMFile(int sr, int ch);

    void parse() override;
    void save(const std::string& outputPath) const override;
    size_t getFrameCount() const override;
    void applyRange(const RangeStrategy& strategy) override;
    void applyCut(const RangeStrategy& strategy) override;
    void merge(const std::string& path) override;

    const std::vector<std::unique_ptr<PCMFrame>>& getFrames() const;
    void addFrame(std::unique_ptr<PCMFrame> frame);
    void applyBassBoost(float gainFactor);

    std::unique_ptr<MP3File> toMP3() const;
};