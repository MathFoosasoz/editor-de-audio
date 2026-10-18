#pragma once

#include "AudioFile.hpp"

// Classe abstrata para arquivos que usam encoding MPEG (MP 1,2,3,4)
class MPEGFile : public AudioFile {
public:
    explicit MPEGFile(const std::string& path);
    virtual ~MPEGFile() = default;

    void open() override;
    void close() override;
};