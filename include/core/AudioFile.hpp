#pragma once

#include <string>
#include <fstream>
#include <cstddef>
#include <utility>

class RangeStrategy;

class AudioFile {
protected:
    std::string filePath;
    std::ifstream fileStream;

public:
    explicit AudioFile(std::string path) : filePath(std::move(path)) {}
    virtual ~AudioFile() = default;

    virtual void open() { fileStream.open(filePath, std::ios::binary); }
    virtual void close() { if (fileStream.is_open()) fileStream.close(); }

    virtual void parse() = 0;
    virtual void save(const std::string& outputPath) const = 0;
    virtual size_t getFrameCount() const = 0;
    virtual void applyRange(const RangeStrategy& strategy) = 0;
    virtual void applyCut(const RangeStrategy& strategy) = 0;
    virtual void merge(const std::string& path) = 0;
};