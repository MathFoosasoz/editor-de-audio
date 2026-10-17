#include "../../include/metadata/ID3v2Tag.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"

void ID3v2Tag::parse(std::ifstream& file) {
    std::streampos startPos = file.tellg();
    char header[10];
    file.read(header, 10);
    
    if (!file || file.gcount() < 10) {
        file.clear();
        file.seekg(startPos);
        return;
    }

    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3') {
        file.clear();
        file.seekg(startPos);
        return;
    }

    uint32_t size = 0;
    size = ((header[6] & 0x7F) << 21) |
           ((header[7] & 0x7F) << 14) |
           ((header[8] & 0x7F) << 7)  |
           (header[9] & 0x7F);

    tagSize = size + 10;
    
    file.seekg(startPos);
    rawData.resize(tagSize);
    file.read(reinterpret_cast<char*>(rawData.data()), tagSize);
    if (!file) {
        throw UnexpectedEOFException();
    }
    hasTag = true;
}

void ID3v2Tag::write(std::ofstream& file) const {
    if (hasTag && !rawData.empty()) {
        file.write(reinterpret_cast<const char*>(rawData.data()), rawData.size());
    }
}

size_t ID3v2Tag::getSize() const {
    return tagSize;
}

bool ID3v2Tag::exists() const {
    return hasTag;
}