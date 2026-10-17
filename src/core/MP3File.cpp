// src/core/MP3File.cpp
#include "../../include/core/MP3File.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/frame/Layer3Frame.hpp"

MP3File::MP3File(const std::string& path) : MPEGFile(path) {}

void MP3File::parse() {
    if (!fileStream.is_open()) {
        open();
    }

    fileStream.seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    char threeBytes[3];
    fileStream.read(threeBytes, 3);
    fileStream.seekg(0, std::ios::beg);

    if (threeBytes[0] == 'I' && threeBytes[1] == 'D' && threeBytes[2] == '3') {
        id3Tag.parse(fileStream);
    }

    while (fileStream.tellg() < fileSize) {
        auto frame = std::make_unique<Layer3Frame>();
        try {
            frame->parse(fileStream);
            frames.push_back(std::move(frame));
        } catch (const AudioException&) {
            break;
        }
    }
}

size_t MP3File::getFrameCount() const {
    return frames.size();
}