#pragma once

#include "PCMFile.hpp" 
#include "MPEGFile.hpp"
#include <vector>
#include <memory>
#include "../frame/AudioFrame.hpp"
#include "../metadata/ID3v2Tag.hpp"
#include "../strategy/RangeStrategy.hpp"

// Arquivo especificamente MP3. Vale notar que um arquivo MP3 também pode conter subsessões MP1 e MP2 (embora isso não seja feito em prática)
// De toda forma, a macroestrutura do mp3 é diferente de mp2 e mp1, então necessita ser uma classe separada
class MP3File : public MPEGFile {
private:
    ID3v2Tag id3Tag;
    std::vector<std::unique_ptr<AudioFrame>> frames;

public:
    explicit MP3File(const std::string& path);
    void parse() override;
    void save(const std::string& outputPath) const;
    size_t getFrameCount() const;
    
    void applyRange(const RangeStrategy& strategy);
    void applyCut(const RangeStrategy& strategy);
    void merge(const std::string& path);
    std::unique_ptr<PCMFile> toPCM() const;
};