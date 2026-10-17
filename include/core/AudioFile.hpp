#pragma once

#include <string>
#include <fstream>

class AudioFile {
protected:
    std::string filename;
    std::ifstream fileStream;

public:
    explicit AudioFile(const std::string& path);
    virtual ~AudioFile() = default;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void parse() = 0;
};