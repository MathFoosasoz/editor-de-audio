#pragma once

#include "MPEGFrame.hpp"
#include <vector>
#include <cstdint>

// Frame de MP3 (MPEG Layer 3)
class Layer3Frame : public MPEGFrame {
private:
    std::vector<uint8_t> data;
    uint32_t version = 3;
    int sampleRate = 44100;

public:
    void parse(std::ifstream& file) override;
    void write(std::ofstream& file) const;
    const std::vector<uint8_t>& getData() const;
    bool hasXingHeader() const;
    void updateXingHeader(uint32_t totalFrames, uint32_t totalBytes);
    double getDurationInSeconds() const;
};