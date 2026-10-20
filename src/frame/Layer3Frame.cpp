#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include <cstdint>

void Layer3Frame::parse(std::ifstream& file) {
    char headerBytes[4];
    file.read(headerBytes, 4);

    if (!file || file.gcount() < 4) {
        throw UnexpectedEOFException();
    }

    // transforma os 4 bytes do header do frame em um uint 32 bits
    uint32_t h = (static_cast<uint8_t>(headerBytes[0]) << 24) |
                 (static_cast<uint8_t>(headerBytes[1]) << 16) |
                 (static_cast<uint8_t>(headerBytes[2]) << 8)  |
                 static_cast<uint8_t>(headerBytes[3]);

    if ((h & 0xFFE00000) != 0xFFE00000) {
        throw InvalidHeaderException("MPEG sync word inválida.");
    }

    version = (h >> 19) & 0x3;
    uint32_t layer = (h >> 17) & 0x3;

    if (layer != 1) {
        throw InvalidHeaderException("Não é um frame MPEG Layer III.");
    }

    uint32_t bitrateIndex = (h >> 12) & 0xF;
    uint32_t sampleRateIndex = (h >> 10) & 0x3;
    uint32_t paddingBit = (h >> 9) & 0x1;

    if (bitrateIndex == 0 || bitrateIndex == 15 || sampleRateIndex == 3) {
        throw InvalidHeaderException("Bitrate ou sample index inválidos.");
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

    sampleRate = sampleRates[srIndex][sampleRateIndex];

    if (sampleRate == 0) {
        throw InvalidHeaderException("Sample rate inválida.");
    }

    frameSize = (144 * br / sampleRate) + paddingBit;

    data.resize(frameSize);
    
    data[0] = headerBytes[0];
    data[1] = headerBytes[1];
    data[2] = headerBytes[2];
    data[3] = headerBytes[3];

    file.read(reinterpret_cast<char*>(data.data() + 4), frameSize - 4);
    if (!file) {
        throw UnexpectedEOFException();
    }
}

double Layer3Frame::getDurationInSeconds() const {
    int samples = (version == 3) ? 1152 : 576;
    return static_cast<double>(samples) / sampleRate;
}

void Layer3Frame::write(std::ofstream& file) const {
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

const std::vector<uint8_t>& Layer3Frame::getData() const {
    return data;
}

bool Layer3Frame::hasXingHeader() const {
    if (data.size() < 40) return false;

    uint32_t h = (static_cast<uint8_t>(data[0]) << 24) |
                 (static_cast<uint8_t>(data[1]) << 16) |
                 (static_cast<uint8_t>(data[2]) << 8)  |
                 static_cast<uint8_t>(data[3]);

    uint32_t version = (h >> 19) & 0x3;      // 3 = MPEG-1, 2 = MPEG-2, 0 = MPEG-2.5
    uint32_t channelMode = (h >> 6) & 0x3;   // 3 = Mono, others = Stereo variants

    size_t xingOffset = 0;
    if (version == 3) { // MPEG-1
        xingOffset = (channelMode == 3) ? 21 : 36;
    } else { // MPEG-2 / 2.5
        xingOffset = (channelMode == 3) ? 13 : 21;
    }

    if (xingOffset + 4 <= data.size()) {
        if ((data[xingOffset] == 'X' && data[xingOffset+1] == 'i' && data[xingOffset+2] == 'n' && data[xingOffset+3] == 'g') ||
            (data[xingOffset] == 'I' && data[xingOffset+1] == 'n' && data[xingOffset+2] == 'f' && data[xingOffset+3] == 'o')) {
            return true;
        }
    }
    return false;
}

// Função obrigatória para manter o arquivo de output MP3 funcionando em tocadores de áudio após as modificações
// Usa vários magic numbers de deslocamento que nos baseamos de outra implementação
// O ideal seria substituir os magic numbers no futuro usando um arquivo de constantes
void Layer3Frame::updateXingHeader(uint32_t totalFrames, uint32_t totalBytes) {
    if (data.size() < 40) return;

    uint32_t h = (static_cast<uint8_t>(data[0]) << 24) |
                 (static_cast<uint8_t>(data[1]) << 16) |
                 (static_cast<uint8_t>(data[2]) << 8)  |
                 static_cast<uint8_t>(data[3]);

    uint32_t version = (h >> 19) & 0x3;
    uint32_t channelMode = (h >> 6) & 0x3;

    size_t xingOffset = 0;
    if (version == 3) {
        xingOffset = (channelMode == 3) ? 21 : 36;
    } else {
        xingOffset = (channelMode == 3) ? 13 : 21;
    }

    if (xingOffset + 16 <= data.size()) {
        if ((data[xingOffset] == 'X' && data[xingOffset+1] == 'i' && data[xingOffset+2] == 'n' && data[xingOffset+3] == 'g') ||
            (data[xingOffset] == 'I' && data[xingOffset+1] == 'n' && data[xingOffset+2] == 'f' && data[xingOffset+3] == 'o')) {
            
            size_t flagsOffset = xingOffset + 4;
            size_t framesOffset = xingOffset + 8;
            size_t bytesOffset = xingOffset + 12;

            uint32_t flags = (static_cast<uint32_t>(data[flagsOffset]) << 24) |
                             (static_cast<uint32_t>(data[flagsOffset + 1]) << 16) |
                             (static_cast<uint32_t>(data[flagsOffset + 2]) << 8)  |
                             static_cast<uint32_t>(data[flagsOffset + 3]);

            flags |= 0x00000001;
            flags |= 0x00000002;
            flags &= ~0x00000004;

            data[flagsOffset]     = (flags >> 24) & 0xFF;
            data[flagsOffset + 1] = (flags >> 16) & 0xFF;
            data[flagsOffset + 2] = (flags >> 8) & 0xFF;
            data[flagsOffset + 3] = flags & 0xFF;

            data[framesOffset]     = (totalFrames >> 24) & 0xFF;
            data[framesOffset + 1] = (totalFrames >> 16) & 0xFF;
            data[framesOffset + 2] = (totalFrames >> 8) & 0xFF;
            data[framesOffset + 3] = totalFrames & 0xFF;

            data[bytesOffset]     = (totalBytes >> 24) & 0xFF;
            data[bytesOffset + 1] = (totalBytes >> 16) & 0xFF;
            data[bytesOffset + 2] = (totalBytes >> 8) & 0xFF;
            data[bytesOffset + 3] = totalBytes & 0xFF;
        }
    }
}

void Layer3Frame::setData(std::vector<uint8_t> newData) {
    data = std::move(newData);
}