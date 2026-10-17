#pragma once
#include <fstream>
#include <cstddef>

class AudioFrame {
public:
    virtual ~AudioFrame() = default;
    virtual void parse(std::ifstream& file) = 0;
    virtual size_t getSize() const = 0;
    virtual double getDurationInSeconds() const = 0;
};