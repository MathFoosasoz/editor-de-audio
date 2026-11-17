#pragma once

#include "MPEGFrame.hpp"
#include <vector>
#include <cstdint>
#include <istream>

// Frame de MP3 (MPEG Layer 3)
class Layer3Frame : public MPEGFrame {
private:
    std::vector<uint8_t> data;
    uint32_t version = 3;
    int sampleRate = 44100;
    int channels = 2;

public:
    void parse(std::ifstream& file) override;
    void parse(std::istream& stream);
    void write(std::ofstream& file) const;
    const std::vector<uint8_t>& getData() const;
    int getSampleRate() const { return sampleRate; }
    int getChannels() const { return channels; }
    bool hasXingHeader() const;
    void updateXingHeader(uint32_t totalFrames, uint32_t totalBytes);
    double getDurationInSeconds() const override;
    void setData(std::vector<uint8_t> newData);
};
