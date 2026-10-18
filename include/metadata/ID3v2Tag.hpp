#pragma once

#include <fstream>
#include <cstdint>
#include <vector>

// Metadados IDV3v2
class ID3v2Tag {
private:
    size_t tagSize = 0;
    std::vector<uint8_t> rawData;
    bool hasTag = false;

public:
    void parse(std::ifstream& file);
    void write(std::ofstream& file) const;
    size_t getSize() const;
    bool exists() const;
};