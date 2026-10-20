#pragma once

#include <string>
#include <fstream>
#include <cstddef>
#include <utility>
#include <vector>
#include <memory>
#include "../frame/AudioFrame.hpp"

// declarações p/ compilação
class RangeStrategy;
class PCMFile;

// Tipo pai abstrato de arquivo de aúdio
class AudioFile {
protected:
    std::string filePath;
    std::ifstream fileStream;
    std::vector<std::unique_ptr<AudioFrame>> frames;

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
    virtual void addFrame(std::unique_ptr<AudioFrame> frame) = 0;
    virtual void applyBassBoost(float gain) {
            throw std::invalid_argument("Bass boost não é suportado para este formato.");
    }

    virtual std::unique_ptr<PCMFile> toPCM() const = 0;  
};