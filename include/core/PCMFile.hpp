#pragma once

#include "AudioFile.hpp"
#include <vector>
#include <memory>
#include "../frame/PCMFrame.hpp"
#include "../strategy/RangeStrategy.hpp"

class MP3File;

// Classe para arquivos do formato PCM. 
// Para tratamento de aúdio, precisamos converter todos os tipo de aúdio para PCM (que é a forma mais bruta)
// De se representar aúdio, e posteriormente convertemos de volta de PCM para o formato da nossa escolha.
// Esse processo de conversão é extremamente complexo (só MP3 <-> PCM são ~20 mil linhas de código), então dependemos do FFMPEG
class PCMFile : public AudioFile {
private:
    int sampleRate = 44100;
    int channels = 2;

public:
    explicit PCMFile(const std::string& path);
    PCMFile(int sr, int ch);

    void parse() override;
    void save(const std::string& outputPath) const override;
    size_t getFrameCount() const override;
    void applyRange(const RangeStrategy& strategy) override;
    void applyCut(const RangeStrategy& strategy) override;
    void merge(const std::string& path) override;
    void addFrame(std::unique_ptr<AudioFrame> frame) override;
    std::unique_ptr<PCMFile> toPCM() const override {
        auto copy = std::make_unique<PCMFile>(sampleRate, channels);
        return copy;
    }

    void applyBassBoost(float gainFactor);

    std::unique_ptr<MP3File> toMP3() const;
};