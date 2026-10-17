#include "../../include/core/MPEGFile.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"

MPEGFile::MPEGFile(const std::string& path) : AudioFile(path) {}

void MPEGFile::open() {
    fileStream.open(filename, std::ios::binary);
    if (!fileStream.is_open()) {
        throw FileOpenException(filename);
    }
}

void MPEGFile::close() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}