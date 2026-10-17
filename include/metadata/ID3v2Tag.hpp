#pragma once

#include <fstream>
#include <cstdint>

class ID3v2Tag {
private:
    size_t tagSize = 0;

public:
    void parse(std::ifstream& file);
    size_t getSize() const;
};