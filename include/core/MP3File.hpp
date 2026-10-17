#pragma once

#include "MPEGFile.hpp"
#include <vector>
#include <memory>
#include "../frame/AudioFrame.hpp"
#include "../metadata/ID3v2Tag.hpp"

class MP3File : public MPEGFile {
private:
    ID3v2Tag id3Tag;
    std::vector<std::unique_ptr<AudioFrame>> frames;

public:
    explicit MP3File(const std::string& path);
    void parse() override;
    size_t getFrameCount() const;
};