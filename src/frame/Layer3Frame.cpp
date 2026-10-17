#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include <cstdint>

void Layer3Frame::parse(std::ifstream& file) {
    char headerBytes[4];
    file.read(headerBytes, 4);

    if (!file || file.gcount() < 4) {
        throw UnexpectedEOFException();
    }

    uint32_t h = (static_cast<uint8_t>(headerBytes[0]) << 24) |
                 (static_cast<uint8_t>(headerBytes[1]) << 16) |
                 (static_cast<uint8_t>(headerBytes[2]) << 8)  |
                 static_cast<uint8_t>(headerBytes[3]);

    if ((h & 0xFFE00000) != 0xFFE00000) {
        throw InvalidHeaderException("Invalid MPEG sync word.");
    }

    uint32_t version = (h >> 19) & 0x3;
    uint32_t layer = (h >> 17) & 0x3;

    if (layer != 1) {
        throw InvalidHeaderException("Not an MPEG Layer III frame.");
    }

    uint32_t bitrateIndex = (h >> 12) & 0xF;
    uint32_t sampleRateIndex = (h >> 10) & 0x3;
    uint32_t paddingBit = (h >> 9) & 0x1;

    if (bitrateIndex == 0 || bitrateIndex == 15 || sampleRateIndex == 3) {
        throw InvalidHeaderException("Invalid bitrate or sample rate index.");
    }

    static const int bitrates[2][16] = {
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}
    };

    static const int sampleRates[3][4] = {
        {44100, 48000, 32000, 0},
        {22050, 24000, 16000, 0},
        {11025, 12000, 8000, 0}
    };

    int vIndex = (version == 3) ? 0 : 1; 
    int br = bitrates[vIndex][bitrateIndex] * 1000;

    int srIndex = 0;
    if (version == 3) srIndex = 0;
    else if (version == 2) srIndex = 1;
    else if (version == 0) srIndex = 2;

    int sr = sampleRates[srIndex][sampleRateIndex];

    if (sr == 0) {
        throw InvalidHeaderException("Invalid sample rate.");
    }

    frameSize = (144 * br / sr) + paddingBit;

    file.seekg(frameSize - 4, std::ios::cur);
    if (!file) {
        throw UnexpectedEOFException();
    }
}