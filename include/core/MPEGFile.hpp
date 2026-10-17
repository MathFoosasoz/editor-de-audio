#pragma once

#include "AudioFile.hpp"

class MPEGFile : public AudioFile {
public:
    explicit MPEGFile(const std::string& path);
    virtual ~MPEGFile() = default;

    void open() override;
    void close() override;
};