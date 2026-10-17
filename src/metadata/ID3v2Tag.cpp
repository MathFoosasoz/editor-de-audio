#include "../../include/metadata/ID3v2Tag.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"

void ID3v2Tag::parse(std::ifstream& file) {
    char header[10];
    file.read(header, 10);
    
    if (!file || file.gcount() < 10) {
        throw UnexpectedEOFException();
    }

    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3') {
        throw InvalidHeaderException("Not a valid ID3v2 tag.");
    }

    uint32_t size = 0;
    size = ((header[6] & 0x7F) << 21) |
           ((header[7] & 0x7F) << 14) |
           ((header[8] & 0x7F) << 7)  |
           (header[9] & 0x7F);

    tagSize = size + 10;
    file.seekg(tagSize, std::ios::beg);
}

size_t ID3v2Tag::getSize() const {
    return tagSize;
}