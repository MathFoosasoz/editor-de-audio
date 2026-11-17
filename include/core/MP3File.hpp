#pragma once

#include "PCMFile.hpp" 
#include "MPEGFile.hpp"
#include <vector>
#include <memory>
#include <istream>
#include "../frame/AudioFrame.hpp"
#include "../metadata/ID3v2Tag.hpp"
#include "../strategy/RangeStrategy.hpp"

// Arquivo especificamente MP3. Vale notar que um arquivo MP3 também pode conter subsessões MP1 e MP2 (embora isso não seja feito em prática)
// De toda forma, a macroestrutura do mp3 é diferente de mp2 e mp1, então necessita ser uma classe separada
class MP3File : public MPEGFile {
private:
    ID3v2Tag id3Tag;
    bool framesEdited = false;

public:
    explicit MP3File(const std::string& path);
    explicit MP3File(std::istream& encoded);
    void parse() override;
    void save(const std::string& outputPath) const override;
    size_t getFrameCount() const override;
    
    void applyRange(const RangeStrategy& strategy) override;
    void applyCut(const RangeStrategy& strategy) override;
    void merge(const std::string& path) override;
    void applyBassBoost(float gain) override;
    void applyFrequencyEffect(const FrequencyEffect& effect) override;
    
    std::unique_ptr<PCMFile> toPCM() const override;
    void addFrame(std::unique_ptr<AudioFrame> frame) override;
    
    const std::vector<std::unique_ptr<AudioFrame>>& getFrames() const {
        return frames;
    }
};
